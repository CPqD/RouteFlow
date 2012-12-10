#ifndef __RFPROTOCOLFACTORY_H__
#define __RFPROTOCOLFACTORY_H__

#include "IPC.h"
#include "RFProtocol.h"

class RFProtocolFactory : public IPCMessageFactory {
    protected:
        IPCMessage* buildForType(int type);
};

#endif /* __RFPROTOCOLFACTORY_H__ */
