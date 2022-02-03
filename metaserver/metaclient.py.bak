import sys, os, time, random
from select import select
from socket import *
from metastruct import *

_SERVER = 'ctpug.org.za'

METASERVER = (_SERVER, 8055)
METASERVER_UDP = (_SERVER, 8055)
METASERVER_URL = 'http://%s:8050/bub-n-bros.html' % (_SERVER,)
VERSION_TAG = 1601

def connect(failure=[]):
    if len(failure) >= 2:
        return None
    print >> sys.stderr, 'Connecting to the meta-server %s:%d...' % METASERVER
    try:
        s = socket(AF_INET, SOCK_STREAM)
        s.connect(METASERVER)
    except error, e:
        print >> sys.stderr, '*** cannot contact meta-server:', str(e)
        failure.append(e)
        return None
    else:
        print >> sys.stderr, 'connected.'
    return s

sys.setcheckinterval(4096)


def float2str(f):
    # don't trust locale issues and write a string with a '.'
    s = str(long(f*1000000.0))
    return s[:-6] + '.' + s[-6:]

def str2float(s):
    try:
        return float(s)
    except:
        # locale issues may prevent float() from decoding the string
        s = s.strip()
        try:
            i = s.index('.')
        except ValueError:
            try:
                i = s.index(',')
            except ValueError:
                i = len(s)
        frac = s[i+1:]
        return float(s[:i] or '0') + float(frac or '0')/(10**len(frac))


# ____________________________________________________________
# Game Servers

