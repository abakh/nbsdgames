from time import time as now
from pipelayer import PipeLayer, InvalidPacket
from pipelayer import FLAG_RANGE_START, FLAG_RANGE_STOP
import socket, struct

SOU_RANGE_START = FLAG_RANGE_START
SOU_MIXED_DATA  = FLAG_RANGE_STOP + 0
SOU_SHUTDOWN    = FLAG_RANGE_STOP + 1
SOU_RANGE_STOP  = FLAG_RANGE_STOP + 2

SHUTDOWN_PACKET = chr(SOU_SHUTDOWN) + '**'    #  < 4 characters

CONGESTION_TIMEOUT = 20.0
#CONSOLIDATE_DELAY  = 0.1


class SocketOverUdp(object):
    RECV_CAN_RETURN_EMPTY = True
    PACKETSIZE = 996
    MIXEDPACKETSIZE = 1080

    def __init__(self, udpsock, initialcrcs):
        self.udpsock = udpsock
        self.pl = PipeLayer(initialcrcs)
        self.congested_since = None
        #self.consolidate_sends = None
        #self.encode_delayed_until = now()

    def close(self):
        try:
            self.udpsock.send(SHUTDOWN_PACKET)
        except socket.error:
            pass
        self.udpsock.close()

    def _progress(self):
        if self.pl.settime(now()) == 0.0:
            self._encode()

    def _encode(self):
        #if self.consolidate_sends:
        #    if self.pl.cur_time < self.encode_delayed_until:
        #        return False
        #    self.encode_delayed_until = self.pl.cur_time + CONSOLIDATE_DELAY
        packet = self.pl.encode(self.PACKETSIZE)
        if packet is not None:
            #print 'send:', repr(packet)
            if self.pl.is_congested():
                if self.congested_since is None:
                    self.congested_since = now()
                else:
                    if now() > self.congested_since + CONGESTION_TIMEOUT:
                        self.udpsock.send(SHUTDOWN_PACKET)
                        raise socket.error("peer not responding, timing out")
            else:
                self.congested_since = None
            #print repr(packet[:10])
            #print "out:", len(packet)
            #print ' ---'
            self.udpsock.send(packet)

    def _decode(self, packet):
        try:
            data = self.pl.decode(packet)
            #print ' ~~~'
            return data
        except InvalidPacket:
            if len(packet) >= 4:
                hdr, reserved, size = struct.unpack("!BBH", packet[:4])
                if hdr == SOU_MIXED_DATA:
                    #print ' ~~~[unmix%d/%d]' % (len(packet[4+size:]),
                    #                            len(packet))
                    self.udp_over_udp_decoder(packet[4:4+size])
                    return self._decode(packet[4+size:])
                else:
                    # non-tiny packets with no recognized hdr byte are
                    # assumed to be pure video traffic
                    #print ' ~~~[video]'
                    self.udp_over_udp_decoder(packet)
                    return ''
            elif packet == SHUTDOWN_PACKET:
                raise socket.error("received an end-of-connexion packet")
            else:
                #print ' ~~~[INVALID%d]' % (len(packet),)
                return ''

    def fileno(self):
        self._progress()
        return self.udpsock.fileno()

    def flush(self):
        while self.pl.settime(now()) == 0.0:
            #self.encode_delayed_until = self.pl.cur_time
            self._encode()

    def recv(self, _ignoredbufsize=None):
        #print 'recv:'
        packet = self.udpsock.recv(65535)
        #print "                 in:", len(packet), hex(ord(packet[0]))
        #print repr(packet)
        self.pl.settime(now())
        data = self._decode(packet)
        #print 'which is really', repr(data)
        self._encode()
        #if data:
        #    print "                              IN:", len(data)
        return data

    def sendall(self, data):
        #print 'queuing', repr(data)
        #print '                        OUT:', len(data)
        self.pl.queue(data)
        #self._progress()
        return len(data)

    send = sendall

    def send_video_data(self, udpdata):
        forced_embedded = SOU_RANGE_START <= ord(udpdata[0]) < SOU_RANGE_STOP
        self.pl.settime(now())
        packet = self.pl.encode(self.PACKETSIZE) or ''
        if not forced_embedded and not packet:
            # no PipeLayer packet, send as plain udp data
            datagram = udpdata
        elif len(packet) + len(udpdata) <= self.MIXEDPACKETSIZE:
            # fits in a single mixed data packet
            datagram = (struct.pack("!BBH", SOU_MIXED_DATA, 0, len(udpdata))
                        + udpdata + packet)
            #print ' ---[mix%d/%d]' % (len(packet), len(datagram))
        else:
            # two packets needed
            #print repr(packet[:10])
            #print "out:", len(packet)
            #print ' ---'
            self.udpsock.send(packet)
            datagram = udpdata
        #print repr(datagram[:10])
        #print "out:", len(datagram), hex(ord(datagram[0]))
        self.udpsock.send(datagram)
        #self.encode_delayed_until = self.pl.cur_time + CONSOLIDATE_DELAY
        #if self.consolidate_sends is None:
        #    self.consolidate_sends = True
        return len(udpdata)

    def udp_over_udp_mixer(self):
        return UdpOverUdpMixer(self)

    def udp_over_udp_decoder(self, data):
        pass    # method overridden by pclient.py

    def getpeername(self):
        return self.udpsock.getpeername()

    def getsockname(self):
        return self.udpsock.getsockname()

    def setsockopt(self, level, opt, value):
        # note that TCP_NODELAY is set by the bub-n-bros client, not the server
        #if level == socket.SOL_TCP and opt == socket.TCP_NODELAY:
        #    self.consolidate_sends = not value
        #else:
        #   ignored
        pass

    def setblocking(self, _ignored):
        pass   # XXX good enough for common/gamesrv.py


class UdpOverUdpMixer(object):
    def __init__(self, sockoverudp):
        self.send = sockoverudp.send_video_data

    def setsockopt(self, *args):
        pass   # ignored
