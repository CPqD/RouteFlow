import struct

import socket
import select

## This module provides library to send and receive messages to NOX's messenger
#
# This is a rewrite of noxmsg.py from OpenRoads (OpenFlow Wireless)
#
# @author ykk (Stanford University)
# @date January, 2010
# @see messenger

def stringarray(string):
    """Output array of binary values in string.
    """
    arrstr = ""
    if (len(string) != 0):
        for i in range(0,len(string)):
            arrstr += "%x " % struct.unpack("=B",string[i])[0]
    return arrstr

def printarray(string):
    """Print array of binary values
    """
    print "Array of length "+str(len(string))
    print stringarray(string)

class channel:
    """TCP channel to communicate to NOX with.
    """
    def __init__(self,ipAddr,portNo=2603,debug=False):
        """Initialize with socket
        """
        ##Socket reference for channel
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((ipAddr,portNo))
        self.debug = debug
        ##Internal buffer for receiving
        self.__buffer = ""
        ##Internal reference to header
        self.__header = messenger_msg()

    def baresend(self, msg):
        """Send bare message"""
        self.sock.send(msg)

    def send(self,msg):
        """Send message
        """
        msgh = messenger_msg()
        remaining = msgh.unpack(msg)
        if (msgh.length != len(msg)):
            msgh.length = len(msg)
            msg = msgh.pack()+remaining
        self.baresend(msg)
        if (self.debug):
            printarray(msg)
        
    def receive(self, recvLen=0,timeout=0):
        """Receive command
        If length == None, nonblocking receive (return None or message)
        With nonblocking receive, timeout is used for select statement

        If length is zero, return single message
        """            
        if (recvLen==0):
            #Receive full message
            msg=""
            length=len(self.__header)
            while (len(msg) < length):
                msg+=self.sock.recv(1)
                #Get length
                if (len(msg) == length):
                    self.__header.unpack(msg)
                    length=self.__header.length
            return msg
        elif (recvLen==None):
            #Non-blocking receive
            ready_to_read = select.select([self.sock],[],[],timeout)[0]
            if (ready_to_read):
                self.__buffer += self.sock.recv(1)
            if (len(self.__buffer) >= len(self.__header)):
                self.__header.unpack(self.__buffer)
                if (self.__header.length == len(self.__buffer)):
                    msg = self.__buffer
                    self.__buffer = ""
                    return msg
            return None
        else:
            #Fixed length blocking receive
            return self.sock.recv(recvLen)

    def __del__(self):
        """Terminate connection
        """
        emsg = messenger_msg()
        emsg.type = MSG_DISCONNECT
        emsg.length = len(emsg)
        self.send(emsg.pack())
        self.sock.shutdown(1)
        self.sock.close()

class sslChannel(channel):
    """SSL channel to communicate to NOX with.
    """
    def __init__(self, ipAddr, portNo=1304,debug=False):
        """Initialize with SSL sock
        """
        NOXChannel.__init__(self, ipAddr, portNo,debug)
        ##Reference to SSL socket for channel
        self.sslsock = socket.ssl(self.sock)

    def baresend(self, msg):
        """Send bare message"""
        self.sslsock.write(msg)

class messenger_msg:
	"""Automatically generated Python class for messenger_msg

	Date 2010-01-20
	Created by lavi.pythonize.msgpythonizer
	"""
	def __init__(self):
		"""Initialize
		Declare members and default values
		"""
		self.length = 0
		self.type = 0
		self.body= []

	def __assert(self):
		"""Sanity check
		"""
		if (not (self.type in msg_type_values)):
			return (False, "type must have values from msg_type")
		return (True, None)

	def pack(self, assertstruct=True):
		"""Pack message
		Packs empty array used as placeholder
		"""
		if(assertstruct):
			if(not self.__assert()[0]):
				return None
		packed = ""
		packed += struct.pack("!HB", self.length, self.type)
		for i in self.body:
			packed += struct.pack("!B",i)
		return packed

	def unpack(self, binaryString):
		"""Unpack message
		Do not unpack empty array used as placeholder
		since they can contain heterogeneous type
		"""
		if (len(binaryString) < 3):
			return binaryString
		(self.length, self.type) = struct.unpack_from("!HB", binaryString, 0)
		return binaryString[3:]

	def __len__(self):
		"""Return length of message
		"""
		l = 3
		l += len(self.body)*1
		return l

msg_type = ['MSG_DISCONNECT', 'MSG_ECHO', 'MSG_ECHO_RESPONSE', 'MSG_AUTH', 'MSG_AUTH_RESPONSE', 'MSG_AUTH_STATUS', 'MSG_NOX_STR_CMD']
MSG_DISCONNECT = 0
MSG_ECHO = 1
MSG_ECHO_RESPONSE = 2
MSG_AUTH = 3
MSG_AUTH_RESPONSE = 4
MSG_AUTH_STATUS = 5
MSG_NOX_STR_CMD = 10
msg_type_values = [0, 1, 2, 3, 4, 5, 10]



