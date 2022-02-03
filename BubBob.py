#! /usr/bin/env python

#
#  This script is used to start the server.
#  For command-line usage please run
#
#    python bubbob/bb.py --help
#

# __________
import os, sys
print('Running on Python', sys.version)
if __name__ == '__main__':
    LOCALDIR = sys.argv[0]
else:
    LOCALDIR = __file__
try:
    LOCALDIR = os.readlink(LOCALDIR)
except:
    pass
sys.argv[0] = os.path.abspath(LOCALDIR)
LOCALDIR = os.path.dirname(sys.argv[0])
# ----------

import socket, tempfile

sys.path.insert(0, LOCALDIR)
os.chdir(LOCALDIR)

try:
    username = '-'+os.getlogin()
except:
    try:
        import pwd
        username = '-'+pwd.getpwuid(os.getuid())[0]
    except:
        username = ''
TAGFILENAME = 'BubBob-%s%s.url' % (socket.gethostname(), username)
TAGFILENAME = os.path.join(tempfile.gettempdir(), TAGFILENAME)


def load_url_file():
    try:
        url = open(TAGFILENAME, 'r').readline().strip()
    except (OSError, IOError):
        return None, None
    if not url.startswith('http://127.0.0.1:'):
        return None, None
    url1 = url[len('http://127.0.0.1:'):]
    try:
        port = int(url1[:url1.index('/')])
    except ValueError:
        return None, None
    return url, port

def look_for_local_server():
    # Look for a running local web server
    url, port = load_url_file()
    if port is None:
        return None
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect(('127.0.0.1', port))
    except socket.error as e:
        return None
    try:
        s.shutdown(2)
        s.close()
    except Exception as e:
        pass
    url2, port2 = load_url_file()
    if port2 != port:
        return None
    return url2

def start_local_server():
    MAINSCRIPT = os.path.join('bubbob', 'bb.py')
    has_server = os.path.exists(MAINSCRIPT)
    if hasattr(os, 'fork') and hasattr(os, 'dup2'):
        if os.fork() == 0:
            # in the child process
            if has_server:
                sys.path.insert(0, os.path.join(LOCALDIR, 'bubbob'))
                import bb
                bb.BubBobGame.Quiet = 1
            else:
                sys.path.insert(0, os.path.join(LOCALDIR, 'http2'))
                import httppages
            import gamesrv, stdlog
            logfile = stdlog.LogFile()
            if has_server:
                bb.start_metaserver(TAGFILENAME, 0)
            else:
                httppages.main(None, TAGFILENAME, 0)
            if logfile:
                print(file=logfile)
                if logfile:
                    print("Logging to", logfile.filename)
                    fd = logfile.f.fileno()
                    try:
                        # detach from parent
                        os.dup2(fd, 1)
                        os.dup2(fd, 2)
                        os.dup2(fd, 0)
                    except OSError:
                        pass
                    logfile.close()
            gamesrv.mainloop()
            sys.exit(0)
    else:
        if not has_server:
            MAINSCRIPT = os.path.join('http2', 'httppages.py')
        args = [sys.executable, MAINSCRIPT, '--quiet',
                '--saveurlto=%s' % TAGFILENAME]
        # (quoting sucks on Windows) ** 42
        if sys.platform == 'win32':
            args[0] = '"%s"' % (args[0],)
        os.spawnv(os.P_NOWAITO, sys.executable, args)


# main
url = look_for_local_server()
if not url:
    start_local_server()
    # wait for up to 5 seconds for the server to start
    for i in range(10):
        import time
        time.sleep(0.5)
        url = look_for_local_server()
        if url:
            break
    else:
        print('The local server is not starting, giving up.', file=sys.stderr)
        sys.exit(1)

try:
    import webbrowser
    browser = webbrowser.get()
    name = getattr(browser, 'name', browser.__class__.__name__)
    print("Trying to open '%s' with '%s'..." % (url, name))
    browser.open(url)
except:
    exc, val, tb = sys.exc_info()
    print('-'*60)
    print("Failed to launch the web browser:", file=sys.stderr)
    print("  %s: %s" % (exc.__name__, val), file=sys.stderr)
    print()
    print("Sorry, I guess you have to go to the following URL manually:")
else:
    print("Done running '%s'." % name)
    if look_for_local_server() != url:
        # assume that browser.open() waited for the browser to finish
        # and that the server has been closed from the browser.
        raise SystemExit
    print()
    print('-'*60)
    print("If the browser fails to open the page automatically,")
    print("you will have to manually go to the following URL:")
print(' ', url)
print('-'*60)
print("Note that the server runs in the background. You have to use")
print("the 'Stop this program' link to cleanly stop it.")
print("Normally, however, running this script multiple times should")
print("not create multiple servers in the background.")