class MetaClientSrv(MessageSocket):
    
    def __init__(self, s, game):
        MessageSocket.__init__(self, s)
        self.game = game
        self.lastwakeup = None
        self.synsockets = {}
        import gamesrv
        gamesrv.addsocket('META', s, self.receive)
        self.closed = 0

    def close(self):
        if not self.closed:
            self.disconnect()
            try:
                self.s.shutdown(2)
            except:
                pass

    def disconnect(self):
        import gamesrv
        gamesrv.removesocket('META', self.s)
        self.closed = 1
        print >> sys.stderr, 'disconnected from the meta-server'

    def send_traceback(self):
        if not self.closed:
            import traceback, cStringIO, sys
            f = cStringIO.StringIO()
            print >> f, sys.version
            print >> f, 'platform:   ', sys.platform
            print >> f, 'executable: ', sys.executable
            print >> f, 'argv:       ', sys.argv
            print >> f, 'cwd:        ', os.getcwd()
            print >> f, 'version tag:', VERSION_TAG
            print >> f
            traceback.print_exc(file = f)
            self.s.sendall(message(MMSG_TRACEBACK, f.getvalue()))

    def msg_wakeup(self, origin, *rest):
        if self.lastwakeup is None or time.time()-self.lastwakeup > 4.0:
            def fastresponses(wakeup):
                sys.setcheckinterval(64)
                time.sleep(12.01)
                if self.lastwakeup == wakeup:
                    sys.setcheckinterval(4096)
                    self.synsockets.clear()
            import thread
            self.lastwakeup = time.time()
            thread.start_new_thread(fastresponses, (self.lastwakeup,))

    def msg_connect(self, origin, port, *rest):
        def connect(origin, port):
            host, _ = origin.split(':')
            addr = host, port
            s = socket(AF_INET, SOCK_STREAM)
            print >> sys.stderr, 'backconnecting to', addr
            try:
                s.connect(addr)
            except error, e:
                print >> sys.stderr, 'backconnecting:', str(e)
            else:
                self.game.newclient_threadsafe(s, addr)
        import thread
        thread.start_new_thread(connect, (origin, port))

    def msg_udp_conn(self, origin, secret, port, *rest):
        def connect(origin, secret, port):
            host, _ = origin.split(':')
            addr = host, port
            s = socket(AF_INET, SOCK_DGRAM)
            print >> sys.stderr, 'udp connecting to', addr
            s.connect(addr)
            mysecret = random.randrange(0, 65536)
            packet = ('B' + chr(  secret & 0xFF) + chr(  secret >> 8)
                          + chr(mysecret & 0xFF) + chr(mysecret >> 8))
            from socketoverudp import SocketOverUdp
            from socketoverudp import SOU_RANGE_START, SOU_RANGE_STOP
            for i in range(5):
                #print 'sending', repr(packet)
                s.send(packet)
                iwtd, owtd, ewtd = select([s], [], [], 0.25)
                if s in iwtd:
                    #print 'reading'
                    try:
                        inbuf = s.recv(SocketOverUdp.PACKETSIZE)
                    except error:
                        inbuf = ''
                        # try again?
                        iwtd, owtd, ewtd = select([s], [], [], 0.35)
                        if s in iwtd:
                            try:
                                inbuf = s.recv(SocketOverUdp.PACKETSIZE)
                            except error:
                                pass
                    #print 'got', repr(inbuf)
                    if (inbuf and
                        SOU_RANGE_START <= ord(inbuf[0]) < SOU_RANGE_STOP):
                        break
            else:
                print >> sys.stderr, 'udp connecting: no answer, giving up'
                return
            sock = SocketOverUdp(s, (mysecret, secret))
            data = sock._decode(inbuf)
            #print 'decoded as', repr(data)
            expected = '[bnb c->s]' + packet[3:5]
            while len(data) < len(expected) + 2:
                #print 'waiting for more'
                iwtd, owtd, ewtd = select([sock], [], [], 5.0)
                if sock not in iwtd:
                    print >> sys.stderr, 'udp connecting: timed out'
                    return
                #print 'decoding more'
                data += sock.recv()
                #print 'now data is', repr(data)
            if data[:-2] != expected:
                print >> sys.stderr, 'udp connecting: bad data'
                return
            sock.sendall('[bnb s->c]' + data[-2:])
            sock.flush()
            #print 'waiting for the last dot...'
            while 1:
                iwtd, owtd, ewtd = select([sock], [], [], 5.0)
                if sock not in iwtd:
                    print >> sys.stderr, 'udp connecting: timed out'
                    return
                data = sock.recv(200)
                if data:
                    break
            if data != '^':
                print >> sys.stderr, 'udp connecting: bad data'
                return
            #print 'done!'
            self.game.newclient_threadsafe(sock, addr)

        import thread
        thread.start_new_thread(connect, (origin, secret, port))

    def msg_ping(self, origin, *rest):
        # ping time1  -->  pong time2 time1
        self.s.sendall(message(MMSG_ROUTE, origin,
                               RMSG_PONG, float2str(time.time()), *rest))

    def msg_sync(self, origin, clientport, time3, time2, time1, *rest):
        time4 = time.time()
        s = socket(AF_INET, SOCK_STREAM)
        s.bind(('', INADDR_ANY))
        _, serverport = s.getsockname()
        self.s.sendall(message(MMSG_ROUTE, origin,
                               RMSG_CONNECT, serverport, clientport))
        #print 'times:', time1, time2, time3, time4
        doubleping = (str2float(time3)-str2float(time1)) + (time4-str2float(time2))
        connecttime = time4 + doubleping / 4.0
        def connect(origin, port, connecttime, s):
            host, _ = origin.split(':')
            addr = host, port
            delay = connecttime - time.time()
            #print 'sleep(%r)' % delay
            if 0.0 <= delay <= 10.0:
                time.sleep(delay)
            print >> sys.stderr, 'synconnecting to', addr
            try:
                s.connect(addr)
            except error, e:
                print >> sys.stderr, 'synconnecting:', str(e)
            else:
                self.game.newclient_threadsafe(s, addr)
        import thread
        thread.start_new_thread(connect, (origin, clientport, connecttime, s))

    MESSAGES = {
        RMSG_CONNECT: msg_connect,
        RMSG_WAKEUP:  msg_wakeup,
        RMSG_PING:    msg_ping,
        RMSG_SYNC:    msg_sync,
        RMSG_UDP_CONN:msg_udp_conn,
        }

