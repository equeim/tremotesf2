#include "jsonparser.h"

#include <QJsonDocument>
#include <QThread>
#include <QVariant>

namespace tremotesf
{
    namespace
    {
        class Worker : public QObject
        {
            Q_OBJECT
        public:
            void parse(const QByteArray& data)
            {
                QJsonParseError error;
                const QVariantMap result(QJsonDocument::fromJson(data, &error).toVariant().toMap());
                emit done(result,
                          error.error == QJsonParseError::NoError);
            }
        signals:
            void done(const QVariantMap& parseResult, bool success);
        };
    }

    JsonParser::JsonParser(const QByteArray& data, QObject* parent)
        : QObject(parent),
          mWorkerThread(new QThread(this))
    {
        auto worker = new Worker();
        worker->moveToThread(mWorkerThread);
        QObject::connect(mWorkerThread, &QThread::finished, worker, &QObject::deleteLater);
        QObject::connect(this, &JsonParser::requestParse, worker, &Worker::parse);
        QObject::connect(worker, &Worker::done, this, &JsonParser::done);
        mWorkerThread->start();
        emit requestParse(data);
    }

    JsonParser::~JsonParser()
    {
        mWorkerThread->quit();
        mWorkerThread->wait();
    }
}

#include "jsonparser.moc"
