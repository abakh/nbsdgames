
import gamesrv, os
from sprmap import sprmap as original_sprmap
from patmap import patmap
import mnstrmap
import pixmap

KEYCOL = 0x010101
MAX = 10

ActiveSprites = []
SpritesByLoc = {}


class ActiveSprite(gamesrv.Sprite):
    touchable = 0
    imgsetter = None
    angry = []
    priority = 0
    
    def __init__(self, *args):
        gamesrv.Sprite.__init__(self, *args)
        if self.priority:
            ActiveSprites.insert(0, self)
        else:
            ActiveSprites.append(self)
        self.ranges = []
        self.gen = []

    def kill(self):
        self.untouchable()
        del self.gen[:]
        ActiveSprites.remove(self)
        gamesrv.Sprite.kill(self)

    def untouchable(self):
        self.touchable = 0
        for key in self.ranges:
            del key[self]
        del self.ranges[:]

    def play(self, snd, volume=0.8):
        import boards
        xmin = 2*boards.CELL
        xmax = boards.bwidth-4*boards.CELL
        snd.play(volume, pad=float(self.x-xmin)/(xmax-xmin))

    def setimages(self, gen):
        if self.imgsetter is not None:
            try:
                self.gen.remove(self.imgsetter)
            except ValueError:
                pass
        self.imgsetter = gen
        if gen is not None:
            self.gen.append(gen)

    def vertical_warp(self):
        # short-cut this method to boards.py
        import boards
        ActiveSprite.vertical_warp = boards.vertical_warp_sprite
        self.vertical_warp()
##            if moebius:
##                self.moebius()
##        return moebius
##    def moebius(self):
##        pass

    # common generators
    def cyclic(self, nimages, speed=5):
        images = [sprget(n) for n in nimages]
        speed = list(range(speed))
        while 1:
            for img in images:
                self.seticon(img)
                for i in speed:
                    yield None

    def imgseq(self, nimages, speed=5, repeat=1):
        images = [sprget(n) for n in nimages]
        for r in range(repeat):
            for img in images:
                self.seticon(img)
                for i in range(speed):
                    yield None

    def die(self, nimages, speed=1):
        for n in nimages:
            if n is not None:
                self.seticon(sprget(n))
            for i in range(speed):
                yield None
        self.kill()

    def straightline(self, dx, dy):
        fx = self.x + 0.5
        fy = self.y + 0.5
        while 1:
            fx += dx
            fy += dy
            self.move(int(fx), int(fy))
            yield None

    def parabolic(self, dxy, warp=0, gravity=0.3):
        import boards
        from boards import CELL
        nx = self.x
        ny = self.y
        dx, dy = dxy
        xmax = boards.bwidth - 2*CELL - self.ico.w
        while ny < boards.bheight:
            nx += dx
            ny += dy
            dy += gravity
            if nx < 2*CELL:
                nx = 2*CELL
                dx = abs(dx)
            elif nx >= xmax:
                nx = xmax
                dx = -abs(dx)
            if warp and (ny < -2*CELL or ny >= boards.bheight):
                nx, ny = boards.vertical_warp(nx, ny)
##                if moebius:
##                    self.moebius()
##                    dx = -dx
            self.move(int(nx), int(ny))
            dxy[:] = [dx, dy]
            yield None

    def following(self, other, dx=0, dy=0):
        while other.alive:
            self.move(other.x + dx, other.y + dy)
            yield None
        self.kill()

    def touchdelay(self, delay):
        for i in range(delay):
            yield None
        self.touchable = 1

    def touching(self, margin=0):
        return touching(self.x, self.y, self.ico.w, self.ico.h, margin)

    def genangry(self):
        # do one more step throught all generators of self.gen
        while 1:
            glist = self.gen[:]
            try:
                for g in glist:
                    if self.alive:
                        next(g)
            except StopIteration:
                try:
                    self.gen.remove(g)
                except ValueError:
                    pass
                for g in glist[glist.index(g)+1:]:
                    if self.alive:
                        try:
                            next(g)
                        except StopIteration:
                            pass
            yield None

