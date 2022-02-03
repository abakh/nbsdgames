import os, sys, random
from io import StringIO
import socket, time

PLAYERNAMES = ['Bub', 'Bob', 'Boob', 'Beb',
               'Biob', 'Bab', 'Bib',
               'Baub', 'Beab', 'Biab']

try:
    FILE = __file__
except NameError:
    FILE = sys.argv[0]
LOCALDIR = os.path.abspath(os.path.dirname(FILE))

sys.path.insert(0, os.path.abspath(os.path.join(LOCALDIR, os.pardir)))
sys.path.insert(0, os.path.abspath(os.path.join(LOCALDIR, os.pardir,'common')))
import gamesrv, httpserver, hostchooser
from metaserver import metaclient
from httpserver import HTTPRequestError


class Options:
    def __init__(self, dict={}):
        self.update(dict)
    def dict(self):
        return self.__dict__.copy()
    def update(self, dict):
        self.__dict__.update(dict)
    def copy(self):
        return Options(self.__dict__)
    def clear(self):
        self.__dict__.clear()
    def __getattr__(self, attr):
        if not attr.startswith('_'):
            return None
        else:
            raise AttributeError(attr)


class PageServer:
    CONFIGFILE = 'config.txt'
    localservers = None

    def __init__(self, Game):
        self.Game = Game
        self.seed = hex(random.randrange(0x1000, 0x10000))
        #self.unique_actions = {}
        self.localhost = gamesrv.HOSTNAME
        self.filename = os.path.join(LOCALDIR, self.CONFIGFILE)
        data = self.loadoptionfile()
        self.globaloptions = Options(data.get('*', {}))
        self.localoptions  = Options(data.get(self.localhost, {}))
        self.reloadports()
        #self.inetserverlist = None
        #self.inetservers = {}
        #self.has_been_published = 0

    def registerpages(self):
        prefix = '%s/' % self.seed
        #httpserver.register('controlcenter.html',  self.controlcenterloader)
        httpserver.register(prefix,                self.indexloader)
        httpserver.register(prefix+'index.html',   self.indexloader)
        #httpserver.register(prefix+'list.html',    self.listloader)
        httpserver.register(prefix+'new.html',     self.newloader)
        httpserver.register(prefix+'run.html',     self.runloader)
        httpserver.register(prefix+'stop.html',    self.stoploader)
        httpserver.register(prefix+'join.html',    self.joinloader)
        #httpserver.register(prefix+'register.html',self.registerloader)
        httpserver.register(prefix+'options.html', self.optionsloader)
        httpserver.register(prefix+'name.html',    self.nameloader)
        for fn in os.listdir(os.path.join(LOCALDIR, 'data')):
            path = prefix + fn
            if not httpserver.is_registered(path):
                httpserver.register(path, httpserver.fileloader(
                    os.path.join(LOCALDIR, 'data', fn)))

    def opensocket(self):
        hs = gamesrv.openhttpsocket()
        if hs is None:
            return 0
        self.httpport = port = gamesrv.displaysockport(hs)
        self.indexurl = 'http://127.0.0.1:%d/%s/' % (port, self.seed)
        if self.Game:
            print(self.Game.FnDesc, end=' ')
        print('server is ready at', self.indexurl)
        return 1

    def getlocalservers(self):
        if self.localservers is None:
            self.searchlocalservers()
        return self.localservers

    def searchlocalservers(self):
        servers = list(hostchooser.find_servers().items())
        servers = list(filter(self.filterserver, servers))
        servers.sort()
        self.localservers = servers

##    def parse_inetserv(self, s):
##        try:
##            host, port, udpport, httpport = s.split(':')
##            return host, int(port), int(udpport)
##        except (ValueError, IndexError):
##            return None, None, None

##    def getinetservers(self):
##        if self.inetserverlist is None:
##            return None
##        result = []
##        for s in self.inetserverlist:
##            host, port, udpport = self.parse_inetserv(s)
##            addr = host, port
##            if addr in self.inetservers:
##                result.append((addr, self.inetservers[addr]))
##        return result

