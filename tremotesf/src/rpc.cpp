#include "rpc.h"

#include <QCoreApplication>

namespace tremotesf
{
    Rpc::Rpc(QObject* parent) : libtremotesf::Rpc(nullptr, parent)
    {
        QObject::connect(this, &Rpc::statusChanged, this, &Rpc::statusStringChanged);
        QObject::connect(this, &Rpc::errorChanged, this, &Rpc::statusStringChanged);
    }

    QString Rpc::statusString() const
    {
        switch (status()) {
        case Disconnected:
            switch (error()) {
            case NoError:
                return qApp->translate("tremotesf", "Disconnected");
            case TimedOut:
                return qApp->translate("tremotesf", "Timed out");
            case ConnectionError:
                return qApp->translate("tremotesf", "Connection error");
            case AuthenticationError:
                return qApp->translate("tremotesf", "Authentication error");
            case ParseError:
                return qApp->translate("tremotesf", "Parse error");
            case ServerIsTooNew:
                return qApp->translate("tremotesf", "Server is too new");
            case ServerIsTooOld:
                return qApp->translate("tremotesf", "Server is too old");
            }
            break;
        case Connecting:
            return qApp->translate("tremotesf", "Connecting...");
        case Connected:
            return qApp->translate("tremotesf", "Connected");
        }

        return QString();
    }
}
