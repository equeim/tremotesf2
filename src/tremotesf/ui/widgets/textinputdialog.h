#ifndef TREMOTESF_TEXTINPUTDIALOG_H
#define TREMOTESF_TEXTINPUTDIALOG_H

#include <QDialog>

class QLineEdit;
class QPlainTextEdit;

namespace tremotesf
{
    class TextInputDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit TextInputDialog(const QString& title,
                                 const QString& labelText,
                                 const QString& text,
                                 const QString& okButtonText,
                                 bool multiline,
                                 QWidget* parent = nullptr);
        QSize sizeHint() const override;
        QString text() const;

    private:
        QLineEdit* mLineEdit = nullptr;
        QPlainTextEdit* mPlainTextEdit = nullptr;
    };
}

#endif