##    def setinetserverlist(self, lst):
##        self.inetserverlist = lst

##    def checkinetserverlist(self):
##        ulist = []
##        for s in self.inetserverlist:
##            host, port, udpport = self.parse_inetserv(s)
##            if host is not None:
##                ulist.append((host, udpport))
##        srvs = hostchooser.find_servers(ulist, delay=0.8)
##        self.inetservers = {}
##        for srv in srvs.items():
##            if not self.filterserver(srv):
##                continue
##            (host, port), info = srv
##            try:
##                host = socket.gethostbyaddr(host)[0]
##            except socket.error:
##                pass
##            self.inetservers[host, port] = info
##        #print 'hostchooser:', self.inetserverlist, '->', self.inetservers

    def filterserver(self, xxx_todo_changeme):
        ((host, port), info) = xxx_todo_changeme
        for c in host+str(port):
            if c not in "-.0123456789:@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_abcdefghijklmnopqrstuvwxyz":
                return 0
        return 1

##    def statusservers(self):
##        result = [], []
##        for s in self.inetserverlist:
##            host, port, udpport = self.parse_inetserv(s)
##            addr = host, port
##            found = addr in self.inetservers
##            result[found].append(s)
##        return result

    def loadoptionfile(self):
        try:
            f = open(self.filename, 'r')
            data = f.read().strip()
            f.close()
        except IOError:
            data = None
        return eval(data or '{}', {}, {})

    def saveoptions(self):
        data = self.loadoptionfile()
        data['*'] = self.globaloptions.dict()
        data[self.localhost] = self.localoptions.dict()
        try:
            f = open(self.filename, 'w')
            print(repr(data), file=f)
            f.close()
        except IOError as e:
            print("! Cannot save config file: " + str(e), file=sys.stderr)

    def reloadports(self):
        import msgstruct
        msgstruct.PORTS.clear()
        for key, value in list(self.localoptions.dict().items()):
            if key.startswith('port_'):
                key = key[5:]
                if key == 'CLIENT' and type(value) == str and ':' in value:
                    udphostname, value = value.split(':')
                    msgstruct.PORTS['sendudpto'] = udphostname
                try:
                    value = int(value)
                except:
                    continue
                msgstruct.PORTS[key] = value

    def startgame(self):
        self.reloadports()
        options = self.globaloptions
        kwds = {}
        if options.beginboard is not None:
            kwds['beginboard'] = int(options.beginboard)
        if options.lvlend is not None and options.lvlend.startswith('n'):
            kwds['finalboard'] = int(options.finalboard)
        if options.stepboard is not None:
            kwds['stepboard'] = int(options.stepboard)
        if options.limit is not None and options.limit.startswith('y'):
            kwds['limitlives'] = int(options.lives)
        if options.limitlifegain is not None:
            kwds['lifegainlimit'] = int(options.lifegainlimit)
        if options.extralife is not None:
            kwds['extralife'] = int(options.extralife)
        if options.autoreset is not None:
            kwds['autoreset'] = options.autoreset.startswith('y')
        if options.metapublish is not None:
            kwds['metaserver'] = options.metapublish.startswith('y')
        self.Game(options.file, **kwds)

    ### loaders ###

    def metaserverpage(self, headers):
        metaserver_url = metaclient.METASERVER_URL
        myhost = my_host(headers)
        joinurl = quote_plus('%s/%s' % (myhost, self.seed))
        return metaserver_url + '?join=%s&time=%s' % (joinurl, time.time())

    def mainpage(self, headers, juststarted=0, justconnected=0):
        running = my_server()
        count = len(gamesrv.clients)
        tim = time.time()
        #if running:
        #    metapublish = my_server_meta_address()
        #    fndesc = quote_plus(gamesrv.game.FnDesc)
        #else:
        #    metapublish = None
        return httpserver.load(os.path.join(LOCALDIR, 'data', 'index.html'),
                               'text/html', locals=locals())

    def indexloader(self, headers, cheat=[], **options):
        if cheat:
            import builtins
            for c in cheat:
                getattr(__builtin__, '__cheat')(c)
        else:
            self.localservers = None
        return self.mainpage(headers, juststarted=('juststarted' in options))

    def controlcenterloader(self, headers, **options):
        host = headers['remote host']
        host = socket.gethostbyname(host)
        if host != '127.0.0.1':
            raise HTTPRequestError("Access denied")
        return None, self.indexurl

