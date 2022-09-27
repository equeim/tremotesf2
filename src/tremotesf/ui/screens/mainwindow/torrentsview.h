#ifndef TREMOTESF_TORRENTSVIEW_H
#define TREMOTESF_TORRENTSVIEW_H

#include "tremotesf/ui/widgets/basetreeview.h"

namespace tremotesf
{
    class TorrentsProxyModel;

    class TorrentsView : public BaseTreeView
    {
        Q_OBJECT
    public:
        TorrentsView(TorrentsProxyModel* model, QWidget* parent = nullptr);
        ~TorrentsView() override;
    };
}

#endif // TREMOTESF_TORRENTSVIEW_H
