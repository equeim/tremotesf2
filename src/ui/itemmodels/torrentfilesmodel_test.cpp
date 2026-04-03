// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#define QTEST_THROW_ON_FAIL

#include <ranges>
#include <set>

#include <QTest>
#include <QSignalSpy>

#include "basetorrentfilesmodel.h"
#include "torrentfilestreebuilder.h"
#include "formatutils.h"
#include "log/log.h"

#include <fmt/ranges.h>

SPECIALIZE_FORMATTER_FOR_QDEBUG(QVariant)

using namespace Qt::StringLiterals;

namespace tremotesf {
    using formatutils::formatByteSize;
    using formatutils::formatProgress;

    namespace {
        QModelIndex indexForPath(QLatin1String path, QAbstractItemModel& model) {
            QModelIndex index{};
            for (const auto& part : path.tokenize(u'/')) {
                const auto childIndexes =
                    std::views::iota(0, model.rowCount(index)) | std::views::transform([&](int row) {
                        return model.index(row, static_cast<int>(BaseTorrentFilesModel::Column::Name), index);
                    });
                const auto found = std::ranges::find(childIndexes, part, [&](const QModelIndex& childIndex) {
                    return childIndex.data().toString();
                });
                if (found == childIndexes.end()) {
                    warning().log("Did not find child with name {}", part);
                    QFAIL("Nope");
                }
                index = *found;
            }
            return index;
        }

        std::pair<QModelIndex, QModelIndex>
        expectedDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight) {
            return {topLeft.siblingAtColumn(0), bottomRight.siblingAtColumn(bottomRight.model()->columnCount() - 1)};
        }

        std::pair<QModelIndex, QModelIndex> expectedDataChanged(const QModelIndex& index) {
            return expectedDataChanged(index, index);
        }

