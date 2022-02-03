from socket import *
from msgstruct import *
#from fcntl import ioctl
#from termios import TIOCOUTQ
from zlib import compressobj, Z_SYNC_FLUSH
import struct

ZeroBuffer = struct.pack("i", 0)


class SocketMarshaller:
    
    def __init__(self, tcpsock, mixer):
        self.tcpsock = tcpsock
        self.mixer   = mixer
        self.mixer_can_mix = mixer.send_can_mix
        self.mixer_send    = mixer.send_buffer
        self.tcpsock_fd = tcpsock.fileno()
        # try to reduce TCP latency
        try:
            tcpsock.setsockopt(SOL_IP, IP_TOS, 0x10)  # IPTOS_LOWDELAY
        except error, e:
            print "Cannot set IPTOS_LOWDELAY for client:", str(e)
        try:
            tcpsock.setsockopt(SOL_TCP, TCP_NODELAY, 1)
        except error, e:
            print "Cannot set TCP_NODELAY for client:", str(e)
        compressor = compressobj(6)
        self.compress = compressor.compress
        self.compress_flush = compressor.flush
    
    def send(self, data):
        if self.mixer_can_mix():
            # discard all packets if there is still data waiting in tcpsock
            # --- mmmh, works much better without this check ---
            #try:
            #    if ioctl(self.tcpsock_fd, TIOCOUTQ, ZeroBuffer) != ZeroBuffer:
            #        return
            #except IOError, e:
            #    print "ioctl(TIOCOUTQ) failed, disconnecting client"
            #    self.mixer.disconnect(e)
            #else:
                data = self.compress(data) + self.compress_flush(Z_SYNC_FLUSH)
                self.mixer_send(message(MSG_INLINE_FRAME, data))
                return len(data)
        return 0
