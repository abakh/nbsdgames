import sys

KeyPressed  = 2
KeyReleased = 3


class BaseDisplay:
    __taskbkgnd = None
    
    def taskbar(self, (x, y, w, h)):
        if self.__taskbkgnd is None:
            pixel = "\x00\x00\x80"
            hole  = "\x01\x01\x01"
            self.__taskbkgnd = self.pixmap(32, 32,
                ((pixel+hole)*16 + (hole+pixel)*16) * 16, 0x010101)
        for j in range(y, y+h, 32):
            for i in range(x, x+w, 32):
                self.putppm(i, j, self.__taskbkgnd,
                            (0, 0, x+w-i, y+h-j))


class Mode:
    low_priority = 0
    
    def __init__(self, name, descr, extraoptsdescr,
                 options={}, url=None):
        self.name = name
        self.descr = descr
        self.extraoptsdescr = extraoptsdescr
        self.options = options.copy()
        self.url = url

    def getmodule(self):
        return __import__(self.prefix + self.name.lower(), globals(),
                          locals(), ['available'])

    def imperror(self):
        try:
            return self.__imperror
        except AttributeError:
            try:
                module = self.getmodule()
            except ImportError:
                result = 'not installed'
            else:
                result = hasattr(module, 'imperror') and module.imperror()
            self.__imperror = result
        return result

    def unique_id(self):
        return self.prefix + self.name
    
    def printline(self, f):
        err = self.imperror()
        if err:
            state = ' [%s]' % err
        else:
            state = ''
        print >> f, '  %-8s %s%s' % (self.name, self.descr, state)
        if self.url:
            print >> f, '             %s' % self.url
        for line in self.extraoptsdescr:
            print >> f, '             %s' % line

    def getformaloptions(self):
        return '', [c+'=' for c in self.options.keys()]

    def setoptions(self, options):
        for key in self.options.keys():
            if options.has_key('--'+key):
                self.options[key] = options['--'+key]

    def currentdriver(self):
        lst = self.options.items()
        lst.sort()
        lst = ['--%s=%s' % keyvalue for keyvalue in lst]
        return ' '.join([self.name] + lst)

    def htmloptionstext(self, *args):
        if self.imperror():
            return None
        module = self.getmodule()
        return (hasattr(module, 'htmloptionstext') and
                module.htmloptionstext(*args))


class GraphicMode(Mode):
    prefix = 'dpy_'


class SoundMode(Mode):
    prefix = 'snd_'


def graphicmodeslist():
    return [
        GraphicMode('X', 'XWindow (Linux/Unix)',
                    ['--shm=yes  use the Shared Memory extension (default)',
                     '--shm=no   disable it (for remote connections or old X servers)',
                     ],
                    {'shm': 'yes'}),
        GraphicMode('windows', 'MS Windows', []),
        GraphicMode('pygame', 'PyGame library (all platforms)',
                    ['--fullscreen=yes    go full screen (Esc key to exit)',
                     '--transparency=yes  slightly transparent bubbles (default)',
                     '--transparency=no   disable it (a bit faster)'],
                    {'transparency': 'yes', 'fullscreen': 'no'},
                    url='http://www.pygame.org'),
        GraphicMode('gtk', 'PyGTK (Gnome)',
                    ['--zoom=xxx%         scale image by xxx %'],
                    {'zoom': '100'},
                    url='http://www.pygtk.org/'),
        ]

def soundmodeslist():
    return [
        SoundMode('pygame', 'PyGame library mixer (all platforms)',
                  [], url='http://www.pygame.org'),
        SoundMode('linux', 'audio mixer for Linux',
                  ['--freq=#  mixer frequency (default 44100)',
                   '--fmt=#   data format (default S16_NE, --fmt=list for a list)'],
                  {'freq': '44100', 'fmt': 'S16_NE'}),
        SoundMode('windows', 'audio mixer for Windows',
                  ['--freq=#  mixer frequency (default 44100)',
                   '--bits=#  bits per sample (8 or default 16)'],
                  {'freq': '44100', 'bits': '16'}),
        SoundMode('off', 'no sounds', []),
        ]

def findmode(name, lst):
    if name is None:
        # find the first installed mode
        last_chance = None
        for info in lst:
            err = info.imperror()
            if err:
                continue
            if info.low_priority:
                if last_chance is None:
                    last_chance = info
            else:
                return info
        if last_chance is not None:
            return last_chance
        raise KeyError, 'no driver available!'
    else:
        # find mode by name
        for info in lst:
            if info.name.upper() == name.upper():
                err = info.imperror()
                if err:
                    raise KeyError, '%s: %s' % (info.name, err)
                return info
        raise KeyError, '%s: no such driver' % name

def findmode_err(*args):
    try:
        return findmode(*args)
    except KeyError, e:
        print >> sys.stderr, str(e)
        sys.exit(1)

def open_dpy(mode, width, height, title):
    driver, sound, extraopts = mode
    ginfo = findmode_err(driver, graphicmodeslist())
    ginfo.setoptions(extraopts)
    dpy = ginfo.getmodule().Display(width, height, title, **ginfo.options)
    print 'graphics driver:', ginfo.currentdriver()
    return dpy

def open_snd(mode):
    driver, sound, extraopts = mode
    sinfo = findmode_err(sound, soundmodeslist())
    sinfo.setoptions(extraopts)
    snd = sinfo.getmodule().Sound(**sinfo.options)
    if snd.has_sound:
        sinfo.options['music'] = 'yes'
        sinfo.setoptions(extraopts)
        if (sinfo.options['music'].startswith('n') or
            sinfo.options['music'] == 'off'):
            snd.has_music = 0
        print 'sound driver:', sinfo.currentdriver()
        return snd
    else:
        return None


def musichtmloptiontext(nameval):
    return '''<font size=-1>
<%s> Background music</input><%s>
</font>''' % (nameval("checkbox", "music", "yes", default="yes", mangling=0),
              nameval("hidden", "music", "no", mangling=0))
