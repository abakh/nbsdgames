from socket import *
import os, sys, time, random
from select import select
from cStringIO import StringIO
from weakref import WeakValueDictionary
from metastruct import *
from common import httpserver, stdlog

if __name__ == '__main__':
    os.chdir(os.path.dirname(sys.argv[0]) or os.curdir)

META_SERVER_HTTP_PORT = 8050
META_SERVER_PORT = 8055
META_SERVER_UDP_PORT = 8055
IMAGE_DIR = "../bubbob/doc/images"
ICONS = [open(os.path.join(IMAGE_DIR, s), 'rb').read()
         for s in os.listdir(IMAGE_DIR) if s.endswith('.png')]
assert ICONS, "you need to run ../bubbob/doc/bonus-doc.py"
MAX_SERVERS = 50
MAX_CONNEXIONS = 60


serversockets = {}

class MetaServer:

    def __init__(self, port=META_SERVER_PORT, udpport=META_SERVER_UDP_PORT):
        s = socket(AF_INET, SOCK_STREAM)
        s.bind(('', port))
        s.listen(5)
        self.parentsock = s
        self.ServersDict = {}
        self.ServersList = []
        serversockets[s] = self.clientconnect, sys.exit
        self.udpsock = socket(AF_INET, SOCK_DGRAM)
        self.udpsock.bind(('', udpport))
        serversockets[self.udpsock] = self.udp_message, None
        self.udpdata = []

    def detach(self):
        pid = os.fork()
        if pid:
            print pid
            os._exit(0)
        # in the child process
        os.setsid()
        logfile = stdlog.LogFile(limitsize=131072)
        if logfile:
            print >> logfile
            print "Logging to", logfile.filename
            fd = logfile.f.fileno()
            try:
                # detach from parent
                os.dup2(fd, 1)
                os.dup2(fd, 2)
                os.dup2(fd, 0)
            except OSError:
                pass
            logfile.close()
        # record pid
        f = open('pid', 'w')
        print >> f, os.getpid()
        f.close()

    def clientconnect(self):
        s, addr = self.parentsock.accept()
        Connexion(s, addr)

    def publish(self, server):
        key = server.serverkey
        if key in self.ServersDict:
            current = self.ServersDict[key]
            if current is server:
                return
            self.ServersList.remove(current)
        elif len(self.ServersDict) >= MAX_SERVERS:
            raise OverflowError
        self.ServersList.append(server)
        self.ServersDict[key] = server
        print '+', key

    def unpublish(self, server):
        key = server.serverkey
        if key in self.ServersDict:
            current = self.ServersDict[key]
            if current is server:
                del self.ServersDict[key]
                self.ServersList.remove(server)
                print '-', key

    def makelist(self):
        items = {}
        for srv in self.ServersList:
            items[srv.serverkey] = encodedict(srv.serverinfo)
        return encodedict(items)

    def getserver(self, key):
        return self.ServersDict[key]

    def udp_message(self):
        data, addr = self.udpsock.recvfrom(32)
        self.udpdata.append((data, addr))
        if len(self.udpdata) > 50:
            del self.udpdata[0]


