#include "textinputdialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace tremotesf
{
    TextInputDialog::TextInputDialog(const QString& title,
                                     const QString& labelText,
                                     const QString& text,
                                     const QString& okButtonText,
                                     bool multiline,
                                     QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(title);

        auto layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        auto label = new QLabel(labelText, this);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        layout->addWidget(label);

        if (multiline) {
            mPlainTextEdit = new QPlainTextEdit(text, this);
            layout->addWidget(mPlainTextEdit);
        } else {
            mLineEdit = new QLineEdit(text, this);
            layout->addWidget(mLineEdit);
        }

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        if (!okButtonText.isEmpty()) {
            dialogButtonBox->button(QDialogButtonBox::Ok)->setText(okButtonText);
        }
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &TextInputDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &TextInputDialog::reject);

        if (text.isEmpty()) {
            dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        }

        const auto onTextChanged = [=](const QString& text) {
            if (text.isEmpty()) {
                dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            } else {
                dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            }
        };

        if (multiline) {
            QObject::connect(mPlainTextEdit, &QPlainTextEdit::textChanged, this, [=] {
                onTextChanged(mPlainTextEdit->toPlainText());
            });
        } else {
            QObject::connect(mLineEdit, &QLineEdit::textChanged, this, onTextChanged);
        }

        layout->addWidget(dialogButtonBox);

        setMinimumSize(minimumSizeHint());
    }

    QSize TextInputDialog::sizeHint() const
    {
        return minimumSizeHint().expandedTo(QSize(256, 0));
    }

    QString TextInputDialog::text() const
    {
        return mLineEdit ? mLineEdit->text() : mPlainTextEdit->toPlainText();
    }
}
