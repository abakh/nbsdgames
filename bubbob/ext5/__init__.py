from __future__ import generators
import os, random
import images, gamesrv
from images import ActiveSprite
import boards
from boards import CELL, HALFCELL, bget
from monsters import Monster
from player import Dragon, BubPlayer
from bubbles import Bubble
import bonuses

LocalDir = os.path.basename(os.path.dirname(__file__))

localmap = {
    ('lem-walk', 1,0) :  ('image1.ppm', (  0,  0, 32, 32)),
    ('lem-walk', 1,1) :  ('image1.ppm', ( 32,  0, 32, 32)),
    ('lem-walk', 1,2) :  ('image1.ppm', ( 64,  0, 32, 32)),
    ('lem-walk', 1,3) :  ('image1.ppm', ( 96,  0, 32, 32)),
    ('lem-walk', 1,4) :  ('image1.ppm', (128,  0, 32, 32)),
    ('lem-walk', 1,5) :  ('image1.ppm', (160,  0, 32, 32)),
    ('lem-walk', 1,6) :  ('image1.ppm', (192,  0, 32, 32)),
    ('lem-walk', 1,7) :  ('image1.ppm', (224,  0, 32, 32)),
    ('lem-fall', 1,0) :  ('image1.ppm', (256,  0, 32, 32)),
    ('lem-fall', 1,1) :  ('image1.ppm', (288,  0, 32, 32)),
    ('lem-fall', 1,2) :  ('image1.ppm', (320,  0, 32, 32)),
    ('lem-fall', 1,3) :  ('image1.ppm', (352,  0, 32, 32)),

    ('lem-fall',-1,3) :  ('image2.ppm', (  0,  0, 32, 32)),
    ('lem-fall',-1,2) :  ('image2.ppm', ( 32,  0, 32, 32)),
    ('lem-fall',-1,1) :  ('image2.ppm', ( 64,  0, 32, 32)),
    ('lem-fall',-1,0) :  ('image2.ppm', ( 96,  0, 32, 32)),
    ('lem-walk',-1,7) :  ('image2.ppm', (128,  0, 32, 32)),
    ('lem-walk',-1,6) :  ('image2.ppm', (160,  0, 32, 32)),
    ('lem-walk',-1,5) :  ('image2.ppm', (192,  0, 32, 32)),
    ('lem-walk',-1,4) :  ('image2.ppm', (224,  0, 32, 32)),
    ('lem-walk',-1,3) :  ('image2.ppm', (256,  0, 32, 32)),
    ('lem-walk',-1,2) :  ('image2.ppm', (288,  0, 32, 32)),
    ('lem-walk',-1,1) :  ('image2.ppm', (320,  0, 32, 32)),
    ('lem-walk',-1,0) :  ('image2.ppm', (352,  0, 32, 32)),

    ('lem-jail',   0) :  ('image4.ppm', (  0,  0, 32, 32)),
    ('lem-jail',   1) :  ('image4.ppm', (  0, 32, 32, 32)),
    ('lem-jail',   2) :  ('image4.ppm', (  0, 64, 32, 32)),
    }
for n in range(16):
    localmap[('lem-crash', n)] = ('image3.ppm', (32*n, 0, 32, 32))

music = gamesrv.getmusic(os.path.join(LocalDir, 'music.wav'))
snd_ouch = gamesrv.getsample(os.path.join(LocalDir, 'ouch.wav'))


class Lemmy:
    right     = [('lem-walk', 1,n) for n in range(8)]
    left      = [('lem-walk',-1,n) for n in range(8)]
    jailed    = [('lem-jail',   n) for n in range(3)]


