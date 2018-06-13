#ifndef TREMOTESF_RPC_H
#define TREMOTESF_RPC_H

#include "libtremotesf/rpc.h"

namespace tremotesf
{
    class Rpc : public libtremotesf::Rpc
    {
        Q_OBJECT
        Q_PROPERTY(QString statusString READ statusString NOTIFY statusStringChanged)
    public:
        explicit Rpc(QObject* parent = nullptr);
        QString statusString() const;
    signals:
        void statusStringChanged();
    };
}

#endif // TREMOTESF_RPC_H