        std::set<std::pair<QModelIndex, QModelIndex>> actualDataChanged(QSignalSpy& spy) {
            return spy
                   | std::views::transform([](QList<QVariant> args) {
                         return std::pair{args.at(0).toModelIndex(), args.at(1).toModelIndex()};
                     })
                   | std::ranges::to<std::set>();
        }
    }

    class TestTorrentFilesModel : public BaseTorrentFilesModel {
        Q_OBJECT
    public:
        TestTorrentFilesModel()
            : BaseTorrentFilesModel(
                  {Column::Name, Column::Size, Column::ProgressBar, Column::Progress, Column::Priority}
              ) {}

        void renameFile(const QModelIndex&, const QString&) override {}

        struct File {
            QString path;
            qint64 size;
            qint64 completedSize;
            TorrentFilesModelEntry::Priority priority;
            bool wanted;
        };

        void populate() {
            const std::array files{
                File{
                    .path = "topdir/subdir1/subsubddir/file1",
                    .size = 666,
                    .completedSize = 0,
                    .priority = TorrentFilesModelEntry::Priority::Normal,
                    .wanted = true
                },
                File{
                    .path = "topdir/subdir1/subsubddir/file2",
                    .size = 100000,
                    .completedSize = 4234,
                    .priority = TorrentFilesModelEntry::Priority::Low,
                    .wanted = false
                },
                File{
                    .path = "topdir/subdir2/file1",
                    .size = 3333333,
                    .completedSize = 0,
                    .priority = TorrentFilesModelEntry::Priority::Normal,
                    .wanted = true
                },
                File{
                    .path = "topdir/subdir2/file2",
                    .size = 111,
                    .completedSize = 0,
                    .priority = TorrentFilesModelEntry::Priority::Normal,
                    .wanted = true
                }
            };
            mRootDirectory = std::make_unique<TorrentFilesModelDirectory>();
            TorrentFilesTreeBuilder builder(mRootDirectory.get(), files.size());
            for (const File& file : files) {
                builder.addFile(
                    file.path.tokenize(u'/', Qt::SkipEmptyParts),
                    file.size,
                    file.completedSize,
                    file.wanted,
                    file.priority
                );
            }
            mFiles = std::move(builder.files);
        }

        using BaseTorrentFilesModel::updateFiles;
    };

    class TorrentFilesModelTest : public QObject {
        Q_OBJECT

    private slots:
        void checkInitialState() {
            TestTorrentFilesModel model{};
            model.populate();

            checkTree(
                model,
                {.name = "topdir"_L1,
                 .size = 3434110,
                 .progress = 4234.0 / 3434110.0,
                 .priority = TorrentFilesModelEntry::Priority::Mixed,
                 .checkState = Qt::CheckState::PartiallyChecked,

                 .children = {
                     {.name = "subdir1"_L1,
                      .size = 100666,
                      .progress = 4234.0 / 100666.0,
                      .priority = TorrentFilesModelEntry::Priority::Mixed,
                      .checkState = Qt::CheckState::PartiallyChecked,

                      .children =
                          {{.name = "subsubddir"_L1,
                            .size = 100666,
                            .progress = 4234.0 / 100666.0,
                            .priority = TorrentFilesModelEntry::Priority::Mixed,
                            .checkState = Qt::CheckState::PartiallyChecked,

                            .children =
                                {{.name = "file1"_L1,
                                  .size = 666,
                                  .progress = 0.0,
                                  .priority = TorrentFilesModelEntry::Priority::Normal,
                                  .checkState = Qt::CheckState::Checked},
                                 {.name = "file2"_L1,
                                  .size = 100000,
                                  .progress = 4234.0 / 100666.0,
                                  .priority = TorrentFilesModelEntry::Priority::Low,
                                  .checkState = Qt::CheckState::Unchecked}}}}},

                     {.name = "subdir2"_L1,
                      .size = 3333444,
                      .progress = 0.0,
                      .priority = TorrentFilesModelEntry::Priority::Normal,
                      .checkState = Qt::CheckState::Checked,

                      .children = {
                          {.name = "file1"_L1,
                           .size = 3333333,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Normal,
                           .checkState = Qt::CheckState::Checked},
                          {.name = "file2"_L1,
                           .size = 111,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Normal,
                           .checkState = Qt::CheckState::Checked}
                      }}
                 }}
            );
        }

        void checkSetFilesWantedFromRoot() {
            TestTorrentFilesModel model{};
            model.populate();

            QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);

            model.setFilesWanted({model.index(0, 0)}, false);

            const auto actualSignals = actualDataChanged(dataChanged);
            const auto expectedSignals = std::set{
                expectedDataChanged(indexForPath("topdir/subdir1/subsubddir/file1"_L1, model)),
                expectedDataChanged(indexForPath("topdir/subdir1/subsubddir"_L1, model)),
                expectedDataChanged(
                    indexForPath("topdir/subdir2/file1"_L1, model),
                    indexForPath("topdir/subdir2/file2"_L1, model)
                ),
                expectedDataChanged(indexForPath("topdir/subdir1"_L1, model), indexForPath("topdir/subdir2"_L1, model)),
                expectedDataChanged(indexForPath("topdir"_L1, model))
            };

            QCOMPARE(actualSignals, expectedSignals);

            checkTree(
                model,
                {.name = "topdir"_L1,
                 .size = 3434110,
                 .progress = 4234.0 / 3434110.0,
                 .priority = TorrentFilesModelEntry::Priority::Mixed,
                 .checkState = Qt::CheckState::Unchecked,

                 .children = {
                     {.name = "subdir1"_L1,
                      .size = 100666,
                      .progress = 4234.0 / 100666.0,
                      .priority = TorrentFilesModelEntry::Priority::Mixed,
                      .checkState = Qt::CheckState::Unchecked,

                      .children =
                          {{.name = "subsubddir"_L1,
                            .size = 100666,
                            .progress = 4234.0 / 100666.0,
                            .priority = TorrentFilesModelEntry::Priority::Mixed,
                            .checkState = Qt::CheckState::Unchecked,

                            .children =
                                {{.name = "file1"_L1,
                                  .size = 666,
                                  .progress = 0.0,
                                  .priority = TorrentFilesModelEntry::Priority::Normal,
                                  .checkState = Qt::CheckState::Unchecked},
                                 {.name = "file2"_L1,
                                  .size = 100000,
                                  .progress = 4234.0 / 100666.0,
                                  .priority = TorrentFilesModelEntry::Priority::Low,
                                  .checkState = Qt::CheckState::Unchecked}}}}},

                     {.name = "subdir2"_L1,
                      .size = 3333444,
                      .progress = 0.0,
                      .priority = TorrentFilesModelEntry::Priority::Normal,
                      .checkState = Qt::CheckState::Unchecked,

                      .children = {
                          {.name = "file1"_L1,
                           .size = 3333333,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Normal,
                           .checkState = Qt::CheckState::Unchecked},
                          {.name = "file2"_L1,
                           .size = 111,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Normal,
                           .checkState = Qt::CheckState::Unchecked}
                      }}
                 }}
            );
        }

        void checkSetFilesWantedFromFile() {
            TestTorrentFilesModel model{};
            model.populate();

            QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);

            model.setFilesWanted({indexForPath("topdir/subdir1/subsubddir/file2"_L1, model)}, true);

            const auto actualSignals = actualDataChanged(dataChanged);
            const auto expectedSignals = std::set{
                expectedDataChanged(indexForPath("topdir/subdir1/subsubddir/file2"_L1, model)),
                expectedDataChanged(indexForPath("topdir/subdir1/subsubddir"_L1, model)),
                expectedDataChanged(indexForPath("topdir/subdir1"_L1, model)),
                expectedDataChanged(indexForPath("topdir"_L1, model))
            };
            QCOMPARE(actualSignals, expectedSignals);

            checkTree(
                model,
                {.name = "topdir"_L1,
                 .size = 3434110,
                 .progress = 4234.0 / 3434110.0,
                 .priority = TorrentFilesModelEntry::Priority::Mixed,
                 .checkState = Qt::CheckState::Checked,

                 .children = {
                     {.name = "subdir1"_L1,
                      .size = 100666,
                      .progress = 4234.0 / 100666.0,
                      .priority = TorrentFilesModelEntry::Priority::Mixed,
                      .checkState = Qt::CheckState::Checked,

                      .children =
                          {{.name = "subsubddir"_L1,
                            .size = 100666,
                            .progress = 4234.0 / 100666.0,
                            .priority = TorrentFilesModelEntry::Priority::Mixed,
                            .checkState = Qt::CheckState::Checked,

                            .children =
                                {{.name = "file1"_L1,
                                  .size = 666,
                                  .progress = 0.0,
                                  .priority = TorrentFilesModelEntry::Priority::Normal,
                                  .checkState = Qt::CheckState::Checked},
                                 {.name = "file2"_L1,
                                  .size = 100000,
                                  .progress = 4234.0 / 100666.0,
                                  .priority = TorrentFilesModelEntry::Priority::Low,
                                  .checkState = Qt::CheckState::Checked}}}}},

                     {.name = "subdir2"_L1,
                      .size = 3333444,
                      .progress = 0.0,
                      .priority = TorrentFilesModelEntry::Priority::Normal,
                      .checkState = Qt::CheckState::Checked,

                      .children = {
                          {.name = "file1"_L1,
                           .size = 3333333,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Normal,
                           .checkState = Qt::CheckState::Checked},
                          {.name = "file2"_L1,
                           .size = 111,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Normal,
                           .checkState = Qt::CheckState::Checked}
                      }}
                 }}
            );
        }

        void checkSetFilesPriorityFromRoot() {
            TestTorrentFilesModel model{};
            model.populate();

            QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);

            model.setFilesPriority({model.index(0, 0)}, TorrentFilesModelEntry::Priority::Low);

            const auto actualSignals = actualDataChanged(dataChanged);
            const auto expectedSignals = std::set{
                expectedDataChanged(indexForPath("topdir/subdir1/subsubddir/file1"_L1, model)),
                expectedDataChanged(indexForPath("topdir/subdir1/subsubddir"_L1, model)),
                expectedDataChanged(
                    indexForPath("topdir/subdir2/file1"_L1, model),
                    indexForPath("topdir/subdir2/file2"_L1, model)
                ),
                expectedDataChanged(indexForPath("topdir/subdir1"_L1, model), indexForPath("topdir/subdir2"_L1, model)),
                expectedDataChanged(indexForPath("topdir"_L1, model))
            };
            QCOMPARE(actualSignals, expectedSignals);

            checkTree(
                model,
                {.name = "topdir"_L1,
                 .size = 3434110,
                 .progress = 4234.0 / 3434110.0,
                 .priority = TorrentFilesModelEntry::Priority::Low,
                 .checkState = Qt::CheckState::PartiallyChecked,

                 .children = {
                     {.name = "subdir1"_L1,
                      .size = 100666,
                      .progress = 4234.0 / 100666.0,
                      .priority = TorrentFilesModelEntry::Priority::Low,
                      .checkState = Qt::CheckState::PartiallyChecked,

                      .children =
                          {{.name = "subsubddir"_L1,
                            .size = 100666,
                            .progress = 4234.0 / 100666.0,
                            .priority = TorrentFilesModelEntry::Priority::Low,
                            .checkState = Qt::CheckState::PartiallyChecked,

                            .children =
                                {{.name = "file1"_L1,
                                  .size = 666,
                                  .progress = 0.0,
                                  .priority = TorrentFilesModelEntry::Priority::Low,
                                  .checkState = Qt::CheckState::Checked},
                                 {.name = "file2"_L1,
                                  .size = 100000,
                                  .progress = 4234.0 / 100666.0,
                                  .priority = TorrentFilesModelEntry::Priority::Low,
                                  .checkState = Qt::CheckState::Unchecked}}}}},

                     {.name = "subdir2"_L1,
                      .size = 3333444,
                      .progress = 0.0,
                      .priority = TorrentFilesModelEntry::Priority::Low,
                      .checkState = Qt::CheckState::Checked,

                      .children = {
                          {.name = "file1"_L1,
                           .size = 3333333,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Low,
                           .checkState = Qt::CheckState::Checked},
                          {.name = "file2"_L1,
                           .size = 111,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Low,
                           .checkState = Qt::CheckState::Checked}
                      }}
                 }}
            );
        }

        void checkSetFilesPriorityFromFile() {
            TestTorrentFilesModel model{};
            model.populate();

            QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);

            model.setFilesPriority(
                {indexForPath("topdir/subdir1/subsubddir/file2"_L1, model)},
                TorrentFilesModelEntry::Priority::Normal
            );

            const auto actualSignals = actualDataChanged(dataChanged);
            const auto expectedSignals = std::set{
                expectedDataChanged(indexForPath("topdir/subdir1/subsubddir/file2"_L1, model)),
                expectedDataChanged(indexForPath("topdir/subdir1/subsubddir"_L1, model)),
                expectedDataChanged(indexForPath("topdir/subdir1"_L1, model)),
                expectedDataChanged(indexForPath("topdir"_L1, model))
            };
            QCOMPARE(actualSignals, expectedSignals);

            checkTree(
                model,
                {.name = "topdir"_L1,
                 .size = 3434110,
                 .progress = 4234.0 / 3434110.0,
                 .priority = TorrentFilesModelEntry::Priority::Normal,
                 .checkState = Qt::CheckState::PartiallyChecked,

                 .children = {
                     {.name = "subdir1"_L1,
                      .size = 100666,
                      .progress = 4234.0 / 100666.0,
                      .priority = TorrentFilesModelEntry::Priority::Normal,
                      .checkState = Qt::CheckState::PartiallyChecked,

                      .children =
                          {{.name = "subsubddir"_L1,
                            .size = 100666,
                            .progress = 4234.0 / 100666.0,
                            .priority = TorrentFilesModelEntry::Priority::Normal,
                            .checkState = Qt::CheckState::PartiallyChecked,

                            .children =
                                {{.name = "file1"_L1,
                                  .size = 666,
                                  .progress = 0.0,
                                  .priority = TorrentFilesModelEntry::Priority::Normal,
                                  .checkState = Qt::CheckState::Checked},
                                 {.name = "file2"_L1,
                                  .size = 100000,
                                  .progress = 4234.0 / 100666.0,
                                  .priority = TorrentFilesModelEntry::Priority::Normal,
                                  .checkState = Qt::CheckState::Unchecked}}}}},

                     {.name = "subdir2"_L1,
                      .size = 3333444,
                      .progress = 0.0,
                      .priority = TorrentFilesModelEntry::Priority::Normal,
                      .checkState = Qt::CheckState::Checked,

                      .children = {
                          {.name = "file1"_L1,
                           .size = 3333333,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Normal,
                           .checkState = Qt::CheckState::Checked},
                          {.name = "file2"_L1,
                           .size = 111,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Normal,
                           .checkState = Qt::CheckState::Checked}
                      }}
                 }}
            );
        }

        void checkUpdateFiles() {
            TestTorrentFilesModel model{};
            model.populate();

            std::array changed{TestTorrentFilesModel::File{}};

            QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);

            model.updateFiles(std::array{0, 1}, [](size_t index, TorrentFilesModelFile* file) {
                switch (index) {
                case 0:
                    file->setWanted(true);
                    file->setCompletedSize(0);
                    file->setPriority(TorrentFilesModelEntry::Priority::High);
                    break;
                case 1:
                    file->setWanted(true);
                    file->setCompletedSize(0);
                    file->setPriority(TorrentFilesModelEntry::Priority::Normal);
                    break;
                }
            });

            const auto actualSignals = actualDataChanged(dataChanged);
            const auto expectedSignals = std::set{
                expectedDataChanged(
                    indexForPath("topdir/subdir1/subsubddir/file1"_L1, model),
                    indexForPath("topdir/subdir1/subsubddir/file2"_L1, model)
                ),
                expectedDataChanged(indexForPath("topdir/subdir1/subsubddir"_L1, model)),
                expectedDataChanged(indexForPath("topdir/subdir1"_L1, model)),
                expectedDataChanged(indexForPath("topdir"_L1, model))
            };
            QCOMPARE(actualSignals, expectedSignals);

            checkTree(
                model,
                {.name = "topdir"_L1,
                 .size = 3434110,
                 .progress = 0.0,
                 .priority = TorrentFilesModelEntry::Priority::Mixed,
                 .checkState = Qt::CheckState::Checked,

                 .children = {
                     {.name = "subdir1"_L1,
                      .size = 100666,
                      .progress = 0.0,
                      .priority = TorrentFilesModelEntry::Priority::Mixed,
                      .checkState = Qt::CheckState::Checked,

                      .children =
                          {{.name = "subsubddir"_L1,
                            .size = 100666,
                            .progress = 0.0,
                            .priority = TorrentFilesModelEntry::Priority::Mixed,
                            .checkState = Qt::CheckState::Checked,

                            .children =
                                {{.name = "file1"_L1,
                                  .size = 666,
                                  .progress = 0.0,
                                  .priority = TorrentFilesModelEntry::Priority::High,
                                  .checkState = Qt::CheckState::Checked},
                                 {.name = "file2"_L1,
                                  .size = 100000,
                                  .progress = 0.0,
                                  .priority = TorrentFilesModelEntry::Priority::Normal,
                                  .checkState = Qt::CheckState::Checked}}}}},

                     {.name = "subdir2"_L1,
                      .size = 3333444,
                      .progress = 0.0,
                      .priority = TorrentFilesModelEntry::Priority::Normal,
                      .checkState = Qt::CheckState::Checked,

                      .children = {
                          {.name = "file1"_L1,
                           .size = 3333333,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Normal,
                           .checkState = Qt::CheckState::Checked},
                          {.name = "file2"_L1,
                           .size = 111,
                           .progress = 0.0,
                           .priority = TorrentFilesModelEntry::Priority::Normal,
                           .checkState = Qt::CheckState::Checked}
                      }}
                 }}
            );
        }

    private:
        struct ExpectedData {
            QLatin1String name;
            long long size;
            double progress;
            TorrentFilesModelEntry::Priority priority;
            Qt::CheckState checkState;

            std::vector<ExpectedData> children{};
        };

        void checkTree(BaseTorrentFilesModel& model, const ExpectedData& expectedRootEntry) {
            checkEntryPresentation(model, {}, 0, expectedRootEntry, 0);
        }

        void checkEntryPresentation(
            BaseTorrentFilesModel& model, const QModelIndex& parent, int row, const ExpectedData& expected, int depth
        ) {
            info().log("{}* {}", u" "_s.repeated(depth), expected.name);

            const auto data = [&](BaseTorrentFilesModel::Column column, int role = Qt::DisplayRole) {
                return model.index(row, static_cast<int>(column), parent).data(role);
            };

            QCOMPARE(data(BaseTorrentFilesModel::Column::Name).toString(), expected.name);
            QCOMPARE(data(BaseTorrentFilesModel::Column::Size).toString(), formatByteSize(expected.size));
            QCOMPARE(data(BaseTorrentFilesModel::Column::Progress).toString(), formatProgress(expected.progress));
            QCOMPARE(
                data(BaseTorrentFilesModel::Column::Name, Qt::CheckStateRole).value<Qt::CheckState>(),
                expected.checkState
            );
            const auto expectedPriorityString = [&] {
                switch (expected.priority) {
                case TorrentFilesModelEntry::Priority::Low:
                    return qApp->translate("tremotesf", "Low");
                case TorrentFilesModelEntry::Priority::Normal:
                    return qApp->translate("tremotesf", "Normal");
                case TorrentFilesModelEntry::Priority::High:
                    return qApp->translate("tremotesf", "High");
                case TorrentFilesModelEntry::Priority::Mixed:
                    return qApp->translate("tremotesf", "Mixed");
                }
                return QString{};
            }();
            QCOMPARE(data(BaseTorrentFilesModel::Column::Priority).toString(), expectedPriorityString);

            const auto thisIndex = model.index(row, 0, parent);
            QCOMPARE(model.rowCount(thisIndex), static_cast<int>(expected.children.size()));
            if (!expected.children.empty()) {
                int i = 0;
                for (const auto& child : expected.children) {
                    checkEntryPresentation(model, thisIndex, i, child, depth + 1);
                    ++i;
                }
            }
        }
    };
}

QTEST_GUILESS_MAIN(tremotesf::TorrentFilesModelTest)

#include "torrentfilesmodel_test.moc"