def touching(x1, y1, w1, h1, margin=0):
    touch = {}
    x1 = int(x1)
    y1 = int(y1)
    xrange = list(range(x1>>5, (x1+w1+31)>>5))
    for y in range(y1>>4, (y1+h1+15)>>4):
        for x in xrange:
            touch.update(SpritesByLoc.get((x,y), {}))
    return [s for s in touch
            if x1+margin < s.x+s.ico.w and y1+margin < s.y+s.ico.h and
               s.x+margin < x1+w1 and s.y+margin < y1+h1]

def action(sprlist, len=len):
    # Main generator dispatch loop
    for self in sprlist:
        glist = self.gen + self.angry
        try:
            for g in glist:
                if self.alive:
                    next(g)
        except StopIteration:
            try:
                self.gen.remove(g)
            except ValueError:
                pass
            for g in glist[glist.index(g)+1:]:
                if self.alive:
                    try:
                        next(g)
                    except StopIteration:
                        pass
        if self.touchable and self.alive:
            # record position
            x = self.x & -8
            y = self.y & -8
            if self.touchable != (x, y):
                self.touchable = x, y
                for key in self.ranges:
                    del key[self]
                del self.ranges[:]
                xrange = list(range(x>>5, (x+self.ico.w+38)>>5))
                for y in range(y>>4, (y+self.ico.h+22)>>4):
                    for x in xrange:
                        key = SpritesByLoc.setdefault((x,y), {})
                        key[self] = 1
                        self.ranges.append(key)

def sprget(n, spriconcache={}):
    try:
        return spriconcache[n]
    except KeyError:
        key = n
        if isinstance(key, tuple) and key[0] in Transformations:
            t, n = key
            transform = Transformations[t]
        else:
            transform = transform_noflip
        filename, rect = sprmap[n]
        bitmap, rect = transform(filename, rect)
        if isinstance(n, tuple):
            n1 = n[0]
        else:
            n1 = n
        if isinstance(n1, int):
            n1 = n1 % 1000
        alpha = transparency.get(n1, 255)
        ico = bitmap.geticon(alpha=alpha, *rect)
        spriconcache[key] = ico
        return ico

def transform_noflip(filename, rect):
    bitmap = gamesrv.getbitmap(filename, KEYCOL)
    return bitmap, rect

def make_transform(datamap, ptmap):
    def transform(filename, rect, datamap=datamap, ptmap=ptmap, cache={}):
        try:
            bitmap, width, height = cache[filename]
        except KeyError:
            f = open(filename, "rb")
            data = f.read()
            f.close()
            width, height, data = pixmap.decodepixmap(data)
            data = datamap(width, height, data)
            dummy, dummy, nwidth, nheight = ptmap(0, 0, width, height)
            data = pixmap.encodepixmap(nwidth, nheight, data)
            bitmap = gamesrv.newbitmap(data, KEYCOL)
            cache[filename] = bitmap, width, height
            #print 'transformed', filename, 'to', nwidth, nheight
        x, y, w, h = rect
        x1, y1, dummy, dummy = ptmap(x,   y,   width, height)
        x2, y2, dummy, dummy = ptmap(x+w, y+h, width, height)
        rect = min(x1,x2), min(y1,y2), abs(x2-x1), abs(y2-y1)
        #print filename, ':', (x,y,w,h), '->', rect
        return bitmap, rect
    return transform

Transformations = {
    '':       transform_noflip,
    'vflip':  make_transform(pixmap.vflip,     lambda x,y,w,h: (x,h-y,w,h)),
    'hflip':  make_transform(pixmap.hflip,     lambda x,y,w,h: (w-x,y,w,h)),
    'cw':     make_transform(pixmap.rotate_cw, lambda x,y,w,h: (h-y,x,h,w)),
    'ccw':    make_transform(pixmap.rotate_ccw,lambda x,y,w,h: (y,w-x,h,w)),
    'rot180': make_transform(pixmap.rotate_180,lambda x,y,w,h: (w-x,h-y,w,h)),
    }

