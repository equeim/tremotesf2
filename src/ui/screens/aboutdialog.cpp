// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "aboutdialog.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <KTitleWidget>

#include "fileutils.h"
#include "literals.h"

namespace tremotesf {
    AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
        //: "About" dialog title
        setWindowTitle(qApp->translate("tremotesf", "About"));

        auto layout = new QVBoxLayout(this);

        auto titleWidget = new KTitleWidget(this);
        titleWidget->setIcon(qApp->windowIcon(), KTitleWidget::ImageLeft);
        titleWidget->setText(QString::fromLatin1("%1 %2").arg(TREMOTESF_APP_NAME ""_l1, qApp->applicationVersion()));
        layout->addWidget(titleWidget);

        auto tabWidget = new QTabWidget(this);

        auto aboutPage = new QWidget(this);
        auto aboutPageLayout = new QVBoxLayout(aboutPage);
        auto aboutPageLabel = new QLabel(
            //: "About" dialog text
            qApp->translate(
                "tremotesf",
                "<p>&#169; 2015-2024 Alexey Rochev &lt;<a "
                "href=\"mailto:equeim@gmail.com\">equeim@gmail.com</a>&gt;</p>\n"
                "<p>Source code: <a "
                "href=\"https://github.com/equeim/tremotesf2\">https://github.com/equeim/tremotesf2</a></p>\n"
                "<p>Translations: <a "
                "href=\"https://www.transifex.com/equeim/tremotesf\">https://www.transifex.com/equeim/tremotesf</a></p>"
            ),
            this
        );
        QObject::connect(aboutPageLabel, &QLabel::linkActivated, this, &QDesktopServices::openUrl);
        aboutPageLayout->addWidget(aboutPageLabel);

        //: "About" dialog's "About" tab title
        tabWidget->addTab(aboutPage, qApp->translate("tremotesf", "About"));

        auto authorsPage = new QWidget(this);
        auto authorsPageLayout = new QVBoxLayout(authorsPage);
        auto authorsWidget = new QTextBrowser(this);
        authorsWidget->setHtml(
            QString(readFile(":/authors.html"))
                .arg(qApp->translate("tremotesf", "Maintainer"), qApp->translate("tremotesf", "Contributor"))
        );
        authorsWidget->setOpenExternalLinks(true);
        authorsPageLayout->addWidget(authorsWidget);
        authorsPageLayout->addStretch();
        //: "About" dialog's "Authors" tab title
        tabWidget->addTab(authorsPage, qApp->translate("tremotesf", "Authors"));

        auto translatorsWidget = new QTextBrowser(this);
        translatorsWidget->setHtml(readFile(":/translators.html"));
        translatorsWidget->setOpenExternalLinks(true);
        //: "About" dialog's "Translators" tab title
        tabWidget->addTab(translatorsWidget, qApp->translate("tremotesf", "Translators"));

        auto licenseWidget = new QTextBrowser(this);
        licenseWidget->setHtml(readFile(":/license.html"));
        licenseWidget->setOpenExternalLinks(true);
        //: "About" dialog's "License" tab title
        tabWidget->addTab(licenseWidget, qApp->translate("tremotesf", "License"));

        layout->addWidget(tabWidget);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &AboutDialog::reject);
        layout->addWidget(dialogButtonBox);

        dialogButtonBox->button(QDialogButtonBox::Close)->setDefault(true);
    }

    QSize AboutDialog::sizeHint() const { return minimumSizeHint().expandedTo(QSize(420, 384)); }
}
