
import random
import gamesrv
import images
import boards
from boards import *
from images import ActiveSprite
from mnstrmap import GreenAndBlue, Bonuses, Ghost
from player import BubPlayer
import bonuses


class Monster(ActiveSprite):
    touchable = 1
    special_prob = 0.2
    shootcls = None
    vx = 2
    vy = 0
    vdir = -1
    is_ghost = 0
    MonsterBonus = bonuses.MonsterBonus

    def __init__(self, mnstrdef, x=None, y=None, dir=None, in_list=None):
        self.mdef = mnstrdef
        self.ptag = None
        if dir is None: dir = mnstrdef.dir
        if x is None: x = mnstrdef.x*CELL
        if y is None: y = mnstrdef.y*CELL
        self.dir = dir
        ActiveSprite.__init__(self, images.sprget(self.imgrange()[0]), x, y)
        self.gen.append(self.waiting())
        if in_list is None:
            in_list = BubPlayer.MonsterList
        self.in_list = in_list
        self.in_list.append(self)
        self.no_shoot_before = 0
        #images.ActiveSprites.remove(self)

    def unlist(self):
        try:
            self.in_list.remove(self)
            return 1
        except ValueError:
            return 0

    def kill(self):
        self.unlist()
        ActiveSprite.kill(self)

    def tagdragon(self):
        lst = bonuses.getvisibledragonlist()
        if lst:
            return random.choice(lst)
        else:
            return None

    def imgrange(self):
        if self.is_ghost:
            if self.dir > 0:
                return Ghost.right
            else:
                return Ghost.left
        elif self.angry:
            if self.dir > 0:
                return self.mdef.right_angry
            else:
                return self.mdef.left_angry
        else:
            if self.dir > 0:
                return self.mdef.right
            else:
                return self.mdef.left

    def imgrange1(self):
        # normally this is self.imgrange()[1]
        lst = self.imgrange()
        return lst[len(lst) > 1]

    def resetimages(self, is_ghost=0):
        self.is_ghost = is_ghost
        if self.gen:
            self.setimages(self.cyclic(self.imgrange(), 3))
        else:  # frozen monster
            self.seticon(images.sprget(self.imgrange()[0]))

    def blocked(self):
        if self.dir < 0:
            x0 = (self.x-1)//16
        else:
            x0 = (self.x+33)//16
        y0 = self.y // 16 + 1
        y1 = (self.y + 31) // 16
        return bget(x0,y0) == '#' or bget(x0,y1) == '#'

    def tryhstep(self):
        if self.blocked():
            self.dir = -self.dir
            self.resetimages()
            return 0
        else:
            self.step(self.vx*self.dir, 0)
            return 1

    def vblocked(self):
        if self.vdir < 0:
            y0 = self.y//16
        else:
            y0 = (self.y+1)//16 + 2
        x0 = self.x // 16
        x1 = self.x // 16 + 1
        x2 = (self.x+31) // 16
        return bget(x0,y0) == '#' or bget(x1,y0) == '#' or bget(x2,y0) == '#'

    def tryvstep(self):
        if self.vblocked():
            self.vdir = -self.vdir
            return 0
        else:
            self.step(0, self.vy*self.vdir)
            self.vertical_warp()
            return 1

    def waiting(self, delay=20):
        for i in range(delay):
            yield None
        self.resetimages()
        self.gen.append(self.default_mode())

    def overlapping(self):
        if self.in_list is BubPlayer.MonsterList:
            for s in self.in_list:
                if (-6 <= s.x-self.x <= 6 and -6 <= s.y-self.y < 6 and
                    #s.dir == self.dir and s.vdir == self.vdir and
                    s.vx == self.vx and s.vy == self.vy and
                    (not s.angry) == (not self.angry)):
                    return s is not self
        return 0

    def walking(self):
        while onground(self.x, self.y):
            yield None
            if random.random() < 0.2 and self.overlapping():
                yield None
            x1 = self.x
            if self.dir > 0:
                x1 += self.vx
            if (x1 & 15) < self.vx and random.random() < self.special_prob:
                self.move(x1 & -16, self.y)
                if self.special():
                    return
            self.tryhstep()
        if self.seedragon():
            self.gen.append(self.hjumping())
        else:
            self.gen.append(self.falling())

    def seedragon(self, dragon=None):
        dragon = dragon or self.tagdragon()
        if dragon is None:
            return False
        return abs(dragon.y - self.y) < 16 and self.dir*(dragon.x-self.x) > 0

    def special(self):
        dragon = self.tagdragon()
        if dragon is None:
            return 0
        if self.seedragon(dragon) and self.shoot():
            return 1
        if dragon.y < self.y-CELL:
           #and abs(dragon.x-self.x) < 2*(self.y-dragon.y):
            for testy in range(self.y-2*CELL, self.y-6*CELL, -CELL):
                if onground(self.x, testy):
                    if random.random() < 0.5:
                        ndir = self.dir
                    elif dragon.x < self.x:
                        ndir = -1
                    else:
                        ndir = 1
                    self.gen.append(self.vjumping(testy, ndir))
                    return 1
        return 0

    def shooting(self, pause):
        for i in range(pause):
            yield None
        self.shootcls(self)
        yield None
        self.gen.append(self.default_mode())

    def shoot(self, pause=10):
        if (self.shootcls is None or
            self.no_shoot_before > BubPlayer.FrameCounter):
            return 0
        else:
            self.gen.append(self.shooting(pause))
            self.no_shoot_before = BubPlayer.FrameCounter + 29
            return 1

    def falling(self):
        bubber = getattr(self, 'bubber', None)
        while not onground(self.x, self.y):
            yield None
            ny = self.y + 3
            if (ny & 15) > 14:
                ny = (ny//16+1)*16
            elif (ny & 15) < 3:
                ny = (ny//16)*16
            nx = self.x
            if nx < 32:
                nx += 1 + (self.vx-1) * (bubber is not None)
            elif nx > boards.bwidth - 64:
                nx -= 1 + (self.vx-1) * (bubber is not None)
            elif bubber:
                dx = bubber.wannago(self.dcap)
                if dx and dx != self.dir:
                    self.dir = dx
                    self.resetimages()
                    self.setimages(None)
                if dx and not self.blocked():
                    nx += self.vx*dx
                    self.seticon(images.sprget(self.imgrange1()))
            self.move(nx, ny)
            if self.y >= boards.bheight:
                self.vertical_warp()
        if bubber:
            nextgen = self.playing_monster
        else:
            nextgen = self.walking
        self.gen.append(nextgen())

##    def moebius(self):
##        self.dir = -self.dir
##        self.resetimages()
##        if hasattr(self, 'dcap'):
##            self.dcap['left2right'] *= -1

    def hjumping(self):
        y0 = self.y
        vspeed = -2.2
        ny = y0-1
        while ny <= y0 and not self.blocked():
            self.move(self.x+2*self.dir, int(ny))
            yield None
            vspeed += 0.19
            ny = self.y + vspeed
        self.gen.append(self.default_mode())

    def vjumping(self, limity, ndir):
        self.setimages(None)
        yield None
        self.dir = -self.dir
        self.seticon(images.sprget(self.imgrange()[0]))
        for i in range(9):
            yield None
        self.dir = -self.dir
        self.seticon(images.sprget(self.imgrange()[0]))
        for i in range(4):
            yield None
        self.dir = ndir
        self.seticon(images.sprget(self.imgrange1()))
        for ny in range(self.y-4, limity-4, -4):
            self.move(self.x, ny)
            if ny < -32:
                self.vertical_warp()
            yield None
        self.resetimages()
        self.gen.append(self.default_mode())

    def regular(self):
        return self.still_playing() and self.touchable and not self.is_ghost

    def still_playing(self):
        return (self.in_list is BubPlayer.MonsterList and
                self in self.in_list)

    def touched(self, dragon):
        if self.gen:
            self.killdragon(dragon)
            if self.is_ghost and not hasattr(self, 'bubber'):
                self.gen = [self.default_mode()]
                self.resetimages()
        else:
            self.argh(getattr(self, 'poplist', None))  # frozen monster

    def killdragon(self, dragon):
        dragon.die()

    def in_bubble(self, bubble):
        if not hasattr(self.mdef, 'jailed'):
            return
        self.untouchable()
        self.angry = []
        bubble.move(self.x, self.y)
        if not hasattr(bubble, 'withmonster'):
            bubble.to_front()
        self.to_front()
        img = self.mdef.jailed
        self.gen = [self.bubbling(bubble)]
        self.setimages(self.cyclic([img[1], img[2], img[1], img[0]], 4))

    def bubbling(self, bubble):
        counter = 0
        while not hasattr(bubble, 'poplist'):
            self.move(bubble.x, bubble.y)
            yield None
            counter += 1
            if counter == 50 and hasattr(self, 'bubber'):
                bubble.setimages(bubble.bubble_red())
        if bubble.poplist is None:
            self.touchable = 1
            self.angry = [self.genangry()]
            self.resetimages()
            self.gen.append(self.default_mode())
        else:
            previous_len = len(BubPlayer.MonsterList)
            self.argh(bubble.poplist)
            dragon = bubble.poplist[0]
            if dragon is not None:
                if previous_len and not BubPlayer.MonsterList:
                    points = 990
                else:
                    points = 90
                dragon.bubber.givepoints(points)

    def argh(self, poplist=None, onplace=0):
        if self not in self.in_list:
            return
        if not poplist:
            poplist = [None]
        poplist.append(self)
        level = len(poplist) - 2
        bonuses.BonusMaker(self.x, self.y, self.mdef.dead, onplace=onplace,
                           outcome=(self.MonsterBonus, level))
        self.kill()

    def freeze(self, poplist):
        # don't freeze monsters largely out of screen, or they'll never come in
        if self.regular() and -self.ico.h < self.y < boards.bheight:
            self.gen = []
            self.poplist = poplist

    def flying(self):
        blocked = 0
        while 1:
            if random.random() < 0.2 and self.overlapping():
                yield None
            hstep = self.tryhstep()
            vstep = self.tryvstep()
            if hstep or vstep:
                blocked = 0
            elif blocked:
                # blocked! go up or back to the play area
                if self.x < 32:
                    self.step(self.vy, 0)
                elif self.x > boards.bwidth - 64:
                    self.step(-self.vy, 0)
                else:
                    self.step(0, -self.vy)
                    self.vertical_warp()
            else:
                blocked = 1
            yield None

    def becoming_monster(self, big=0, immed=0):
        if big:
            self.is_ghost = 1
            self.seticon(images.sprget(self.imgrange()[0]))
            images.Snd.Hell.play()
        for i in range(5):
            ico = self.ico
            self.seticon(self.bubber.icons[11 + immed, self.dir])
            yield None
            yield None
            self.seticon(ico)
            yield None
            yield None
        self.resetimages(is_ghost=big)
        self.gen.append(self.playing_monster())

    def become_monster(self, bubber, saved_caps, big=0, immed=0):
        self.timeoutgen = self.back_to_dragon()
        self.default_mode = self.playing_monster
        self.bubber = bubber
        self.dcap = saved_caps
        self.gen = [self.becoming_monster(big, immed)]

    def back_to_dragon(self):
        for t in range(259):
            yield None
            if bonuses.getdragonlist():
                yield None
                yield None
                yield None
        from player import Dragon
        d = Dragon(self.bubber, self.x, self.y, self.dir, self.dcap)
        d.dcap['shield'] = 50
        self.bubber.dragons.append(d)
        self.kill()

    def playing_monster(self):
        if self.timeoutgen not in self.gen:
            self.gen.append(self.timeoutgen)
        bubber = self.bubber
        while self.is_ghost:
            # ghost
            self.angry = []
            key, dx, dy = max([(bubber.key_left, -1, 0),
                               (bubber.key_right, 1, 0),
                               (bubber.key_jump, 0, -1),
                               (bubber.key_fire, 0, 1)])
            if key:
                if dx and self.dir != dx:
                    self.dir = dx
                    self.resetimages(is_ghost=1)
                nx = self.x + 10*dx
                ny = self.y + 9*dy
                if nx < 0: nx = 0
                if nx > boards.bwidth-2*CELL: nx = boards.bwidth-2*CELL
                if ny < -CELL: ny = -CELL
                if ny > boards.bheight-CELL: ny = boards.bheight-CELL
                self.move(nx, ny)
            yield None
        if self.vy:
            # flying monster
            while 1:
                dx = bubber.wannago(self.dcap)
                if dx and dx != self.dir:
                    self.dir = dx
                    self.resetimages()
                if bubber.key_jump and bubber.key_jump > bubber.key_fire:
                    dy = self.vdir = -1
                elif bubber.key_fire:
                    dy = self.vdir = 1
                else:
                    dy = 0
                hstep = dx and self.tryhstep()
                vstep = dy and self.tryvstep()
                if dx and dy and not (hstep or vstep):
                    # blocked?
                    self.dir = -self.dir
                    self.vdir = -self.vdir
                    blocked = self.blocked() and self.vblocked()
                    self.dir = -self.dir
                    self.vdir = -self.vdir
                    if blocked:
                        # completely blocked! accept move or force back to
                        # play area
                        if self.x < 32:
                            self.step(self.vy, 0)
                        elif self.x > boards.bwidth - 64:
                            self.step(-self.vy, 0)
                        else:
                            self.step(self.vx*dx, self.vy*dy)
                            self.vertical_warp()
                yield None
        elif not isinstance(self, Springy):
            # walking monster
            jumping_y = 0
            imgsetter = self.imgsetter
            while onground(self.x, self.y) or jumping_y:
                dx = bubber.wannago(self.dcap)
                if dx and dx != self.dir:
                    self.dir = dx
                    self.resetimages()
                    imgsetter = self.imgsetter
                if dx and not self.blocked():
                    self.step(self.vx*dx, 0)
                    if not jumping_y:
                        self.setimages(imgsetter)
                    else:
                        self.seticon(images.sprget(self.imgrange1()))
                        self.setimages(None)
                else:
                    self.setimages(None)
                    dx = 0
                yield None
                if not jumping_y:
                    wannafire = bubber.key_fire
                    wannajump = bubber.key_jump
                    if wannafire and self.shoot(1):
                        return
                    if wannajump:
                        jumping_y = CELL
                if jumping_y:
                    self.step(0, -4)
                    if self.y < -32:
                        self.vertical_warp()
                    if onground(self.x, self.y):
                        jumping_y = 0
                    else:
                        jumping_y -= 1
            self.gen.append(self.falling())
        else:
            # springy
            if not onground(self.x, self.y):
                self.gen.append(self.falling())
                return
            prevx = self.x
            for t in self.walking():
                dx = bubber.wannago(self.dcap)
                if dx:
                    if dx != self.dir:
                        self.dir = dx
                        self.resetimages()
                    if self.blocked() and (self.x-prevx)*dx <= 0:
                        dx = 0
                    self.move(prevx + self.vx*dx, self.y)
                yield None
                prevx = self.x

    def become_ghost(self):
        self.gen = [self.ghosting()]
        self.resetimages(is_ghost=1)

    def ghosting(self):
        counter = 0
        while counter < 5:
            for i in range(50):
                yield None
            dragon = self.tagdragon()
            if dragon is None:
                counter += 1
            else:
                counter = 0
                px, py = dragon.x, dragon.y
                if abs(px-self.x) < abs(py-self.y):
                    dx = 0
                    if py > self.y:
                        dy = 1
                    else:
                        dy = -1
                else:
                    dy = 0
                    if px > self.x:
                        dx = 1
                    else:
                        dx = -1
                    self.dir = dx
                    self.resetimages(is_ghost=1)
                dx *= 10
                dy *= 9
                distance = 1E10
                while 1:
                    self.angry = []
                    self.step(dx, dy)
                    yield None
                    dist1 = (px-self.x)*(px-self.x)+(py-self.y)*(py-self.y)
                    if dist1 > distance:
                        break
                    distance = dist1
        self.angry = []
        self.gen = [self.default_mode()]
        self.resetimages()

    default_mode = falling


def argh_em_all():
    poplist = [None]
    for s in images.ActiveSprites[:]:
        if isinstance(s, Monster):
            s.argh(poplist)

def freeze_em_all():
    poplist = [None]
    for s in images.ActiveSprites:
        if isinstance(s, Monster):
            s.freeze(poplist)


class MonsterShot(ActiveSprite):
    speed = 6
    touchable = 1
    
    def __init__(self, owner, dx=CELL, dy=0):
        self.owner = owner
        self.speed = owner.dir * self.speed
        if owner.dir < 0:
            nimages = owner.mdef.left_weapon
        else:
            nimages = owner.mdef.right_weapon
        ActiveSprite.__init__(self, images.sprget(nimages[0]),
                              owner.x, owner.y + dy)
        self.step((owner.ico.w - self.ico.w) // 2,
                  (owner.ico.h - self.ico.h) // 2)
        if not self.blocked():
            self.step(dx*owner.dir, 0)
        if len(nimages) > 1:
            self.setimages(self.cyclic(nimages, 3))
        self.gen.append(self.moving())

    def blocked(self):
        if self.speed < 0:
            x0 = (self.x-self.speed-8)//16
        else:
            x0 = (self.x+self.ico.w+self.speed-8)//16
        y0 = (self.y+8) // 16 + 1
        return not (' ' == bget(x0,y0) == bget(x0+1,y0))

    def moving(self):
        while not self.blocked():
            yield None
            self.step(self.speed, 0)
        self.hitwall()

    def hitwall(self):
        self.untouchable()
        self.gen.append(self.die(self.owner.mdef.decay_weapon, 2))

    def touched(self, dragon):
        dragon.die()


class BoomerangShot(MonsterShot):
    speed = 8
    
    def hitwall(self):
        self.gen.append(self.moveback())

    def moveback(self):
        owner = self.owner
        if self.speed > 0:
            nimages = owner.mdef.left_weapon
        else:
            nimages = owner.mdef.right_weapon
        self.setimages(self.cyclic(nimages, 3))
        while (owner.x-self.x) * self.speed < 0:
            yield None
            self.step(-self.speed, 0)
            if self.blocked():
                break
        self.kill()

class FastShot(MonsterShot):
    speed = 15


class DownShot(MonsterShot):

    def __init__(self, owner):
        MonsterShot.__init__(self, owner, 0, CELL)

    def moving(self):
        while self.y < boards.bheight:
            yield None
            self.step(0, 7)
        self.kill()


##class DragonShot(MonsterShot):
##    speed = 8

##    def __init__(self, owner):
##        MonsterShot.__init__(self, owner)
##        self.untouchable()
##        self.gen.append(self.touchdelay(4))

##    def touched(self, dragon):
##        if dragon is not self.owner:
##            if dragon.bubber.bonbons == 0:
##                dragon.die()
##            else:
##                from player import scoreboard
##                from bonuses import Sugar1, Sugar2
##                from bonuses import BonusMaker
##                if random.random() < 0.2345:
##                    start = 1
##                else:
##                    start = 0
##                loose = min(2, dragon.bubber.bonbons)
##                for i in range(start, loose):
##                    cls = random.choice([Sugar1, Sugar2])
##                    BonusMaker(self.x, self.y, [cls.nimage],
##                               outcome=(cls,))
##                dragon.bubber.bonbons -= loose
##                scoreboard()
##                dragon.dcap['shield'] = 25
##                self.owner.play(images.Snd.Yippee)
##                self.kill()

##    def blocked(self):
##        return self.x < -self.ico.w or self.x >= gamesrv.game.width
##        #return self.x < CELL or self.x >= boards.bwidth - 3*CELL


class Nasty(Monster):
    pass

class Monky(Monster):
    shootcls = MonsterShot

class Ghosty(Monster):
    default_mode = Monster.flying
    vy = 2

class Flappy(Monster):
    default_mode = Monster.flying
    vy = 1

class Springy(Monster):
    spring_down = 0
    def imgrange(self):
        if self.spring_down and not self.is_ghost:
            if self.angry:
                if self.dir > 0:
                    r = self.mdef.right_jump_angry
                else:
                    r = self.mdef.left_jump_angry
            else:
                if self.dir > 0:
                    r = self.mdef.right_jump
                else:
                    r = self.mdef.left_jump
            return [r[self.spring_down-1]]
        else:
            return Monster.imgrange(self)
    def walking(self):
        self.spring_down = 1
        self.resetimages()
        for t in range(2+self.overlapping()):
            yield None
        self.spring_down = 2
        self.resetimages()
        for t in range(4+2*self.overlapping()):
            yield None
        self.spring_down = 1
        self.resetimages()
        for t in range(2+self.overlapping()):
            yield None
        self.spring_down = 0
        self.resetimages()
        g = 10.0/43
        vy = -20*g
        yf = self.y
        for t in range(40):
            yf += vy
            vy += g
            if self.blocked():
                self.dir = -self.dir
                self.resetimages()
            nx = self.x + self.dir*self.vx
            if self.y//16 < int(yf)//16:
                if onground(self.x, (self.y//16+1)*16):
                    break
                if onground(nx, (self.y//16+1)*16):
                    self.move(nx, self.y)
                    break
            nx, yf = vertical_warp(nx, yf)
            self.move(nx, int(yf))
##            if moebius:
##                self.moebius()
            yield None
        self.gen.append(self.falling())

class Orcy(Monster):
    shootcls = FastShot

class Gramy(Monster):
    shootcls = BoomerangShot
    vx = 3

class Blitzy(Monster):
    shootcls = DownShot
    vx = 3

    def seedragon(self, dragon=None):
        return 0

    def special(self):
        if random.random() < 0.3:
            self.shootcls(self)
        return 0

    def shoot(self, pause=0):
        # no pause (only used when controlled by the player)
        if self.no_shoot_before > BubPlayer.FrameCounter:
            pass
        else:
            self.shootcls(self)
            self.no_shoot_before = BubPlayer.FrameCounter + 29
        return 0

MonsterClasses = [c for c in list(globals().values())
                  if type(c)==type(Monster) and issubclass(c, Monster)]
MonsterClasses.remove(Monster)


class Butterfly(Monster):
    MonsterBonus = bonuses.IceMonsterBonus
    fly_away = False

    def waiting(self, delay=0):
        return Monster.waiting(self, delay)

    def imgrange(self):
        self.angry = []
        return Monster.imgrange(self)

    def killdragon(self, dragon):
        if self.is_ghost:
            Monster.killdragon(self, dragon)
        else:
            self.fly_away = True, dragon.x

    def flying(self):
        repeat = 0
        while 1:
            r = random.random()
            if self.x < 64:
                bump = self.dir < 0
            elif self.x > boards.bwidth - 64:
                bump = self.dir > 0
            elif self.fly_away:
                wannago = self.x - self.fly_away[1]
                if self.x < 100:
                    wannago = 1
                elif self.x > boards.bwidth - 100:
                    wannago = -1
                bump = self.dir * wannago < 0
                if repeat:
                    self.fly_away = False
                    repeat = 0
                else:
                    repeat = 1
            else:
                bump = r < 0.07
            if bump:
                self.dir = -self.dir
                self.resetimages()
            elif r > 0.92:
                self.vdir = -self.vdir
            self.step(self.dir * (2 + (r < 0.5)), self.vdir * 2)
            self.vertical_warp()
            if not repeat:
                yield None

    default_mode = flying


class Sheep(Monster):

    def playing_monster(self):
        from bonuses import Bonus
        bubber = self.bubber
        vy = None
        imgsetter = self.imgsetter
        poplist = [None]
        while 1:
            dx = bubber.wannago(self.dcap)
            if dx and dx != self.dir:
                self.dir = dx
                self.resetimages()
                imgsetter = self.imgsetter
            if dx and vy is None:
                self.setimages(imgsetter)
            else:
                self.setimages(None)
                if vy is not None:
                    if vy < 0:
                        n = 1
                    else:
                        n = 3
                    self.seticon(images.sprget(self.imgrange()[n]))
            if dx and not self.blocked():
                self.step(self.vx*dx, 0)
            yield None
            impulse = 0.0
            wannajump = bubber.key_jump
            if vy is not None:
                vy += 0.33
                if vy > 12.0:
                    vy = 12.0
                yf = self.y + yfp + vy
                yfp = yf - int(yf)
                delta = int(yf) - self.y
                if delta > 0:
                    by_y = {}
                    for s in images.ActiveSprites:
                        if isinstance(s, Bonus) and s.touchable:
                            if abs(s.x - self.x) <= 22:
                                by_y[s.y] = s
                    for monster in BubPlayer.MonsterList:
                        if abs(monster.x - self.x) <= 22:
                            if monster.regular():
                                by_y[monster.y] = monster
                    for ny in range(self.y - 1, self.y + delta + 1):
                        self.move(self.x, ny)
                        self.vertical_warp()
                        if onground(self.x, self.y):
                            poplist = [None]
                            impulse = vy
                            vy = None
                            break
                        key = self.y + 29
                        if key in by_y:
                            s = by_y[key]
                            if isinstance(s, Monster):
                                self.play(images.Snd.Extra)
                                s.argh(poplist)
                            elif isinstance(s, Bonus):
                                s.reallytouched(self)
                            yfp = 0.0
                            vy = -3.3
                            break
                else:
                    self.step(0, delta)
                    self.vertical_warp()
            if vy is None:
                if onground(self.x, self.y):
                    if wannajump:
                        yfp = 0.0
                        vy = - max(1.0, impulse) - 2.02
                        impulse = 0.0
                        self.play(images.Snd.Jump)
                else:
                    yfp = vy = 0.0
            if impulse > 8.1:
                break
        self.play(images.Snd.Pop)
        for n in range(2):
            for letter in 'abcdefg':
                ico = images.sprget(('sheep', letter))
                nx = self.x + random.randrange(-1, self.ico.w - ico.w + 2)
                ny = self.y + random.randrange(0, self.ico.h - ico.h + 2)
                dxy = [random.random() * 5.3 - 2.65, random.random() * 4 - 4.4]
                s = images.ActiveSprite(ico, nx, ny)
                s.gen.append(s.parabolic(dxy))
                s.gen.append(s.die([None], random.randrange(35, 54)))
        self.move(-99, 0)
        for t in range(68):
            yield None
        self.kill()

    default_mode = falling = playing_monster

    def argh(self, *args, **kwds):
        pass