metaclisrv = None

def meta_register(game):
    global metaclisrv
    import gamesrv
    info = {}
    if game.FnDesc:
        info['desc'] = game.FnDesc or ''
        info['extradesc'] = game.FnExtraDesc() or ''

    s = gamesrv.opentcpsocket()
    hs = gamesrv.openhttpsocket()
    port = int(gamesrv.displaysockport(s))
    info['httpport'] = gamesrv.displaysockport(hs)

    if not metaclisrv or metaclisrv.closed:
        s = connect()
        if not s:
            return
        metaclisrv = MetaClientSrv(s, game)
    metaclisrv.s.sendall(message(MMSG_INFO, encodedict(info)) +
                         message(MMSG_START, port))

def meta_unregister(game):
    global metaclisrv
    if metaclisrv:
        metaclisrv.close()
        metaclisrv = None


# ____________________________________________________________
# Game Clients

class Event:
    def __init__(self):
        import thread
        self.lock = thread.allocate_lock()
        self.lock.acquire()
    def signal(self):
        try:
            self.lock.release()
        except:
            pass
    def wait1(self):
        self.lock.acquire()


class MetaClientCli:
    fatalerror = False
    
    def __init__(self, serverkey, backconnectport):
        self.resultsocket = None
        self.serverkey = serverkey
        self.backconnectport = backconnectport
        self.threads = {}

    def run(self):
        import thread
        print >> sys.stderr, 'Trying to connect to', self.serverkey
        self.ev = Event()
        self.ev2 = Event()
        self.buffer = ""
        self.sendlock = thread.allocate_lock()
        self.recvlock = thread.allocate_lock()
        self.inputmsgqueue = []
        self.gotudpport = None
        if not (PORTS.get('CLIENT') or PORTS.get('sendudpto')):
            self.s = connect()
            thread.start_new_thread(self.acquire_udp_port, ())
        else:
            self.s = None
            self.ev2.signal()
            self.startthread(self.try_udp_connect)

        thread.start_new_thread(self.bipbip, ())
        self.startthread(self.try_direct_connect, 0.75)
        self.startthread(self.try_indirect_connect, 1.50)
        while self.resultsocket is None:
            self.threadsleft()
            self.ev.wait1()
        self.ev2.wait1()
        return self.resultsocket

    def done(self):
        sys.setcheckinterval(4096)

    def bipbip(self):
        while self.resultsocket is None:
            time.sleep(0.31416)
            self.ev.signal()

    def startthread(self, fn, sleep=0.0, args=()):
        import thread
        def bootstrap(fn, atom, sleep, args):
            try:
                time.sleep(sleep)
                if self.resultsocket is None:
                    fn(*args)
            finally:
                del self.threads[atom]
                self.ev.signal()
        atom = object()
        self.threads[atom] = time.time()
        thread.start_new_thread(bootstrap, (fn, atom, sleep, args))

    def threadsleft(self):
        if self.fatalerror:
            sys.exit(1)
        now = time.time()
        TIMEOUT = 11
        for starttime in self.threads.values():
            if now < starttime + TIMEOUT:
                break
        else:
            if self.threads:
                print >> sys.stderr, '*** time out, giving up.'
            else:
                print >> sys.stderr, '*** failed to connect.'
            sys.exit(1)

    def try_direct_connect(self):
        host, port = self.serverkey.split(':')
        port = int(port)
        s = socket(AF_INET, SOCK_STREAM)
        try:
            s.connect((host, port))
        except error, e:
            print >> sys.stderr, 'direct connexion failed:', str(e)
        else:
            print >> sys.stderr, 'direct connexion accepted.'
            self.resultsocket = s

    def try_indirect_connect(self):
        import thread, time
        if not self.s:
            self.s = connect()
        if not self.s:
            return
        self.routemsg(RMSG_WAKEUP)
        self.startthread(self.try_backconnect)
        self.socketcache = {}
        tries = [0.6, 0.81, 1.2, 1.69, 2.6, 3.6, 4.9, 6.23]
        for delay in tries:
            self.startthread(self.send_ping, delay)
        while self.resultsocket is None:
            msg = self.inputmsg()
            now = time.time()
            if self.resultsocket is not None:
                break
            if msg[0] == RMSG_CONNECT:
                # connect serverport clientport
                self.startthread(self.try_synconnect, args=msg[1:])
            if msg[0] == RMSG_PONG:
                # pong time2 time1  -->  sync port time3 time2 time1
                if len(self.socketcache) < len(tries):
                    s = socket(AF_INET, SOCK_STREAM)
                    s.bind(('', INADDR_ANY))
                    _, port = s.getsockname()
                    self.socketcache[port] = s
                    self.routemsg(RMSG_SYNC, port, float2str(now), *msg[2:])

    def sendmsg(self, data):
        self.sendlock.acquire()
        try:
            self.s.sendall(data)
        finally:
            self.sendlock.release()

    def routemsg(self, *rest):
        self.sendmsg(message(MMSG_ROUTE, self.serverkey, *rest))

    def _readnextmsg(self, blocking):
        self.recvlock.acquire()
        try:
            while 1:
                msg, self.buffer = decodemessage(self.buffer)
                if msg is not None:
                    if msg[0] == RMSG_UDP_ADDR:
                        if len(msg) > 2:
                            self.gotudpport = msg[1], int(msg[2])
                        continue
                    if msg[0] == RMSG_NO_HOST and msg[1] == self.serverkey:
                        print >> sys.stderr, ('*** server %r is not registered'
                                             ' on the meta-server' % (msg[1],))
                        self.fatalerror = True
                        sys.exit()
                    self.inputmsgqueue.append(msg)
                    return
                iwtd, owtd, ewtd = select([self.s], [], [], 0)
                if not iwtd:
                    if self.inputmsgqueue or not blocking:
                        return
                data = self.s.recv(2048)
                if not data:
                    print >> sys.stderr, 'disconnected from the meta-server'
                    sys.exit()
                self.buffer += data
        finally:
            self.recvlock.release()

    def inputmsg(self):
        self._readnextmsg(blocking=True)
        return self.inputmsgqueue.pop(0)

    def try_backconnect(self):
        s1 = socket(AF_INET, SOCK_STREAM)
        s1.bind(('', self.backconnectport or INADDR_ANY))
        s1.listen(1)
        _, port = s1.getsockname()
        self.routemsg(RMSG_CONNECT, port)
        print >> sys.stderr, 'listening for backward connection'
        iwtd, owtd, ewtd = select([s1], [], [], 7.5)
        if s1 in iwtd:
            s, addr = s1.accept()
            print >> sys.stderr, 'accepted backward connection from', addr
            self.resultsocket = s

    def send_ping(self):
        sys.stderr.write('. ')
        self.routemsg(RMSG_PING, float2str(time.time()))

    def try_synconnect(self, origin, remoteport, localport, *rest):
        sys.stderr.write('+ ')
        s = self.socketcache[localport]
        remotehost, _ = origin.split(':')
        remoteaddr = remotehost, remoteport
        try:
            s.connect(remoteaddr)
        except error, e:
            print >> sys.stderr, 'SYN connect failed:', str(e)
            return
        print >> sys.stderr, ('simultaneous SYN connect succeeded with %s:%d' %
                              remoteaddr)
        self.resultsocket = s

    def try_udp_connect(self):
        for i in range(3):     # three attempts
            self.attempt_udp_connect()
            if self.resultsocket is not None:
                break

    def attempt_udp_connect(self):
        if '*udpsock*' in PORTS:
            s, (host, port) = PORTS['*udpsock*']
        else:
            s = socket(AF_INET, SOCK_DGRAM)
            s.bind(('', PORTS.get('CLIENT', INADDR_ANY)))
            host, port = s.getsockname()
            if 'sendudpto' in PORTS:
                host = PORTS['sendudpto']
        secret = originalsecret = random.randrange(0, 65536)
        self.routemsg(RMSG_UDP_CONN, secret, port)
        secret = 'B' + chr(secret & 0xFF) + chr(secret >> 8)
        while True:
            iwtd, owtd, ewtd = select([s], [], [], 2.94)
            if s not in iwtd:
                return
            packet, addr = s.recvfrom(200)
            if packet.startswith(secret) and len(packet) == 5:
                break
        s.connect(addr)
        #print 'got', repr(packet)
        remotesecret = ord(packet[3]) | (ord(packet[4]) << 8)
        secret = random.randrange(0, 65536)
        secret = chr(secret & 0xFF) + chr(secret >> 8)
        packet = '[bnb c->s]' + packet[3:5] + secret
        for name in ('*udpsock*', 'CLIENT'):
            if name in PORTS:
                del PORTS[name]
        from socketoverudp import SocketOverUdp
        sock = SocketOverUdp(s, (originalsecret, remotesecret))
        #print 'sending', repr(packet)
        sock.sendall(packet)
        sock.flush()
        data = ''
        expected = '[bnb s->c]' + secret
        while len(data) < len(expected):
            #print 'waiting'
            iwtd, owtd, ewtd = select([sock], [], [], 2.5)
            if sock not in iwtd:
                print >> sys.stderr, 'socket-over-udp timed out'
                return
            #print 'we get:'
            data += sock.recv()
            #print repr(data)
        if data != expected:
            print >> sys.stderr, 'bad udp data from', addr
        else:
            sock.sendall('^')
            sock.flush()
            print 'udp connexion handshake succeeded'
            self.resultsocket = sock

    def acquire_udp_port(self):
        try:
            s = socket(AF_INET, SOCK_DGRAM)
            s.bind(('', INADDR_ANY))
            randomdata = hex(random.randrange(0, sys.maxint))
            for i in range(5):
                s.sendto(randomdata, METASERVER_UDP)
                time.sleep(0.05)
                self.sendmsg(message(MMSG_UDP_ADDR, randomdata))
                time.sleep(0.05)
                self._readnextmsg(blocking=False)
                if self.gotudpport:
                    PORTS['*udpsock*'] = s, self.gotudpport
                    udphost, udpport = self.gotudpport
                    print >> sys.stderr, ('udp port %d is visible from '
                                          'outside on %s:%d' % (
                        s.getsockname()[1], udphost, udpport))
                    self.startthread(self.try_udp_connect)
                    break
        finally:
            self.ev2.signal()


