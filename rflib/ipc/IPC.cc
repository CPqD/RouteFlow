#include "IPC.h"

string IPCMessageService::get_id() {
    return this->id;
}

void IPCMessageService::set_id(string id) {
    this->id = id;
}
