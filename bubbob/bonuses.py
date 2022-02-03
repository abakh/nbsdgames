
import random, os, math
import random as random_module
import gamesrv
import images
import boards
from boards import *
from images import ActiveSprite
from mnstrmap import GreenAndBlue, Bonuses, Diamonds, Stars, BigImages
from mnstrmap import PotionBonuses, Fire
from player import BubPlayer


questionmarklist = ['questionmark3',
                    'questionmark4',
                    'questionmark5',
                    'questionmark4',
                    'questionmark3',
                    'questionmark2',
                    'questionmark1',
                    'questionmark2']

class Bonus(ActiveSprite):
    bubblable = 1
    touchable = 1
    points = 750
    timeout = 250
    sound = 'Fruit'
    endaction = None
    multiply = 1
    killgens = 1

    def __init__(self, x, y, nimage=None, points=None, falling=1):
        if nimage is not None:
            self.nimage = nimage
        if points is not None:
            self.points = points
        ActiveSprite.__init__(self, images.sprget(self.nimage), x, y)
        self.taken_by = []
        self.gen.append(self.timeouter())
        if falling:
            self.gen.append(self.faller())

    def buildoutcome(self):
        return (self.__class__,)

    def faller(self):
        while self.y < boards.bheight:
            if onground_nobottom(self.x, self.y):
                yield None
                yield None
            else:
                self.move(self.x, (self.y+4) & ~3)
            yield None
        self.kill()

    def timeouter(self):
        for i in range(self.timeout):
            yield None
        if self.timeout:
            self.kill()

    def touched(self, dragon):
        dx, dy, dw, dh = dragon.x, dragon.y, dragon.ico.w, dragon.ico.h
        if (dx + dw > self.x + 10 and
            dy + dh > self.y + 8  and
            self.x + self.ico.w > dx + 10 and
            self.y + self.ico.h > dy + 10):
            self.reallytouched(dragon)

    def reallytouched(self, dragon):
        if not self.taken_by:
            if self.killgens:
                self.gen = []
            self.gen.append(self.taking())
            sound = self.sound
            if sound:
                if isinstance(sound, str):
                    sound = getattr(images.Snd, sound)
                self.play(sound)
        if dragon not in self.taken_by:
            self.taken_by.append(dragon)
            if isinstance(self, (RandomBonus, MonsterBonus)):
                s_bonus = dragon.bubber.stats.setdefault('bonus', {})
                s_bonus[self.nimage] = s_bonus.get(self.nimage, 0) + 1

    def taking(self, follow_dragons=0, delay=1):
        from player import Dragon
        for t in range(delay):
            yield None   # time to be taken by several dragons
        if self.points:
            for p in self.taken_by:
                if follow_dragons and p.alive:
                    s = p
                else:
                    s = self
                points(s.x + s.ico.w//2, s.y + s.ico.h//2 - CELL, p, self.points)
        dragons = [d for d in self.taken_by if isinstance(d, Dragon)]
        if self.taken1(dragons) != -1:
            self.kill()

    def taken1(self, dragons):
        for d in dragons * self.multiply:
            if d.alive:
                self.taken(d)

    def taken(self, dragon):
        pass

    def in_bubble(self, bubble):
        self.untouchable()
        bubble.move(self.x, self.y)
        bubble.to_front()
        self.to_front()
        self.gen = [self.bubbling(bubble, self.ico)]
        self.move(bubble.x+8, bubble.y+8, images.sprget('questionmark3'))
        self.setimages(self.cyclic(questionmarklist, 2))

    def bubbling(self, bubble, ico):
        while not hasattr(bubble, 'poplist'):
            self.move(bubble.x+8, bubble.y+8)
            yield None
        if bubble.poplist is not None:
            dragon = bubble.poplist[0]
            if dragon is not None:
                self.play(images.Snd.Yippee)
                if dragon not in self.taken_by:
                    self.taken_by.append(dragon)
                if self.points > 10:
                    dragon.bubber.givepoints(self.points - 10)
                    pn = dragon.bubber.pn
                    if self.points in GreenAndBlue.points[pn]:
                        Points(bubble.x + bubble.ico.w//2, bubble.y, pn, self.points)
                self.taken1(BubPlayer.DragonList)
                p = Parabolic(ico, bubble.x, bubble.y)
                p.gen.append(p.moving(-1.0))
        self.kill()

    def is_on_ground(self):
        return onground(self.x, self.y)


def points(x, y, dragon, points):
    dragon.bubber.givepoints(abs(points))
    pn = dragon.bubber.pn
    if points in GreenAndBlue.points[pn]:
        Points(x, y, pn, points)

class Points(ActiveSprite):

    def __init__(self, x, y, pn, points):
        ico = images.sprget(GreenAndBlue.points[pn][points])
        ActiveSprite.__init__(self, ico, x - ico.w//2, max(8, y))
        self.nooverlap = 1
        self.gen.append(self.raiser())

    def raiser(self):
        wait = 0
        for s in images.ActiveSprites:
            if s is self:
                break
            if (isinstance(s, Points) and s.nooverlap and
                abs(self.x-s.x)<self.ico.w*2//3 and
                abs(self.y-s.y)<self.ico.h):
                wait += 5
        for t in range(wait):
            yield None
        for i in range(25):
            if i == 7:
                self.nooverlap = 0
            self.step(0, -2)
            yield None
            if self.y <= 0:
                break
        for i in range(20):
            yield None
        self.kill()


class Parabolic(ActiveSprite):
    fallstraight = 0
    fallspeed = 4
    
    def moving(self, y_amplitude = -8.0):
        bottom_up = self.fallspeed < 0
        dxy = [(random.random()-0.5) * 15.0,
               (random.random()+0.5) * y_amplitude * (1,-1)[bottom_up]]
        if bottom_up:
            kw = {'gravity': -0.3}
        else:
            kw = {}
        for n in self.parabolic(dxy, self.fallstraight, **kw):
            progress = self.parabole_progress = dxy[1] * (1,-1)[bottom_up]
            yield n
            if progress >= 4.0 and self.fallstraight:
                del self.parabole_progress
                self.gen.append(self.falling())
                return
        self.kill()

    def falling(self):
        nx, ny = vertical_warp(self.x, self.y & ~3)
        if self.fallspeed < 0:
            groundtest = underground
        else:
            groundtest = onground
        while not groundtest(nx, ny):
            ny += self.fallspeed
            nx, ny1 = vertical_warp(nx, ny)
            if ny1 != ny:
                ny = ny1
                self.wrapped_around()
            self.move(nx, ny)
            yield None
        self.move(nx, ny)
        self.build()
        self.kill()

    def killmonsters(self, poplist):
        from monsters import Monster
        while 1:
            for s in self.touching(0):
                if isinstance(s, Monster):
                    s.argh(poplist)
            yield None

    def build(self):
        pass

    def wrapped_around(self):
        pass


class Parabolic2(Parabolic):
    points = 0

    def __init__(self, x, y, imglist, imgspeed=3, onplace=0, y_amplitude=-8.0):
        Parabolic.__init__(self, images.sprget(imglist[0]), x, y)
        if onplace:
            self.gen.append(self.falling())
        else:
            self.gen.append(self.moving(y_amplitude))
        if len(imglist) > 1:
            self.setimages(self.cyclic(imglist, imgspeed))

    def touched(self, dragon, rect=None):
        if self.points:
            points(self.x + self.ico.w/2, self.y + self.ico.h/2 - CELL,
                   dragon, self.points)
            self.kill()


class BonusMaker(Parabolic2):
    fallstraight = 1
    touchable = 1

    def __init__(self, x, y, imglist, imgspeed=3, onplace=0, outcome=None):
        assert outcome
        self.outcome = outcome
        if outcome == (Flower2,):
            self.fallspeed = -self.fallspeed
        Parabolic2.__init__(self, x, y, imglist, imgspeed, onplace)

    def falling(self):
        cls = self.outcome[0]
        if issubclass(cls, Megabonus):
            self.build()
            return self.die([])
        else:
            return Parabolic2.falling(self)

    def wrapped_around(self):
        cls = self.outcome[0]
        if issubclass(cls, RandomBonus) and not boards.curboard.playingboard:
            self.kill()

    def build(self):
        cls = self.outcome[0]
        args = self.outcome[1:]
        if issubclass(cls, RandomBonus) and not boards.curboard.playingboard:
            return None
        else:
            return cls(self.x, self.y, *args)

    def touched(self, dragon, rect=None):
        pass

    def in_bubble(self, bubble):
        bonus = self.build()
        self.kill()
        if bonus:
            bonus.in_bubble(bubble)
        return bonus

class BonusMakerExtraStar(ActiveSprite):

    def __init__(self, x, y, sx, sy, colorname):
        self.sx = sx
        self.sy = sy
        imglist = [('smstar', colorname, k) for k in range(2)]
        ActiveSprite.__init__(self, images.sprget(imglist[-1]),
                                    x + HALFCELL, y + HALFCELL)
        self.setimages(self.cyclic(imglist, speed=2))

    def follow_bonusmaker(self, bm):
        for t in range(4):
            yield None
            if hasattr(bm, 'parabole_progress'):
                break
        else:
            self.kill()
            return
        start = bm.parabole_progress
        if start < 3.9:
            while bm.alive and hasattr(bm, 'parabole_progress'):
                f = (bm.parabole_progress-start) / (4.0-start)
                self.move(bm.x + HALFCELL + int(f*self.sx),
                          bm.y + HALFCELL + int(f*self.sy))
                yield None
        self.kill()


class MonsterBonus(Bonus):

    def __init__(self, x, y, multiple, forceimg=0):
        self.level = multiple
        if multiple >= len(Bonuses.monster_bonuses):
            multiple = len(Bonuses.monster_bonuses) - 1
        img, pts = Bonuses.monster_bonuses[multiple]
        Bonus.__init__(self, x, y, forceimg or img, pts)

    def buildoutcome(self):
        return (self.__class__, self.level)

    def taken(self, dragon):
        dragon.carrybonus(self, 543)

class IceMonsterBonus(MonsterBonus):

    def __init__(self, x, y, multiple):
        self.level = multiple
        if multiple >= 1:
            img, pts = Bonuses.violet_ice, 750
        else:
            img, pts = Bonuses.cyan_ice, 700
        Bonus.__init__(self, x, y, img, pts)



class DustStar(ActiveSprite):
    localrandom = random.Random()

    def __init__(self, x, y, basedx, basedy, big=1, clock=0):
        self.colorname = self.localrandom.choice(Stars.COLORS)
        self.imgspeed = self.localrandom.randrange(3, 6)
        self.rotation_reversed = self.localrandom.random() < 0.5
        ico, imggen = self.select_ico(getattr(Stars, self.colorname))
        ActiveSprite.__init__(self, ico, x, y)
        self.setimages(imggen)
        self.gen.append(self.fly(basedx, basedy, big))
        if not big:
            self.make_small()
        elif clock:
            self.setimages(None)
            self.seticon(images.sprget(Bonuses.clock))

    def select_ico(self, imglist):
        if self.rotation_reversed:
            imglist = list(imglist)
            imglist.reverse()
        return (images.sprget(imglist[-1]),
                self.cyclic(imglist, self.imgspeed))

    def make_small(self):
        images = [('smstar', self.colorname, k) for k in range(2)]
        ico, imggen = self.select_ico(images)
        self.seticon(ico)
        self.setimages(imggen)

    def fly(self, dx, dy, big):
        random = self.localrandom
        dx += (random.random() - 0.5) * 2.8
        dy += (random.random() - 0.5) * 2.8
        fx = self.x
        fy = self.y
        if big:
            j = 0
        else:
            j = 2
        while j < 3:
            ttl = random.expovariate(1.0 / 12)
            if ttl > 35:
                ttl = 35
            for i in range(int(ttl)+4):
                fx += dx
                fy += dy
                self.move(int(fx), int(fy))
                yield None
            if j == 0:
                self.make_small()
                fx += 8
                fy += 8
            j += 1
        self.kill()


class RandomBonus(Bonus):
    timeout = 500

class TemporaryBonus(RandomBonus):
    captime = 0
    bonusleveldivider = 2
    def taken(self, dragon):
        dragon.dcap[self.capname] += 1
        self.carried(dragon)
    def carried(self, dragon):
        captime = self.captime
        if boards.curboard.bonuslevel:
            captime = (captime or 999) // self.bonusleveldivider
        if captime:
            dragon.carrybonus(self, captime)
        else:
            dragon.carrybonus(self)
            self.endaction = None
    def endaction(self, dragon):
        if dragon.dcap[self.capname] >= 1:
            dragon.dcap[self.capname] -= 1


class ShoeSpeed(RandomBonus):
    "Fast Runner. Cumulative increase of horizontal speed."
    nimage = Bonuses.shoe
    bigbonus = {'multiply': 3}
    bigdoc = "Run Really Fast."
    def taken(self, dragon):
        dragon.dcap['hspeed'] += 1
        dragon.carrybonus(self)

class CoffeeSpeed(RandomBonus):
    "Caffeine. Cumulative increase of the horizontal speed and fire rate."
    nimage = Bonuses.coffee
    big = 0
    bigbonus = {'big': 1, 'multiply': 3}
    bigdoc = "Super-Excited!  Break through walls!"
    def taken(self, dragon):
        dragon.dcap['hspeed'] += 0.5
        dragon.dcap['firerate'] += 1
        if self.big:
            dragon.dcap['breakwalls'] = 1
        dragon.carrybonus(self)

class Butterfly(TemporaryBonus):
    "Lunar Gravity. Allows you to jump twice as high as before."
    nimage = Bonuses.butterfly
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Butterflies all around."
    def taken1(self, dragons):
        if self.big:
            import mnstrmap, monsters
            for i in range(17):
                monsters.Butterfly(mnstrmap.Butterfly,
                                   self.x + random.randrange(-40, 41),
                                   self.y + random.randrange(-30, 31),
                                   random.choice([-1, 1]))
        else:
            TemporaryBonus.taken1(self, dragons)
    def taken(self, dragon):
        dragon.dcap['gravity'] *= 0.5
        self.carried(dragon)
    def endaction(self, dragon):
        dragon.dcap['gravity'] *= 2.0

class Cocktail(TemporaryBonus):
    "Short Lived Bubbles. Makes your bubbles explode more quickly."
    nimage = Bonuses.cocktail
    points = 2000
    capname = 'bubbledelay'
    bigbonus = {'multiply': 3}
    bigdoc = "Makes your bubbles explode at once.  Dangerous!"

class Extend(RandomBonus):
    "E X T E N D. Gives you your missing letters and clear the level. "
    nimage = Bonuses.extend
    points = 0
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "A lot of letter bubbles! Run! Run!"

    def taken1(self, dragons):
        if self.big:
            self.letterexplosion()
        else:
            RandomBonus.taken1(self, dragons)
            
    def taken(self, dragon):
        from bubbles import extend_name
        names = [extend_name(l) for l in range(6)]
        missing = [name for name in names if name not in dragon.bubber.letters]
        x = dragon.x + dragon.ico.w//2
        y = dragon.y
        points(x, y, dragon, 10000*len(missing))
        for l in range(6):
            if extend_name(l) in missing:
                dragon.bubber.giveletter(l, promize=0)

    def letterexplosion(self):
        from bubbles import LetterBubble
        playercount = len([p for p in BubPlayer.PlayerList if p.isplaying()])
        N = 3 + (playercount > 3)
        angles = [i*(2.0*math.pi/N) for i in range(N)]
        for l, dx, dy in [(0,  5,  9), (1, 16, 10), (2, 26,  8),
                          (3,  7, 23), (4, 15, 24), (5, 25, 24)]:
            delta = 2.0*math.pi * random.random()
            angles = [angle-delta for angle in angles]
            x = self.x + self.ico.w//2 + 3*(dx-16)
            y = self.y + self.ico.h//2 + 3*(dy-16)
            for angle in angles:
                bubble = LetterBubble(None, l)
                bubble.thrown_bubble(x, y, 7.0 + 4.0 * random.random(),
                                     (math.cos(angle), math.sin(angle)))

class HeartPoison(RandomBonus):
    "Heart Poison. Freeze all free monsters."
    nimage = Bonuses.heart_poison
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Freeze all other players too!"
    def taken1(self, dragons):
        import monsters
        monsters.freeze_em_all()
        if self.big:
            def heart_pause(dragon, gen):
                for i in range(222):
                    yield None
                dragon.gen = gen
            for d in BubPlayer.DragonList:
                if d not in dragons:
                    d.gen = [heart_pause(d, d.gen)]

class VioletNecklace(RandomBonus):
    "Monster Duplicator. Double the number of free monsters."
    points = 650
    nimage = Bonuses.violet_necklace
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "This level's boring, let's bring even more monsters..."
    def taken1(self, dragons):
        if self.big:
            import monsters, mnstrmap
            mlist = 2*['Nasty', 'Monky', 'Springy', 'Orcy', 'Gramy', 'Blitzy']
            wrange = (boards.bwidth - 8*CELL) // 2
            for dir in [1, -1]:
                for i in range(len(mlist)):
                    name = mlist[i]
                    mdef = getattr(mnstrmap, name)
                    cls = getattr(monsters, name)
                    x = wrange * i // len(mlist)
                    if dir == 1:
                        x = 2*CELL + HALFCELL + x
                    else:
                        x = boards.bwidth - 4*CELL - HALFCELL - x
                    y = -2*CELL - i * 2*CELL
                    cls(mdef, x, y, dir)
        else:
            for s in BubPlayer.MonsterList[:]:
                if s.regular():
                    for i in range(self.multiply):
                        s.__class__(s.mdef, s.x, s.y, -s.dir * (-1)**i)

class WandBonus(RandomBonus):
    "Wand/Chest. Turn the bubble into bonuses at the end of the level."
    nimages = [Bonuses.brown_wand,   Bonuses.yellow_wand, Bonuses.green_wand,
               Bonuses.violet_wand,  Bonuses.blue_wand,   Bonuses.red_wand,
               Bonuses.violet_chest, Bonuses.blue_chest,  Bonuses.red_chest,
               Bonuses.yellow_chest,
               ]
    Modes = [
        (Bonuses.brown_wand,   750,  Bonuses.cyan_ice,    700,  BigImages.cyan_ice,    20000),
        (Bonuses.yellow_wand,  750,  Bonuses.violet_ice,  750,  BigImages.violet_ice,  20000),
        (Bonuses.green_wand,   750,  Bonuses.peach2,      800,  BigImages.peach2,      30000),
        (Bonuses.violet_wand,  750,  Bonuses.pastec2,     850,  BigImages.pastec2,     30000),
        (Bonuses.blue_wand,    750,  Bonuses.cream_pie,   900,  BigImages.cream_pie,   40000),
        (Bonuses.red_wand,     750,  Bonuses.sugar_pie,   950,  BigImages.sugar_pie,   40000),
        (Bonuses.violet_chest, 2000, Diamonds.violet,     6000, BigImages.violet,      60000),
        (Bonuses.blue_chest,   2000, Diamonds.blue,       7000, BigImages.blue,        60000),
        (Bonuses.red_chest,    2000, Diamonds.red,        8000, BigImages.red,         70000),
        (Bonuses.yellow_chest, 2000, Diamonds.yellow,     9000, BigImages.yellow,      70000),
        ]
    def __init__(self, x, y):
        self.mode = random.choice(WandBonus.Modes)
        RandomBonus.__init__(self, x, y, *self.mode[:2])
    def taken1(self, dragons):
        BubPlayer.BubblesBecome = self.bubble_outcome
        BubPlayer.MegaBonus     = self.mega_bonus
    def bubble_outcome(self, bubble):
        if bubble.pop():
            x = bubble.x
            if x < 2*CELL:
                x = 2*CELL
            elif x > boards.bwidth - 4*CELL:
                x = boards.bwidth - 4*CELL
            Bonus(x, bubble.y, *self.mode[2:4])
    def mega_bonus(self):
        nico, npoints = self.mode[4:6]
        ico = images.sprget(nico)
        x = random.randrange(0, boards.bwidth-ico.w)
        mb = Megabonus(x, -ico.h, nico, npoints)
        mb.outcome = (Bonus,) + self.mode[2:4]
        mb.outcome_image = self.mode[2]
WandBonus1 = WandBonus  # increase probability

class Megabonus(Bonus):
    touchable = 0
    vspeed = 6
    sound = 'Extra'
    coverwithbonus = 99
    fallerdelay = 71
    
    def faller(self):
        self.fullpoints = self.points
        self.bubbles = {}
        for t in range(self.fallerdelay):
            yield None
        self.ready_to_go()
        self.bubbles_pos = list(self.bubbles_position())
        self.gen.append(self.animate_bubbles())
        y0 = self.y - HALFCELL
        ymax = boards.bheight - CELL - self.ico.h
        self.touchable = 1
        ny = self.y
        while self.y >= y0:
            if self.vspeed:
                ny += self.vspeed
                if ny > ymax:
                    ny = ymax
                    self.vspeed = 0
                self.move(self.x, int(ny))
            yield None
        self.kill()

    def ready_to_go(self):
        pass

    def is_on_ground(self):
        return self.y == boards.bheight - CELL - self.ico.h

    def kill(self):
        for bubble in list(self.bubbles.values()):
            bubble.pop()
        Bonus.kill(self)

    def taken(self, dragon):
        poplist = [dragon]
        for bubble in list(self.bubbles.values()):
            bubble.pop(poplist)

    def bubbles_position(self):
        import time; start=time.time()
        cx = self.ico.w//2 - CELL
        cy = self.ico.h//2 - CELL
        positions = []
        pi2 = math.pi * 2
        dist = 10.0
        for i in range(31):
            while 1:
                angle = random.random() * pi2
                nx = cx + int(dist*math.sin(angle))
                ny = cy + int(dist*math.cos(angle))
                for ox, oy in positions:
                    if (nx-ox)*(nx-ox) + (ny-oy)*(ny-oy) < 220:
                        dist += 0.3
                        break
                else:
                    break
            positions.append((nx, ny))
        #print time.time()-start
        return positions
##        nx = 5
##        ny = 6
##        xmargin = 2
##        ymargin = 7
##        xstep = (self.ico.w+2*xmargin-2*CELL) / float(nx-1)
##        ystep = (self.ico.h+2*ymargin-2*CELL) / float(ny-1)
##        for dx in range(nx):
##            corner = dx in [0, nx-1]
##            for dy in range(corner, ny-corner):
##                dx1 = int(dx*xstep)-xmargin
##                dy1 = int(dy*ystep)-ymargin
##                yield (dx1 + random.randrange(-2,3),
##                       dy1 + random.randrange(-2,3))

    def nearest_free_point(self, x0, y0):
        distlst = [((x0-x)*(x0-x)+(y0-y)*(y0-y)+random.random(), x, y)
                   for x, y in self.bubbles_pos if (x, y) not in self.bubbles]
        if distlst:
            ignored, dx, dy = min(distlst)
            return dx, dy
        else:
            return None, None

    def in_bubble(self, bubble):
        if not self.touchable:
            return   # bubbling a BonusMaker about to make a big bonus
        dx, dy = self.nearest_free_point(bubble.x-self.x, bubble.y-self.y)
        if dx is not None:
            self.cover_bubble(dx, dy, bubble.d.bubber)
            self.gen.append(self.cover_bubbles(bubble.d.bubber))
        bubble.kill()

    def cover_bubbles(self, bubber):
        while 1:
            for t in range(2):
                yield None
            bubbles = [dxy for dxy, b in list(self.bubbles.items())
                           if b.bubber is bubber]
            if not bubbles:
                break
            dx, dy = self.nearest_free_point(*random.choice(bubbles))
            if dx is None:
                break
            self.cover_bubble(dx, dy, bubber)
        self.untouchable()

    def cover_bubble(self, dx, dy, bubber):
        if (dx, dy) in self.bubbles:
            return
        from bubbles import Bubble
        if len(self.bubbles) & 1:
            MegabonusBubble = Bubble
        elif self.coverwithbonus:
            self.coverwithbonus -= 1
            outcome = self.outcome
            outcome_image = self.outcome_image
            class MegabonusBubble(Bubble):
                def popped(self, dragon):
                    BonusMaker(self.x, self.y, [outcome_image], outcome=outcome)
                    return 10
        else:
            MegabonusBubble = Bubble
        
        nimages = GreenAndBlue.normal_bubbles[bubber.pn]
        b = MegabonusBubble(images.sprget(nimages[1]), self.x+dx, self.y+dy)
        b.dx = dx
        b.dy = dy
        b.bubber = bubber
        b.nimages = nimages
        self.bubbles[dx, dy] = b
        self.timeout = 0
        f = float(len(self.bubbles)) / len(self.bubbles_pos)
        self.vspeed = -0.73*f + self.vspeed*(1.0-f)
        self.points = int(self.fullpoints*(1.0-f) / 10000.0 + 0.9999) * 10000

    def animate_bubbles(self):
        if 0:  # disabled clipping
            d = {}
            for dx, dy in self.bubbles_pos:
                d[dx] = d[dy] = None
            north = d.copy()
            south = d.copy()
            west = d.copy()
            east = d.copy()
            del d
            for dx, dy in self.bubbles_pos:
                lst = [y for x, y in self.bubbles_pos if x==dx and y<dy]
                if lst: north[dy] = max(lst)
                lst = [y for x, y in self.bubbles_pos if x==dx and y>dy]
                if lst: south[dy] = min(lst)
                lst = [x for x, y in self.bubbles_pos if x<dx and y==dy]
                if lst: west[dx] = max(lst)
                lst = [x for x, y in self.bubbles_pos if x>dx and y==dy]
                if lst: east[dx] = min(lst)
            W = 2*CELL
            H = 2*CELL
        bubbles = self.bubbles
        while 1:
            for cycle in [1]*8 + [2]*10 + [1]*8 + [0]*10:
                yield None
                for (dx, dy), bubble in list(bubbles.items()):
                    if not hasattr(bubble, 'poplist'):
                        if 0:   # disabled clipping
                            if (dx, north[dy]) in bubbles:
                                margin_n = (north[dy]+H-dy)//2
                            else:
                                margin_n = 0
                            if (dx, south[dy]) in bubbles:
                                margin_s = (dy+H-south[dy])//2
                            else:
                                margin_s = 0
                            if (west[dx], dy) in bubbles:
                                margin_w = (west[dx]+W-dx)//2
                            else:
                                margin_w = 0
                            if (east[dx], dy) in bubbles:
                                margin_e = (dx+W-east[dx])//2
                            else:
                                margin_e = 0
                            r = (margin_w,
                                 margin_n,
                                 W-margin_w-margin_e,
                                 H-margin_n-margin_s)
                            bubble.move(self.x + bubble.dx + margin_w,
                                        self.y + bubble.dy + margin_n,
                                        images.sprget_subrect(
                                            bubble.nimages[cycle], r))
                        else:
                            bubble.move(self.x + bubble.dx,
                                        self.y + bubble.dy,
                                        images.sprget(bubble.nimages[cycle]))
                    elif len(bubbles) == len(self.bubbles_pos):
                        self.pop_bubbles(bubble.poplist)
                        return

    def pop_bubbles(self, poplist):
        def bubble_timeout(bubble, vspeed):
            ny = bubble.y
            for t in range(random.randrange(15,25)):
                if hasattr(bubble, 'poplist'):
                    return
                ny += vspeed
                bubble.move(bubble.x, int(ny))
                yield None
            bubble.pop(poplist)

        for bubble in list(self.bubbles.values()):
            bubble.gen.append(bubble_timeout(bubble, self.vspeed))
        self.bubbles.clear()
        self.kill()

class Cactus(RandomBonus):
    "Cactus. Drop a big version of a random bonus."
    points = 600
    nimage = 'cactus'
    extra_cheat_arg = None
    bigbonus = {'multiply': 3}
    bigdoc = "Let's get more big bonuses!"
    
    def taken1(self, dragons):
        count = 0
        while count < self.multiply:
            args = ()
            if self.extra_cheat_arg:
                cls = globals()[self.extra_cheat_arg]
                self.extra_cheat_arg = None
            elif bigclockticker and bigclockticker.state == 'pre':
                cls = Clock
                args = (1,)
            else:
                cls = random.choice(Classes)
            if makecactusbonus(cls, *args):
                count += 1
        cactusbonussound()

#Cactus1 = Cactus # increase probability

OFFSCREEN = -3*CELL
def makecactusbonus(cls, *args):
    bonus = cls(OFFSCREEN, 0, *args)
    if not bonus.alive or getattr(bonus, 'bigbonus', None) is None:
        if bonus.alive:
            bonus.kill()
        return None
    bonus.__dict__.update(bonus.bigbonus)
    bonus.untouchable()
    bonus.gen = []
    megacls = bonus.bigbonus.get('megacls', Cactusbonus)
    mb = megacls(0, -3*CELL, 'cactus', 10000)   # temp image
    mb.outcome = (cls,) + (args or bonus.bigbonus.get('outcome_args', ()))
    mb.outcome_image = bonus.nimage
    mb.bonus = bonus
    mb.gen.append(mb.prepare_image())
    mb.gen.append(mb.remove_if_no_bonus())
    return mb

def cactusbonussound():
    gamesrv.set_musics([], [])
    boards.curboard.set_musics(prefix=[images.music_modern])
    boards.curboard.set_musics()

class Cactusbonus(Megabonus):
    coverwithbonus = 5

    def prepare_image(self):
        while images.computebiggericon(self.bonus.ico) is None:
            yield None

    def remove_if_no_bonus(self):
        while self.bonus.alive:
            yield None
        self.kill()

    def ready_to_go(self):
        ico = images.biggericon(self.bonus.ico)
        x = random.randrange(0, boards.bwidth-ico.w)
        self.move(x, -ico.h, ico)

    def taken1(self, dragons):
        d1 = list(dragons)
        Megabonus.taken1(self, dragons)
        if self.bonus.alive:
            x = self.x + self.ico.w//2 - CELL
            y = self.y + self.ico.h//2 - CELL
            self.bonus.move(x, y)
            res = self.bonus.taken1(d1)
            self.untouchable()
            if res == -1:
                self.taken_by = []
                self.gen.append(self.touchdelay(10))
                self.bonus.move(OFFSCREEN, 0)
            else:
                self.bonus.kill()
            return res

    def kill(self):
        Megabonus.kill(self)
        if self.bonus.alive:
            self.bonus.kill()

class LongDurationCactusbonus(Cactusbonus):
    timeout = 500
    killgens = 0

def starexplosion(x, y, multiplyer, killmonsters=0, outcomes=[]):
    outcomes = list(outcomes)
    poplist = [None]
    for i in range(multiplyer):
        colors = list(Stars.COLORS)
        random.shuffle(colors)
        for colorname in colors:
            images = getattr(Stars, colorname)
            if outcomes:
                outcome = outcomes.pop()
                extra_stars = []
                if hasattr(outcome[0], 'extra_stars_location'):
                    for sx, sy in outcome[0].extra_stars_location:
                        extra_stars.append(BonusMakerExtraStar(x, y, sx, sy,
                                                               colorname))
                bm = BonusMaker(x, y, images, outcome=outcome)
                for star in extra_stars:
                    star.gen.append(star.follow_bonusmaker(bm))
            else:
                b = Parabolic2(x, y, images)
                if killmonsters:
                    b.gen.append(b.killmonsters(poplist))

class HomingStar(ActiveSprite):
    def __init__(self, x, y, colorname, poplist):
        imglist = getattr(Stars, colorname)
        ActiveSprite.__init__(self, images.sprget(imglist[0]), x, y)
        self.colorname = colorname
        self.setimages(self.cyclic(imglist, 2))
        self.gen.append(self.homing(poplist))

    def homing(self, poplist):
        from monsters import Monster
        target = None
        vx = (random.random() - 0.5) * 6.6
        vy = (random.random() - 0.5) * 4.4
        nx = self.x
        ny = self.y
        counter = 10
        while 1:
            if random.random() < 0.02:
                target = None
            if target is None or not target.alive:
                bestdist = 1E10
                for s in BubPlayer.MonsterList:
                    if isinstance(s, Monster):
                        dx = s.x - nx
                        dy = s.y - ny
                        dist = dx*dx + dy*dy + (random.random() * 25432.1)
                        if dist < bestdist:
                            bestdist = dist
                            target = s
                if target is None:
                    break
            dx = target.x - nx
            dy = target.y - ny
            dist = dx*dx + dy*dy
            if dist <= 3*CELL*CELL:
                target.argh(poplist)
                break
            yield None
            vx = (vx + dx * 0.005) * 0.96
            vy = (vy + dy * 0.005) * 0.96
            nx += vx
            ny += vy
            self.move(int(nx), int(ny))
            if counter:
                counter -= 1
            else:
                img = ('smstar', self.colorname, random.randrange(2))
                s = ActiveSprite(images.sprget(img), self.x + 8, self.y + 8)
                s.gen.append(s.die([None], speed=10))
                counter = 3
        self.kill()

class Book(RandomBonus):
    "Magic Bomb. Makes a magical explosion killing touched monsters."
    points = 2000
    nimage = Bonuses.book
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Homing Magical Stars."
    def taken1(self, dragons):
        if self.big:
            poplist = [None]
            x = self.x + (self.ico.w - 2*CELL) // 2
            y = self.y + (self.ico.h - 2*CELL) // 2
            colors = list(Stars.COLORS)
            random.shuffle(colors)
            for colorname in colors + colors[len(colors)//2:]:
                HomingStar(x, y, colorname, poplist)
        else:
            starexplosion(self.x, self.y, self.multiply, killmonsters=1)

class Potion(RandomBonus):
    "Potions. Clear the level and fill its top with bonuses."
    nimages = [Bonuses.red_potion, Bonuses.green_potion, Bonuses.yellow_potion,
               'potion4']
    Potions = [(Bonuses.red_potion,    150,  [(PotionBonuses.coin,        350),
                                              (PotionBonuses.rainbow,     600)]),
               (Bonuses.green_potion,  350,  [(PotionBonuses.flower,     1000),
                                              (PotionBonuses.trefle,     2000)]),
               (Bonuses.yellow_potion, 550,  [(PotionBonuses.green_note, 2000),
                                              (PotionBonuses.blue_note,  3000)]),
               ('potion4',             750,  None),
               ]
    LocalDir = os.path.dirname(__file__) or os.curdir
    Extensions = [s for s in os.listdir(LocalDir)
                    if s.startswith('ext') and
                       os.path.isdir(os.path.join(LocalDir, s))]
    random.shuffle(Extensions)
    extra_cheat_arg = None
    big = 0
    bigdoc = "Fill the whole level with bonuses."

    def __init__(self, x, y):
        p_normal = 3
        if boards.curboard.bonuslevel:
            p_extension = 2       # make extensions rare in the bonus level
        else:
            p_extension = 5
        if self.extra_cheat_arg:
            Potion.Extensions.append(self.extra_cheat_arg)
            p_normal = 0
        if not Potion.Extensions:
            p_extension = 0
        choices = []
        for mode in Potion.Potions:
            if mode[2] is None:
                p = p_extension
            else:
                p = p_normal
            choices += [mode] * p
        self.mode = random.choice(choices)
        if self.mode[2] is not None:
            self.bigbonus = {'big': 1}
        RandomBonus.__init__(self, x, y, *self.mode[:2])
    def taken1(self, dragons):
        blist = self.mode[2]
        if blist is not None:
            if random.random() < 0.6:
                blist = [random.choice(blist)]
            boards.replace_boardgen(boards.potion_fill(blist, self.big))
        else:
            n_players = len([p for p in BubPlayer.PlayerList if p.isplaying()])
            while Potion.Extensions:
                ext = Potion.Extensions.pop()
                #print "Trying potion:", ext
                ext = __import__(ext, globals(),locals(), ['run','min_players'])
                if n_players >= ext.min_players:
                    ext.run()
                    boards.BoardGen.append(boards.extra_bkgnd_black(self.x, self.y))
                    #print "Accepted because:", n_players, ">=", ext.min_players
                    break
                else:
                    #print "Rejected because:", n_players, "<", ext.min_players
                    pass

class FireBubble(RandomBonus):
    "Fire Bubbles. Makes you fire napalm bubbles."
    nimage = Bonuses.hamburger
    bubkind = 'FireBubble'
    bubcount = 10
    bigbonus = {'bubkind': 'BigFireBubble'}
    bigdoc = "Makes you shoot fire - you're a dragon after all."
    def taken(self, dragon):
        dragon.dcap['shootbubbles'] = [self.bubkind] * self.bubcount
        dragon.carrybonus(self)

class WaterBubble(FireBubble):
    "Water Bubbles. Your bubbles will now be filled with water."
    nimage = Bonuses.beer
    bubkind = 'WaterBubble'
    bigbonus = {'bubkind': 'SnookerBubble'}
    bigdoc = "Snooker balls."

class LightningBubble(FireBubble):
    "Lightning Bubbles."
    nimage = Bonuses.french_fries
    bubkind = 'LightningBubble'
    bigbonus = {'bubkind': 'BigLightBubble'}
    bigdoc = "Even-more-lightning Bubbles."

class Megadiamond(Megabonus):
    nimage = BigImages.red
    points = 20000
    fallerdelay = 0
    outcome = (MonsterBonus, -1)
    outcome_image = Bonuses.monster_bonuses[-1][0]
    extra_stars_location = [ (-24,-28),(0,-28),(24,-28),
                            (-40,-11),          (40,-11),
                              (-32, 8),       (32, 8),
                                 (-16,23), (16,23),
                                       (0,38),         ]
    def __init__(self, x, y):
        ico = images.sprget(self.nimage)
        x -= (ico.w - 2*CELL) // 2
        y -= (ico.h - 2*CELL) // 2
        Megabonus.__init__(self, x, y)

class Door(RandomBonus):
    "Magic Door. Let bonuses come in!"
    points = 1000
    nimage = Bonuses.door
    diamond_outcome = (MonsterBonus, -1)
    bigbonus = {'diamond_outcome': (Megadiamond,)}
    bigdoc = "Let bigger bonuses come in!"
    def taken1(self, dragons):
        starexplosion(self.x, self.y, 2,
                      outcomes = [self.diamond_outcome] * 10)

class LongFire(RandomBonus):
    "Long Fire. Increase the range of your bubble throw out."
    nimage = Bonuses.softice1
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Throw bubbles that split into more bubbles."
    def taken(self, dragon):
        if self.big:
            dragon.dcap['shootbubbles'] = ['MoreBubblesBubble'] * 10
        else:
            dragon.dcap['shootthrust'] *= 1.5
        dragon.carrybonus(self)

class Glue(RandomBonus):
    "Glue.  Triple fire."
    nimage = 'glue'
    points = 850
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Heptuple fire. (That's 7.)"
    def taken(self, dragon):
        if self.big:
            dragon.dcap['flower'] = -16   # heptuple fire
        elif dragon.dcap['flower'] >= 0:
            dragon.dcap['flower'] = -1    # triple fire
        else:
            dragon.dcap['flower'] -= 1    # cumulative effect
        dragon.carrybonus(self)

class ShortFire(RandomBonus):
    "Short Fire. Shorten the range of your bubble throw out."
    nimage = Bonuses.softice2
    points = 300
    factor = 1 / 1.5
    bigbonus = {'factor': 0}
    bigdoc = "What occurs if you throw bubbles at range zero?"
    def taken(self, dragon):
        dragon.dcap['shootthrust'] *= self.factor
        dragon.carrybonus(self)

class HighSpeedFire(RandomBonus):
    "High Speed Fire. Increase your fire rate."
    nimage = Bonuses.custard_pie
    points = 700
    bigbonus = {'multiply': 4}
    bigdoc = "Machine-gun speed!"
    def taken(self, dragon):
        dragon.dcap['firerate'] += 1.5
        dragon.carrybonus(self)

class Mushroom(TemporaryBonus):
    "Bouncy Bouncy. Makes you jump continuously."
    nimage = Bonuses.mushroom
    points = 900
    capname = 'pinball'
    captime = 625
    bigbonus = {'captime': captime*2, 'multiply': 2}
    bigdoc = "The same, but even more annoying."

class AutoFire(TemporaryBonus):
    "Auto Fire. Makes you fire continuously."
    nimage = Bonuses.rape
    points = 800
    capname = 'autofire'
    captime = 675
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Adds many bubbles to the level."
    def taken1(self, dragons):
        if self.big:
            boards.extra_boardgen(boards.extra_bubbles(900))
        else:
            TemporaryBonus.taken1(self, dragons)

class Insect(RandomBonus):
    "Crush World."
    nimage = Bonuses.insect
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "What if the level looked like that instead... Or like that... Or..."
    def taken1(self, dragons):
        if self.big:
            if dragons:
                d = random.choice(dragons)
                cx, cy = d.x, d.y
            else:
                cx, cy = None, None
            boards.extra_boardgen(boards.extra_make_random_level(cx, cy))
        else:
            boards.extra_boardgen(boards.extra_walls_falling())

class Ring(TemporaryBonus):
    "The One Ring."
    nimage = Bonuses.ring
    points = 4000
    capname = 'ring'
    captime = 700
    bonusleveldivider = 5
    bigbonus = {'multiply': 3}
    bigdoc = "Where am I?"

class GreenPepper(TemporaryBonus):
    "Hot Pepper. Run! Run! That burns."
    nimage = Bonuses.green_pepper
    capname = 'hotstuff'
    captime = 100
    bigbonus = {'captime': 250, 'multiply': 2}
    bigdoc = "That burns a lot!"

class Lollipop(TemporaryBonus):
    "Yo Man! Makes you walk backward."
    nimage = Bonuses.lollipop
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Just swapping 'left' and 'right' is not confusing enough."
    def taken(self, dragon):
        dragon.dcap['left2right'] = -dragon.dcap['left2right']
        if self.big:
            perm = list(range(4))
            while perm[0] == 0 or perm[1] == 1 or perm[2] == 2 or perm[3] == 3:
                random.shuffle(perm)
            names = ('key_left', 'key_right', 'key_jump', 'key_fire')
            dragon.dcap['key_right'] = names[perm[0]]
            dragon.dcap['key_left']  = names[perm[1]]
            dragon.dcap['key_jump']  = names[perm[2]]
            dragon.dcap['key_fire']  = names[perm[3]]
        self.carried(dragon)
    def endaction(self, dragon):
        dragon.dcap['left2right'] = -dragon.dcap['left2right']
        for name in ('key_left', 'key_right', 'key_jump', 'key_fire'):
            dragon.dcap[name] = name

class Chickpea(TemporaryBonus):
    "Basilik. Allows you to touch the monsters."
    nimage = Bonuses.chickpea
    points = 800
    capname = 'overlayglasses'
    captime = 400
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Turn off the light."

    def taken1(self, dragons):
        if self.big:
            boards.extra_boardgen(boards.extra_light_off(597), 1)
        else:
            TemporaryBonus.taken1(self, dragons)

    def taken(self, dragon):
        TemporaryBonus.taken(self, dragon)
        dragon.dcap['shield'] += 420

class IceCream(RandomBonus):
    "Icecream. An icecream which is so good you'll always want more."
    nimages = [Bonuses.icecream6, Bonuses.icecream5,
               Bonuses.icecream4, Bonuses.icecream3]
    IceCreams = [(Bonuses.icecream6,  250),
                 (Bonuses.icecream5,  500),
                 (Bonuses.icecream4,  1000),
                 (Bonuses.icecream3,  2000)]
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "BIG ice creams!"
    def __init__(self, x, y, generation=0):
        self.generation = generation
        RandomBonus.__init__(self, x, y, *self.IceCreams[generation])
    def taken1(self, dragons):
        nextgen = self.generation + 1
        if nextgen < len(self.IceCreams):
            for i in range(2):
                if self.big:
                    makecactusbonus(IceCream, nextgen)
                else:
                    x, y = chooseground(200)
                    if x is None:
                        return
                    IceCream(x, y, nextgen)
            if self.big:
                cactusbonussound()

class Grenade(RandomBonus):
    "Barbecue."
    nimage = Bonuses.grenade
    points = 550
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "360-degree flames."
    def taken1(self, dragons):
        from bubbles import FireFlame
        poplist = [None]
        for y in range(1, boards.height-1):
            for x in range(2, boards.width-2):
                if bget(x,y) != ' ':
                    continue
                if bget(x,y+1) == '#':
                    FireFlame(x, y, poplist)
                elif self.big:
                    if bget(x,y-1) == '#':
                        FireFlame(x, y, poplist, flip='vflip')
                    elif bget(x-1,y) == '#':
                        FireFlame(x, y, poplist, flip='cw')
                    elif bget(x+1,y) == '#':
                        FireFlame(x, y, poplist, flip='ccw')

class Conch(RandomBonus):
    "Sea Shell. Let's bring the sea here!"
    nimage = Bonuses.conch
    points = 650
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Aquarium."
    def taken1(self, dragons):
        if self.big:
            gen = boards.extra_aquarium
        else:
            gen = boards.extra_water_flood
        boards.extra_boardgen(gen())

def fire_rain(x, poplist):
    from bubbles import FireDrop
    FireDrop(x, -CELL, poplist)

def water_rain(x, poplist):
    from bubbles import watercell
    watercell(x, 0, poplist)

def ball_rain(x, poplist):
    from bubbles import SpinningBall
    SpinningBall(x, -CELL, poplist)

class Umbrella(RandomBonus):
    "Umbrellas. Beware of what's going to fall on everyone's head!"
    nimages = [Bonuses.brown_umbrella, Bonuses.grey_umbrella,
               Bonuses.violet_umbrella]
    Umbrellas = [(Bonuses.brown_umbrella,  900,  fire_rain,  10, 60),
                 (Bonuses.grey_umbrella,   950,  water_rain, 5,  60),
                 (Bonuses.violet_umbrella, 1000, ball_rain,  9, 120)]
    bigbonus = {'multiply': 3.1416}
    bigdoc = "It's raining hard."
    def __init__(self, x, y):
        self.mode = random.choice(Umbrella.Umbrellas)
        RandomBonus.__init__(self, x, y, *self.mode[:2])
    def taken1(self, dragons):
        for i in range(self.multiply):
            boards.extra_boardgen(self.raining())
    def raining(self):
        builder, drops, timemax = self.mode[2:]
        timemax = int(timemax * math.sqrt(self.multiply))
        drops = int(drops * self.multiply)
        times = [random.randrange(0, timemax) for i in range(drops)]
        poplist = [None]
        for t in range(timemax):
            for i in range(times.count(t)):
                x = random.randrange(2*CELL, bwidth-3*CELL+1)
                builder(x, poplist)
            yield 0

class Fruits(RandomBonus):
    "Fruits. A small little bonus. But the size doesn't matter, does it? If you're lucky enough you might get a great shower!"
    nimages = [Bonuses.kirsh, Bonuses.erdbeer, Bonuses.tomato,
               Bonuses.apple, Bonuses.corn, Bonuses.radish]
    bubblable = 0
    sound = 'Extra'
    Fruits = [(Bonuses.kirsh,      100),
              #(Bonuses.icecream1,  150),
              (Bonuses.erdbeer,    150),
              #(Bonuses.fish1,      250),
              (Bonuses.tomato,     200),
              #(Bonuses.donut,      250),
              (Bonuses.apple,      250),
              (Bonuses.corn,       300),
              #(Bonuses.icecream2,  600),
              (Bonuses.radish,     350),
              ]
    def __init__(self, x, y):  # x and y ignored !
        fine = 0
        for i in range(20):
            x0 = random.randint(3, boards.width-5)
            y0 = random.randint(1, boards.height-3)
            for xt in range(x0-1, x0+3):
                if xt == x0-1 or xt == x0+2:
                    yplus = 1
                else:
                    yplus = 0
                for yt in range(y0+yplus, y0+4-yplus):
                    if bget(xt,yt) != ' ':
                        break
                else:
                    continue
                break
            else:
                x, y = x0*CELL, y0*CELL
                fine = 1
                break
        mode = random.choice(Fruits.Fruits)
        RandomBonus.__init__(self, x, y, falling=0, *mode)
        self.repeatcount = 0
        if not fine:
            self.kill()
        elif random.random() < 0.04:
            self.superfruit = mode
            self.sound = 'Shh'
            self.points = 0
            self.repeatcount = random.randrange(50,100)
    def taken1(self, dragons):
        if self.repeatcount:
            image, points = self.superfruit
            f = Parabolic2(self.x, self.y, [image], y_amplitude = -1.5)
            f.points = points
            f.touchable = 1
            self.repeatcount -= 1
            self.gen.append(self.taking(1, 2))
            return -1
Fruits1 = Fruits  # increase probability
Fruits2 = Fruits
Fruits3 = Fruits
Fruits4 = Fruits
Fruits5 = Fruits
Fruits6 = Fruits

class BlueNecklace(RandomBonus):
    "Self Duplicator. Mirror yourself."
    points = 1000
    nimage = Bonuses.blue_necklace
    copies = 1
    bigbonus = {'copies': 3}
    bigdoc = "Mirrors vertically too."
    def taken(self, dragon):
        dragons = [dragon]
        modes = [(-1, 1), (1, -1), (-1, -1)][:self.copies]
        modes.reverse()
        dcap = dragon.dcap.copy()
        for sign, gravity in modes:
            if len(dragon.bubber.dragons) >= 7:
                break  # avoid burning the server with two much dragons
            d1 = self.makecopy(dragon, sign, gravity, dcap)
            dragons.append(d1)
        d1 = random.choice(dragons)
        d1.carrybonus(self, 250)

    def makecopy(self, dragon, sign=-1, gravity=1, dcap=None):
        from player import Dragon
        dcap = dcap or dragon.dcap
        d = Dragon(dragon.bubber, dragon.x, dragon.y, -dragon.dir, dcap)
        d.dcap['left2right'] = sign * dcap['left2right']
        d.dcap['gravity'] = gravity * dcap['gravity']
        d.up = dragon.up
        s = (dcap['shield'] + 12) & ~3
        dragon.dcap['shield'] = s+2
        if sign*gravity > 0:
            s += 2
        d.dcap['shield'] = s
        dragon.bubber.dragons.append(d)
        return d

class Monsterer(RandomBonus):
    "Monsterificator. Let's play on the other side!"
    nimages = [Bonuses.red_crux, Bonuses.blue_crux]
    Sizes = [(Bonuses.red_crux, 800), (Bonuses.blue_crux, 850)]
    mlist = [['Nasty',  'Monky',  'Springy', 'Orcy'],
             ['Ghosty', 'Flappy', 'Gramy',   'Blitzy']
             ]
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Ta, ta ta, ta, taaaaaa..."
    def __init__(self, x, y):
        self.mode = random.choice([0,1])
        RandomBonus.__init__(self, x, y, *self.Sizes[self.mode])
    def taken(self, dragon):
        mcls = random.choice(self.mlist[self.mode])
        dragon.become_monster(mcls, self.big)

Monsterer1 = Monsterer # increase probability

class Bubblizer(RandomBonus):
    "Bubblizer."
    points = 750
    nimage = Bonuses.gold_crux
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Special powers for your bubble."
    def taken(self, dragon):
        args = (dragon.bubber.pn,)
        if self.big:
            from bubbles import SnookerBubble, BigLightBubble
            bcls = random.choice([SnookerBubble, BigLightBubble])
            if bcls is SnookerBubble:
                args = (dragon, dragon.x, dragon.y, 1000000)
        else:
            from bubbles import FireBubble, WaterBubble, LightningBubble
            bcls = random.choice([FireBubble, WaterBubble, LightningBubble])
        b = bcls(*args)
        b.move(dragon.x, dragon.y)
        if not dragon.become_bubblingeyes(b):
            b.kill()

class Carrot(RandomBonus):
    "Angry Monster. Turns all free monsters angry."
    nimage = Bonuses.carrot
    points = 950
    ghost = 0
    bigbonus = {'ghost': 1}
    bigdoc = "What do angry monsters turn into if you don't hurry up?"
    def taken1(self, dragons):
        from monsters import Monster
        lst = [s for s in images.ActiveSprites
               if isinstance(s, Monster) and s.regular()]
        if lst:
            if self.ghost:
                images.Snd.Hell.play()
                for s in lst:
                    s.become_ghost()
            else:
                for s in lst:
                    s.angry = [s.genangry()]
                    s.resetimages()

class Egg(RandomBonus):
    "Teleporter. Exchange yourself with somebody else."
    nimage = Bonuses.egg
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Exchange colors too."
    def taken1(self, dragons):
        if self.big:
            self.exchange_bubbers()
        else:
            self.exchange_dragons(dragons)

    def exchange_dragons(self, dragons):
        dragons = [d for d in dragons if d in d.bubber.dragons]
        alldragons = [d for d in BubPlayer.DragonList if d in d.bubber.dragons]
        others = [d for d in alldragons if d not in dragons]
        xchg = {}
        random.shuffle(dragons)
        random.shuffle(others)
        while dragons and others:
            d1 = dragons.pop()
            d2 = others.pop()
            xchg[d1] = d2.bubber
            xchg[d2] = d1.bubber
        if len(dragons) > 1:
            copy = dragons[:]
            for i in range(10):
                random.shuffle(copy)
                for j in range(len(dragons)):
                    if dragons[j] == copy[j]:
                        break
                else:
                    break
            for d1, d2 in zip(dragons, copy):
                xchg[d1] = d2.bubber
        elif len(dragons) == 1:
            x, y = chooseground(200)
            if x is not None:
                d1 = dragons[0]
                d1.move(x, y)
                d1.dcap['shield'] = 50
        for d1, bubber2 in list(xchg.items()):
            d1.bubber.dragons.remove(d1)
            d1.bubber = bubber2
            bubber2.dragons.append(d1)
            d1.dcap['shield'] = 50

    def exchange_bubbers(self):
        self.exchange_dragons(list(BubPlayer.DragonList))
        players = [p for p in BubPlayer.PlayerList
                   if p.isplaying()]
        if len(players) > 1:
            while 1:
                copy = players[:]
                random.shuffle(copy)
                for j in range(len(players)):
                    if players[j] is copy[j]:
                        break
                else:
                    break
            for b1, b2 in zip(players, copy):
                for d in b1.dragons:
                    d.dcap['bubbericons'] = b2

class Bomb(RandomBonus):
    "Baaoouuuummmm! Explode that wall!"
    nimage = Bonuses.bomb
    bigbonus = {'multiply': 3.8}
    bigdoc = "Makes a BIG hole."
    def taken1(self, dragons):
        bomb_explosion(self.x, self.y, self.multiply)

def bomb_explosion(x0, y0, multiply=1, starmul=2):
    RADIUS = 3.9 * CELL * math.sqrt(multiply)
    Radius2 = RADIUS * RADIUS
    brd = boards.curboard
    cx = x0 + HALFCELL
    cy = y0 + HALFCELL - RADIUS/2
    for y in range(0, brd.height):
        dy1 = abs(y*CELL - cy)
        dy2 = abs((y-(brd.height-1))*CELL - cy)
        dy3 = abs((y+(brd.height-1))*CELL - cy)
        dy = min(dy1, dy2, dy3)
        for x in range(2, brd.width-2):
            dx = x*CELL - cx
            if dx*dx + dy*dy < Radius2:
                try:
                    brd.killwall(x,y)
                except KeyError:
                    pass
    brd.reorder_walls()
    starexplosion(x0, y0, starmul)
    gen = boards.extra_display_repulse(x0+CELL, y0+CELL,
                                       15000 * multiply,
                                       1000 * multiply)
    boards.extra_boardgen(gen)

class Ham(RandomBonus):
    "Protein. Let's build something!"
    nimage = Bonuses.ham
    bigbonus = {'multiply': 3.4}
    bigdoc = "Builds something BIG."
    def taken1(self, dragons):
        RADIUS = 3.9 * CELL * math.sqrt(self.multiply)
        Radius2 = RADIUS * RADIUS
        brd = boards.curboard
        cx = self.x + HALFCELL
        cy = self.y + HALFCELL - RADIUS/2
        xylist = []
        for y in range(0, brd.height):
            dy1 = abs(y*CELL - cy)
            dy2 = abs((y-(brd.height-1))*CELL - cy)
            dy3 = abs((y+(brd.height-1))*CELL - cy)
            dy = min(dy1, dy2, dy3)
            for x in range(2, brd.width-2):
                dx = x*CELL - cx
                if dx*dx + dy*dy < Radius2:
                    if (y,x) not in brd.walls_by_pos and random.random() < 0.5:
                        brd.putwall(x,y)
                        xylist.append((x, y))
        brd.reorder_walls()
        boards.extra_boardgen(boards.single_blocks_falling(xylist))
        gen = boards.extra_display_repulse(self.x+CELL, self.y+CELL,
                                           5000 * self.multiply,
                                           1000 * self.multiply)
        boards.extra_boardgen(gen)

class Chestnut(RandomBonus):
    "Relativity. Speed up or slow down the game."
    nimage = Bonuses.chestnut
    sound = None
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Relative relativity - not the same one for players and monsters."
    def taken1(self, dragons):
        timeout = 500
        if not self.big:
            ft = random.choice([0.5, 2.0])
            boards.set_frametime(ft)
            if ft == 2.0:
                timeout = 430
        else:
            if random.randrange(0, 2) == 1:
                # super-fast game
                boards.set_frametime(0.25)
                timeout = 1800
            else:
                # board unchanged, players slower
                boards.set_frametime(1.0, privtime=250)
                timeout = 800
        BubPlayer.MultiplyerReset = BubPlayer.FrameCounter + timeout
        self.play(images.Snd.Fruit)


try:
    import statesaver
except ImportError:
    print("'statesaver' module not compiled, no clock bonus")
    Clock = None
else:
    import new
    try:
        from statesaver import standard_build      # PyPy
    except ImportError:
        def standard_build(self):
            return new.instance(self.__class__)
    boards.Copyable.inst_build = standard_build
    gamesrv.Sprite.inst_build = standard_build

    def copygamestate():
        # makes a copy of the game state.
        ps = []
        for p1 in BubPlayer.PlayerList:
            #if p1.isplaying():
            d = p1.__dict__.copy()
            for key in BubPlayer.TRANSIENT_DATA:
                if key in d:
                    del d[key]
            ps.append(d)
            #else:
            #    ps.append(None)
        topstate = (
            [g for g in boards.BoardGen if not g.gi_running],
            boards.curboard,
            images.ActiveSprites,
            images.SpritesByLoc,
            list(BubPlayer.__dict__.items()),
            gamesrv.sprites,
            gamesrv.sprites_by_n,
            ps,
            list(images.Snd.__dict__.items()),
            )
        #import pdb; pdb.set_trace()
        return statesaver.copy(topstate)

    def restoregamestate(savedstate):
        (boards.BoardGen,
         boards.curboard,
         images.ActiveSprites,
         images.SpritesByLoc,
         BubPlayerdictitems,
         gamesrv.sprites,
         gamesrv.sprites_by_n,
         ps,
         imagesSnddictitems,
         ) = savedstate
        for key, value in BubPlayerdictitems:
            try:
                setattr(BubPlayer, key, value)
            except (AttributeError, TypeError):
                pass
        for key, value in imagesSnddictitems:
            try:
                setattr(images.Snd, key, value)
            except (AttributeError, TypeError):
                pass

        for p, d in zip(BubPlayer.PlayerList, ps):
            #if d is None:
            #    p.reset()
            #else:
            p.__dict__.update(d)
            if not p.isplaying():
                p.zarkoff()

    class Clock(RandomBonus):
        "Time Machine. Let's do it again!"
        touchable = 0
        points = 0
        nimage = Bonuses.clock
        ticker = None
        bigdoc = "Let's do the whole level again - with the help of ghosts from your own past."

        def __init__(self, x, y, big=0):
            RandomBonus.__init__(self, -boards.bwidth, 0)
            #print "starting clock"
            self.savedstate = None
            self.savedscreens = []
            if bigclockticker:
                if not big:
                    self.kill()   # confusion between the two levels of saving
                    return
                if x == OFFSCREEN:
                    if bigclockticker.state == 'pre':
                        self.bigbonus = {'ticker': bigclockticker}
                        bigclockticker.state = 'seen'
                else:
                    # when taken, this has the same effect as the big clock
                    self.ticker = bigclockticker
                    self.move(x, y)
                    self.touchable = 1
                    return
            self.gen = [self.delayed_show()]

        def delayed_show(self):
            boards.extra_boardgen(self.state_saver())
            for i in range(10):
                yield None
            if self.savedstate is not None:
                for i in range(55):
                    yield None
                x, y = chooseground(200)
                if x is not None:
                    self.move(x, y)
                    self.touchable = 1
                    self.gen.append(self.timeouter())
                    self.gen.append(self.faller())
                    return
            self.kill()

        def taken1(self, dragons):
            if self.ticker:
                return self.ticker.taken(dragons)
            savedstate = self.savedstate
            self.savedstate = None
            if savedstate is not None:
                boards.replace_boardgen(self.state_restorer(savedstate,
                                                            self.savedscreens,
                                                            self))
                self.untouchable()
                return -1

        def state_saver(self):
            # called from BoardGen
            self.savedstate = copygamestate()
            while self.alive:
                gamesrv.sprites[0] = ''
                data = ''.join(gamesrv.sprites)
                self.savedscreens.append(data)
                yield 0
                yield 0
                self.savedscreens.append(data)
                yield 0
                yield 0
            self.savedscreens = []
        def state_restorer(self, savedstate, savedscreens, blinkme):
            # called from BoardGen
            from player import scoreboard
            status = 0
            for t in range(10):
                if not (t & 1):
                    gamesrv.sprites[0] = ''
                    savedscreens.append(''.join(gamesrv.sprites))
                time = boards.normal_frame()
                for i in range(t):
                    status += 1
                    if status % 3 == 0 and blinkme.alive:
                        if status % 6 == 0:
                            blinkme.step(boards.bwidth, 0)
                        else:
                            blinkme.step(-boards.bwidth, 0)
                    yield time
            yield boards.force_singlegen()
            yield 15.0
            for p1 in BubPlayer.PlayerList:
                del p1.dragons[:]
            delay = 8.5
            gamesrv.clearsprites()
            while savedscreens:
                gamesrv.sprites[:] = ['', savedscreens.pop()]
                if delay > 0.6:
                    delay *= 0.9
                yield delay
            yield 15.0
            restoregamestate(savedstate)
            scoreboard()
            yield 2.5

    class DragonGhost(ActiveSprite):
        def __init__(self, entry):
            ActiveSprite.__init__(self, entry.ico, entry.x, entry.y)

        def setentry(self, entry):
            #ico = images.make_darker(entry.ico, True)
            self.lastx = self.x
            self.lasty = self.y
            self.move(entry.x, entry.y, entry.ico)
            self.entry = entry
            self.bubber = entry.d.bubber
            self.dir = entry.dir
            self.poplist = [self]

        def integrate(self):
            self.play(images.Snd.Shh)
            for j in range(15):
                DustStar(self.x, self.y, 0, -3, clock=j==14)

        def disintegrate(self):
            self.play(images.Snd.Shh)
            dx = self.x - self.lastx
            dy = self.y - self.lasty
            if dx < -4: dx = -4
            if dy < -4: dy = -4
            if dx >  4: dx =  4
            if dy >  4: dy =  4
            for j in range(15):
                DustStar(self.x, self.y, dx, dy, clock=j==14)
            self.kill()

        def bottom_up(self):
            return self.entry.dcap['gravity'] < 0.0

    class SavedDragonEntry(object):
        __slots__ = ['d', 'x', 'y', 'ico', 'flag', 'dir', 'dcap']
        def __init__(self, d, flag, dir, dcap):
            self.d = d
            self.x = d.x
            self.y = d.y
            self.ico = d.ico
            self.flag = flag
            self.dir = dir
            self.dcap = dcap

    class SavedFrameEntry(object):
        __slots__ = ['saved_next', 'tick', 'dragonlist', 'shoots1']
        def __init__(self, tick, dragonlist):
            self.saved_next = None
            self.tick = tick
            self.dragonlist = dragonlist
            self.shoots1 = []

    class BigClockTicker:
        dragonlist = None
        tick = 1000

        def __init__(self):
            global random
            random = random_module.Random()
            localrandom = DustStar.localrandom
            self.state = 'pre'
            self.randombase1 = hash(localrandom.random()) * 914971
            self.randombase2 = hash(localrandom.random()) * 914971
            self.saved_next = None
            self.saved_last = self
            random.seed(self.randombase1)
            random_module.seed(self.randombase2)
            self.latest_entries = {}

        def common_tick(self, entry):
            self.dragonlist = entry.dragonlist
            random.seed(self.randombase1 - entry.tick)
            random_module.seed(self.randombase2 - entry.tick)
            bonus_frame_tick()
            random.seed(self.randombase1 + entry.tick)
            random_module.seed(self.randombase2 + entry.tick)

        def save_frame_tick(self):
            entry = self.save_frame()
            self.common_tick(entry)

        def save_frame(self):
            from player import Dragon
            from bubbles import DragonBubble
            tick = self.saved_last.tick + 1
            dragonlist = []
            new_entries = {}
            for bubber in BubPlayer.PlayerList:
                if bubber.isplaying():
                    for d in bubber.dragons:
                        try:
                            dcap = self.latest_entries[d].dcap
                        except KeyError:
                            dcap = None
                        dir = getattr(d, 'dir', 1)
                        cur_dcap = getattr(d, 'dcap', Dragon.DCAP)
                        if dcap != cur_dcap:
                            dcap = cur_dcap.copy()
                        if isinstance(d, Dragon):
                            if d.monstervisible():
                                flag = 'visible'
                            else:
                                flag = 'hidden'
                        else:
                            flag = 'other'
                        entry = SavedDragonEntry(d, flag, dir, dcap)
                        new_entries[d] = entry
                        dragonlist.append(entry)
            self.latest_entries = new_entries
            entry = SavedFrameEntry(tick, dragonlist)
            self.saved_last.saved_next = entry
            self.saved_last = entry
            return entry

        def taken(self, dragons):
            boards.replace_boardgen(self.jump_to_past())

        def jump_to_past(self):
            self.state = 'restoring'
            boards.replace_boardgen(boards.next_board(fastreenter=True), 1)

        def restore(self):
            self.ghosts = {}
            random.seed(self.randombase1)
            random_module.seed(self.randombase2)

        def show_ghosts(self, dragonlist, interact):
            new_ghosts = {}
            for entry in dragonlist:
                try:
                    ghost = self.ghosts[entry.d]
                except KeyError:
                    ghost = DragonGhost(entry)
                ghost.setentry(entry)
                new_ghosts[entry.d] = ghost
                if (interact and entry.flag != 'other' and
                    not entry.dcap.get('infinite_shield')):
                    touching = images.touching(entry.x+1, entry.y+1, 30, 30)
                    touching.reverse()
                    for s in touching:
                        if isinstance(s, interact):
                            s.touched(ghost)
            for d, ghost in list(self.ghosts.items()):
                if d not in new_ghosts:
                    ghost.kill()
            self.ghosts = new_ghosts

        def restore_frame_tick(self):
            from bubbles import Bubble, DragonBubble
            interact = (Bonus, Parabolic2, Bubble)
            self.save_frame()
            entry = self.saved_next
            self.saved_next = entry.saved_next
            self.common_tick(entry)
            self.show_ghosts(entry.dragonlist, interact)
            for args in entry.shoots1:
                DragonBubble(*args)
            if self.state == 'restoring' and self.ghosts:
                self.state = 'post'
                for ghost in list(self.ghosts.values()):
                    ghost.integrate()

        def flush_ghosts(self):
            if self.latest_entries:
                for ghost in list(self.ghosts.values()):
                    ghost.disintegrate()
                self.latest_entries.clear()
            self.dragonlist = None

bigclockticker = None

class MultiStones(RandomBonus):
    "Gems. Very demanded stones. It will take time to pick it up."
    nimages = [Bonuses.emerald, Bonuses.sapphire, Bonuses.ruby]
    Stones = [(Bonuses.emerald,    1000),
              (Bonuses.sapphire,   2000),
              (Bonuses.ruby,       3000),
              ]
    killgens = 0
    big = 0
    bigdoc = "Stones so big you will jump of joy picking them up."
    def __init__(self, x, y, mode=None):
        mode = mode or random.choice(MultiStones.Stones)
        RandomBonus.__init__(self, x, y, *mode)
        self.bigbonus = {'big': 1, 'outcome_args': (mode,),
                         'megacls': LongDurationCactusbonus}
        self.multi = 10
    def taken1(self, dragons):
        if self.big:
            self.repulse(dragons)
        self.multi -= (len(dragons) or 1)
        if self.multi > 0:
            self.taken_by = []
            self.untouchable()
            self.gen.append(self.touchdelay(5))
            return -1     # don't go away
    def repulse(self, dragons):
        for d in dragons:
            repulse_dragon(d)

def repulse_dragon(d):
    if hasattr(d, 'become_bubblingeyes'):
        from bubbles import Bubble
        ico = images.sprget(GreenAndBlue.normal_bubbles[d.bubber.pn][0])
        b = Bubble(ico, d.x, d.y)
        d.become_bubblingeyes(b)
        b.pop()

class Slippy(TemporaryBonus):
    "Greased Feet. Do you want some ice skating?"
    nimage = Bonuses.orange_thing
    points = 900
    capname = 'slippy'
    captime = 606
    bigbonus = {'multiply': 3}
    bigdoc = "Zip zip zip bouncing off walls!"

class Aubergine(TemporaryBonus):
    "Mirror. The left hand is the one with the thumb on the right, right?"
    nimage = Bonuses.aubergine
    big = 0
    bigbonus = {'big': 1, 'multiply': 2}
    bigdoc = "Super Bonus-catching teleport ability."
    def taken(self, dragon):
        if self.big:
            dragon.dcap['teleport'] = dragon.bubber.pcap['teleport'] = 1
        else:
            dragon.dcap['lookforward'] = -dragon.dcap['lookforward']
        self.carried(dragon)
    def endaction(self, dragon):
        if self.big:
            pass
        else:
            dragon.dcap['lookforward'] = -dragon.dcap['lookforward']

class WhiteCarrot(TemporaryBonus):
    "Fly. Become a great flying dragon!"
    nimage = Bonuses.white_carrot
    points = 650
    capname = 'fly'
    captime = 650
    bigbonus = {'capname': 'jumpdown',
                'captime': 999999}
    bigdoc = "Jump down off the ground!"
    def taken(self, dragon):
        TemporaryBonus.taken(self, dragon)
        dragon.bubber.pcap['jumpdown'] = dragon.dcap['jumpdown']

class AmphetamineSpeed(TemporaryBonus):
    "Amphetamine Dose. Increase of your general speed!"
    nimage = Bonuses.tin
    points = 700
    bigbonus = {'multiply': 3}
    bigdoc = "Let's move!"
    def taken(self, dragon):
        dragon.angry = dragon.angry + [dragon.genangry()]
        dragon.carrybonus(self, 633)
    def endaction(self, dragon):
        dragon.angry = dragon.angry[1:]

class Sugar1(Bonus):
    nimage = Bonuses.yellow_sugar
    timeout = 2600
    points = 250
    def taken(self, dragon):
        #if boards.curboard.wastingplay is None:
            dragon.carrybonus(self, 99999)
        #else:
        #    from player import scoreboard
        #    dragon.bubber.bonbons += 1
        #    scoreboard()

class Sugar2(Sugar1):
    timeout = 2500
    points = 500
    nimage = Bonuses.blue_sugar

class Pear(RandomBonus):
    "Pear. Will explode into sugars for your pockets but watch out or you'll lose them!"
    points = 1000
    nimage = Bonuses.green_thing
    bigbonus = {'multiply': 4}
    bigdoc = "The more the better."
    def taken1(self, dragons):
        starexplosion(self.x, self.y, 3 * self.multiply,
                      outcomes = [random.choice([(Sugar1,), (Sugar2,)])
                                  for i in range(18 * self.multiply)])

class Megalightning(ActiveSprite):
    def __init__(self, dragon):
        ActiveSprite.__init__(self, images.sprget(BigImages.blitz),
                              gamesrv.game.width, gamesrv.game.height)
        self.gen.append(self.killing(dragon))

    def killing(self, dragon):
        from monsters import Monster
        from bubbles import Bubble
        poplist = [dragon]
        while 1:
            for s in self.touching(10):
                if isinstance(s, Monster):
                    s.argh(poplist)
                elif isinstance(s, Bubble):
                    s.pop(poplist)
            yield None
            yield None

    def moving_to(self, x1, y1):
        x0 = self.x
        y0 = self.y
        x1 += CELL - self.ico.w//2
        y1 += CELL - self.ico.h//2
        deltax = x1 - x0
        if deltax > -100:
            deltax = -100
        deltay = y1 - y0
        a = - deltay / float(deltax*deltax)
        b = 2 * deltay / float(deltax)
        for x in range(self.x, -self.ico.w, -13):
            x1 = x - x0
            self.move(x, y0 + int((a*x1+b)*x1))
            yield None
        self.kill()

class Fish2(RandomBonus):
    "Rotten Fish. Will blast monsters up to here, so move it around!"
    points = 3000
    nimage = Bonuses.fish2
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Gives seven blasts."
    def taken1(self, dragons):
        dragon = random.choice(dragons or [None])
        if not self.big:
            m = Megalightning(dragon)
            m.gen.append(m.moving_to(self.x, self.y))
        else:
            N = 7
            base = random.random() * 2*math.pi
            angles = [base + (math.pi*2 * n)/N for n in range(N)]
            random.shuffle(angles)
            for angle in angles:
                m = Megalightning(dragon)
                dx = 13 * math.cos(angle)
                dy = 12 * math.sin(angle)
                maxlive = max((gamesrv.game.width + m.ico.w) // 13,
                              (gamesrv.game.height + m.ico.h) // 12)
                m.move(self.x + (self.ico.w - m.ico.w) // 2 - int(dx*maxlive),
                       self.y + (self.ico.h - m.ico.h) // 2 - int(dy*maxlive))
                m.gen.append(m.straightline(dx, dy))
                m.gen.append(m.die([None], maxlive*2))


class Sheep(RandomBonus):
    "Sheep. What a stupid beast!"
    nimage = 'sheep-sm'
    points = 800
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "You're a sheep. Let's bounce around."
    def __init__(self, x, y):
        RandomBonus.__init__(self, x, y)
        if boards.curboard.bonuslevel:
            self.kill()
    def taken1(self, dragons):
        if not self.big:
            self.points0 = {}
            for p in BubPlayer.PlayerList:
                self.points0[p] = p.points
            BubPlayer.LeaveBonus = self.boardleave()
        else:
            from player import Dragon
            BubPlayer.SuperSheep = True
            for p in BubPlayer.PlayerList:
                for d in p.dragons:
                    if isinstance(d, Dragon):
                        d.become_monster('Sheep')

    def boardleave(self):
        from player import BubPlayer
        BubPlayer.OverridePlayerIcon = images.sprget(self.nimage)
        gamesrv.set_musics([], [])
        images.Snd.Yippee.play()
        slist = []
        ico = images.sprget('sheep-big')
        for p in BubPlayer.PlayerList:
            if p.isplaying() and p.dragons:
                d = random.choice(p.dragons)
                dx = (d.ico.w - ico.w) // 2
                dy = (d.ico.h - ico.h) // 2
                s = ActiveSprite(ico, d.x + dx, d.y + dy)
                dir = getattr(d, 'dir', None)
                if dir not in [-1, 1]:
                    dir = random.choice([-1, 1])
                s.gen.append(s.parabolic([dir, -2.0]))
                slist.append(s)
                for d in p.dragons[:]:
                    d.kill()
        delta = {}
        for p in BubPlayer.PlayerList:
            if p.points or p.isplaying():
                delta[p] = 2 * (self.points0[p] - p.points)
        vy = 0
        while delta or slist:
            ndelta = {}
            for p, dp in list(delta.items()):
                if dp:
                    d1 = max(-250, min(250, dp))
                    p.givepoints(d1)
                    if p.points > 0:
                        ndelta[p] = dp - d1
            delta = ndelta
            images.action(slist)
            slist = [s for s in slist if s.y < boards.bheight]
            yield 1

class Flower(RandomBonus):
    "Flower.  Fire in all directions."
    nimage = 'flower'
    points = 800
    big = 0
    bigbonus = {'big': 1, 'multiply': 5}
    bigdoc = "Rotational Bubble Thrower (tm)."
    def taken(self, dragon):
        if self.big:
            dragon.dcap['bigflower'] = -99
            dragon.dcap['autofire'] = 22
        else:
            dragon.dcap['flower'] += 12
        dragon.carrybonus(self)

class Flower2(TemporaryBonus):
    "Bottom-up Flower.  Turn you upside-down."
    nimage = 'flower2'
    points = 1000
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Turn the level upside-down."
    def __init__(self, *args):
        RandomBonus.__init__(self, *args)
        if self.x != OFFSCREEN:
            while not underground(self.x, self.y):
                self.step(0, -CELL)
                if self.y < 0:
                    self.kill()
                    return
    def taken1(self, dragons):
        if self.big:
            boards.extra_boardgen(boards.extra_swap_up_down())
        else:
            RandomBonus.taken1(self, dragons)
    def faller(self):
        while self.y >= 0:
            if underground(self.x, self.y):
                yield None
                yield None
            else:
                self.move(self.x, (self.y-1) & ~3)
            yield None
        self.kill()
    def taken(self, dragon):
        dragon.dcap['gravity'] *= -1.0
        self.carried(dragon)
    def endaction(self, dragon):
        dragon.dcap['gravity'] *= -1.0
    def is_on_ground(self):
        return underground(self.x, self.y)

##class Moebius(RandomBonus):
##    "Moebius Band.  Bottom left is top right and bottom right is top left... or vice-versa."
##    nimage = 'moebius'
##    points = 900
##    def taken1(self, dragons):
##        BubPlayer.Moebius = not BubPlayer.Moebius

class StarBubble(FireBubble):
    "Star Bubbles. Makes you fire bonus bubbles."
    nimage = 'moebius'
    bubkind = 'StarBubble'
    bubcount = 3
    bigbonus = {'bubcount': 10}
    bigdoc = "More bonus bubbles => more confusion."

class Donut(RandomBonus):
    "Donut.  Catch every free monster in a bubble."
    nimage = Bonuses.donut
    points = 950
    big = 0
    bigbonus = {'big': 1}
    bigdoc = "Catch dragons too."

    def taken1(self, dragons):
        extra_boardgen(boards.extra_catch_all_monsters(dragons, self.big))
        if self.big:
            # catch all dragons as well
            from bubbles import NormalBubble
            for dragon in BubPlayer.DragonList[:]:
                b = NormalBubble(dragon, dragon.x, dragon.y, 542)
                if not dragon.become_bubblingeyes(b):
                    b.kill()


Classes = [c for c in list(globals().values())
           if type(c)==type(RandomBonus) and issubclass(c, RandomBonus)]
Classes.remove(RandomBonus)
Classes.remove(TemporaryBonus)
Cheat = []
#Classes = [Cactus, Insect]  # CHEAT

AllOutcomes = ([(c,) for c in Classes if c is not Fruits] +
               2 * [(MonsterBonus, lvl)
                    for lvl in range(len(Bonuses.monster_bonuses))])

for c in Classes:
    assert (getattr(c, 'points', 0) or 100) in GreenAndBlue.points[0], c

def getdragonlist():
    if bigclockticker and bigclockticker.dragonlist is not None:
        return [entry for entry in bigclockticker.dragonlist
                if entry.flag != 'other']
    else:
        return BubPlayer.DragonList

def getvisibledragonlist():
    if bigclockticker and bigclockticker.dragonlist is not None:
        return [entry for entry in bigclockticker.dragonlist
                if entry.flag == 'visible']
    else:
        return [d for d in BubPlayer.DragonList if d.monstervisible()]

def record_shot(args):
    if bigclockticker:
        entry = bigclockticker.saved_last
        if hasattr(entry, 'shoots1'):
            entry.shoots1.append(args)


def chooseground(tries=15):
    avoidlist = getdragonlist()
    for i in range(tries):
        x0 = random.randint(2, boards.width-4)
        y0 = random.randint(1, boards.height-3)
        if (' ' == bget(x0,y0+1) == bget(x0+1,y0+1) and
            '#' == bget(x0,y0+2) == bget(x0+1,y0+2)):
            x0 *= CELL
            y0 *= CELL
            for dragon in avoidlist:
                if abs(dragon.x-x0) < 3*CELL and abs(dragon.y-y0) < 3*CELL:
                    break
            else:
                return x0, y0
    else:
        return None, None

def newbonus():
    others = [s for s in images.ActiveSprites if isinstance(s, RandomBonus)]
    if others:
        return
    if BubPlayer.SuperSheep:
        return
    x, y = chooseground()
    if x is None:
        return
    cls = random.choice(Classes)
    cls(x, y)

##def newbonus():
##    others = [s for s in images.ActiveSprites if isinstance(s, RandomBonus)]
##    if others:
##        return
##    for cls in Classes:
##        x, y = chooseground(200)
##        if x is not None:
##            cls(x, y)

def cheatnew():
    if Cheat:
        x, y = chooseground()
        if x is None:
            return
        cls = random.choice(Cheat)
        if not isinstance(cls, tuple):
            cls = cls,
        else:
            Cheat.remove(cls)
        if len(cls) > 1:
            class C(cls[0]):
                extra_cheat_arg = cls[1]
            cls = (C,)
        cls[0](x, y)

def bonus_frame_tick():
    if random.random() < 0.04:
        cheatnew()
        if random.random() < 0.15:
            newbonus()
        else:
            import bubbles
            bubbles.newbubble()

def start_normal_play():
    global bigclockticker
    if bigclockticker and bigclockticker.state == 'restoring':
        bigclockticker.restore()
        return bigclockticker.restore_frame_tick
    if (Clock and not boards.curboard.bonuslevel and
        random.choice(Classes) is Clock):
        bigclockticker = BigClockTicker()
        return bigclockticker.save_frame_tick
    else:
        bigclockticker = None
        return bonus_frame_tick

def end_normal_play():
    if bigclockticker and bigclockticker.state == 'post':
        bigclockticker.flush_ghosts()

# hack hack hack!
def __cheat(c):
    c = c.split(',')
    c[0] = globals()[c[0]]
    assert issubclass(c[0], Bonus)
    Cheat.append(tuple(c))
import builtins
builtins.__cheat = __cheat