if 0:  # disabled clipping
    def sprget_subrect(n, subrect):
        x, y, w, h = subrect
        filename, (x0, y0, w0, h0) = sprmap[n]
        key = (n, 'subrect', subrect)
        sprmap[key] = filename, (x0+x, y0+y, w, h)
        return sprget(key)

def make_darker(ico, is_dragon, bmpcache={}):
    bmp, rect = ico.getorigin()
    try:
        darkbmp = bmpcache[bmp, is_dragon]
    except KeyError:
        image = pixmap.decodepixmap(bmp.read())
        if is_dragon:
            translation = pixmap.translation_dragon
        else:
            translation = pixmap.translation_darker
        darkimage = pixmap.make_dark(image, translation)
        data = pixmap.encodepixmap(*darkimage)
        darkbmp = gamesrv.newbitmap(data, bmp.colorkey)
        bmpcache[bmp, is_dragon] = darkbmp
    return darkbmp.geticon(*rect)

def haspat(n):
    return n in patmap

def loadpattern(n, keycol=None):
    if not haspat(n):
        n = (n[0] % 100,) + n[1:]
    filename, rect = patmap[n]
    filename = os.path.join('tmp', filename)
    bitmap = gamesrv.getbitmap(filename, keycol)
    return bitmap, rect

def makebkgndpattern(bitmap, xxx_todo_changeme, darker={}):
    (x,y,w,h) = xxx_todo_changeme
    from boards import CELL
    try:
        nbitmap, hscale, vscale = darker[bitmap]
    except KeyError:
        data = bitmap.read()
        width, height, data = pixmap.decodepixmap(data)
        nwidth, nheight, data = pixmap.makebkgnd(width, height, data)
        hscale = float(nwidth) / width
        vscale = float(nheight) / height
        data = pixmap.encodepixmap(nwidth, nheight, data)
        nbitmap = gamesrv.newbitmap(data, None)
        darker[bitmap] = nbitmap, hscale, vscale
    x = int(x*hscale)
    y = int(y*vscale)
    w = int(CELL*hscale)
    h = int(CELL*vscale)
    return nbitmap, (x,y,w,h)

def computebiggericon(ico, bigger={}):
    try:
        result, computing = bigger[ico]
    except KeyError:
        bigger[ico] = None, pixmap.imagezoomer(*ico.getimage())
        return None
    if computing is not None:
        result = next(computing) or next(computing) or next(computing)
        if not result:
            return None   # still computing
        w, h, data = result
        data = pixmap.encodepixmap(w, h, data)
        result = gamesrv.newbitmap(data, KEYCOL).geticon(0, 0, w, h)
        bigger[ico] = result, None
    return result

def biggericon(ico):
    result = None
    while result is None:
        result = computebiggericon(ico)
    return result