class Connexion(MessageSocket):

    def __init__(self, s, addr):
        MessageSocket.__init__(self, s)
        self.serverinfo = {
            'time': int(time.time()),
            'icon': random.choice(ICONS),
            'iconformat': 'png',
            }
        self.addr = addr
        self.key = '%s:%d' % addr
        self.serverkey = None
        print '[', self.key
        self.backlinks = WeakValueDictionary()
        if len(serversockets) >= MAX_CONNEXIONS:
            self.disconnect()
            raise OverflowError
        serversockets[s] = self.receive, self.disconnect

    def disconnect(self):
        metaserver.unpublish(self)
        try:
            del serversockets[self.s]
        except KeyError:
            pass
        print ']', self.key

    def msg_serverinfo(self, info, *rest):
        print '|', self.key
        if len(info) > 15000:
            raise OverflowError
        info = decodedict(info)
        self.serverinfo.update(info)

    def msg_startserver(self, port, *rest):
        serverkey = '%s:%d' % (self.addr[0], port)
        if self.serverkey and self.serverkey != serverkey:
            metaserver.unpublish(self)
        self.serverkey = serverkey
        metaserver.publish(self)

    def msg_stopserver(self, *rest):
        metaserver.unpublish(self)

    def msg_list(self, *rest):
        self.s.sendall(message(RMSG_LIST, metaserver.makelist()))

    def msg_route(self, targetkey, *rest):
        try:
            target = metaserver.getserver(targetkey)
        except KeyError:
            try:
                target = self.backlinks[targetkey]
            except KeyError:
                self.s.sendall(message(RMSG_NO_HOST, targetkey))
                return
        target.route(self, *rest)

    def route(self, origin, msgcode, *rest):
        self.backlinks[origin.key] = origin
        self.s.sendall(message(msgcode, origin.key, *rest))

    def msg_traceback(self, tb, *rest):
        f = stdlog.LogFile('tb-%s.log' % (self.addr[0],))
        if f:
            print >> f, tb
            f.close()

    def msg_udp_addr(self, pattern, *rest):
        for data, addr in metaserver.udpdata:
            if data == pattern:
                try:
                    host, port = addr
                    port = int(port)
                except ValueError:
                    continue
                self.s.sendall(message(RMSG_UDP_ADDR, host, port))
                return
        else:
            self.s.sendall(message(RMSG_UDP_ADDR))

    MESSAGES = {
        MMSG_INFO:  msg_serverinfo,
        MMSG_START: msg_startserver,
        MMSG_STOP:  msg_stopserver,
        MMSG_LIST:  msg_list,
        MMSG_ROUTE: msg_route,
        MMSG_TRACEBACK: msg_traceback,
        MMSG_UDP_ADDR:  msg_udp_addr,
        }


# ____________________________________________________________

import htmlentitydefs
text_to_html = {}
for key, value in htmlentitydefs.entitydefs.items():
    text_to_html[value] = '&' + key + ';'
for i in range(32):
    text_to_html[chr(i)] = '?'
def htmlquote(s, getter=text_to_html.get):
    lst = [getter(c, c) for c in s if ' ' <= c < '\x7F']
    return ''.join(lst)
