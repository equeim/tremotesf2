#ifndef TREMOTESF_FILEMANAGERLAUNCHER_H
#define TREMOTESF_FILEMANAGERLAUNCHER_H

#include <utility>
#include <vector>
#include <QObject>
#include <QPointer>
#include <QString>

class QWidget;

namespace tremotesf {
    namespace impl {
        class FileManagerLauncher : public QObject {
            Q_OBJECT
        public:
            static FileManagerLauncher* createInstance();
            void launchFileManagerAndSelectFiles(const std::vector<QString>& files, QPointer<QWidget> parentWidget);

        protected:
            FileManagerLauncher() = default;
            virtual void launchFileManagerAndSelectFiles(const std::vector<std::pair<QString, std::vector<QString>>>& directories, QPointer<QWidget> parentWidget);
            virtual void fallbackForDirectory(const QString& dirPath, QPointer<QWidget> parentWidget);

        signals:
            void done();
        };
    }

    void launchFileManagerAndSelectFiles(const std::vector<QString>& files, QWidget* parentWidget);
}

#endif
