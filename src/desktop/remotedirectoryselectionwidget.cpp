#include "remotedirectoryselectionwidget.h"

#include "servers.h"

#include <QCoreApplication>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

namespace tremotesf
{
    RemoteDirectorySelectionWidget::RemoteDirectorySelectionWidget(const QString& directory, bool serverIsLocal, QWidget* parent)
        : FileSelectionWidget(true, QString(), serverIsLocal, parent)
    {
        const bool mounted = Servers::instance()->currentServerHasMountedDirectories();
        selectionButton()->setEnabled(serverIsLocal || mounted);
        setLineEditText(directory);
        if (mounted && !serverIsLocal) {
            const auto onTextEdited = [=](const QString& text) {
                const QString directory(Servers::instance()->fromRemoteToLocalDirectory(text));
                if (!directory.isEmpty()) {
                    setFileDialogDirectory(directory);
                }
            };
            QObject::connect(lineEdit(), &QLineEdit::textEdited, this, onTextEdited);
            const QString localDownloadDirectory(Servers::instance()->fromRemoteToLocalDirectory(directory));
            if (localDownloadDirectory.isEmpty()) {
                setFileDialogDirectory(Servers::instance()->firstLocalDirectory());
            } else {
                setFileDialogDirectory(localDownloadDirectory);
            }

            QObject::connect(this, &FileSelectionWidget::fileDialogAccepted, this, [=](const QString& filePath) {
                const QString directory(Servers::instance()->fromLocalToRemoteDirectory(filePath));
                if (directory.isEmpty()) {
                    QMessageBox::warning(this, qApp->translate("tremotesf", "Error"), qApp->translate("tremotesf", "Selected directory should be inside mounted directory"));
                } else {
                    setLineEditText(directory);
                    setFileDialogDirectory(filePath);
                }
            });
        }
    }
}
