#!/usr/bin/env python
import sys
import pymongo

import rflib.defs as defs
import rflib.ipc.MongoIPC as MongoIPC
from rflib.ipc.RFProtocolFactory import RFProtocolFactory

def usage():
    print('Usage: %s <channel>\n' % sys.argv[0])
    print('channel: "rfclient" or "rfproxy"')
    sys.exit(1)

def parse_args(args=sys.argv):
    if (len(sys.argv) < 2):
        return None
    if (args[1] == 'rfclient'):
        return defs.RFCLIENT_RFSERVER_CHANNEL
    if (args[1] == 'rfproxy'):
        return defs.RFSERVER_RFPROXY_CHANNEL
    return None

if __name__ == '__main__':
    connection = pymongo.Connection('localhost', 27017)
    db = connection[defs.MONGO_DB_NAME]
    channel = parse_args()

    if (channel == None):
        usage()

    factory = RFProtocolFactory()
    for entry in db[channel].find():
        msg = MongoIPC.take_from_envelope(entry, factory)
        print(msg)
