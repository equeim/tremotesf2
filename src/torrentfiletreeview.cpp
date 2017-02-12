#include "torrentfiletreeview.h"
#include "torrentfiletreeview.moc"

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QSortFilterProxyModel>

#include "enums.h"
#include "torrentfilemodel.h"

TorrentFileTreeDelegate::TorrentFileTreeDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void TorrentFileTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.column() == TorrentFileTreeColumns::Progress) {
        QStyleOptionProgressBar progressBar;
        progressBar.rect = option.rect;
        progressBar.minimum = 0;
        progressBar.maximum = 1000;
        progressBar.progress = index.data(TorrentFileModelRoles::Progress).toInt();
        progressBar.text = index.data().toString();
        progressBar.textVisible = true;
        QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBar, painter);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

TorrentFileTreeView::TorrentFileTreeView(TorrentFileModel* model, QWidget* parent)
    : QTreeView(parent)
{
    m_model = model;

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    setAlternatingRowColors(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setItemDelegate(new TorrentFileTreeDelegate(this));
    setModel(m_proxyModel);

    QObject::connect(m_model, &QAbstractItemModel::modelReset, this, &TorrentFileTreeView::enableSorting);
}

void TorrentFileTreeView::enableSorting()
{
    setSortingEnabled(true);
    expand(indexAt(QPoint(0, 0)));
}
