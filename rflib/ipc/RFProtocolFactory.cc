#include "RFProtocolFactory.h"

IPCMessage* RFProtocolFactory::buildForType(int type) {
    switch (type) {
        case VM_REGISTER_REQUEST:
            return new VMRegisterRequest();
        case VM_REGISTER_RESPONSE:
            return new VMRegisterResponse();
        case VM_CONFIG:
            return new VMConfig();
        case DATAPATH_CONFIG:
            return new DatapathConfig();
        case ROUTE_INFO:
            return new RouteInfo();
        case FLOW_MOD:
            return new FlowMod();
        case DATAPATH_JOIN:
            return new DatapathJoin();
        case DATAPATH_LEAVE:
            return new DatapathLeave();
        case VM_MAP:
            return new VMMap();
        default:
            return NULL;
    }
}

