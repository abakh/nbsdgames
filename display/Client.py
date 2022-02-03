#! /usr/bin/env python

# __________
import os, sys
if __name__ == '__main__':
    LOCALDIR = sys.argv[0]
else:
    LOCALDIR = __file__
try:
    LOCALDIR = os.readlink(LOCALDIR)
except:
    pass
LOCALDIR = os.path.dirname(os.path.abspath(LOCALDIR))
# ----------

sys.path.insert(0, os.path.dirname(LOCALDIR))
sys.path.insert(0, LOCALDIR)
import common
from . import pclient
from . import modes


UdpLookForServer = [
    '127.0.0.1',
    '<broadcast>',
    ]

def parse_cmdline(argv):
    # parse command-line
    def usage():
        print('usage:', file=sys.stderr)
        print('  python Client.py [-d#] [-s#] [extra options] [host[:port]]', file=sys.stderr)
        print(file=sys.stderr)
        print('options:', file=sys.stderr)
        print('  host              search for a game on the given machine', file=sys.stderr)
        print('  host:port         connect to the given game server', file=sys.stderr)
        print('                      (default search for any local server)', file=sys.stderr)
        print('  -d#  --display=#  graphic driver (see below)', file=sys.stderr)
        print('  -s#  --sound=#    sound driver (see below)', file=sys.stderr)
        print('       --music=no   disable background music', file=sys.stderr)
        print('  -h   --help       display this text', file=sys.stderr)
        print('  -m   --metaserver connect with the help of the metaserver', file=sys.stderr)
        print('                      (list servers with Client.py -m)', file=sys.stderr)
        print('  -t   --tcp        for slow or proxy connections', file=sys.stderr)
        print('  -u   --udp        for fast direct connections', file=sys.stderr)
        print('                      (default is to autodetect tcp or udp)', file=sys.stderr)
        print('  --port UDP=# or #:#   fixed inbound udp port or host:port', file=sys.stderr)
        print('  --port TCP=#          fixed inbound tcp port (-m only)', file=sys.stderr)
        print(file=sys.stderr)
        print('graphic drivers:', file=sys.stderr)
        for info in modes.graphicmodeslist():
            info.printline(sys.stderr)
        print(file=sys.stderr)
        print('sound drivers:', file=sys.stderr)
        for info in modes.soundmodeslist():
            info.printline(sys.stderr)
        print(file=sys.stderr)
        sys.exit(2)

    shortopts = 'd:s:htum'
    longopts = ['display=', 'sound=', 'music=', 'help', 'tcp', 'udp',
                'cfg=', 'metaserver', 'port=']
    for info in modes.graphicmodeslist() + modes.soundmodeslist():
        short, int = info.getformaloptions()
        shortopts += short
        longopts += int
    try:
        from getopt import gnu_getopt as getopt
    except ImportError:
        from getopt import getopt
    from getopt import error
    try:
        opts, args = getopt(argv, shortopts, longopts)
    except error as e:
        print('Client.py: %s' % str(e), file=sys.stderr)
        print(file=sys.stderr)
        usage()

    metaserver = 0
    driver = sound = None
    extraopts = {}
    for key, value in opts:
        if key in ('-d', '--display'):
            driver = value
        elif key in ('-s', '--sound'):
            sound = value
        elif key in ('-t', '--tcp'):
            extraopts['udp_over_tcp'] = 1
        elif key in ('-u', '--udp'):
            extraopts['udp_over_tcp'] = 0
        elif key in ('-m', '--metaserver'):
            metaserver = 1
        elif key == '--port':
            import common.msgstruct
            try:
                portname, value = value.split('=')
                if portname == 'UDP':
                    portname = 'CLIENT'
                elif portname == 'TCP':
                    portname = 'BACK'
            except ValueError:
                portname = 'CLIENT'
            if portname == 'CLIENT' and ':' in value:
                udphostname, value = value.split(':')
                common.msgstruct.PORTS['sendudpto'] = udphostname
            common.msgstruct.PORTS[portname] = int(value)
        elif key == '--cfg':
            extraopts['cfgfile'] = value
        elif key in ('-h', '--help'):
            usage()
        else:
            extraopts[key] = value
    mode = driver, sound, extraopts

    if metaserver:
        if len(args) == 0:
            metalist()
            sys.exit(0)
        elif len(args) != 1 or ':' not in args[0]:
            usage()
        return metaconnect(args[0]), mode

    if args:
        if len(args) > 1:
            usage()
        hosts = args[0].split(':')
        if len(hosts) == 1:
            host, = hosts
            from common import hostchooser
            server = hostchooser.pick([host] * 5)
        elif len(hosts) == 2:
            host, port = hosts
            try:
                port = int(port)
            except ValueError:
                usage()
            server = host, port
        else:
            usage()
        return directconnect(server), mode

    from common import hostchooser
    server = hostchooser.pick(UdpLookForServer * 3)
    return directconnect(server), mode

def directconnect(sockaddr):
    print("connecting to %s:%d..." % sockaddr)
    from socket import socket, AF_INET, SOCK_STREAM
    s = socket(AF_INET, SOCK_STREAM)
    s.connect(sockaddr)
    return s, sockaddr

def metaconnect(metaaddr):
    from metaserver import metaclient
    import common.msgstruct
    port = common.msgstruct.PORTS.get('BACK')
    s = metaclient.meta_connect(metaaddr, port)
    sockaddr = s.getpeername()
    return s, sockaddr

def metalist():
    from metaserver import metaclient
    metaclient.print_server_list()

def main():
    (s, sockaddr), mode = parse_cmdline(sys.argv[1:])
    pclient.run(s, sockaddr, mode)

if __name__ == '__main__':
    main()