extramap = {
    'shield-left':  ('extra1.ppm', (0, 0, 32, 32)),
    'shield-right': ('extra1.ppm', (0, 32, 32, 32)),
    'moebius':      ('extra1.ppm', (0, 64, 32, 32)),
    'flower':       ('extra1.ppm', (0, 96, 32, 32)),
    'flower2':      ('extra1.ppm', (0, 128, 32, 32)),
    'potion4':      ('extra1.ppm', (0, 160, 32, 32)),
    ('glasses', -1):('extra1.ppm', (0, 192, 32, 16)),
    ('glasses', +1):('extra1.ppm', (0, 208, 32, 16)),
    'cactus':       ('extra1.ppm', (0, 224, 32, 32)),
    'questionmark3':('extra2.ppm', (0, 0, 16, 16)),
    'questionmark1':('extra2.ppm', (0, 16, 16, 16)),
    'questionmark5':('extra2.ppm', (0, 32, 16, 16)),
    'questionmark2':('extra2.ppm', (0, 48, 16, 16)),
    'questionmark4':('extra2.ppm', (0, 64, 16, 16)),
    'percent':      ('extra2.ppm', (0, 80, 16, 16)),
    'colon':        ('extra2.ppm', (0, 96, 16, 16)),
    'gameoverbkgnd':('extra2.ppm', (0, 112, 16, 16)),
    ('eyes', 0,0):  ('extra3.ppm', (0, 0, 32, 32)),
    ('eyes', 0,-1): ('extra3.ppm', (0, 32, 32, 32)),
    ('eyes', -1,0): ('extra3.ppm', (0, 64, 32, 32)),
    ('eyes', -1,-1):('extra3.ppm', (0, 96, 32, 32)),
    ('eyes', 1,0):  ('extra3.ppm', (0, 128, 32, 32)),
    ('eyes', 1,-1): ('extra3.ppm', (0, 160, 32, 32)),
    'eyes-blink':   ('extra3.ppm', (0, 192, 32, 32)),
    ('smstar','blue'   ,0): ('extra4.ppm', ( 0,  0, 16, 16)),
    ('smstar','blue'   ,1): ('extra4.ppm', ( 0, 16, 16, 16)),
    ('smstar','yellow' ,0): ('extra4.ppm', ( 0, 32, 16, 16)),
    ('smstar','yellow' ,1): ('extra4.ppm', ( 0, 48, 16, 16)),
    ('smstar','red'    ,0): ('extra4.ppm', (16,  0, 16, 16)),
    ('smstar','red'    ,1): ('extra4.ppm', (16, 16, 16, 16)),
    ('smstar','green'  ,0): ('extra4.ppm', (16, 32, 16, 16)),
    ('smstar','green'  ,1): ('extra4.ppm', (16, 48, 16, 16)),
    ('smstar','magenta',0): ('extra4.ppm', (32,  0, 16, 16)),
    ('smstar','magenta',1): ('extra4.ppm', (32, 16, 16, 16)),
    ('smstar','cyan'   ,0): ('extra4.ppm', (32, 32, 16, 16)),
    ('smstar','cyan'   ,1): ('extra4.ppm', (32, 48, 16, 16)),
    ('starbub','blue'   ,0): ('extra5.ppm', (0,  0, 32, 32)),
    ('starbub','blue'   ,1): ('extra5.ppm', (0, 32, 32, 32)),
    ('starbub','blue'   ,2): ('extra5.ppm', (0, 64, 32, 32)),
    ('starbub','yellow' ,0): ('extra5.ppm', (0, 96, 32, 32)),
    ('starbub','yellow' ,1): ('extra5.ppm', (0,128, 32, 32)),
    ('starbub','yellow' ,2): ('extra5.ppm', (0,160, 32, 32)),
    ('starbub','red'    ,0): ('extra5.ppm', (0,192, 32, 32)),
    ('starbub','red'    ,1): ('extra5.ppm', (0,224, 32, 32)),
    ('starbub','red'    ,2): ('extra5.ppm', (0,256, 32, 32)),
    ('starbub','green'  ,0): ('extra5.ppm', (0,288, 32, 32)),
    ('starbub','green'  ,1): ('extra5.ppm', (0,320, 32, 32)),
    ('starbub','green'  ,2): ('extra5.ppm', (0,352, 32, 32)),
    ('starbub','magenta',0): ('extra5.ppm', (0,384, 32, 32)),
    ('starbub','magenta',1): ('extra5.ppm', (0,416, 32, 32)),
    ('starbub','magenta',2): ('extra5.ppm', (0,448, 32, 32)),
    ('starbub','cyan'   ,0): ('extra5.ppm', (0,480, 32, 32)),
    ('starbub','cyan'   ,1): ('extra5.ppm', (0,512, 32, 32)),
    ('starbub','cyan'   ,2): ('extra5.ppm', (0,544, 32, 32)),
    'sheep-sm':     ('extra6.ppm', (0, 0, 32, 32)),
    'sheep-big':    ('extra6.ppm', (0, 32, 46, 50)),
    ('emotic', 0): ('extra7.ppm', (0,  0, 8, 8)),
    ('emotic', 1): ('extra7.ppm', (0,  8, 8, 8)),
    ('emotic', 2): ('extra7.ppm', (0, 16, 8, 8)),
    ('emotic', 3): ('extra7.ppm', (0, 24, 8, 8)),
    ('emotic', 4): ('extra7.ppm', (0, 32, 8, 8)),
    ('emotic', 5): ('extra7.ppm', (0, 40, 8, 8)),
    ('emotic', 6): ('extra7.ppm', (0, 48, 8, 8)),
    ('butterfly', 'jailed', 0): ('butterfly.ppm', (0,   0, 32, 32)),
    ('butterfly', 'jailed', 1): ('butterfly.ppm', (0,  32, 32, 32)),
    ('butterfly', 'jailed', 2): ('butterfly.ppm', (0,  64, 32, 32)),
    ('butterfly', 'dead',   0): ('butterfly.ppm', (0,  96, 32, 32)),
    ('butterfly', 'dead',   1): ('butterfly.ppm', (0, 128, 32, 32)),
    ('butterfly', 'dead',   2): ('butterfly.ppm', (0, 160, 32, 32)),
    ('butterfly', 'dead',   3): ('butterfly.ppm', (0, 192, 32, 32)),
    ('butterfly', 'fly',    0): ('butterfly.ppm', (0, 224, 32, 32)),
    ('butterfly', 'fly',    1): ('butterfly.ppm', (0, 256, 32, 32)),
    'glue': ('glue.ppm', (0, 0, 32, 32)),
    'black': ('black.ppm', (0, 0, 32, 32)),
    ('sheep',-1, 0): ('sheep.ppm', (0,   0, 32, 32)),
    ('sheep',-1, 1): ('sheep.ppm', (0,  32, 32, 32)),
    ('sheep',-1, 2): ('sheep.ppm', (0,  64, 32, 32)),
    ('sheep',-1, 3): ('sheep.ppm', (0,  96, 32, 32)),
    ('sheep', 1, 0): ('sheep.ppm', (0, 128, 32, 32)),
    ('sheep', 1, 1): ('sheep.ppm', (0, 160, 32, 32)),
    ('sheep', 1, 2): ('sheep.ppm', (0, 192, 32, 32)),
    ('sheep', 1, 3): ('sheep.ppm', (0, 224, 32, 32)),
    ('sheep', 'a'):  ('sheep.ppm', (2, 263, 7, 8)),
    ('sheep', 'b'):  ('sheep.ppm', (11, 262, 6, 10)),
    ('sheep', 'c'):  ('sheep.ppm', (17, 264, 11, 8)),
    ('sheep', 'd'):  ('sheep.ppm', (18, 272, 11, 7)),
    ('sheep', 'e'):  ('sheep.ppm', (18, 279, 11, 8)),
    ('sheep', 'f'):  ('sheep.ppm', (4, 273, 10, 12)),
    ('sheep', 'g'):  ('sheep.ppm', (19, 257, 11, 8)),
    }
