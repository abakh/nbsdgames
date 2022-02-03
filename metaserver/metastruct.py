import sys, os
LOCALDIR = __file__
LOCALDIR = os.path.abspath(os.path.dirname(LOCALDIR))
sys.path.insert(0, os.path.dirname(LOCALDIR))

from common.msgstruct import *
from socket import error

MMSG_INFO     = 'I'
MMSG_START    = '+'
MMSG_STOP     = '-'
MMSG_LIST     = 'L'
MMSG_ROUTE    = 'R'
MMSG_TRACEBACK= 'T'
MMSG_UDP_ADDR = 'U'

RMSG_WAKEUP   = 'w'
RMSG_PING     = 'p'
RMSG_PONG     = 'o'
RMSG_SYNC     = 'y'
RMSG_CONNECT  = 'c'
RMSG_LIST     = 'l'
RMSG_UDP_ADDR = 'u'
RMSG_UDP_CONN = 'd'
RMSG_NO_HOST  = '?'


def encodedict(dict):
    data = []
    for key, value in list(dict.items()):
        data.append(message('#', key, value))
    return ''.join(data)

def encodelist(list):
    return message('[', *list)

def decodedict(buffer):
    result = {}
    while 1:
        msg, buffer = decodemessage(buffer)
        if msg is None or len(msg) < 3 or msg[0] != '#':
            break
        result[msg[1]] = msg[2]
    return result

def decodelist(buffer):
    msg, buffer = decodemessage(buffer)
    assert msg[0] == '['
    return list(msg[1:])


class MessageSocket:
    
    def __init__(self, s):
        self.s = s
        self.buffer = ""

    def receive(self):
        try:
            data = self.s.recv(2048)
        except error:
            data = ''
        if not data:
            self.disconnect()
            return
        self.buffer += data
        while 1:
            msg, self.buffer = decodemessage(self.buffer)
            if msg is None:
                break
            if msg[0] not in self.MESSAGES:
                print('unknown message %r' % (msg[0],), file=sys.stderr)
            else:
                fn = self.MESSAGES[msg[0]]
                fn(self, *msg[1:])
