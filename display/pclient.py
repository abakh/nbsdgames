#! /usr/bin/env python

import sys, os
from socket import *
from select import select
import struct, zlib
import time
from common.msgstruct import *
from common.pixmap import decodepixmap
from common import hostchooser
import modes
from modes import KeyPressed, KeyReleased
import caching

#import psyco; psyco.full()

# switch to udp_over_tcp if the udp socket didn't receive at least 60% of
# the packets sent by the server
UDP_EXPECTED_RATIO = 0.60


def loadpixmap(dpy, data, colorkey=None):
    w, h, data = decodepixmap(data)
    if colorkey is None:
        colorkey = -1
    elif colorkey < 0:
        r, g, b = struct.unpack("BBB", self.data[:3])
        colorkey = b | (g<<8) | (r<<16)
    return dpy.pixmap(w, h, data, colorkey)

class Icon:
    alpha = 255
    def __init__(self, playfield):
        self.playfield = playfield
        self.size = 0, 0
    def __getattr__(self, attr):
        if attr == 'pixmap':
            self.pixmap = self.playfield.getpixmap(self.bmpcode)
            if hasattr(self.playfield.dpy, 'getopticon'):
                ico = self.playfield.dpy.getopticon(
                    self.pixmap, self.originalrect, self.alpha)
                if ico is not None:
                    self.pixmap = ico
                    self.rect = None
            return self.pixmap
        elif attr in ('bmpcode', 'rect'):
            raise KeyError, attr
        elif attr == 'originalrect':
            self.originalrect = self.rect
            return self.originalrect
        raise AttributeError, attr
    def clear(self):
        if self.__dict__.has_key('pixmap'):
            del self.pixmap

class DataChunk(caching.Data):
    SOURCEDIR = os.path.abspath(os.path.join(os.path.dirname(caching.__file__),
                                             os.pardir))
    CACHEDIR  = os.path.join(SOURCEDIR, 'cache')
    TOTAL = 0

    def __init__(self, fileid):
        caching.Data.__init__(self)
        self.fileid = fileid
        self.pending = []
        self.progresshook = None

    def server_md5(self, playfield, filename, position, length, checksum):
        if not self.loadfrom(filename, position, length, checksum):
            self.pending.append((0, position))
            playfield.s.sendall(message(CMSG_DATA_REQUEST, self.fileid,
                                        position, length))

    def server_patch(self, position, data, lendata):
        #print 'server_patch', self.fileid, position, len(data)
        prev = DataChunk.TOTAL >> 10
        DataChunk.TOTAL += lendata
        total = DataChunk.TOTAL >> 10
        if total != prev:
            print "downloaded %dkb of data from server" % total
        self.store(position, data)
        try:
            self.pending.remove((0, position))
        except ValueError:
            pass
        else:
            while self.pending and self.pending[0][0]:
                callback = self.pending[0][1]
                del self.pending[0]
                callback(self)

    def when_ready(self, callback):
        if self.pending:
            self.pending.append((1, callback))
        else:
            callback(self)


class Playfield:
    TASKBAR_HEIGHT = 48
    
    def __init__(self, s, sockaddr):
        self.s = s
        self.sockaddr = sockaddr
        try:
            self.s.setsockopt(SOL_IP, IP_TOS, 0x10)  # IPTOS_LOWDELAY
        except error, e:
            print >> sys.stderr, "Cannot set IPTOS_LOWDELAY:", str(e)
        try:
            self.s.setsockopt(SOL_TCP, TCP_NODELAY, 1)
        except error, e:
            print >> sys.stderr, "Cannot set TCP_NODELAY:", str(e)

        initialbuf = ""
        while 1:
            t = self.s.recv(200)
            if not t and not hasattr(self.s, 'RECV_CAN_RETURN_EMPTY'):
                raise error, "connexion closed"
            initialbuf += t
            if len(initialbuf) >= len(MSG_WELCOME):
                head = initialbuf[:len(MSG_WELCOME)]
                tail = initialbuf[len(MSG_WELCOME):]
                if head != MSG_WELCOME:
                    raise error, "connected to something not a game server"
                if '\n' in tail:
                    break
        n = tail.index('\n')
        line2 = tail[:n]
        self.initialbuf = tail[n+1:]

        self.gameident = line2.strip()
