#! /usr/bin/python

import cgi, os, string, time
form = cgi.FieldStorage()

def txtfilter(s):
    l = [c for c in s if c in "!$*,-.0123456789:@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`abcdefghijklmnopqrstuvwxyz{|}"]
    return string.join(l, '')

def identity(s):
    return s

def fieldlist(name, filter=txtfilter):
    result = []
    if name in form:
        field = form[name]
        if type(field) is type([]):
            for f in field:
                result.append(list(filter(f.value)))
        else:
            result.append(list(filter(field.value)))
    return result

def goodmatch(addr1, addr2):
    l1 = string.split(addr1, '.')
    if len(l1) >= 4: del l1[-1]
    l2 = string.split(addr2, '.')
    if len(l2) >= 4: del l2[-1]
    return l1 == l2


class Entry:
    Notice = ''
    FIELDS = [('server', 128), ('desc', 128), ('icon', 64),
              ('orig', 64), ('dele1', 64), ('dele2', 64)]
    
    def __init__(self, f):
        self.pos = f.tell()
        for fname, flen in self.FIELDS:
            s = f.read(flen)
            if not s:
                self.filled = 0
                break
            setattr(self, fname, string.rstrip(s))
        else:
            self.filled = 1
            
    def __bool__(self):
        return self.filled

    def write(self, f):
        if not getattr(self, 'icon', ''):
            try:
                import random
                lst = os.listdir('../htdocs/images')
            except:
                pass
            else:
                lst = [x for x in lst if x[-4:]=='.png' and 'A'<=x[0]<='Z']
                if lst:
                    self.icon = random.choice(lst)
        f.seek(self.pos)
        for fname, flen in self.FIELDS:
            data = getattr(self, fname, '')[:flen]
            f.write(data + ' '*(flen-len(data)))


def main():
    DATABASE    = 'servers'
    SIZEMAX     = 65536
    Serv        = 0
    Orig        = 1
    Dele1       = 2
    REMOTE_ADDR = os.environ['REMOTE_ADDR']
    REMOTE_ID   = REMOTE_ADDR + time.strftime('@%d%m', time.localtime(time.time()))
    OPS         = {}

    for srv in fieldlist('d'):
        OPS[srv] = 'd'
    for srv in fieldlist('a'):
        OPS[srv] = 'a'
    desc = (fieldlist('desc', identity) or [''])[0]

    f = open(DATABASE, 'r+b')
    freelist = []
    published = []
    while 1:
        e = Entry(f)
        if not e:
            break
        if e.server:
            if OPS.get(e.server) == 'a':
                validdesc = desc and goodmatch(REMOTE_ADDR, e.orig)
                if e.dele1 or e.dele2 or (validdesc and desc != e.desc):
                    e.dele1 = e.dele2 = ''      # re-enable server
                    if validdesc: e.desc = desc
                    e.write(f)
                del OPS[e.server]
            elif OPS.get(e.server) == 'd':
                if goodmatch(REMOTE_ADDR, e.orig) or (
                    REMOTE_ID != e.dele1 and REMOTE_ID != e.dele2):
                    if goodmatch(REMOTE_ADDR, e.orig):
                        e.server = ''      # remove server
                    elif e.dele1 == '':
                        e.dele1 = REMOTE_ID
                    elif e.dele2 == '':
                        e.dele2 = REMOTE_ID
                    else:
                        e.server = ''      # remove server
                    e.write(f)
                    Entry.Notice = 'd'
        if e.server:
            published.append((e.pos, e))
        else:
            freelist.append(e)

    for srv, action in list(OPS.items()):
        if action == 'a':
            if freelist:
                e = freelist[-1]
                del freelist[-1]
            else:
                f.seek(0, 2)
                e = Entry(f)
                if e.pos >= SIZEMAX:
                    raise Exception("Sorry, server database too big")
            hostname = string.split(srv, ':')[0]
            if '.' not in hostname:
                Entry.Notice = 'Server hostname "%s" incomplete.' % hostname
            else:
                import socket
                try:
                    result = socket.gethostbyaddr(hostname)
                except socket.error as e:
                    Entry.Notice = ('%s: %s' % (hostname, e))
                else:
                    if result[0] == 'projects.sourceforge.net':  # ????
                        Entry.Notice = ('Server hostname "%s" does not exist.' %
                                        hostname)
                    else:
                        e.server = srv
                        e.icon = ''
                        e.desc = desc
                        e.orig = REMOTE_ADDR
                        e.dele1 = e.dele2 = ''
                        e.write(f)
                        published.append((e.pos, e))
                        Entry.Notice = 'a'

    f.close()

    published.sort()
    return [pos_e[1] for pos_e in published]


