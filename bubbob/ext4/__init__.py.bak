from __future__ import generators
import os, math, random
import images, gamesrv
from images import ActiveSprite
from boards import CELL, HALFCELL, bget
from mnstrmap import GreenAndBlue
from bubbles import BubblingEyes, Bubble
from bonuses import Bonus, points

LocalDir = os.path.basename(os.path.dirname(__file__))


localmap = {
    't-brick1':  ('image1-%d.ppm', (0, 0, 16, 16)),
    't-brick2':  ('image1-%d.ppm', (16, 0, 16, 16)),
    }

music = gamesrv.getmusic(os.path.join(LocalDir, 'music.wav'))


class BrickEyes(BubblingEyes):

    Patterns = [[(-2,0), (-1,0), (0,0), (1,0)],
                [(-2,0), (-1,0), (0,0), (0,-1)],
                [(-1,-1), (-1,0), (0,0), (1,0)],
                [(-1,0), (0,0), (0,-1), (1,0)],
                [(-1,-1), (0,-1), (0,0), (1,0)],
                [(-2,0), (-1,0), (-1,-1), (0,-1)],
                [(-1,-1), (-1,0), (0,-1), (0,0)]]

    def __init__(self, tetris, bubber, saved_caps, olddragon):
        BubblingEyes.__init__(self, bubber, saved_caps, olddragon)
        self.tetris = tetris
        self.bricks = []

    def playing_bubble(self, oldsprite):
        import boards
        self.pat = random.choice(self.Patterns)
        self.orientation = 1,0
        xmin = 2              - min([x for x,y in self.pat])
        xmax = boards.width-3 - max([x for x,y in self.pat])
        x = int(random.normalvariate(oldsprite.x, boards.bwidth/4))
        x = (x+HALFCELL) // CELL
        if x<xmin: x=xmin
        if x>xmax: x=xmax
        y = -1
        self.tx = x
        self.ty = y
        self.move((x-1)*CELL, (y-1)*CELL)
        for i in range(5):
            yield None
        self.bricks = [Brick(self.bubber, px, py)
                       for px, py in self.brick_positions()]
        self.gen.append(self.step_control())
        self.gen.append(self.rotate_control())
        self.gen.append(self.fall_control())
        self.gen.append(self.move_eyes())

    def bottom_up(self):
        return 0

    def kill(self):
        for b in self.bricks:
            b.stop(self.tetris)
            b.remove()
        self.bricks = []
        BubblingEyes.kill(self)

    def brick_positions(self):
        ox, oy = self.orientation
        result = []
        cx = self.tx*CELL - HALFCELL
        cy = self.ty*CELL - HALFCELL
        for px, py in self.pat:
            px = px*CELL + HALFCELL
            py = py*CELL + HALFCELL
            result.append((cx+px*ox-py*oy, cy+px*oy+py*ox))
        return result

    def save_position(self):
        return self.tx, self.ty, self.orientation

    def restore_position(self, p):
        self.tx, self.ty, self.orientation = p

    def moved(self, old_position):
        for b in self.bricks:
            b.set(' ')
        try:
            for px, py in self.brick_positions():
                if bget(px//CELL, py//CELL) != ' ':
                    self.restore_position(old_position)
                    return 0
            for b, (px, py) in zip(self.bricks, self.brick_positions()):
                b.follow(px, py)
        finally:
            for b in self.bricks:
                b.set('!')   # note: we need '!' < '#'
        return 1

    def onground(self):
        for b in self.bricks:
            if b.gen:    # brick still moving
                return 0
        for px, py in self.brick_positions():
            if bget(px//CELL, py//CELL+1) >= '#':
                return 1
        return 0

    def step_control(self):
        while 1:
            while not self.bubber.wannago(self.dcap):
                yield None
            pos = self.save_position()
            self.tx += self.bubber.wannago(self.dcap)
            if self.moved(pos):
                for i in range(4):
                    yield None
            yield None

    def fall_control(self):
        delay = 1
        while 1:
            for i in range(delay and 14):
                if self.bubber.key_fire:
                    break
                yield None
            pos = self.save_position()
            self.ty += 1
            delay = self.moved(pos)
            if delay:
                for i in range(3):
                    yield None
            elif self.onground() and self.tetris.ready:
                self.gen = [self.stopping()]
            yield None

    def rotate_control(self):
        while 1:
            while not self.bubber.key_jump:
                yield None
            pos = self.save_position()
            ox, oy = self.orientation
            self.orientation = oy, -ox
            if self.moved(pos):
                for i in range(7):
                    yield None
            yield None

    def stopping(self):
        self.move(self.x, -self.ico.h)
        positions = [(py//CELL, px//CELL) for px, py in self.brick_positions()
                     if py >= 0]
        positions.sort()
        positions = [(px, py) for py, px in positions]
        for b in self.bricks:
            b.stop(self.tetris)
            if b.ty < 0:
                b.remove()
        self.bricks = []
        staticbricks = self.tetris.staticbricks
        pts = 500
        while 1:
            for px, py in positions:
                y = py
                x1 = px
                while (x1-1, y) in staticbricks:
                    x1 -= 1
                if bget(x1-1, y) != '#':
                    continue
                x2 = px
                while (x2, y) in staticbricks:
                    x2 += 1
                if bget(x2, y) != '#':
                    continue
                if x2-x1 < 2:
                    continue
                # full line
                ico = images.sprget(Bubble.exploding_bubbles[0])
                self.tetris.score[self.bubber] = self.tetris.score.get(
                    self.bubber, 0) + 1
                xlist = range(x1, x2)
                for x in xlist:
                    s = ActiveSprite(ico,
                                     x*CELL + random.randrange(CELL) - CELL,
                                     y*CELL + random.randrange(CELL) - CELL)
                    s.gen.append(s.die(Bubble.exploding_bubbles))
                    s = staticbricks[x, y]
                    points(x*CELL + HALFCELL, y*CELL + HALFCELL, s, pts)
                    s.remove()
                if pts == 500:
                    self.play(images.Snd.Fruit)
                elif pts == 4000:
                    self.play(images.Snd.Extralife)
                else:
                    self.play(images.Snd.Extra)
                pts *= 2
                for y in range(py-1, -1, -1):
                    if not [x for x in xlist if (x, y) in staticbricks]:
                        break
                    for t in range(4):
                        yield None
                    if [x for x in xlist if (x, y+1) in staticbricks]:
                        break
                    for x in xlist:
                        if (x, y) in staticbricks:
                            staticbricks[x, y].shiftdown()
                yield None
                break
            else:
                break
        if self.tetris.ready < 2:
            self.gen.append(self.playing_bubble(self))

    def move_eyes(self):
        while 1:
            tx = (self.tx-1) * CELL
            ty = (self.ty-1) * CELL
            for i in range(3):
                if tx < self.x:
                    dx = -1
                elif tx > self.x:
                    dx = +1
                else:
                    dx = 0
                if ty > self.y:
                    dy = +1
                else:
                    dy = 0
                self.step(2*dx, 2*dy)
            key = ('eyes', dx, 0)
            self.seticon(images.sprget(key))
            yield None


class Brick(ActiveSprite):

    def __init__(self, bubber, x, y):
        ico = images.sprget(('t-brick1', bubber.pn))
        ActiveSprite.__init__(self, ico, x, y)
        self.tx = x//CELL
        self.ty = y//CELL
        self.bubber = bubber

    def follow(self, x, y):
        self.tx = x//CELL
        self.ty = y//CELL
        self.gen = [self.following(x, y)]

    def following(self, nx, ny):
        dx = (nx - self.x) / 7.0
        dy = (ny - self.y) / 7.0
        for i in range(6, 0, -1):
            self.move(nx - int(i*dx), ny - int(i*dy))
            yield None
        self.move(nx, ny)

    def set(self, c):
        x, y = self.tx, self.ty
        if 0 <= x < curboard.width and 0 <= y < curboard.height:
            line = curboard.walls[y]
            curboard.walls[y] = line[:x] + c + line[x+1:]

    def stop(self, tetris):
        self.set('X')
        self.seticon(images.sprget(('t-brick2', self.bubber.pn)))
        images.ActiveSprites.remove(self)
        tetris.staticbricks[self.tx, self.ty] = self
        self.staticbricks = tetris.staticbricks

    def remove(self):
        del self.staticbricks[self.tx, self.ty]
        self.set(' ')
        gamesrv.Sprite.kill(self)

    def shiftdown(self):
        del self.staticbricks[self.tx, self.ty]
        self.set(' ')
        self.ty += 1
        self.set('X')
        self.staticbricks[self.tx, self.ty] = self
        self.step(0, CELL)


class Tetris:
    
    def bgen(self, limittime = 90.1): # 1:30
        import boards
        from player import BubPlayer

        self.score = {}
        for t in boards.initsubgame(music, self.displaypoints):
            yield t

        tc = boards.TimeCounter(limittime)
        self.ready = 0
        self.staticbricks = {}
        finished = 0
        for t in self.frame():
            t = boards.normal_frame()
            self.build_eyes()
            yield t
            tc.update(t)
            if tc.time == 0.0:
                self.ready = 2
                finished += not self.still_playing()
                if finished > 16:
                    break
            if (BubPlayer.FrameCounter & 15) == 7:
                for s in images.ActiveSprites:
                    if isinstance(s, Bubble):
                        s.pop()
                    elif isinstance(s, Bonus):
                        s.kill()

        tc.restore()
        for t in boards.result_ranking(self.score):
            self.remove_eyes()
            yield t
        for s in self.staticbricks.values():
            s.remove()

    def displaypoints(self, bubber):
        return self.score.get(bubber, 0)

    def frame(self):
        heights = {1: curboard.height,
                   curboard.width-2: curboard.height}
        ymax = curboard.height-1
        maxheight = curboard.height*3//4
        for x in range(2, curboard.width-2):
            if bget(x, ymax) == ' ':
                curboard.putwall(x, ymax)
            height = 1
            for y in range(ymax-1, -1, -1):
                if bget(x, y) == '#':
                    if height == maxheight:
                        curboard.killwall(x, y)
                    else:
                        height += 1
            heights[x] = height
        xlist = range(2, curboard.width-2)
        random.shuffle(xlist)
        for x in xlist:
            h = heights[x]
            x1 = x2 = x
            while heights[x1-1] == h:
                x1 -= 1
            while heights[x2] == h:
                x2 += 1
            parts = (x2-x1) // 8
            if not parts:
                continue
            left = 0
            if heights[x1-1] > h:
                x1 -= 1
                left += 1
            right = parts+1
            if heights[x2] > h:
                x2 += 1
                right -= 1
            for p in range(left, right):
                x = x1 + ((x2-x1-1)*p+parts//2)//parts
                y = ymax
                for i in range(2):
                    while bget(x, y) == '#':
                        y -= 1
                    if y >= 3:
                        curboard.putwall(x, y)
                        heights[x] += 1
        curboard.reorder_walls()
        
        walls_by_pos = curboard.walls_by_pos
        moves = 1
        s = 8.0
        while moves:
            moves = 0
            for y in range(curboard.height-3, -1, -1):
                for x in range(2, curboard.width-2):
                    if ((y,x) in walls_by_pos and
                        (y+1,x) not in walls_by_pos):
                        y0 = y
                        while (y0-1,x) in walls_by_pos:
                            y0 -= 1
                        w = curboard.killwall(x, y0, 0)
                        curboard.putwall(x, y+1, w)
                        moves = 1
            curboard.reorder_walls()
            for i in range(int(s)+2):
                yield None
            s *= 0.95
        self.ready = 1
        while 1:
            yield None

    def build_eyes(self):
        from player import BubPlayer
        for p in BubPlayer.PlayerList:
            dragons = [d for d in p.dragons if not isinstance(d, BrickEyes)]
            if dragons and len(p.dragons) == len(dragons):
                dragon = random.choice(dragons)
                eyes = BrickEyes(self, p, dragon.dcap, dragon)
                p.dragons.append(eyes)
                #p.emotic(dragon, 4)
            for d in dragons:
                d.kill()

    def still_playing(self):
        from player import BubPlayer
        for p in BubPlayer.PlayerList:
            for d in p.dragons:
                if d.gen:
                    return 1
        return 0

    def remove_eyes(self):
        from player import BubPlayer
        for p in BubPlayer.PlayerList:
            for d in p.dragons:
                d.kill()

# This game is suitable for at least min_players players
min_players = 1

def run():
    global curboard
    import boards
    from boards import curboard
    boards.replace_boardgen(Tetris().bgen())

def setup():
    from player import BubPlayer
    for key, (filename, rect) in localmap.items():
        filename = os.path.join(LocalDir, filename)
        if filename.find('%d') >= 0:
            for p in BubPlayer.PlayerList:
                images.sprmap[key, p.pn] = (filename % p.pn, rect)
        else:
            images.sprmap[key] = (filename, rect)
setup()
