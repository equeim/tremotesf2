#ifndef TREMOTESF_SERVERSTATSDIALOG_H
#define TREMOTESF_SERVERSTATSDIALOG_H

#include <QDialog>

namespace tremotesf
{
    class Rpc;

    class ServerStatsDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit ServerStatsDialog(Rpc* rpc, QWidget* parent = nullptr);
        QSize sizeHint() const override;
    };
}

#endif // TREMOTESF_SERVERSTATSDIALOG_H
