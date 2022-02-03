import sys
from io import StringIO
from . import puremixer
from .music1 import Music


class Sound:
    # Mono only
    has_sound = has_music = 0   # until initialized

    BUFFERTIME = 0.09
    FLOPTIME   = 0.07

    Formats = [
        ('U8',     8, 0, None),
        ('S8',     8, 1, None),
        ('S16_NE', 16, 1, None),
        ('S16_LE', 16, 1, 'little'),
        ('S16_BE', 16, 1, 'big'),
        ('U16_LE', 16, 0, 'little'),
        ('U16_BE', 16, 0, 'big'),
        ]

    def __init__(self, freq=44100, fmt='S16_NE'):
        self.f = None
        self.freq = int(freq)
        self.format = fmt.upper()
        self.params = p = {}
        for name, p['bits'], p['signed'], p['byteorder'] in self.Formats:
            if name == self.format:
                break
        else:
            print('available sound formats:', file=sys.stderr)
            for name, bits, signed, byteorder in self.Formats:
                print('  %-8s  %s' % (name, nicefmttext(
                    bits, signed, byteorder)), file=sys.stderr)
            sys.exit(2)

        import linuxaudiodev
        try:
            f = linuxaudiodev.open('w')
            f.setparameters(self.freq, p['bits'], 1,
                            getattr(linuxaudiodev, 'AFMT_' + self.format))
        except Exception as e:
            print("sound disabled: %s: %s" % (
                e.__class__.__name__, e), file=sys.stderr)
            return
        self.f = f
        self.mixer = mixer = puremixer.PureMixer(**p)
        buffertime = self.BUFFERTIME
        self.bufsize = int(mixer.bytespersample*mixer.freq*buffertime +
                           255.5) & ~ 255
        if self.bufsize > f.bufsize():
            self.bufsize = f.bufsize()
            buffertime = self.bufsize / float(freq)
        self.buffertime = buffertime
        self.mixer_channels = []
        self.mixer_accum = {}
        self.has_sound = 1
        self.has_music = 1

    def close(self):
        self.f.close()
        self.f = None

    def sound(self, f):
        return self.mixer.wavesample(f.fopen())

    def flop(self):
        self.mixer_accum = {}
        if self.f is None:
            return
        for i in range(3):
            bufsize = self.bufsize - self.f.obufcount()
            if bufsize <= 0:
                break
            self.f.write(self.mixer.mix(self.mixer_channels, bufsize))
        #cnt = getattr(self, 'CNT', 0)
        #import time
        #print cnt, time.time()
        #self.CNT = cnt+1
        return self.FLOPTIME

    def play(self, sound, lvolume, rvolume):
        # volume ignored
        if sound not in self.mixer_accum:
            self.mixer_channels.append(StringIO(sound))
            self.mixer_accum[sound] = 1

    def play_musics(self, musics, loop_from):
        self.cmusics = musics, loop_from, -1
        if self.mixer_channels[:1] != [self]:
            self.mixer_channels.insert(0, self)

    def read(self, size):
        "Provide some more data to self.mixer.poll()."
        musics, loop_from, c = self.cmusics
        if c < 0:
            data = ''
        else:
            data = musics[c].mixed.decode(self.mixer, size)
        if not data:
            c += 1
            if c >= len(musics):  # end
                c = loop_from
                if c >= len(musics):
                    return ''
            self.cmusics = musics, loop_from, c
            try:
                mixed = musics[c].mixed
            except AttributeError:
                mixed = musics[c].mixed = Music(musics[c].freezefilename())
            mixed.openchannel()
            data = mixed.decode(self.mixer, size)
        if 0 < len(data) < size:
            data += self.read(size - len(data))
        return data

    def fadeout(self, millisec):
        self.cmusics = [], 0, -1


def imperror():
    try:
        import linuxaudiodev
    except ImportError:
        if sys.platform.startswith('linux'):
            return 'linuxaudiodev module not installed'
        else:
            return 'only available on Linux'

def nicefmttext(bits, signed, byteorder):
    s = '%s %d bits' % (signed and 'signed' or 'unsigned', bits)
    if byteorder:
        s += ' %s endian' % byteorder
    return s

def htmloptionstext(nameval):
    from . import modes
    l = ['<font size=-1>Sampling <%s>' % nameval('select', 'fmt')]
    for name, bits, signed, byteorder in Sound.Formats:
        l.append('<'+nameval('option', 'fmt', name, default='S16_NE')+'>'+
                 nicefmttext(bits, signed, byteorder))
    l+= ['</select> rate ',
         '<%s size=5>Hz</font>' % nameval('text', 'freq', default='44100'),
         '<br>',
         modes.musichtmloptiontext(nameval)]
    return '\n'.join(l)