##        self.datapath = None
##        if self.gameident.endswith(']'):
##            i = self.gameident.rfind('[')
##            if i >= 0:
##                self.gameident, self.datapath = (self.gameident[:i].strip(),
##                                                 self.gameident[i+1:-1])
        print "connected to %r." % self.gameident
        self.s.sendall(message(CMSG_PROTO_VERSION, 3))

    def setup(self, mode, udp_over_tcp):
        self.playing = {}   # 0, 1, or 'l' for local
        self.keys = {}
        self.keycodes = {}
        self.last_key_event = (None, None)
        self.dpy = None
        self.snd = None
        self.pixmaps = {}   # {bmpcode: dpy_pixmap}
        self.bitmaps = {}   # {bmpcode: (fileid_or_data, colorkey)}
        self.icons = {}
        self.sounds = {}
        self.currentmusic = None
        self.fileids = {}
        self.sprites = []
        self.playingsounds = {}
        self.playericons = {}
        self.screenmode = mode
        self.initlevel = 0
        if mode[-1].has_key('udp_over_tcp'):
            udp_over_tcp = mode[-1]['udp_over_tcp']
        self.trackcfgmtime = None
        if mode[-1].has_key('cfgfile'):
            self.trackcfgfile = mode[-1]['cfgfile']
        else:
            self.trackcfgfile = os.path.join(DataChunk.SOURCEDIR,
                                             'http2', 'config.txt')
        self.udpsock = None
        self.udpsock_low = None
        self.udpsock2 = None
        self.accepted_broadcast = 0
        self.tcpbytecounter = 0
        self.udpbytecounter = 0
        if udp_over_tcp == 1:
            self.start_udp_over_tcp()
        else:
            self.pending_udp_data = None
            if udp_over_tcp == 'auto':
                self.udpsock_low = 0
        self.dyndecompress = [[None, None, None, None] for i in range(8)]
        self.dynnorepeat = None

    def run(self, mode, udp_over_tcp='auto'):
        self.setup(mode, udp_over_tcp)
        try:
            self.mainloop()
        finally:
            if self.dpy:
                self.dpy.close()
            try:
                self.s.close()
            except:
                pass

    def mainloop(self):
        pss = hostchooser.serverside_ping()
        self.initial_iwtd = [self.s, pss]
        self.iwtd = self.initial_iwtd[:]
        self.animdelay = 0.0
        inbuf = self.process_inbuf(self.initialbuf)
        self.initialbuf = ""
        errors = 0
        while 1:
            if self.dpy:
                self.processkeys()
            iwtd, owtd, ewtd = select(self.iwtd, [], [], self.animdelay)
            self.animdelay = 0.5
            if self.dpy:
                self.processkeys()
            if self.s in iwtd:
                inputdata = self.s.recv(0x6000)
                self.tcpbytecounter += len(inputdata)
                inbuf += inputdata
                inbuf = self.process_inbuf(inbuf)
            if self.dpy:
                if self.udpsock in iwtd:
                    udpdata1 = None
                    while self.udpsock in iwtd:
                        try:
                            udpdata = self.udpsock.recv(65535)
                        except error, e:
                            print >> sys.stderr, e
                            errors += 1
                            if errors > 10:
                                raise
                            break
                        self.udpbytecounter += len(udpdata)
                        if len(udpdata) > 3 and '\x80' <= udpdata[0] < '\x90':
                            udpdata = self.dynamic_decompress(udpdata)
                        if udpdata is not None:
                            udpdata1 = udpdata
                        iwtd, owtd, ewtd = select(self.iwtd, [], [], 0)
                    if udpdata1 is not None:
                        self.update_sprites(udpdata1)
                if self.udpsock2 in iwtd:
                    while self.udpsock2 in iwtd:
                        udpdata = self.udpsock2.recv(65535)
                        self.udpbytecounter += len(udpdata)
                        if udpdata == BROADCAST_MESSAGE:
                            if not self.accepted_broadcast:
                                self.s.sendall(message(CMSG_UDP_PORT, '*'))
                                self.accepted_broadcast = 1
                                #self.udpsock_low = None
                            udpdata = ''
                        iwtd, owtd, ewtd = select(self.iwtd, [], [], 0)
                    if udpdata and self.accepted_broadcast:
                        self.update_sprites(udpdata)
                if self.pending_udp_data:
                    self.update_sprites(self.pending_udp_data)
                    self.pending_udp_data = ''
                erasetb = self.taskbarmode and self.draw_taskbar()
                d = self.dpy.flip()
                if d:
                    self.animdelay = min(self.animdelay, d)
                if self.snd:
                    d = self.snd.flop()
                    if d:
                        self.animdelay = min(self.animdelay, d)
                if erasetb:
                    self.erase_taskbar(erasetb)
            if pss in iwtd:
                hostchooser.answer_ping(pss, self.gameident, self.sockaddr)

    def process_inbuf(self, inbuf):
        while inbuf:
            values, inbuf = decodemessage(inbuf)
            if not values:
                break  # incomplete message
            fn = Playfield.MESSAGES.get(values[0], self.msg_unknown)
            fn(self, *values[1:])
        return inbuf

    def dynamic_decompress(self, udpdata):
        # Format of a UDP version 3 packet:
        #    header byte:     0x80 - 0x87    packet from thread 0 - 7
        #                  or 0x88 - 0x8F    reset packet from thread 0 - 7
        #    previous frame in same thread (1 byte)
        #    frame number (1 byte)
        thread = self.dyndecompress[ord(udpdata[0]) & 7]
        # thread==[decompress, lastframenumber, recompressed, lastframedata]
        prevframe = udpdata[1]
        thisframe = udpdata[2]
        #print '---'
        #for t in self.dyndecompress:
        #    print repr(t)[:120]
        #print
        #print `udpdata[:3]`

        if udpdata[0] >= '\x88':
            # reset
            d = zlib.decompressobj().decompress
            if prevframe != thisframe:  # if not global sync point
                # sync point from a previous frame
                # find all threads with the same prevframe
                threads = [t for t in self.dyndecompress if prevframe == t[1]]
                if not threads:
                    return None   # lost
                # find a thread with already-recompressed data
                for t in threads:
                    if t[2]:
                        data = t[3]
                        break
                else:
                    # recompress and cache the prevframe data
                    t = threads[0]
                    data = t[3]
                    co = zlib.compressobj(6)
                    data = co.compress(data) + co.flush(zlib.Z_SYNC_FLUSH)
                    t[2] = 1
                    t[3] = data
                d(data)  # use it to initialize the state of the decompressobj
                #print d
            thread[0] = d
        elif prevframe != thread[1]:
            #print 'lost'
            return None   # lost
        else:
            d = thread[0]
        # go forward in thread
        try:
            framedata = d(udpdata[3:])
            #print d
            thread[1] = thisframe
            thread[2] = 0
            thread[3] = framedata
            if thisframe == self.dynnorepeat:
                return None
            self.dynnorepeat = thisframe
            return framedata
        except zlib.error:
            #print 'crash'
            return None

    def geticon(self, icocode):
        try:
            return self.icons[icocode]
        except KeyError:
            ico = self.icons[icocode] = Icon(self)
            return ico

    def getpixmap(self, bmpcode):
        try:
            return self.pixmaps[bmpcode]
        except KeyError:
            data, colorkey = self.bitmaps[bmpcode]
            if type(data) is type(''):
                data = zlib.decompress(data)
            else:
                if data.pending:
                    raise KeyError
                data = data.read()
            pixmap = loadpixmap(self.dpy, data, colorkey)
            self.pixmaps[bmpcode] = pixmap
            return pixmap

    def update_sprites(self, udpdata):
        sprites = self.sprites
        unpack = struct.unpack

        currentsounds = {}
        base = 0
        while udpdata[base+4:base+6] == '\xFF\xFF':
            key, lvol, rvol = struct.unpack("!hBB", udpdata[base:base+4])
            try:
                snd = self.sounds[key]
            except KeyError:
                pass  # ignore sounds with bad code  (probably not defined yet)
            else:
                n = self.playingsounds.get(key)
                if n:
                    currentsounds[key] = n-1
                elif self.snd:
                    self.snd.play(snd,
                                  lvol / 255.0,
                                  rvol / 255.0)
                    currentsounds[key] = 4
            base += 6
        self.playingsounds = currentsounds
        
        for j in range(len(sprites)):
            if sprites[j][0] != udpdata[base:base+6]:
                removes = sprites[j:]
                del sprites[j:]
                removes.reverse()
                eraser = self.dpy.putppm
                for reserved, eraseargs in removes:
                    eraser(*eraseargs)
                break
            base += 6
        #print "%d sprites redrawn" % (len(udpdata)/6-j)
        try:
            overlayer = self.dpy.overlayppm
        except AttributeError:
            getter = self.dpy.getppm
            setter = self.dpy.putppm
            for j in range(base, len(udpdata)-5, 6):
                info = udpdata[j:j+6]
                x, y, icocode = unpack("!hhh", info[:6])
                try:
                    ico = self.icons[icocode]
                    sprites.append((info, (x, y, getter((x, y) + ico.size))))
                    setter(x, y, ico.pixmap, ico.rect)
                except KeyError:
                    #print "bad ico code", icocode
                    pass  # ignore sprites with bad ico (probably not defined yet)
        else:
            for j in range(base, len(udpdata)-5, 6):
                info = udpdata[j:j+6]
                x, y, icocode = unpack("!hhh", info[:6])
                try:
                    ico = self.icons[icocode]
                    overlay = overlayer(x, y, ico.pixmap, ico.rect, ico.alpha)
                    sprites.append((info, overlay))
                except KeyError:
                    #print "bad ico code", icocode
                    pass  # ignore sprites with bad ico (probably not defined yet)

        t0, n = self.painttimes
        n = n + 1
        if n == 50:
            t = time.time()
            t, t0 = t-t0, t
            if t:
                print "%.2f images per second,  %.1f kbytes per second" % (
                    float(n)/t,
                    float(self.tcpbytecounter+self.udpbytecounter)/1024/t)
                self.tcpbytecounter = -self.udpbytecounter
            n = 0
        self.painttimes = t0, n

    def get_taskbar(self):
        y0 = self.height - self.TASKBAR_HEIGHT
        iconlist = []
        f = 1.5 * time.time()
        f = f-int(f)
        pi = self.playericons.items()
        pi.sort()
        xpos = 0
        for id, ico in pi:
            if self.playing.get(id) != 'l':
                w, h = ico.size
                xpos += int(w * 5 / 3)
                if not self.playing.get(id):
                    y = self.height - h
                    if self.keydefinition and id == self.keydefinition[0]:
                        num, icons = self.keys[self.nextkeyname()]
                        ico = icons[int(f*len(icons))-1]
                        y = y0 + int((self.TASKBAR_HEIGHT-ico.size[1])/2)
                        self.animdelay = 0.04
                    iconlist.append((xpos-w, y, ico, id))
        pi.reverse()
        f = f * (1.0-f) * 4.0
        xpos = self.width
        for id, ico in pi:
            if self.playing.get(id) == 'l':
                w, h = ico.size
                xpos -= int(w * 5 / 3)
                dy = self.TASKBAR_HEIGHT - h - 1
                y = self.height - h - int(dy*f)
                iconlist.append((xpos, y, ico, id))
                self.animdelay = 0.04
        return y0, iconlist

    def clic_taskbar(self, (cx,cy)):
        y0, icons = self.get_taskbar()
        if cy >= y0:
            for x, y, ico, id in icons:
                if x <= cx < x+ico.size[0]:
                    return id
        return None

    def draw_taskbar(self):
        y0, icons = self.get_taskbar()
        rect = (0, y0, self.width, self.TASKBAR_HEIGHT)
        bkgnd = self.dpy.getppm(rect)
        self.dpy.taskbar(rect)
        for x, y, ico, id in icons:
            try:
                self.dpy.putppm(x, y, ico.pixmap, ico.rect)
            except KeyError:
                pass
        return y0, bkgnd

    def erase_taskbar(self, (y0, bkgnd)):
        self.dpy.putppm(0, y0, bkgnd)

    def nextkeyname(self):
        pid, df = self.keydefinition
        undef = [(num, keyname) for keyname, (num, icons) in self.keys.items()
                 if not df.has_key(keyname) and icons]
        if undef:
            num, keyname = min(undef)
            return keyname
        else:
            return None

    def startplaying(self):
        args = ()
        if hasattr(self.s, 'udp_over_udp_mixer'):
            # for SocketOverUdp: reuse the UDP address
            port = self.s.getsockname()[1]
            self.udpsock_low = None
            self.s.udp_over_udp_decoder = self.udp_over_udp_decoder
            self.start_udp_over_tcp()
        elif self.pending_udp_data is not None:
            port = MSG_INLINE_FRAME
        else:
            if '*udpsock*' in PORTS:
                self.udpsock, (host, port) = PORTS['*udpsock*']
                args = (host,)
            else:
                self.udpsock = socket(AF_INET, SOCK_DGRAM)
                self.udpsock.bind(('', PORTS.get('CLIENT', INADDR_ANY)))
                host, port = self.udpsock.getsockname()
            # Send a dummy UDP message to the server.  Some NATs will
            # then let through the UDP messages from the server.
            self.udpsock.sendto('.', self.s.getpeername())
            self.iwtd.append(self.udpsock)
            self.initial_iwtd.append(self.udpsock)
        if 'sendudpto' in PORTS:
            args = (PORTS['sendudpto'],)
        outbound = []
        outbound.append(message(CMSG_UDP_PORT, port, *args))
        if self.snd and self.snd.has_music:
            outbound.append(message(CMSG_ENABLE_MUSIC, 1))
            outbound.append(message(CMSG_PING))
        self.s.sendall(''.join(outbound))

    def start_udp_over_tcp(self):
        self.pending_udp_data = ''
        self.udp_over_tcp_decompress = zlib.decompressobj().decompress
        self.udpsock_low = None
        for name in ('udpsock', 'udpsock2'):
            sock = getattr(self, name)
            if sock is not None:
                try:
                    self.iwtd.remove(sock)
                except ValueError:
                    pass
                try:
                    self.initial_iwtd.remove(sock)
                except ValueError:
                    pass
                sock.close()
                setattr(self, name, None)

    def udp_over_udp_decoder(self, udpdata):
        if len(udpdata) > 3 and '\x80' <= udpdata[0] < '\x90':
            data = self.dynamic_decompress(udpdata)
            if data:
                self.pending_udp_data = data

    def processkeys(self):
        keyevents = self.dpy.keyevents()
        if keyevents:
            now = time.time()
            pending = {}
            for keysym, event in keyevents:
                pending[keysym] = event
            for keysym, event in pending.items():
                code = self.keycodes.get((keysym, event))
                if code and self.playing.get(code[0]) == 'l':
                    if (code == self.last_key_event[0] and
                        now - self.last_key_event[1] < 0.77):
                        continue   # don't send too much events for auto-repeat
                    self.last_key_event = code, now
                    self.s.sendall(code[1])
                elif self.keydefinition:
                    self.define_key(keysym)
        pointermotion = self.dpy.pointermotion()
        if pointermotion:
            x, y = pointermotion
            self.settaskbar(y >= self.height - 2*self.TASKBAR_HEIGHT)
        mouseevents = self.dpy.mouseevents()
        if mouseevents:
            self.settaskbar(1)
            self.keydefinition = None
            for clic in mouseevents:
                clic_id = self.clic_taskbar(clic)
                if clic_id is not None:
                    if self.playing.get(clic_id) == 'l':
                        self.s.sendall(message(CMSG_REMOVE_PLAYER, clic_id))
                    else:
                        self.keydefinition = clic_id, {}
        if self.taskbartimeout is not None and time.time() > self.taskbartimeout:
            self.settaskbar(0)

    def settaskbar(self, nmode):
        self.taskbartimeout = None
        if self.taskbarfree:
            self.taskbarmode = (nmode or
                                'l' not in self.playing.values() or
                                (self.keydefinition is not None))
            if nmode:
                self.taskbartimeout = time.time() + 5.0
            if hasattr(self.dpy, 'settaskbar'):
                self.dpy.settaskbar(self.taskbarmode)

    def define_key(self, keysym):
        clic_id, df = self.keydefinition
        if keysym in df.values():
            return
        df[self.nextkeyname()] = keysym
        if self.nextkeyname() is not None:
            return
        self.keydefinition = None
        self.s.sendall(message(CMSG_ADD_PLAYER, clic_id))
        for keyname, (num, icons) in self.keys.items():
            if keyname[:1] == '-':
                event = KeyReleased
                keyname = keyname[1:]
            else:
                event = KeyPressed
            if df.has_key(keyname):
                keysym = df[keyname]
                self.keycodes[keysym, event] = \
                                      clic_id, message(CMSG_KEY, clic_id, num)

    def msg_unknown(self, *rest):
        print >> sys.stderr, "?"

    def msg_player_join(self, id, local, *rest):
        if local:
            self.playing[id] = 'l'
            self.settaskbar(0)
            self.checkcfgfile(1)
        else:
            self.playing[id] = 1

    def msg_player_kill(self, id, *rest):
        self.playing[id] = 0
        for key, (pid, msg) in self.keycodes.items():
            if pid == id:
                del self.keycodes[key]

    def msg_broadcast_port(self, port):
        if self.pending_udp_data is not None:
            return
        if self.udpsock2 is not None:
            try:
                self.iwtd.remove(self.udpsock2)
            except ValueError:
                pass
            try:
                self.initial_iwtd.remove(self.udpsock2)
            except ValueError:
                pass
            self.udpsock2.close()
            self.udpsock2 = None
            self.accepted_broadcast = 0
        try:
            self.udpsock2 = socket(AF_INET, SOCK_DGRAM)
            self.udpsock2.bind(('', port))
            self.udpsock2.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
        except error, e:
            print "Cannot listen on the broadcast port %d" % port, str(e)
            self.udpsock2 = None
        else:
            self.iwtd.append(self.udpsock2)
            self.initial_iwtd.append(self.udpsock2)

    def msg_def_playfield(self, width, height, backcolor=None,
                          gameident=None, *rest):
        #if self.snd is not None:
        #    self.snd.close()
        if self.dpy is not None:
            # clear all pixmaps
            for ico in self.icons.values():
                ico.clear()
            self.pixmaps.clear()
            self.dpy.close()
            del self.sprites[:]
        self.width = width
        self.height = height
        if gameident:
            self.gameident = gameident
        self.dpy = modes.open_dpy(self.screenmode, width, height, self.gameident)
        self.snd = self.snd or modes.open_snd(self.screenmode)
        if self.snd:
            self.s.sendall(message(CMSG_ENABLE_SOUND))
        self.iwtd = self.dpy.selectlist() + self.initial_iwtd
        self.dpy.clear()   # backcolor is ignored
        self.painttimes = (time.time(), 0)
        self.s.sendall(message(CMSG_PING))
        self.taskbarmode = 0
        self.taskbarfree = 0
        self.taskbartimeout = None
        self.keydefinition = None

    def msg_def_key(self, name, num, *icons):
        self.keys[name] = num, [self.geticon(ico) for ico in icons]

    def msg_def_icon(self, bmpcode, icocode, x, y, w, h, alpha=255, *rest):
