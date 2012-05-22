#ifndef __IPC_H__
#define __IPC_H__

#include <string>

using namespace std;

/** Abstract class for a message transmited through the IPC */
class IPCMessage {
    public:
        /** Get the type of the message.
        * @return the type of the message */
        virtual int get_type() = 0;
        
        /** Sets the fields of this message to those given in the BSON data.
        * @param data the BSON data from which to load */
        virtual void from_BSON(const char* data) = 0;
        
        /** Creates a BSON representation of this message.
        * @return the binary representation of the message in BSON */        
        virtual const char* to_BSON() = 0;

        /**  Get a string representation of the message.
        * @return the string representation of the message */              
        virtual string str() = 0;
};

/** Abstract class for an IPC message factory. 
A factory is responsible for creating message objects for a given type. */
class IPCMessageFactory {
    public:
        /** This method should build messages and return them. 
        Implementations can initialize the message with default application 
        values, for example.
        @param type the type of the message to build
        @return a pointer to the built message */
        virtual IPCMessage* buildForType(int type) = 0;
};

/** Abstract class for an IPC message processor. 
A processor deals with received messages and implement behavior based on the 
needs of the application. */
class IPCMessageProcessor {
    public:
        /** This method should process messages based on the desired behavior.
        @param from the message sender
        @param to the message receiver
        @param channel the channel the message was sent
        @param msg the message to process
        @return true if the message was successfully processed, false otherwise
        */
        virtual bool process(const string &from, const string &to, const string &channel, IPCMessage& msg) = 0;
};

/** Abstract class for an IPC messaging service using the Publish/Subscribe 
model. 
Every time a service is created, an ID is supplied by its user, which tells the
publish/subscribe model where to deliver the messages sent to that ID.
*/
class IPCMessageService {
    public:
        /** Returns the id of the service user.
        @return a string with the user ID */
        string get_id();
        
        /** Sets the id of the service user.
        @param id a string with the user ID */
        void set_id(string id);
        
        /** Listen to messages. Empty messages are built using the factory,
        populated based on the received data and sent to processing by the
        processor. The method can be blocking or not.
        @param channelId the channel to listen to messages on
        @param factory the message factory
        @param processor the message processor
        @param block true if blocking behavior is desired, false otherwise */
        virtual void listen(const string &channelId, IPCMessageFactory *factory, IPCMessageProcessor *processor, bool block=true) = 0;
        
        /** Send a message to another user on a channel.
        @param channelId the channel to send the message to
        @param to the user to send the message to
        @param msg the message
        @return true if the message was sent, false otherwise */        
        virtual bool send(const string &channelId, const string &to, IPCMessage& msg) = 0;
        
    private:
        string id;
};

#endif /* __IPC_H__ */
