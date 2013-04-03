#include "MongoIPC.h"
#include <boost/thread.hpp>

MongoIPCMessageService::MongoIPCMessageService(const string &address, const string db, const string id) {
    this->set_id(id);
    this->db = db;
    this->address = address;
    this->connect(producerConnection, this->address);
}

void MongoIPCMessageService::createChannel(mongo::DBClientConnection &con, const string &ns) {
    con.createCollection(ns, CC_SIZE, true);
    con.ensureIndex(ns, BSON("_id" << 1));
    con.ensureIndex(ns, BSON(TO_FIELD << 1));
}

void MongoIPCMessageService::connect(mongo::DBClientConnection &connection, const string &address) {
    try {
        connection.connect(address);
    }
    catch(mongo::DBException &e) {
        cout << "Exception: " << e.what() << endl;
        exit(1);
    }
}
void MongoIPCMessageService::listenWorker(const string &channelId, IPCMessageFactory *factory, IPCMessageProcessor *processor) {
    string ns = this->db + "." + channelId;

    mongo::DBClientConnection connection;
    this->connect(connection, this->address);

    this->createChannel(connection, ns);
    mongo::Query query = QUERY(TO_FIELD << this->get_id() << READ_FIELD << false).sort("$natural");
    while (true) {
        auto_ptr<mongo::DBClientCursor> cur = connection.query(ns, query,
                                                               PENDINGLIMIT);
        while (cur->more()) {
            mongo::BSONObj envelope = cur->nextSafe();
            IPCMessage *msg = takeFromEnvelope(envelope, factory);
            processor->process(envelope["from"].String(), this->get_id(), channelId, *msg);
            delete msg;

            connection.update(ns,
                QUERY("_id" << envelope["_id"]), 
                BSON("$set" << BSON(READ_FIELD << true)),
                false, true);
        }
        usleep(50000); // 50ms
    }
}

void MongoIPCMessageService::listen(const string &channelId, IPCMessageFactory *factory, IPCMessageProcessor *processor, bool block) {
    boost::thread t(&MongoIPCMessageService::listenWorker, this, channelId, factory, processor);
    if (block)
        t.join();
    else
        t.detach();
}

bool MongoIPCMessageService::send(const string &channelId, const string &to, IPCMessage& msg) {
    boost::lock_guard<boost::mutex> lock(ipcMutex);
    string ns = this->db + "." + channelId;

    this->createChannel(producerConnection, ns);
    this->producerConnection.insert(ns, putInEnvelope(this->get_id(), to, msg));

    return true;
}

mongo::BSONObj putInEnvelope(const string &from, const string &to, IPCMessage &msg) {
    mongo::BSONObjBuilder envelope;

    envelope.genOID();
    envelope.append(FROM_FIELD, from);
    envelope.append(TO_FIELD, to);
    envelope.append(TYPE_FIELD, msg.get_type());
    envelope.append(READ_FIELD, false);

    const char* data = msg.to_BSON();
    envelope.append(CONTENT_FIELD, mongo::BSONObj(data));
    delete data;

    return envelope.obj();
}

IPCMessage* takeFromEnvelope(mongo::BSONObj envelope, IPCMessageFactory *factory) {
   IPCMessage* msg = factory->buildForType(envelope[TYPE_FIELD].Int());
   msg->from_BSON(envelope[CONTENT_FIELD].Obj().objdata());
   return msg;
}