##        if h<0:
##            try:
##                bitmap, height = self.flippedbitmaps[bmpcode]
##            except KeyError:
##                bitmap, height = self.dpy.vflipppm(self.bitmaps[bmpcode])
##                self.flippedbitmaps[bmpcode] = bitmap, height
##            y = height - y
##            h = - h
##        else:
        ico = self.geticon(icocode)
        ico.bmpcode = bmpcode
        ico.rect = x, y, w, h
        ico.size = w, h
        if alpha < 255:
            ico.alpha = alpha

    def msg_def_bitmap(self, bmpcode, data, colorkey=None, *rest):
        if type(data) is not type(''):
            data = self.fileids[data]
        self.bitmaps[bmpcode] = data, colorkey

    def msg_def_sample(self, smpcode, data, *rest):
        def ready(f, self=self, smpcode=smpcode):
            if self.snd:
                self.sounds[smpcode] = self.snd.sound(f)
            f.clear()
        if type(data) is type(''):
            data = zlib.decompress(data)
            f = DataChunk(None)
            f.store(0, data)
            ready(f)
        else:
            f = self.fileids[data]
            f.when_ready(ready)

    def msg_patch_file(self, fileid, position, data, lendata=None, *rest):
        if self.fileids.has_key(fileid):
            f = self.fileids[fileid]
        else:
            f = self.fileids[fileid] = DataChunk(fileid)
        f.server_patch(position, data, lendata or len(data))

    def msg_zpatch_file(self, fileid, position, data, *rest):
        data1 = zlib.decompress(data)
        self.msg_patch_file(fileid, position, data1, len(data), *rest)

    def msg_md5_file(self, fileid, filename, position, length, checksum, *rest):
        if self.fileids.has_key(fileid):
            f = self.fileids[fileid]
        else:
            f = self.fileids[fileid] = DataChunk(fileid)
        f.server_md5(self, filename, position, length, checksum)

    def msg_play_music(self, loop_from, *codes):
        codes = [self.fileids[c] for c in codes]
        self.currentmusic = loop_from, codes, list(codes)
        self.activate_music()

    def activate_music(self, f=None):
        loop_from, codes, checkcodes = self.currentmusic
        if checkcodes:
            checkcodes.pop().when_ready(self.activate_music)
        elif self.snd:
            self.snd.play_musics(codes, loop_from)

    def msg_fadeout(self, time, *rest):
        if self.snd:
            self.snd.fadeout(time)

    def msg_player_icon(self, pid, icocode, *rest):
        self.playericons[pid] = self.geticon(icocode)

    def checkcfgfile(self, force=0):
        if self.trackcfgfile:
            try:
                st = os.stat(self.trackcfgfile)
            except OSError:
                pass
            else:
                if force or (st.st_mtime != self.trackcfgmtime):
                    self.trackcfgmtime = st.st_mtime
                    try:
                        f = open(self.trackcfgfile, 'r')
                        data = f.read().strip()
                        f.close()
                        d = eval(data or '{}', {}, {})
                    except:
                        print >> sys.stderr, 'Invalid config file format'
                    else:
                        d = d.get(gethostname(), {})
                        namemsg = ''
                        for id, local in self.playing.items():
                            keyid = 'player%d' % id
                            if local == 'l' and d.has_key(keyid):
                                namemsg = namemsg + message(
                                    CMSG_PLAYER_NAME, id, d[keyid])
                        if namemsg:
                            self.s.sendall(namemsg)

    def msg_ping(self, *rest):
        self.s.sendall(message(CMSG_PONG, *rest))
        self.checkcfgfile()
        if rest and self.udpsock_low is not None:
            udpkbytes = rest[0]
            if not udpkbytes:
                return
            #inp = self.udpbytecounter / (udpkbytes*1024.0)
            #print "(%d%% packet loss)" % int(100*(1.0-inp))
            if (udpkbytes<<10) * UDP_EXPECTED_RATIO > self.udpbytecounter:
                # too many packets were dropped (including, maybe, all of them)
                self.udpsock_low += 1
                if self.udpsock_low >= 3 and self.initlevel >= 1:
                    # third time now -- that's too much
                    print "Note: routing UDP traffic over TCP",
                    inp = self.udpbytecounter / (udpkbytes*1024.0)
                    print "(%d%% packet loss)" % int(100*(1.0-inp))
                    self.start_udp_over_tcp()
                    self.s.sendall(message(CMSG_UDP_PORT, MSG_INLINE_FRAME))
            else:
                # enough packets received
                self.udpsock_low = 0

    def msg_pong(self, *rest):
        if self.initlevel == 0:
            self.startplaying()
            self.initlevel = 1
        elif self.initlevel == 1:
            if self.snd and self.snd.has_music:
                self.s.sendall(message(CMSG_ENABLE_MUSIC, 2))
            self.initlevel = 2
        if not self.taskbarfree and not self.taskbarmode:
            self.taskbarfree = 1
            self.settaskbar(1)

    def msg_inline_frame(self, data, *rest):
        if self.pending_udp_data is not None:
            self.pending_udp_data = self.udp_over_tcp_decompress(data)
    
    MESSAGES = {
        MSG_BROADCAST_PORT:msg_broadcast_port,
        MSG_DEF_PLAYFIELD: msg_def_playfield,
        MSG_DEF_KEY      : msg_def_key,
        MSG_DEF_ICON     : msg_def_icon,
        MSG_DEF_BITMAP   : msg_def_bitmap,
        MSG_DEF_SAMPLE   : msg_def_sample,
        MSG_PLAY_MUSIC   : msg_play_music,
        MSG_FADEOUT      : msg_fadeout,
        MSG_PLAYER_JOIN  : msg_player_join,
        MSG_PLAYER_KILL  : msg_player_kill,
        MSG_PLAYER_ICON  : msg_player_icon,
        MSG_PING         : msg_ping,
        MSG_PONG         : msg_pong,
        MSG_INLINE_FRAME : msg_inline_frame,
        MSG_PATCH_FILE   : msg_patch_file,
        MSG_ZPATCH_FILE  : msg_zpatch_file,
        MSG_MD5_FILE     : msg_md5_file,
##        MSG_LOAD_PREFIX  : msg_load_prefix,
        }


def run(s, sockaddr, *args, **kw):
    try:
        import psyco
    except ImportError:
        pass
    else:
        psyco.bind(Playfield.update_sprites)
    Playfield(s, sockaddr).run(*args, **kw)