def txtfilter(s, maxlen=200):
    s = str(s)[:maxlen]
    l = [c for c in s if c in "!$*,-.0123456789:@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "^_`abcdefghijklmnopqrstuvwxyz{|}"]
    return ''.join(l)

f = open('index.html', 'r')
HEADER, ROW, FOOTER = f.read().split('\\')
f.close()

def makehosthtml(srv, bottommsg, join):
    info = srv.serverinfo
    hostname, port = srv.serverkey.split(':')
    try:
        fullhostname = gethostbyaddr(hostname)[0]
    except:
        fullhostname = hostname
    url = None
    if join:
        url = "http://%s/join.html?host=%s&port=%s&httpport=%s&m=%s" % (
            join, hostname, port, info.get('httpport') or 'off', time.time())
    else:
        try:
            httpport = int(info.get('httpport'))
        except:
            pass
        else:
            url = "http://%s:%s/" % (hostname, httpport)
            javamsg = """
<p>Click on a server above to join the game.  This only works if:
<ul><li>your browser understands Java;
    <li>the server is not behind a firewall;
    <li>you don't mind not hearing the nice background music.
</ul></p>
<p>Alternatively, install the
<a href="http://bub-n-bros.sourceforge.net/download.html">Python version</a>
of the client, which can cope with all of the above problems.</p>
<br>"""
            if javamsg not in bottommsg:
                bottommsg.append(javamsg)
    result = '<strong>%s</strong>:%s' % (fullhostname, port)
    if url:
        result = '<a href="%s">%s</a>' % (url, result)
    return result

def indexloader(headers, join=[], head=[], **options):
    if join:
        join = join[0]
    data = [HEADER % (head and head[0] or '')]
    bottommsg = []
    if metaserver.ServersList:
        counter = 0
        for srv in metaserver.ServersList:
            info = srv.serverinfo
            icon = '<img src="ico?key=%s">' % srv.serverkey
            bgcolor = ('#C0D0D0', '#E0D0A8')[counter&1]
            hosthtml = makehosthtml(srv, bottommsg, join)
            desc = htmlquote(info.get('desc')) or ''
            extradesc = htmlquote(info.get('extradesc')) or ''
            if isinstance(info.get('time'), int):
                stime = time.strftime('%a %b %d<br>%H:%M GMT',
                                      time.gmtime(info['time']))
            else:
                stime = ''
            data.append(ROW % locals())
            counter += 1
    else:
        data.append('''<tr><td bgcolor="#FFFFFF">
            Sorry, there is no registered server at the moment.
        </td></tr>''')
    if join:
        extrafooter = '''<p><img src="home.png">
        <a href="http://%s/?time=%s">Back to local games</a></p>''' % (
            join, time.time())
    else:
        extrafooter = ''
    bottommsg = '\n'.join(bottommsg)
    tbfiles = [s for s in os.listdir('.') if s.startswith('tb-')]
    if tbfiles:
        tbfiles = len(tbfiles)
    else:
        tbfiles = ''
    data.append(FOOTER % locals())
    return StringIO(''.join(data)), 'text/html'

def icoloader(key, **options):
    srv = metaserver.getserver(key[0])
    iconformat = txtfilter(srv.serverinfo['iconformat'], 32)
    return StringIO(srv.serverinfo['icon']), 'image/' + iconformat

httpserver.register('', indexloader)
httpserver.register('index.html', indexloader)
httpserver.register('bub-n-bros.html', indexloader)
httpserver.register('ico', icoloader)
httpserver.register('mbub.png', httpserver.fileloader('mbub.png', 'image/png'))
httpserver.register('home.png', httpserver.fileloader('home.png', 'image/png'))

def openhttpsocket(port = META_SERVER_HTTP_PORT):
    from BaseHTTPServer import HTTPServer
    class ServerClass(HTTPServer):
        def get_request(self):
            sock, addr = self.socket.accept()
            sock.settimeout(5.0)
            return sock, addr
    HandlerClass = httpserver.MiniHandler
    server_address = ('', port)
    httpd = ServerClass(server_address, HandlerClass)
    s = httpd.socket
    serversockets[s] = httpd.handle_request, None


# ____________________________________________________________

def mainloop():
    while 1:
        iwtd = serversockets.keys()
        iwtd, owtd, ewtd = select(iwtd, [], iwtd)
        #print iwtd, owtd, ewtd, serversockets
        for s in iwtd:
            if s in serversockets:
                input, close = serversockets[s]
                try:
                    input()
                except:
                    import traceback
                    print "-"*60
                    traceback.print_exc()
                    print "-"*60
        for s in ewtd:
            if s in serversockets:
                input, close = serversockets[s]
                try:
                    close()
                except:
                    import traceback
                    print "-"*60
                    traceback.print_exc()
                    print "-"*60


if __name__ == '__main__':
    metaserver = MetaServer()
    if sys.argv[1:2] == ['-f']:
        metaserver.detach()
    try:
        openhttpsocket()
        print 'listening to client port tcp %d / http %d / udp %d.' % (
            META_SERVER_PORT,
            META_SERVER_HTTP_PORT,
            META_SERVER_UDP_PORT)
        mainloop()
    finally:
        if metaserver.ServersList:
            print '*** servers still connected, waiting 5 seconds'
            time.sleep(5)
        print '*** leaving at', time.ctime()