##    def listloader(self, headers, s=[], **options):
##        self.setinetserverlist(s)
##        self.checkinetserverlist()
##        query = []
##        missing, found = self.statusservers()
##        for s in missing:
##            query.append('d=' + s)
##        for s in found:
##            query.append('a=' + s)
##        return self.mainpage(headers, query)

    def newloader(self, headers, **options):
        if not self.Game:
            raise HTTPRequestError("Complete bub-n-bros installation needed")
        locals = {
            'Game': self.Game,
            'options': self.globaloptions,
            'running': gamesrv.game is not None,
            }
        return httpserver.load(os.path.join(LOCALDIR, 'data', 'new.html'),
                               'text/html', locals=locals)

    def runloader(self, headers, **options):
        self.globaloptions.metapublish = 'n'
        self.globaloptions.autoreset = 'n'
        for key, value in list(options.items()):
            if len(value) == 1:
                setattr(self.globaloptions, key, value[0])
        self.saveoptions()
        self.startgame()
        return None, 'index.html?juststarted=%s' % time.time()

    def stoploader(self, headers, really=[], **options):
        count = len(gamesrv.clients)
        if count == 0 or really:
            locals = {
                'self': self,
                #'metaserver': METASERVER,
                #'metapublish': gamesrv.game and my_server_meta_address(),
                #'localdir': LOCALDIR,
                }
            gamesrv.closeeverything()
            return httpserver.load(os.path.join(LOCALDIR, 'data', 'stop.html'),
                                   'text/html', locals=locals)
        else:
            locals = {
                'count': count,
                }
            return httpserver.load(os.path.join(LOCALDIR, 'data', 'confirm.html'),
                                   'text/html', locals=locals)

