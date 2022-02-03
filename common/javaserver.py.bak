import sys, os
from cStringIO import StringIO
import httpserver

PLAYERNAMES = ['Bub', 'Bob', 'Boob', 'Beb',
               'Biob', 'Bab', 'Bib',
               'Baub', 'Beab', 'Biab']
LOCALDIR = os.path.abspath(os.path.dirname(__file__))
DATADIR  = os.path.join(LOCALDIR, os.pardir, 'http2', 'data')

EMPTY_PAGE = '''<html>
<head><title>No server is running</title></head>
<body><h1>No server is running at the moment.</h1>
</body>
</html>
'''

INDEX_PAGE = '''<html>
<head><title>%(title)s</title></head>
<body><h1>%(title)s</h1>
 <applet code=pclient.class width=%(width)s height=%(height)s>
  <param name="gameport" value="%(gameport)d">
  %(names1)s
 </applet>
<br>
<p align="center"><a href="name.html?%(names2)s">Player Names &amp; Teams</a></p>
</body>
</html>
'''

NAME_LINE1 = '<param name="%s" value="%s">'
NAME_SEP1  = '\n'
NAME_LINE2 = '%s=%s'
NAME_SEP2  = '&'

def playernames(options):
    NUM_PLAYERS = len(PLAYERNAMES)
    result = {}
    anyname = None
    for id in range(NUM_PLAYERS):
        keyid = 'player%d' % id
        if keyid in options:
            value = options[keyid][0]
            anyname = anyname or value
            teamid = 'team%d' % id
            if teamid in options:
                team = options[teamid][0]
                if len(team) == 1:
                    value = '%s (%s)' % (value, team)
            result[keyid] = value
    if 'c' in options:
        for id in range(NUM_PLAYERS):
            keyid = 'player%d' % id
            try:
                del result[keyid]
            except KeyError:
                pass
    if 'f' in options:
        for id in range(NUM_PLAYERS):
            keyid = 'player%d' % id
            if not result.get(keyid):
                result[keyid] = anyname or PLAYERNAMES[id]
            else:
                anyname = result[keyid]
    return result

def indexloader(**options):
    if 'cheat' in options:
        for opt in options.pop('cheat'):
            __cheat(opt)
    import gamesrv
    if gamesrv.game is None:
        indexdata = EMPTY_PAGE
    else:
        names = playernames(options).items()
        indexdata = INDEX_PAGE % {
            'title':    gamesrv.game.FnDesc,
            'width':    gamesrv.game.width,
            'height':   gamesrv.game.height,
            'gameport': gamesrv.game.address[1],
            'names1':   NAME_SEP1.join([NAME_LINE1 % kv for kv in names]),
            'names2':   NAME_SEP2.join([NAME_LINE2 % kv for kv in names]),
            }
    return StringIO(indexdata), 'text/html'

def nameloader(**options):
    if 's' in options:
        return indexloader(**options)
    locals = {
        'options': playernames(options),
        }
    return httpserver.load(os.path.join(DATADIR, 'name.html'),
                           'text/html', locals=locals)


wave_cache = {}

def wav2au(data):
    # Very limited! Assumes a standard 8-bit mono .wav as input
    import audioop, struct
    freq, = struct.unpack("<i", data[24:28])
    data = data[44:]
    data = audioop.bias(data, 1, -128)
    data, ignored = audioop.ratecv(data, 1, 1, freq, 8000, None)
    data = audioop.lin2ulaw(data, 1)
    data = struct.pack('>4siiiii8s',
                       '.snd',                         # header
                       struct.calcsize('>4siiiii8s'),  # header size
                       len(data),                      # data size
                       1,                              # encoding
                       8000,                           # sample rate
                       1,                              # channels
                       'magic.au') + data
    return data

def sampleloader(code=[], **options):
    import gamesrv
    try:
        data = wave_cache[code[0]]
    except KeyError:
        for key, snd in gamesrv.samples.items():
            if str(getattr(snd, 'code', '')) == code[0]:
                data = wave_cache[code[0]] = wav2au(snd.read())
                break
        else:
            raise KeyError, code[0]
    return StringIO(data), 'audio/wav'


def setup():
    dir = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                       os.pardir,
                                       'java'))
    if not os.path.isdir(dir):
        return

    # register all '.class' files
    for name in os.listdir(dir):
        if name.endswith('.class'):
            httpserver.register(name, httpserver.fileloader(os.path.join(dir, name)))

    # register a '', an 'index.html', and a 'name.html' file
    httpserver.register('', indexloader)
    httpserver.register('index.html', indexloader)
    httpserver.register('name.html',  nameloader)

    # 'name.html' has a few images, list the .png files in DATADIR
    for fn in os.listdir(DATADIR):
        fn = fn.lower()
        if fn.endswith('.png'):
            httpserver.register(fn, httpserver.fileloader(
                os.path.join(DATADIR, fn)))

    # register the sample loader
    httpserver.register('sample.wav', sampleloader)

setup()