hatmap = {
    ('hat', 0, -1,1):('hat2.ppm',(  0, 0, 32, 48)),
    ('hat', 0, -1,2):('hat2.ppm',( 32, 0, 32, 48)),
    ('hat', 0, -1,3):('hat2.ppm',( 64, 0, 32, 48)),
    ('hat', 0,  1,3):('hat2.ppm',( 96, 0, 32, 48)),
    ('hat', 0,  1,2):('hat2.ppm',(128, 0, 32, 48)),
    ('hat', 0,  1,1):('hat2.ppm',(160, 0, 32, 48)),
    ('hat', 1, -1,1):('hat1.ppm',(  0, 0, 32, 48)),
    ('hat', 1, -1,2):('hat1.ppm',( 32, 0, 32, 48)),
    ('hat', 1, -1,3):('hat1.ppm',( 64, 0, 32, 48)),
    ('hat', 1,  1,3):('hat1.ppm',( 96, 0, 32, 48)),
    ('hat', 1,  1,2):('hat1.ppm',(128, 0, 32, 48)),
    ('hat', 1,  1,1):('hat1.ppm',(160, 0, 32, 48)),
    ('hat', 0)      :('hat5.ppm',( 32, 0, 32, 48)),
    ('hat', 1)      :('hat5.ppm',(  0, 0, 32, 48)),
    }

