#include "textinputdialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace tremotesf
{
    TextInputDialog::TextInputDialog(const QString& title,
                                     const QString& labelText,
                                     const QString& text,
                                     QWidget* parent)
        : QDialog(parent),
          mLineEdit(new QLineEdit(text, this))
    {
        setWindowTitle(title);

        auto layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        auto label = new QLabel(labelText, this);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        layout->addWidget(label);
        layout->addWidget(mLineEdit);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &TextInputDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &TextInputDialog::reject);

        if (text.isEmpty()) {
            dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        }
        QObject::connect(mLineEdit, &QLineEdit::textChanged, this, [=](const QString& text) {
            if (text.isEmpty()) {
                dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            } else {
                dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            }
        });

        layout->addWidget(dialogButtonBox);
    }

    QSize TextInputDialog::sizeHint() const
    {
        return layout()->totalMinimumSize().expandedTo(QSize(256, 0));
    }

    QString TextInputDialog::text() const
    {
        return mLineEdit->text();
    }
}
