#ifndef TREMOTESF_COMMONDELEGATE_H
#define TREMOTESF_COMMONDELEGATE_H

#include <QStyledItemDelegate>

namespace tremotesf
{
    class CommonDelegate : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        explicit CommonDelegate(int progressBarColumn, int progressBarRole, int textElideModeRole, QObject* parent = nullptr);
        inline explicit CommonDelegate(QObject* parent = nullptr) : CommonDelegate(-1, -1, -1, parent) {}
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        bool helpEvent(QHelpEvent* event,
                       QAbstractItemView* view,
                       const QStyleOptionViewItem& option,
                       const QModelIndex& index) override;

    private:
        int mProgressBarColumn;
        int mProgressBarRole;
        int mTextElideModeRole;
    };
}

#endif // TREMOTESF_COMMONDELEGATE_H
