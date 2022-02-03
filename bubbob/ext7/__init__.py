
import os, random, math
import images, gamesrv
from images import ActiveSprite
import boards
from boards import CELL
from player import Dragon, BubPlayer, scoreboard
from bubbles import Bubble
from bonuses import Bonus
from mnstrmap import PlayerBubbles
from mnstrmap import Monky
import bonuses
from ext6 import snd_crash

LocalDir = os.path.basename(os.path.dirname(__file__))

ANGLE_COUNT = 24
ANGLE_STEP  = 360 / ANGLE_COUNT
ANGLE_TABLE = {}
for i in range(ANGLE_COUNT):
    a = i*ANGLE_STEP*math.pi/180.0
    ANGLE_TABLE[i*ANGLE_STEP] = (math.cos(a), math.sin(a))


localmap = {}
for i in range(ANGLE_COUNT):
    localmap['camel', i] = ('image1-%d.ppm', (0, i*36, 36, 36))

music = gamesrv.getmusic(os.path.join(LocalDir, 'music.wav'))
snd_fire = gamesrv.getsample(os.path.join(LocalDir, 'fire.wav'))
snd_hit  = gamesrv.getsample(os.path.join(LocalDir, 'hit.wav'))


class Plane(ActiveSprite):
    lock = None

    def __init__(self, camel, bubber, dcap, x, y, dirhint=None):
        self.bubber = bubber
        self.dcap = dcap
        self.camel = camel
        self.shotlist = camel.score.setdefault(bubber, {})

        if x < x_min:
            x = x_min
        elif x > x_max:
            x = x_max

        if y < 4*CELL:
            y = 4*CELL
        elif y > (curboard.height-4)*CELL - 36:
            y = (curboard.height-4)*CELL - 36

        if dirhint not in (1, -1):
            controldelay = 5
            if x < boards.bwidth//2:
                dir = 1
            else:
                dir = -1
        else:
            controldelay = 20
            if x < boards.bwidth//3:
                dir = 1
            elif x >= boards.bwidth*2//3:
                dir = -1
            else:
                dir = dirhint
        if dir > 0:
            self.angle = 0
            self.flipped = False
        else:
            self.angle = 180
            self.flipped = True

        ActiveSprite.__init__(self, self.getico(), x, y)
        self.fx = self.x
        self.fy = self.y
        self.controlgen = self.control(delay=controldelay)
        self.gen.append(self.fly())
        self.gen.append(self.controlgen)
        self.gen.append(self.blink())
        self.gen.append(self.bob())

    def controlled(self):
        return self.controlgen in self.gen

    def getico(self):
        a = self.angle // ANGLE_STEP
        if self.flipped:
            if a:
                a = ANGLE_COUNT-a
            key = 'vflip', ('camel', a, self.bubber.pn)
        else:
            key = 'camel', a, self.bubber.pn
        return images.sprget(key)

    def blink(self):
        for i in range(10):
            yield None
            yield None
            self.setdisplaypos(-256, -256)
            yield None
            self.setdisplaypos(-256, -256)
            yield None
        self.touchable = 1

    def bob(self):
        f = 3.0
        for i in range(0, 1080, ANGLE_STEP):
            self.fy += f * ANGLE_TABLE[i % 360][1]
            f *= 0.98
            yield None

