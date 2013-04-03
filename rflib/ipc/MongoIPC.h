#ifndef __MONGOIPC_H__
#define __MONGOIPC_H__

#include <mongo/client/dbclient.h>
#include "IPC.h"

#define FROM_FIELD "from"
#define TO_FIELD "to"
#define TYPE_FIELD "type"
#define READ_FIELD "read"
#define CONTENT_FIELD "content"

// 1 MB for the capped collection
#define CC_SIZE 1048576

// Handle a maximum of 10 messages at a time
#define PENDINGLIMIT 10

mongo::BSONObj putInEnvelope(const string &from, const string &to, IPCMessage &msg);
IPCMessage* takeFromEnvelope(mongo::BSONObj envelope, IPCMessageFactory *factory);

/** An IPC message service that uses MongoDB as its backend. */
class MongoIPCMessageService : public IPCMessageService {
    public:
        /** Creates and starts an IPC message service using MongoDB.
        @param address the address and port of the mongo server in the format 
                       address:port
        @param db the name of the database to use
        @param id the ID of this IPC service user */
        MongoIPCMessageService(const string &address, const string db, const string id);
        virtual void listen(const string &channelId, IPCMessageFactory *factory, IPCMessageProcessor *processor, bool block=true);
        virtual bool send(const string &channelId, const string &to, IPCMessage& msg);
        
    private:
        string db;
        string address;
        mongo::DBClientConnection producerConnection;
        boost::mutex ipcMutex;
        void listenWorker(const string &channelId, IPCMessageFactory *factory, IPCMessageProcessor *processor);
        void createChannel(mongo::DBClientConnection &con, const string &ns);
        void connect(mongo::DBClientConnection &connection, const string &address);
};

#endif /* __MONGOIPC_H__ */