def generate_sprmap():
    # check and maybe regenerate the colored image files
    file = os.path.join('images', 'buildcolors.py')
    g = {'__name__': '__auto__', '__file__': file}
    exec(compile(open(file, "rb").read(), file, 'exec'), g)
    # replace the entries 'filename_%d.ppm' by a family of entries,
    # one for each color
    sprmap = {}
    for n, (filename, rect) in (list(original_sprmap.items()) +
                                list(extramap.items()) + list(hatmap.items())):
        if filename.find('%d') >= 0:
            for i in range(MAX):
                sprmap[n+1000*i] = (os.path.join('images',filename % i), rect)
        else:
            sprmap[n] = (os.path.join('images', filename), rect)
    return sprmap
sprmap = generate_sprmap()

transparency = {
    mnstrmap.GreenAndBlue.new_bubbles[0][0]: 0xA0,
    mnstrmap.GreenAndBlue.new_bubbles[0][1]: 0xB0,
    mnstrmap.GreenAndBlue.new_bubbles[0][2]: 0xC0,
    mnstrmap.GreenAndBlue.new_bubbles[0][3]: 0xD0,
    mnstrmap.GreenAndBlue.normal_bubbles[0][0]: 0xE0,
    mnstrmap.GreenAndBlue.normal_bubbles[0][1]: 0xE0,
    mnstrmap.GreenAndBlue.normal_bubbles[0][2]: 0xE0,
    mnstrmap.DyingBubble.first[0]: 0xD0,
    mnstrmap.DyingBubble.first[1]: 0xD0,
    mnstrmap.DyingBubble.first[2]: 0xD0,
    mnstrmap.DyingBubble.medium[0]: 0xC0,
    mnstrmap.DyingBubble.medium[1]: 0xC0,
    mnstrmap.DyingBubble.medium[2]: 0xC0,
    mnstrmap.DyingBubble.last[0]: 0xB0,
    mnstrmap.DyingBubble.last[1]: 0xB0,
    mnstrmap.DyingBubble.last[2]: 0xB0,
    'starbub': 0xE0,
    }

def sprcharacterget(c, filename=os.path.join('images', 'extra8.ppm')):
    n = ord(c) - 32
    if 0 <= n < 95:
        return gamesrv.getbitmap(filename, KEYCOL).geticon(n*8, 0, 8, 15)
    else:
        return None

def writestr(x, y, text):
    result = []
    for c in text:
        ico = sprcharacterget(c)
        if ico is not None:
            result.append(gamesrv.Sprite(ico, x, y))
            x += 7
    return result

def writestrlines(lines):
    import boards
    width = boards.bwidth + 9*boards.CELL
    y = 50
    for text in lines:
        if text:
            writestr((width - 7*len(text)) // 2, y, text)
            y += 28
        else:
            y += 14



def getsample(fn, freq):
    return gamesrv.getsample(os.path.join('sounds', fn), freq)

SoundList = ['Pop', 'Jump', 'Die', 'LetsGo', 'Extralife',
             'Fruit', 'Extra', 'Yippee', 'Hurry', 'Hell', 'Shh']

class Snd:
    pass

def loadsounds(freqfactor=1):
    for key in SoundList:
        setattr(Snd, key, getsample(key.lower()+'.wav', freqfactor))

loadsounds()
music_intro  = gamesrv.getmusic('music/Snd1-8.wav')
music_game   = gamesrv.getmusic('music/Snd2-8.wav')
music_potion = gamesrv.getmusic('music/Snd3-8.wav')
music_modern = gamesrv.getmusic('music/Snd4-8.wav')
music_old    = gamesrv.getmusic('music/Snd5-8.wav')
music_game2  = gamesrv.getmusic('music/Snd6-8.wav')
#gamesrv.set_musics([music_intro, music_game], 1)