##    def registerloader(self, headers, a=[], d=[], **options):
##        if a:  # the lists 'a' and 'd' contain dummies !!
##            self.globaloptions.metapublish = 'y'
##            self.has_been_published = 1
##            kwd = 'a'
##        else:
##            self.globaloptions.metapublish = 'n'
##            kwd = 'd'
##        url = "%s?cmd=register&%s=%s" % (METASERVER,
##                                         kwd, my_server_meta_address())
##        if a and gamesrv.game:
##            url += '&desc=' + quote_plus(gamesrv.game.FnDesc)
##        return None, url

    def joinloader(self, headers, host=[], port=[], httpport=[],
                   m=[], **options):
        args = self.buildclientoptions()
        assert len(host) == 1
        host = host[0]
        if len(port) == 1:
            port = port[0]
        else:
            try:
                host, port = host.split(':')
            except:
                port = None
        if args is None:
            # redirect to the Java applet
            try:
                httpport = int(httpport[0])
            except (ValueError, IndexError):
                if port:
                    raise HTTPRequestError("This server is not running HTTP.")
                else:
                    raise HTTPRequestError("Sorry, I cannot connect the Java applet to a server using this field.")
            return None, 'http://%s:%s/' % (host, httpport)

        # now is a good time to generate the color files if we can
        file = os.path.join(LOCALDIR, os.pardir, 'bubbob', 'images',
                            'buildcolors.py')
        if os.path.exists(file):
            g = {'__name__': '__auto__', '__file__': file}
            exec(compile(open(file, "rb").read(), file, 'exec'), g)

        if port:
            address = '%s:%s' % (host, port)
        else:
            address = host
        nbclients = len(gamesrv.clients)
        script = os.path.join(LOCALDIR, os.pardir, 'display', 'Client.py')
        script = no_quote_worries(script)
        if m:
            args.insert(0, '-m')
        args = [script] + args + [address]
        schedule_launch(args)
        if m:
            time.sleep(0.5)
            s = 'Connecting to %s.' % address
            return None, self.metaserverpage(headers) + '&head=' + quote_plus(s)
        #elif my_server_address() == address:
        #    endtime = time.time() + 3.0
        #    while gamesrv.recursiveloop(endtime, []):
        #        if len(gamesrv.clients) > nbclients:
        #            break
        return self.mainpage(headers, justconnected=1)

    def optionsloader(self, headers, reset=[], savetime=[], **options):
        if reset:
            self.localoptions.clear()
            self.globaloptions.clear()
            self.saveoptions()
        elif savetime:
            self.localoptions.port_CLIENT = None
            self.localoptions.port_LISTEN = None
            self.localoptions.port_HTTP = None
            for key, value in list(options.items()):
                setattr(self.localoptions, key, value[0])
            self.saveoptions()
        locals = {
            'self'   : self,
            'options': self.localoptions,
            }
        return httpserver.load(os.path.join(LOCALDIR, 'data', 'options.html'),
                               'text/html', locals=locals)

    def nameloader(self, headers, **options):
        MAX = len(PLAYERNAMES)
        if options:
            anyname = None
            for id in range(MAX):
                keyid = 'player%d' % id
                if keyid in options:
                    value = options[keyid][0]
                    anyname = anyname or value
                    teamid = 'team%d' % id
                    if teamid in options:
                        team = options[teamid][0]
                        if len(team) == 1:
                            value = '%s (%s)' % (value, team)
                    setattr(self.localoptions, keyid, value)
            if 'c' in options:
                for id in range(MAX):
                    keyid = 'player%d' % id
                    try:
                        delattr(self.localoptions, keyid)
                    except AttributeError:
                        pass
            if 'f' in options:
                for id in range(MAX):
                    keyid = 'player%d' % id
                    if not getattr(self.localoptions, keyid):
                        setattr(self.localoptions, keyid,
                                anyname or PLAYERNAMES[id])
                    else:
                        anyname = getattr(self.localoptions, keyid)
            self.saveoptions()
            if 's' in options:
                return self.mainpage(headers)
        locals = {
            'options': self.localoptions.dict(),
            }
        return httpserver.load(os.path.join(LOCALDIR, 'data', 'name.html'),
                               'text/html', locals=locals)

    def graphicmodeslist(self):
        try:
            return self.GraphicModesList
        except AttributeError:
            import display.modes
            self.GraphicModesList = display.modes.graphicmodeslist()
            javamode = display.modes.GraphicMode(
                'java', 'Java Applet (for Java browsers)', [])
            javamode.low_priority = 1
            javamode.getmodule = lambda : None
            self.GraphicModesList.insert(0, javamode)
            return self.GraphicModesList

    def soundmodeslist(self):
        try:
            return self.SoundModesList
        except AttributeError:
            import display.modes
            self.SoundModesList = display.modes.soundmodeslist()
            return self.SoundModesList

    def localmodes(self):
        import display.modes
        currentmodes = []
        options = self.localoptions
        for name, lst in [(options.dpy_, self.graphicmodeslist()),
                          (options.snd_, self.soundmodeslist())]:
            try:
                mode = display.modes.findmode(name, lst)
            except KeyError:
                try:
                    mode = display.modes.findmode(None, lst)
                except KeyError as e:
                    print(str(e), file=sys.stderr)  # no mode!
                    mode = None
            currentmodes.append(mode)
        return currentmodes

    def buildclientoptions(self):
        dpy, snd = self.localmodes()
        if dpy.getmodule() is None:
            return None  # redirect to the Java applet
        if dpy is None or snd is None:
            raise HTTPRequestError("No installed graphics or sounds drivers. See the settings page.")
        options = self.localoptions
        result = ['--cfg='+no_quote_worries(self.filename)]
        for key, value in list(options.dict().items()):
            if key.startswith('port_') and value:
                result.append('--port')
                result.append('%s=%s' % (key[5:], value))
        if options.datachannel == 'tcp': result.append('--tcp')
        if options.datachannel == 'udp': result.append('--udp')
        if options.music       == 'no':  result.append('--music=no')
        for optname, mode in [('--display', dpy),
                              ('--sound',   snd)]:
            result.append(optname + '=' + mode.name)
            uid = mode.unique_id() + '_'
            for key, value in list(options.dict().items()):
                if key.startswith(uid):
                    result.append('--%s=%s' % (key[len(uid):], value))
        return result