class Lemming(Monster):

    def __init__(self, lemmings, x, y, dir):
        Monster.__init__(self, Lemmy, x, y, dir, in_list=lemmings.lemlist)
        self.lemmings = lemmings

    def argh(self, *args, **kwds):
        self.untouchable()
        self.gen = [self.jumpout()]

    def resetimages(self):
        pass

    def touched(self, dragon):
        if 20 >= abs(self.x - dragon.x) >= 14:
            if self.x < dragon.x:
                self.dir = -1
            else:
                self.dir = 1

    def in_bubble(self, bubble):
        self.move(bubble.x, bubble.y)
        Monster.in_bubble(self, bubble)
        return -1

    def bubbling(self, bubble):
        dx = random.randrange(-3, 4)
        dy = random.randrange(-4, 2)
        counter = 0
        while not hasattr(bubble, 'poplist'):
            if self.y < -CELL and bubble.y > CELL:  # bubble wrapped
                self.leaveboard(bubble)
                return
            self.move(bubble.x+dx, bubble.y+dy)
            yield None
        if bubble.poplist is None and bubble.y <= -2*CELL+1:
            self.leaveboard(bubble)
            return
        self.setimages(None)
        self.gen = [self.jumpout()]

    def jumpout(self):
        # jumping out of the bubble
        self.seticon(images.sprget(self.mdef.jailed[1]))
        dxy = [(random.random()-0.5) * 9.0,
               (random.random()+0.5) * (-5.0)]
        for n in self.parabolic(dxy):
            yield n
            if dxy[1] >= 2.0:
                break
        if dxy[0] < 0:
            self.dir = -1
        else:
            self.dir = 1
        self.touchable = 1
        self.gen.append(self.falling())

    def falling(self):
        self.setimages(None)
        n = 0
        lemmap = self.lemmings.lemmap
        while not self.onground():
            yield None
            self.move(self.x, (self.y + 4) & ~3,
                      lemmap['lem-fall', self.dir, n&3])
            n += 1
            if self.y >= boards.bheight:
                self.kill()
                return
            yield None
        if n <= 33:
            self.gen.append(self.walking())
        else:
            self.play(snd_ouch)
            self.untouchable()
            self.to_front()
            self.gen = [self.die([('lem-crash', n) for n in range(16)], 2)]

    def walking(self):
        self.setimages(None)
        n = 0
        lemmap = self.lemmings.lemmap
        y0 = self.y // 16
        while self.y == y0*16:
            yield None
            nx = self.x + self.dir*2
            x0 = (nx+15) // 16
            if bget(x0, y0+1) == ' ':
                if bget(x0, y0+2) == ' ':
                    y0 += 1  # fall
            elif bget(x0, y0) != ' ':
                self.dir = -self.dir
                self.resetimages()
                continue
            else:  # climb
                y0 -= 1
                n2 = 0
                while self.y > y0*16:
                    self.step(0, -2)
                    if n2:
                        n2 -= 1
                    else:
                        self.seticon(lemmap['lem-walk', self.dir, n&7])
                        n += 1
                        n2 = 2
                    yield None
            self.move(nx, self.y, lemmap['lem-walk', self.dir, n&7])
            n += 1
            yield None
            yield None
        self.gen.append(self.falling())

    def onground(self):
        if self.y & 15:
            return 0
        x0 = (self.x+15) // 16
        y0 = self.y // 16 + 2
        return bget(x0, y0) != ' ' == bget(x0, y0-1)

    def leaveboard(self, bubble):
        if hasattr(bubble, 'd'):
            bubble.play(images.Snd.Extra)
            score = self.lemmings.score
            bubber = bubble.d.bubber
            score[bubber] = score.get(bubber, 0) + 1
            bonuses.points(bubble.x, bubble.y, bubble.d, 500)
        self.kill()

    default_mode = walking


class Lemmings:
    
    def bgen(self, limittime = 60.1): # 0:60
        self.score = {}
        for t in boards.initsubgame(music, self.displaypoints):
            yield t
        self.lemmap = {}
        for key in localmap:
            self.lemmap[key] = images.sprget(key)

        tc = boards.TimeCounter(limittime)
        self.lemlist = []
        self.lemtotal = 0
        for t in self.frame():
            t = boards.normal_frame()
            yield t
            tc.update(t)
            if tc.time == 0.0:
                break

        tc.restore()
        for s in self.lemlist[:]:
            if s.alive:
                s.kill()
        for s in images.ActiveSprites[:]:
            if isinstance(s, Bubble):
                s.pop()
        for t in boards.result_ranking(self.score.copy(), self.lemtotal):
            yield t

    def displaypoints(self, bubber):
        return self.score.get(bubber, 0)

    def frame(self):
        windline = '>>' + '^'*(curboard.width-4) + '<<'
        curboard.winds = [windline] * curboard.height

        countholes = 0
        ymax = curboard.height-1
        for x in range(2, curboard.width-2):
            if bget(x, ymax) == ' ':
                countholes += 1

        xrange = []
        try:
            for delta in range(2, curboard.width):
                for x in [delta, curboard.width-delta-1]:
                    if x in xrange: raise StopIteration
                    xrange.append(x)
        except StopIteration:
            pass

        for x in xrange:
            if countholes > curboard.width//6 and bget(x, ymax) == ' ':
                curboard.putwall(x, ymax)
                curboard.reorder_walls()
                countholes -= 1
            for y in range(0, ymax):
                if bget(x, y) == ' ':
                    break
                curboard.killwall(x, y)
            yield None

        testing = {}
        def addlemming():
            for x, y in testing.items():
                if bget(x, y) != ' ' == bget(x, y-1):
                    if x <= curboard.width//2:
                        dir = 1
                    else:
                        dir = -1
                    s = Lemming(self, x*CELL-HALFCELL, (y-2)*CELL, dir)
                    self.lemtotal += 1
                if y < ymax:
                    testing[x] = y+1
                else:
                    del testing[x]
        for x in xrange:
            testing[x] = 1
            addlemming()
            yield None
        while testing:
            addlemming()
            yield None

        while self.lemlist:
            yield None

# This game is suitable for at least min_players players
min_players = 1

def run():
    global curboard
    from boards import curboard
    boards.replace_boardgen(Lemmings().bgen())

def setup():
    for key, (filename, rect) in localmap.items():
        filename = os.path.join(LocalDir, filename)
        images.sprmap[key] = (filename, rect)
setup()