def publish_list(serverlist):
    url = (fieldlist('url', identity) or ['http://127.0.0.1:8000'])[0]
    query = []
    for s in fieldlist('redirected'):
        query.append('redirected=' + s)
    for s in serverlist:
        query.append('s=' + txtfilter(s.server))
    url = url + '?' + string.join(query, '&')
    for s in fieldlist('frag'):
        url = url + '#' + s
    print('Content-Type: text/html')
    print('Location:', url)
    print()
    print('<html><head></head><body>')
    print('Please <a href="%s">click here</a> to continue.' % url)
    print('</body></html>')

def publish_default(serverlist):
    import html.entities
    text_to_html = {}
    for key, value in list(html.entities.entitydefs.items()):
        text_to_html[value] = '&' + key + ';'
    for i in range(32):
        text_to_html[chr(i)] = '?'
    def htmlquote(s, getter=text_to_html.get):
        lst = []
        for c in s: lst.append(getter(c, c))
        return string.join(lst, '')

    REMOTE_ADDR = os.environ['REMOTE_ADDR']
    f = open('started.html', 'r')
    header, row, footer = string.split(f.read(), '\\')
    f.close()
    import sys
    print('Content-Type: text/html')
    print()
    sys.stdout.write(header % "List of registered Internet Servers")
    counter = 0
    for s in serverlist:
        if s.icon:
            s1 = '<img width=32 height=32 src="/images/%s">' % s.icon
        else:
            s1 = ''
        lst = string.split(txtfilter(s.server), ':')
        hostname, port, udpport, httpport = (lst+['?','?','?','?'])[:4]
        s2 = '<strong>%s</strong>' % hostname
        try:
            int(httpport)
        except ValueError:
            pass
        else:
            s2 = '<a href="http://%s:%s/">%s:%s</a>' % (hostname, httpport,
                                                        s2, port)
        s2 = '<font size=+1>' + s2 + '</font>'
        if s.desc:
            s2 = s2 + '&nbsp;&nbsp;&nbsp;playing&nbsp;&nbsp;<strong>%s</strong>' % htmlquote(s.desc)
        if goodmatch(REMOTE_ADDR, s.orig):
            s2 = s2 + '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href="bb12.py?d=%s"><i>(if this server is dead, click here to remove it)</i></a>' % s.server
        sys.stdout.write(row % (s1, ('#C0D0D0', '#E0D0A8')[counter&1], s2))
        counter = counter + 1
    if not serverlist:
        sys.stdout.write(row % ('', '#FFFFFF', 'There is no registered server at the moment.'))
    sys.stdout.write(footer % "If your browser understands Java, you can click on a server to join the game. Note however that you will still need to install the whole Python client to benefit from the background musics, as this feature is missing from the Java client.<br><br>This list might contain already-dead servers; such 'zombies' disappear after some time.")

def publish_raw(serverlist):
    print('Content-Type: text/plain')
    print()
    print('Raw list produced for', os.environ['REMOTE_ADDR'])
    print()
    for s in serverlist:
        print(repr((s.server, s.desc, s.icon, s.orig)))

def publish_img(serverlist):
    import sys
    print('Content-Type: image/png')
    print()
    f = open('sfbub.png', 'rb')
    sys.stdout.write(f.read())
    f.close()

def publish_register(serverlist):
    if Entry.Notice == 'a':
        banner = 'The game server is now registered to SourceForge.'
    elif Entry.Notice == 'd':
        banner = ('Server <font color="#FF8000">unregistered</font> '
                  'from SourceForge.')
    elif Entry.Notice == '':
        if fieldlist('a'):
            banner = "The game server is already registered to SourceForge."
        elif fieldlist('d'):
            banner = 'The game server was <font color="#FF8000">already absent</font> from SourceForge.'
        else:
            publish_default(serverlist)
            return
    else: # errors
        banner = ('%s<br><br>' % Entry.Notice +
                  'If you are behind a firewall or NAT device (e.g. ADSL routers) you can still make your server reachable but it requires manual configuration.  (Instructions not available yet -- sorry)')
    f = open('started.html', 'r')
    header, row, footer = string.split(f.read(), '\\')
    f.close()
    import sys
    print('Content-Type: text/html')
    print()
    sys.stdout.write(header % banner)
    sys.stdout.write(footer % 'Press <a href="javascript: back()">Back</a> to come back to the main page.')


try:
    slist = main()
    cmd = (fieldlist('cmd') or ['?'])[0]
    publish = globals().get('publish_'+cmd, publish_default)
    publish(slist)
except:
    import traceback, sys
    print("Content-Type: text/plain")
    print()
    print("ERROR REPORT")
    print()
    traceback.print_exc(file=sys.stdout)