def my_host(headers):
    return headers.get('host') or httpserver.my_host()

def my_server():
    if gamesrv.game:
        s = gamesrv.opentcpsocket()
        return ((gamesrv.HOSTNAME, gamesrv.displaysockport(s)),
                gamesrv.game.FnDesc)
    else:
        return None

def my_server_address():
    running = my_server()
    if running:
        (host, port), info = running
        return '%s:%d' % (host, port)
    else:
        return None

##def my_server_meta_address():
##    s = gamesrv.opentcpsocket()
##    ps = gamesrv.openpingsocket()
##    hs = gamesrv.openhttpsocket()
##    fullname = gamesrv.HOSTNAME
##    try:
##        fullname = socket.gethostbyaddr(fullname)[0]
##    except socket.error:
##        pass
##    return '%s:%s:%s:%s' % (fullname,
##                            gamesrv.displaysockport(s),
##                            gamesrv.displaysockport(ps),
##                            gamesrv.displaysockport(hs))

##def meta_register():
##    # Note: this tries to open a direct HTTP connection to the meta-server
##    #       which may not work if the proxy is not configured in $http_proxy
##    try:
##        import urllib
##    except ImportError:
##        print >> sys.stderr, "cannot register with the meta-server: Python's urllib missing"
##        return
##    print "registering with the meta-server...",
##    sys.stdout.flush()
##    addr = my_server_meta_address()
##    try:
##        f = urllib.urlopen('%s?a=%s&desc=%s' % (
##            METASERVER, addr, quote_plus(gamesrv.game.FnDesc)))
##        f.close()
##    except Exception, e:
##        print
##        print >> sys.stderr, "cannot contact the meta-server (check $http_proxy):"
##        print >> sys.stderr, "%s: %s" % (e.__class__.__name__, e)
##    else:
##        print "ok"
##        unregister_at_exit(addr)

##def meta_unregister(addr):
##    import urllib
##    print "unregistering from the meta-server...",
##    sys.stdout.flush()
##    try:
##        f = urllib.urlopen(METASERVER + '?d=' + addr)
##        f.close()
##    except Exception, e:
##        print "failed"
##    else:
##        print "ok"

##def unregister_at_exit(addr, firsttime=[1]):
##    if firsttime:
##        import atexit
##        atexit.register(meta_unregister, addr)
##        del firsttime[:]

QuoteTranslation = {}
for c in ('ABCDEFGHIJKLMNOPQRSTUVWXYZ'
          'abcdefghijklmnopqrstuvwxyz'
          '0123456789' '_.-'):
    QuoteTranslation[c] = c
del c
QuoteTranslation[' '] = '+'

def quote_plus(s):
    """Quote the query fragment of a URL; replacing ' ' with '+'"""
    getter = QuoteTranslation.get
    return ''.join([getter(c, '%%%02X' % ord(c)) for c in s])


def main(Game, save_url_to=None, quiet=0):
    #gamesrv.openpingsocket(0)  # try to reserve the standard UDP port
    srv = PageServer(Game)
    srv.registerpages()
    if not srv.opensocket():
        print("server aborted.", file=sys.stderr)
        sys.exit(1)
    if quiet:
        if Game:
            Game.Quiet = 1
        import stdlog
        f = stdlog.LogFile()
        if f:
            print("Logging to", f.filename)
            sys.stdout = sys.stderr = f
    if save_url_to:
        data = srv.indexurl + '\n'
        def try_to_unlink(fn):
            try:
                os.unlink(fn)
            except:
                pass
        import atexit
        atexit.register(try_to_unlink, save_url_to)
        try:
            fno = os.open(save_url_to, os.O_CREAT | os.O_TRUNC | os.O_WRONLY,
                          0o600)
            if os.write(fno, data) != len(data):
                raise OSError
            os.close(fno)
        except:
            f = open(save_url_to, 'w')
            f.write(data)
            f.close()
    #if webbrowser:
    #    srv.launchbrowser()


