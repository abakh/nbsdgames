from socket import *
import time, select, sys
from errno import ETIMEDOUT

UDP_PORT     = 8056
PING_MESSAGE = "pclient-game-ping"
PONG_MESSAGE = "server-game-pong"


def serverside_ping(only_port=None):
    port = only_port or UDP_PORT
    s = socket(AF_INET, SOCK_DGRAM)
    try:
        s.bind(('', port))
        s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
        return s
    except error, e:
        if only_port is None:
            s = socket(AF_INET, SOCK_DGRAM)
            s.bind(('', INADDR_ANY))
            return s
        else:
            return None

def answer_ping(s, descr, addr, extra='', httpport=''):
    try:
        data, source = s.recvfrom(100)
    except error, e:
        print >> sys.stderr, 'ping error:', str(e)
        return
    if data == PING_MESSAGE:
        print >> sys.stderr, "ping by", source
        answer = '%s:%s:%s:%s:%s:%s' % (PONG_MESSAGE, descr,
                                        addr[0], addr[1], extra, httpport)
        s.sendto(answer, source)
    else:
        print >> sys.stderr, \
              "unexpected data on UDP port %d by" % UDP_PORT, source


def pick(hostlist, delay=1):
    s = socket(AF_INET, SOCK_DGRAM)
    s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
    for host in hostlist:
        print >> sys.stderr, "* Looking for a server on %s... " % host
        try:
            s.sendto(PING_MESSAGE, (host, UDP_PORT))
        except error, e:
            print >> sys.stderr, 'send:', str(e)
            continue
        while 1:
            iwtd, owtd, ewtd = select.select([s], [], [], delay)
            if not iwtd:
                break
            try:
                data, answer_from = s.recvfrom(200)
            except error, e:
                if e.args[0] != ETIMEDOUT:
                    print >> sys.stderr, 'recv:', str(e)
                    continue
                break
            data = data.split(':')
            if len(data) >= 4 and data[0] == PONG_MESSAGE:
                hostname = data[2] or answer_from[0]
                try:
                    port = int(data[3])
                except ValueError:
                    pass
                else:
                    result = (hostname, port)
                    print >> sys.stderr, "* Picking %r at" % data[1], result
                    return result
            print >> sys.stderr, "got an unexpected answer", data, "from", answer_from
    print >> sys.stderr, "no server found."
    raise SystemExit

def find_servers(hostlist=[('127.0.0.1', None), ('<broadcast>', None)],
                 tries=2, delay=0.5, verbose=1, port_needed=1):
    import gamesrv
    if verbose:
        print >> sys.stderr, 'Looking for servers in the following list:'
        for host, udpport in hostlist:
            print >> sys.stderr, '    %s,  UDP port %s' % (
                host, udpport or ("%s (default)" % UDP_PORT))
    events = {}
    replies = []
    s = socket(AF_INET, SOCK_DGRAM)
    s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
    for trynum in range(tries):
        for host, udpport in hostlist:
            try:
                ipaddr = host
                if host != '<broadcast>':
                    try:
                        ipaddr = gethostbyname(host)
                    except error, e:
                        print >> sys.stderr, 'gethostbyname:', str(e)
                s.sendto(PING_MESSAGE, (ipaddr, udpport or UDP_PORT))
                hostsend, hostrecv = events.setdefault(ipaddr, ([], []))
                hostsend.append(time.time())
            except error, e:
                print >> sys.stderr, 'send:', str(e)
                continue
        endtime = time.time() + delay
        while gamesrv.recursiveloop(endtime, [s]):
            try:
                data, answer_from = s.recvfrom(200)
            except error, e:
                if e.args[0] != ETIMEDOUT:
                    print >> sys.stderr, 'recv:', str(e)
                    continue
                break
            try:
                ipaddr = gethostbyname(answer_from[0])
            except error:
                ipaddr = answer_from[0]
            else:
                hostsend, hostrecv = events.setdefault(ipaddr, ([], []))
                hostrecv.append(time.time())
            data = data.split(':')
            if len(data) >= 4 and data[0] == PONG_MESSAGE:
                try:
                    port = int(data[3])
                except ValueError:
                    if port_needed:
                        continue
                    port = ''
                if data[2]:
                    hostname = data[2]
                    realhostname = [hostname]
                else:
                    hostname = answer_from[0]
                    realhostname = lazy_gethostbyaddr(hostname)
                server = ':'.join(data[1:2]+data[4:])
                replies.append((hostname, realhostname, port, server, ipaddr))
            else:
                print >> sys.stderr, "got an unexpected answer from", answer_from
    servers = {}
    aliases = {}
    timeout = time.time() + 2.0     # wait for gethostbyaddr() for 2 seconds
    while replies:
        i = 0
        now = time.time()
        while i < len(replies):
            hostname, realhostname, port, server, ipaddr = replies[i]
            if realhostname:
                hostname = realhostname[0]    # got an answer
            elif now < timeout:
                i += 1     # must wait some more time
                continue
            result = (hostname, port)
            servers[result] = server
            aliases[hostname] = ipaddr
            del replies[i]
        if replies:
            time.sleep(0.08)   # time for gethostbyaddr() to finish
    if verbose:
        print >> sys.stderr, "%d answer(s):" % len(servers), servers.keys()
    for host, port in servers.keys():
        ping = None
        ipaddr = aliases[host]
        if ipaddr in events:
            hostsend, hostrecv = events[ipaddr]
            if len(hostsend) == len(hostrecv) == tries:
                ping = min([t2-t1 for t1, t2 in zip(hostsend, hostrecv)])
        servers[host, port] = (servers[host, port], ping)
    sys.setcheckinterval(4096)
    return servers

# ____________________________________________________________

HOSTNAMECACHE = {}

def _lazygetter(hostname, resultlst):
    try:
        try:
            hostname = gethostbyaddr(hostname)[0]
            if hostname == 'localhost':
                from msgstruct import HOSTNAME as hostname
        except error:
            pass
    finally:
        resultlst.append(hostname)

def lazy_gethostbyaddr(hostname):
    try:
        return HOSTNAMECACHE[hostname]
    except KeyError:
        resultlst = HOSTNAMECACHE[hostname] = []
        import thread
        thread.start_new_thread(_lazygetter, (hostname, resultlst))
        return resultlst
