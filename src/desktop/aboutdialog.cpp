/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

#include "../utils.h"

namespace tremotesf
{
    AboutDialog::AboutDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(qApp->translate("tremotesf", "About"));

        auto layout = new QVBoxLayout(this);

        auto titleWidget = new KTitleWidget(this);
        titleWidget->setIcon(qApp->windowIcon(), KTitleWidget::ImageLeft);
        titleWidget->setText(QString::fromLatin1("%1 %2").arg(QLatin1String(TREMOTESF_APP_NAME), qApp->applicationVersion()));
        layout->addWidget(titleWidget);

        auto tabWidget = new QTabWidget(this);

        auto aboutPage = new QWidget(this);
        auto aboutPageLayout = new QVBoxLayout(aboutPage);
        auto aboutPageLabel = new QLabel(qApp->translate("tremotesf", "<p>&#169; 2015-2020 Alexey Rochev &lt;<a href=\"mailto:equeim@gmail.com\">equeim@gmail.com</a>&gt;</p>\n"
                                                                      "<p>Source code: <a href=\"https://github.com/equeim/tremotesf2\">https://github.com/equeim/tremotesf2</a></p>\n"
                                                                      "<p>Translations: <a href=\"https://www.transifex.com/equeim/tremotesf\">https://www.transifex.com/equeim/tremotesf</a></p>"),
                                         this);
        QObject::connect(aboutPageLabel, &QLabel::linkActivated, this, &QDesktopServices::openUrl);
        aboutPageLayout->addWidget(aboutPageLabel);

        tabWidget->addTab(aboutPage, qApp->translate("tremotesf", "About"));

        auto authorsPage = new QWidget(this);
        auto authorsPageLayout = new QVBoxLayout(authorsPage);
        auto authorsWidget = new QTextBrowser(this);
        authorsWidget->setText(Utils::readTextResource(":/authors.html"));
        authorsWidget->setOpenExternalLinks(true);
        authorsPageLayout->addWidget(authorsWidget);
        authorsPageLayout->addStretch();
        tabWidget->addTab(authorsPage, qApp->translate("tremotesf", "Authors"));

        auto translatorsWidget = new QTextBrowser(this);
        translatorsWidget->setText(Utils::readTextResource(":/translators.html"));
        translatorsWidget->setOpenExternalLinks(true);
        tabWidget->addTab(translatorsWidget, qApp->translate("tremotesf", "Translators"));

        auto licenseWidget = new QTextBrowser(this);
        licenseWidget->setText(Utils::readTextResource(":/license.html"));
        licenseWidget->setOpenExternalLinks(true);
        tabWidget->addTab(licenseWidget, qApp->translate("tremotesf", "License"));

        layout->addWidget(tabWidget);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &AboutDialog::reject);
        layout->addWidget(dialogButtonBox);

        dialogButtonBox->button(QDialogButtonBox::Close)->setDefault(true);

        setMinimumSize(minimumSizeHint());
    }

    QSize AboutDialog::sizeHint() const
    {
        return minimumSizeHint().expandedTo(QSize(420, 384));
    }
}
