#ifndef TREMOTESF_IPCSERVER_H
#define TREMOTESF_IPCSERVER_H

#include <QObject>

namespace tremotesf
{
    class IpcServer : public QObject
    {
        Q_OBJECT
    public:
        static IpcServer* createInstance(QObject* parent = nullptr);

        inline explicit IpcServer(QObject* parent = nullptr) : QObject(parent) {};

    signals:
        void windowActivationRequested(const QString& torrentHash, const QByteArray& startupNoficationId);
        void torrentsAddingRequested(const QStringList& files, const QStringList& urls, const QByteArray& startupNoficationId);
    };
}

#endif // TREMOTESF_IPCSERVER_H