# ____________________________________________________________
# Hack hack hack - workaround for the fact that on Windows
# the socket is inherited by the subprocess, which is quite
# bad because it keeps the browser-server connexion alive
# and the browser gets confused


def schedule_launch(args):
    httpserver.actions_when_finished.append(lambda args=args: launch(args))

def launch(args):
    # platform-specific hacks
    print('Running client ->  ', ' '.join(args))
    if 0:  # OLD CODE sys.platform == 'darwin':   # must start as a UI process
        import tempfile
        cmdname = tempfile.mktemp('_BubBob.py')
        f = open(cmdname, 'w')
        print('import sys, os', file=f)
        print('try: os.unlink(%r)' % cmdname, file=f)
        print('except OSError: pass', file=f)
        print('sys.argv[:] = %r' % (args,), file=f)
        print('__file__ = %r' % cmdname, file=f)
        print('execfile(%r)' % args[0], file=f)
        f.close()
        os.system('/usr/bin/open -a PythonLauncher "%s"' % cmdname)
    else:
        args.insert(0, sys.executable)
        # try to close the open fds first
        if hasattr(os, 'fork'):
            try:
                from resource import getrlimit, RLIMIT_NOFILE, error
            except ImportError:
                pass
            else:
                try:
                    soft, hard = getrlimit(RLIMIT_NOFILE)
                except error:
                    pass
                else:
                    if os.fork():
                        return # in parent -- done, continue
                    # in child
                    for fd in range(3, min(16384, hard)):
                        try:
                            os.close(fd)
                        except OSError:
                            pass
                    os.execv(args[0], args)
                    # this point should never be reached
        # fall-back
        # (quoting sucks on Windows) ** 42
        if sys.platform == 'win32':
            args[0] = '"%s"' % (args[0],)
        if hasattr(os, 'P_DETACH'):
            mode = os.P_DETACH
        elif hasattr(os, 'P_NOWAIT0'):
            mode = os.P_NOWAIT0
        else:
            mode = os.P_NOWAIT
        os.spawnv(mode, sys.executable, args)

if sys.platform != "win32":
    def no_quote_worries(s):
        return s
else:
    def no_quote_worries(s):      # quoting !&?+*:-(
        s = os.path.normpath(os.path.abspath(s))
        absroot = os.path.join(LOCALDIR, os.pardir)
        absroot = os.path.normpath(os.path.abspath(absroot))
        ROOTDIR = os.curdir
        while os.path.normpath(os.path.abspath(ROOTDIR)) != absroot:
            if ROOTDIR == os.curdir:
                ROOTDIR = os.pardir
            else:
                ROOTDIR = os.path.join(ROOTDIR, os.pardir)
            if len(ROOTDIR) > 200:
                # cannot find relative path!  try with absolute one anyway
                ROOTDIR = absroot
                break
        assert s.startswith(absroot)
        if absroot.endswith(os.sep):    #  'C:\'
            absroot = absroot[:-1]
        assert s[len(absroot)] == os.sep
        relpath = s[len(absroot)+1:]
        result = os.path.join(ROOTDIR, relpath)
        print("no_quote_worries %r => %r" % (s, result))
        return result


if __name__ == '__main__':
    if (len(sys.argv) != 3 or sys.argv[1] != '--quiet' or
        not sys.argv[2].startswith('--saveurlto=')):
        print("This script should only be launched by BubBob.py.", file=sys.stderr)
        sys.exit(2)
    main(None, sys.argv[2][len('--saveurlto='):], quiet=1)
    gamesrv.mainloop()
