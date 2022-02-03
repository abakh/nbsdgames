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
    except error as e:
        if only_port is None:
            s = socket(AF_INET, SOCK_DGRAM)
            s.bind(('', INADDR_ANY))
            return s
        else:
            return None

def answer_ping(s, descr, addr, extra='', httpport=''):
    try:
        data, source = s.recvfrom(100)
    except error as e:
        print('ping error:', str(e), file=sys.stderr)
        return
    if data == PING_MESSAGE:
        print("ping by", source, file=sys.stderr)
        answer = '%s:%s:%s:%s:%s:%s' % (PONG_MESSAGE, descr,
                                        addr[0], addr[1], extra, httpport)
        s.sendto(answer, source)
    else:
        print("unexpected data on UDP port %d by" % UDP_PORT, source, file=sys.stderr)


def pick(hostlist, delay=1):
    s = socket(AF_INET, SOCK_DGRAM)
    s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
    for host in hostlist:
        print("* Looking for a server on %s... " % host, file=sys.stderr)
        try:
            s.sendto(PING_MESSAGE, (host, UDP_PORT))
        except error as e:
            print('send:', str(e), file=sys.stderr)
            continue
        while 1:
            iwtd, owtd, ewtd = select.select([s], [], [], delay)
            if not iwtd:
                break
            try:
                data, answer_from = s.recvfrom(200)
            except error as e:
                if e.args[0] != ETIMEDOUT:
                    print('recv:', str(e), file=sys.stderr)
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
                    print("* Picking %r at" % data[1], result, file=sys.stderr)
                    return result
            print("got an unexpected answer", data, "from", answer_from, file=sys.stderr)
    print("no server found.", file=sys.stderr)
    raise SystemExit

def find_servers(hostlist=[('127.0.0.1', None), ('<broadcast>', None)],
                 tries=2, delay=0.5, verbose=1, port_needed=1):
    from . import gamesrv
    if verbose:
        print('Looking for servers in the following list:', file=sys.stderr)
        for host, udpport in hostlist:
            print('    %s,  UDP port %s' % (
                host, udpport or ("%s (default)" % UDP_PORT)), file=sys.stderr)
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
                    except error as e:
                        print('gethostbyname:', str(e), file=sys.stderr)
                s.sendto(PING_MESSAGE, (ipaddr, udpport or UDP_PORT))
                hostsend, hostrecv = events.setdefault(ipaddr, ([], []))
                hostsend.append(time.time())
            except error as e:
                print('send:', str(e), file=sys.stderr)
                continue
        endtime = time.time() + delay
        while gamesrv.recursiveloop(endtime, [s]):
            try:
                data, answer_from = s.recvfrom(200)
            except error as e:
                if e.args[0] != ETIMEDOUT:
                    print('recv:', str(e), file=sys.stderr)
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
                print("got an unexpected answer from", answer_from, file=sys.stderr)
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
        print("%d answer(s):" % len(servers), list(servers.keys()), file=sys.stderr)
    for host, port in list(servers.keys()):
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
                from .msgstruct import HOSTNAME as hostname
        except error:
            pass
    finally:
        resultlst.append(hostname)

def lazy_gethostbyaddr(hostname):
    try:
        return HOSTNAMECACHE[hostname]
    except KeyError:
        resultlst = HOSTNAMECACHE[hostname] = []
        import _thread
        _thread.start_new_thread(_lazygetter, (hostname, resultlst))
        return resultlst