def meta_connect(serverkey, backconnectport=None):
    global METASERVER
    if PORTS.get('SSH_RELAY'):
        METASERVER = PORTS['SSH_RELAY']
    c = MetaClientCli(serverkey, backconnectport)
    s = c.run()
    c.done()
    return s

def print_server_list():
    s = connect()
    if s is not None:
        s.sendall(message(MMSG_LIST))
        buffer = ""
        while decodemessage(buffer)[0] is None:
            buffer += s.recv(8192)
        s.close()
        msg = decodemessage(buffer)[0]
        assert msg[0] == RMSG_LIST
        entries = decodedict(msg[1])
        if not entries:
            print >> sys.stderr, 'No registered server.'
        else:
            print
            print ' %-25s | %-30s | %s' % (
                'server', 'game', 'players')
            print '-'*27+'+'+'-'*32+'+'+'-'*11
            for key, value in entries.items():
                if ':' in key:
                    try:
                        addr, _, _ = gethostbyaddr(key[:key.index(':')])
                    except:
                        pass
                    else:
                        addr = '%-27s' % (addr,)
                        if len(addr) < 28: addr += '|'
                        addr = '%-60s' % (addr,)
                        if len(addr) < 61: addr += '|'
                        print addr
                value = decodedict(value)
                print ' %-25s | %-30s | %s' % (
                    key, value.get('desc', '<no description>'),
                    value.get('extradesc', ''))
            print

if __name__ == '__main__':
    print_server_list()
