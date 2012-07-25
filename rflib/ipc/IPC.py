class IPCMessage:
    def get_type(self):
        raise NotImplementedError
    
    def from_bson(self, data):
        raise NotImplementedError
        
    def to_bson(self):
        raise NotImplementedError
        
    def str(self):
        raise NotImplementedError

    def __str__(self):
        return self.str()

class IPCMessageFactory:
    def build_for_type(self, type_):
        raise NotImplementedError
        
class IPCMessageProcessor:
    def process(self, from_, to, channel, msg):
        raise NotImplementedError
        
class IPCMessageService:
    def get_id(self):
        return self._id
    
    def set_id(self, id_):
        self._id = id_
        
    def listen(channel_id, factory, processor, block=True):
        raise NotImplementedError
        
    def send(channel_id, to, msg):
        raise NotImplementedError
