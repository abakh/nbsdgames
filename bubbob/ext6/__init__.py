
import os, random
import images, gamesrv
from images import ActiveSprite
import boards
from boards import CELL, HALFCELL, bget
from player import Dragon, BubPlayer
from mnstrmap import Monky
from bubbles import Bubble
from bonuses import Bonus

LocalDir = os.path.basename(os.path.dirname(__file__))

localmap = {
    ('trn-head', 0,-1):   ('image1-%d.ppm', (0, 0, 8, 8)),
    ('trn-head',-1, 0):   ('image1-%d.ppm', (0, 8, 8, 8)),
    ('trn-head', 0, 1):   ('image1-%d.ppm', (0,16, 8, 8)),
    ('trn-head', 1, 0):   ('image1-%d.ppm', (0,24, 8, 8)),
    ('trn', 0,-1, 1, 0):  ('image1-%d.ppm', (0,32, 8, 8)),
    ('trn', 0, 1, 1, 0):  ('image1-%d.ppm', (0,40, 8, 8)),
    ('trn', 1, 0, 0,-1):  ('image1-%d.ppm', (0,48, 8, 8)),
    ('trn', 1, 0, 0, 1):  ('image1-%d.ppm', (0,56, 8, 8)),
    ('trn', 1, 0, 1, 0):  ('image1-%d.ppm', (0,64, 8, 8)),
    ('trn', 0, 1, 0, 1):  ('image1-%d.ppm', (0,72, 8, 8)),
    }

music = gamesrv.getmusic(os.path.join(LocalDir, 'music.wav'))
snd_crash = gamesrv.getsample(os.path.join(LocalDir, 'crash.wav'))


