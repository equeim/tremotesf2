#ifndef TREMOTESF_ABOUTDIALOG_H
#define TREMOTESF_ABOUTDIALOG_H

#include <QDialog>

namespace tremotesf
{
    class AboutDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit AboutDialog(QWidget* parent = nullptr);
        QSize sizeHint() const override;
    };
}

#endif // TREMOTESF_ABOUTDIALOG_H