##    def loosealtitude(self, y0, angle0):
##        if 90 <= angle0 < 270 or (angle0 == 270 and not self.flipped):
##            angledir = -1
##        else:
##            angledir = 1
##        for i in range(0, 180, ANGLE_STEP):
##            if i % (4*ANGLE_STEP) == 0 and not (45 <= angle0 <= 135):
##                angle0 += ANGLE_STEP * angledir
##                angle0 = (angle0 + 360) % 360
##            y0 += 4.0 * ANGLE_TABLE[i][1]
##            if y0 > self.fy:
##                self.fy = y0
##            self.angle = angle0
##            yield None

    def turn(self, dir):
        self.angle += ANGLE_STEP * dir
        self.angle = (self.angle + 360) % 360

    def control(self, delay=0):
        bubber = self.bubber
        prev_key_jump = 0
        for i in range(delay):
            yield None
        shootdelay = 0
        while True:
            wannago = bubber.wannago(self.dcap)
            self.turn(wannago)
            if shootdelay:
                shootdelay -= 1
            elif bubber.key_fire:
                x = self.x + self.ico.w//2
                y = self.y + self.ico.h//2
                acos, asin = ANGLE_TABLE[self.angle]
                x += acos * 20
                y += asin * 20
                if self.flipped:
                    acos = -acos
                    asin = -asin
                x -= asin * 5
                y += acos * 5
                self.play(snd_fire)
                Shot(self, int(x), int(y), self.angle, 2)
                Shot(self, int(x), int(y), self.angle)
                shootdelay = 7
            for i in range(2):
                if bubber.key_jump > prev_key_jump:
                    self.flipped = not self.flipped
                prev_key_jump = bubber.key_jump
                yield None
            for s in self.touching(12):
                if isinstance(s, Plane) and s is not self:
                    ico = images.sprget(Monky.decay_weapon[1])
                    s1 = ActiveSprite(ico,
                                      (self.x+s.x)//2 + self.ico.w//2 - CELL,
                                      (self.y+s.y)//2 + self.ico.h//2 - CELL)
                    s1.gen.append(s1.die(Monky.decay_weapon[1:], 4))
                    s1.play(snd_crash)
                    self.gen = [self.godowninflames(s)]
                    s.gen = [s.godowninflames(self)]

    def fly(self, speed=3.3):
        while True:
            if (self.y < 0 and not (0 < self.angle < 180) and
                ((abs(270 - self.angle) < -4*self.y) or random.random() < 0.2)):
                if (90 <= self.angle < 270 or
                    (self.angle == 270 and not self.flipped)):
                    self.turn(-1)
                else:
                    self.turn(1)
            ico = self.getico()
            acos, asin = ANGLE_TABLE[self.angle]
            self.fx += acos * speed
            self.fy += asin * speed
            self.move(int(self.fx), int(self.fy), ico)
            if self.x < x_min:
                self.angle = 2 * ANGLE_STEP
                self.flipped = not self.flipped
                self.gen = [self.godowninflames()]
                self.play(images.Snd.Pop)
            elif self.x > x_max:
                self.angle = 180 - 2 * ANGLE_STEP
                self.flipped = not self.flipped
                self.gen = [self.godowninflames()]
                self.play(images.Snd.Pop)
            elif self.y > y_max:
                self.gen = [self.crashed()]
            yield None

    def godowninflames(self, hit_by_plane=None):
        if hit_by_plane and hit_by_plane in self.shotlist:
            hittime = self.shotlist[hit_by_plane]
            if BubPlayer.FrameCounter < hittime + 60:
                del self.shotlist[hit_by_plane]
                scoreboard()
        self.seticon(self.getico())
        self.gen.append(self.fly())
        trail = [(self.x, self.y)] * 7
        ico = images.sprget(PlayerBubbles.explosion[0])
        s = ActiveSprite(ico, self.x + self.ico.w//2 - CELL,
                              self.y + self.ico.h//2 - CELL)
        s.gen.append(s.die(PlayerBubbles.explosion))
        self.bubber.emotic(self, 4)
        while True:
            yield None
            if random.random() < 0.37:
                ico = images.sprget(Bubble.exploding_bubbles[0])
                x, y = random.choice(trail)
                x += random.randint(-10, 10)
                y += random.randint(-10, 10)
                s = ActiveSprite(ico, x+2, y+2)
                s.gen.append(s.die(Bubble.exploding_bubbles))
            if random.random() < 0.5:
                yield None
                if 90 <= self.angle < 270:
                    lst = [0, 0, 0, 0, -1, -1, -1, 1, 1]
                else:
                    lst = [0, 0, 0, 0, -1, -1, 1, 1, 1]
                self.turn(random.choice(lst))
            trail.pop(0)
            trail.append((self.x, self.y))

    def crashed(self):
        self.untouchable()
        self.play(snd_crash)
        ico = images.sprget(Monky.decay_weapon[1])
        self.seticon(ico)
        self.step(self.ico.w//2 - CELL,
                  self.ico.h//2 - CELL)
        self.gen.append(self.die(Monky.decay_weapon[1:], 4))
        yield None

    def kill(self):
        try:
            self.bubber.dragons.remove(self)
        except ValueError:
            pass
        ActiveSprite.kill(self)


class Shot(ActiveSprite):

    def __init__(self, plane, x, y, angle, steps=0):
        ico = images.sprcharacterget('.')
        ActiveSprite.__init__(self, ico, x-4, y-12)
        self.plane = plane
        self.angle = angle
        self.gen.append(self.moving(steps))

    def moving(self, steps=0):
        minx = 2*CELL - 4
        maxx = (curboard.width-2)*CELL  - 4
        maxy = (curboard.height-1)*CELL - 12
        fx = self.x
        fy = self.y
        dx, dy = ANGLE_TABLE[self.angle]
        dx *= 7.6
        dy *= 7.6
        fx += dx * steps
        fy += dy * steps
        for i in range(22-steps):
            for s in images.touching(self.x+3, self.y+11, 2, 2):
                if isinstance(s, Plane) and s is not self.plane:
                    self.play(snd_hit)
                    self.kill()
                    if s.controlled():
                        s.gen = [s.godowninflames(self.plane)]
                        self.plane.shotlist[s] = BubPlayer.FrameCounter
                    bonuses.points(self.x + 4 - CELL, self.y + 12 - CELL,
                                   self.plane, 100)
                    return
            fx += dx
            fy += dy
            self.move(int(fx), int(fy))
            if self.x < minx or self.x > maxx or self.y > maxy:
                break
            yield None
        self.kill()


class Camel:
    
    def bgen(self, limittime = 90.1): # 1:30
        self.score = {}
        for t in boards.initsubgame(music, self.displaypoints):
            yield t

        tc = boards.TimeCounter(limittime)
        for t in self.frame(tc):
            t = boards.normal_frame()
            self.build_planes()
            yield t
            tc.update(t)
            if (BubPlayer.FrameCounter & 15) == 7:
                for s in images.ActiveSprites:
                    if isinstance(s, Bubble):
                        s.pop()
                    elif isinstance(s, Bonus):
                        s.kill()

        tc.restore()
        score = {}
        for player, shotlist in list(self.score.items()):
            score[player] = len(shotlist)
        for t in boards.result_ranking(score):
            for p in BubPlayer.PlayerList:
                for d in p.dragons[:]:
                    d.kill()
            yield t
        self.remove_planes()

    def displaypoints(self, bubber):
        return len(self.score.get(bubber, ()))

    def build_planes(self):
        for p in BubPlayer.PlayerList:
            dragons = [d for d in p.dragons if not isinstance(d, Plane)]
            if dragons and len(p.dragons) == len(dragons):
                dragon = random.choice(dragons)
                if dragon.dcap['infinite_shield']:
                    start_position = self.select_start_position()
                    dirhint = None
                else:
                    start_position = dragon.x-2, dragon.y-2
                    dirhint = getattr(dragon, 'dir', None)
                plane = Plane(self, p, dragon.dcap,
                              start_position[0], start_position[1], dirhint)
                p.dragons.append(plane)
                p.emotic(plane, 4)
            for d in dragons:
                d.kill()

    def remove_planes(self):
        for p in BubPlayer.PlayerList:
            for d in p.dragons[:]:
                d.kill()

    def select_start_position(self):
        planes = [d for p in BubPlayer.PlayerList
                    for d in p.dragons
                    if isinstance(d, Plane)]
        distmin = 180
        while True:
            x = random.choice([x_min, x_max])
            y = random.randint(2*CELL, (curboard.height-4)*CELL - 36)
            for d in planes:
                dist = (x-d.x)*(x-d.x) + (y-d.y)*(y-d.y)
                if dist < distmin*distmin:
                    break
            else:
                return x, y
            distmin = int(distmin * 0.94)

    def frame(self, tc):
        y = curboard.height-1
        for x in range(2, curboard.width-2):
            if (y, x) not in curboard.walls_by_pos:
                curboard.putwall(x, y)
        curboard.reorder_walls()
        for y in range(0, curboard.height-1):
            yield None
            for x in range(2, curboard.width-2):
                if (y, x) in curboard.walls_by_pos:
                    curboard.killwall(x, y)
        while tc.time != 0.0:
            yield None

# This game is suitable for at least min_players players
min_players = 2

def run():
    global curboard, x_min, x_max, y_max
    from boards import curboard
    x_min = 2*CELL - 3
    x_max = (curboard.width-2)*CELL - 36 + 3
    y_max = (curboard.height-1)*CELL - 36 + 7
    boards.replace_boardgen(Camel().bgen())

def setup():
    for key, (filename, rect) in list(localmap.items()):
        filename = os.path.join(LocalDir, filename)
        if filename.find('%d') >= 0:
            for p in BubPlayer.PlayerList:
                images.sprmap[key + (p.pn,)] = (filename % p.pn, rect)
        else:
            images.sprmap[key] = (filename, rect)
setup()
