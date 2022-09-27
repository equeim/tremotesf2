#ifndef TREMOTESF_BASETREEVIEW_H
#define TREMOTESF_BASETREEVIEW_H

#include <QTreeView>

namespace tremotesf
{
    class BaseTreeView : public QTreeView
    {
        Q_OBJECT
    public:
        explicit BaseTreeView(QWidget* parent = nullptr);
    };
}

#endif // TREMOTESF_BASETREEVIEW_H