class TronHead(ActiveSprite):

    def __init__(self, tron, bubber, dcap, cx, cy, dir):
        self.tron = tron
        self.bubber = bubber
        self.dcap = dcap
        self.cx = cx
        self.cy = cy
        self.dir = dir
        self.icons = {}
        for key in localmap:
            ico = images.sprget((key, bubber.pn))
            key = key[1:]
            self.icons[key] = ico
            if len(key) == 4:
                dx1, dy1, dx2, dy2 = key
                key = -dx2, -dy2, -dx1, -dy1
                self.icons[key] = ico
        ActiveSprite.__init__(self, self.icons[self.dir],
                              self.cx*8-2, self.cy*8-2)
        self.gen.append(self.trailing())

    def forward_step(self, dir):
        s = gamesrv.Sprite(self.icons[self.dir + dir], self.x, self.y)
        self.tron.trailsprites.append(s)
        self.dir = dir
        self.cx += dir[0]
        self.cy += dir[1]
        self.move(self.cx*8-2, self.cy*8-2, self.icons[dir])

    def trailing(self):
        unoccupied = self.tron.unoccupied
        bubber = self.bubber
        # first go straight forward until we enter the playing board itself
        while (self.cx, self.cy) not in unoccupied:
            self.forward_step(self.dir)
            yield None
            yield None
        # playing!
        unoccupied[self.cx, self.cy] = False
        while True:
            # turn
            d = [(bubber.key_left, -1, 0),
                 (bubber.key_right, 1, 0),
                 (bubber.key_jump,  0,-1),
                 (bubber.key_fire,  0, 1)]
            d.sort()
            newdir = self.dir
            if d[-1][0] > d[-2][0]:
                newdir = d[-1][1:]
            if (self.dir + newdir) not in self.icons:
                newdir = self.dir   # forbidden 180-degree turn
            # move one step forward
            self.forward_step(newdir)
            # crash?
            if not unoccupied.get((self.cx, self.cy)):
                self.crash()
                return
            unoccupied[self.cx, self.cy] = False
            yield None
            yield None

    def to_front(self):
        if self.gen:
            ActiveSprite.to_front(self)

    def crash(self):
        self.move(self.x - self.dir[0], self.y - self.dir[1],
                  self.icons[self.dir+self.dir])
        self.to_back()
        self.play(snd_crash)
        ico = images.sprget(Monky.decay_weapon[1])
        s = ActiveSprite(ico, self.x + self.ico.w//2 - CELL,
                              self.y + self.ico.h//2 - CELL)
        s.gen.append(s.die(Monky.decay_weapon[1:], 4))
        self.stop()

    def stop(self):
        del self.gen[:]
        try:
            self.tron.trons.remove(self)
        except ValueError:
            pass

    def kill(self):
        self.stop()
        try:
            self.bubber.dragons.remove(self)
        except ValueError:
            pass
        ActiveSprite.kill(self)


class Tron:
    
    def bgen(self, limittime = 60.1): # 1:00
        self.score = {}
        for t in boards.initsubgame(music, self.displaypoints):
            yield t

        self.ready = 0
        self.trons = []
        self.trailsprites = []
        self.playerlist = BubPlayer.PlayerList[:]
        tc = boards.TimeCounter(limittime)
        for t in self.frame(tc):
            t = boards.normal_frame()
            self.build_trons()
            yield t
            tc.update(t)
            if (BubPlayer.FrameCounter & 15) == 7:
                for s in images.ActiveSprites:
                    if isinstance(s, Bubble):
                        s.pop()
                    elif isinstance(s, Bonus):
                        s.kill()

        self.ready = 0
        tc.restore()
        for t in boards.result_ranking(self.score):
            for p in BubPlayer.PlayerList:
                for d in p.dragons[:]:
                    d.kill()
            yield t
        self.remove_trons()

    def displaypoints(self, bubber):
        return self.score.get(bubber, 0)

    def build_trons(self):
        if self.ready == 0:
            self.remove_trons()
            return
        for p in self.playerlist:
            dragons = [d for d in p.dragons if not isinstance(d, TronHead)]
            if self.ready < 10 and dragons and len(p.dragons) == len(dragons):
                self.score.setdefault(p, 0)
                dragon = random.choice(dragons)
                x, y, dir = self.select_start_point()
                head = TronHead(self, p, dragon.dcap, x, y, dir)
                self.trons.append(head)
                p.dragons.append(head)
                #p.emotic(head, 4)
            for d in dragons:
                d.kill()

    def remove_trons(self):
        for p in BubPlayer.PlayerList:
            for d in p.dragons[:]:
                d.kill()
        for s in self.trailsprites:
            s.kill()
        del self.trailsprites[:]

    def select_start_point(self):
        distmin = 12
        while True:
            x, y, dir = random.choice(self.start_points)
            for head in self.trons:
                if abs(x-head.cx//2) + abs(y-head.cy//2) < distmin:
                    break
            else:
                break
            distmin *= 0.95
        if (y, x) in curboard.walls_by_pos:
            curboard.killwall(x, y)
        x = 2*x+1
        y = 2*y - dir[1]
        if dir[1] < 0:
            y += 2
        return x, y, dir

    def frame(self, tc):
        y1 = 1
        y2 = curboard.height-2
        while y1 <= y2:
            for y in [y1, y2]:
                for x in range(2, curboard.width-2):
                    if (y, x) in curboard.walls_by_pos:
                        curboard.killwall(x, y)
            yield None
            y1 += 1
            y2 -= 1

        self.start_points = []
        for x in range(4, curboard.width-3):
            self.start_points.append((x, 0, (0, 1)))
            self.start_points.append((x, curboard.height-1, (0, -1)))

        while tc.time != 0.0:
            for y in [0, curboard.height-1]:
                for x in range(2, curboard.width-2):
                    if (y, x) not in curboard.walls_by_pos:
                        curboard.putwall(x, y)
            curboard.reorder_walls()
            self.unoccupied = {}
            for x in range(5, 2*curboard.width-4):
                for y in range(3, 2*curboard.height-2):
                    self.unoccupied[x, y] = True
            random.shuffle(self.playerlist)
            for i in range(5):
                yield None

            min_players = 1
            while self.ready < 20 or len(self.trons) >= min_players:
                if len(self.trons) >= 2:
                    min_players = 2
                self.ready += 1
                yield None

            if len(self.trons) == 1:
                bubber = self.trons[0].bubber
                self.score[bubber] += 1
                bubber.givepoints(100)
                self.trons[0].stop()
                self.ready = 99

            for i in range(28):
                yield None
            self.ready = 0

# This game is suitable for at least min_players players
min_players = 2

def run():
    global curboard
    from boards import curboard
    boards.replace_boardgen(Tron().bgen())

def setup():
    for key, (filename, rect) in list(localmap.items()):
        filename = os.path.join(LocalDir, filename)
        if filename.find('%d') >= 0:
            for p in BubPlayer.PlayerList:
                images.sprmap[key, p.pn] = (filename % p.pn, rect)
        else:
            images.sprmap[key] = (filename, rect)
setup()
