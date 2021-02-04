from __future__ import generators
import os, math, random
import images, gamesrv
from images import ActiveSprite
import boards
from boards import CELL, HALFCELL, bget
from mnstrmap import GreenAndBlue, Fire
from bonuses import Bonus
from player import Dragon, BubPlayer
import monsters
from bubbles import Bubble

LocalDir = os.path.basename(os.path.dirname(__file__))

localmap = {
    'gala': ('image1-%d.ppm', (0, 0, 32, 32)),
    }

music = gamesrv.getmusic(os.path.join(LocalDir, 'music.wav'))
snd_shoot = gamesrv.getsample(os.path.join(LocalDir, 'shoot.wav'))


class Ship(ActiveSprite):

    def __init__(self, galaga, bubber, x, y):
        ico = images.sprget(('gala', bubber.pn))
        ActiveSprite.__init__(self, ico, x, y)
        self.galaga = galaga
        self.bubber = bubber
        self.gen.append(self.movedown())
        self.gen.append(self.playing_ship())
        self.gen.append(self.doomed())
        self.galaga.ships.append(self)

    def movedown(self):
        import boards
        target_y = boards.bheight - self.ico.h
        while self.y < target_y:
            yield None
            self.move(self.x, self.y + 3)
        self.move(self.x, target_y)

    def playing_ship(self):
        import boards
        bubber = self.bubber
        xmin = HALFCELL
        xmax = boards.bwidth - HALFCELL - self.ico.w
        fire = 0
        while 1:
            wannago = bubber.wannago(self.dcap)
            nx = self.x + 2*wannago
            if nx < xmin:
                nx = xmin
            elif nx > xmax:
                nx = xmax
            self.move(nx, self.y)
            if fire:
                fire -= 1
            elif bubber.key_fire:
                self.firenow()
                fire = 28
            yield None

    def firenow(self):
        ico = images.sprget(GreenAndBlue.new_bubbles[self.bubber.pn][0])
        s = Shot(ico, self.x, self.y)
        s.d = self
        s.gen = [s.straightup(self)]
        self.play(snd_shoot)

    def doomed(self):
        dangerous = Alien, monsters.MonsterShot
        while 1:
            touching = images.touching(self.x+3, self.y+3, 26, 26)
            for s in touching:
                if isinstance(s, dangerous):
                    self.kill()
                    return
            yield None
            yield None

    def kill(self):
        try:
            self.bubber.dragons.remove(self)
        except ValueError:
            pass
        images.Snd.Pop.play(1.0, pad=0.0)
        images.Snd.Pop.play(1.0, pad=1.0)
        ico = images.sprget(Bubble.exploding_bubbles[0])
        for i in range(11):
            s = ActiveSprite(ico,
                             self.x + random.randrange(self.ico.w) - CELL,
                             self.y + random.randrange(self.ico.h) - CELL)
            s.gen.append(s.die(Bubble.exploding_bubbles))
        try:
            self.galaga.ships.remove(self)
        except ValueError:
            pass
        ActiveSprite.kill(self)


class Shot(Bubble):
    touchable = 0
    
    def straightup(self, ship):
        ymin = -self.ico.h
        while self.y > ymin:
            self.step(0, -10)
            touching = images.touching(self.x+CELL-1, self.y+CELL-1, 2, 2)
            touching = [s for s in touching if isinstance(s, Alien)]
            if touching:
                alien = random.choice(touching)
                self.gen = []
                self.touchable = 1
                self.move(alien.x, alien.y)
                self.pop([ship])
                alien.kill()
                scores = ship.galaga.scores
                scores[ship.bubber] = scores.get(ship.bubber, 0) + 1
                ship.bubber.givepoints(100)
                return
            yield None

    def popped(self, dragon):
        return 200


class Alien(monsters.Monster):
    ANGLES = 32
    SPEED = 5
    ANGLE_TABLE = [(SPEED * math.cos(a*2.0*math.pi/ANGLES),
                    -SPEED * math.sin(a*2.0*math.pi/ANGLES))
                   for a in range(ANGLES)]
    touchable = 0
    
    def __init__(self, galaga, squadron, rank, relativey):
        centerx = boards.bwidth // 2
        go_left = squadron % 2
        dx = (1,-1)[go_left]
        halfspan = centerx*7//12
        relativex = - halfspan + 4*CELL*rank
        if relativex > halfspan:
            raise StopIteration
        
        if squadron % 3 == 2:
            from mnstrmap import Ghosty as mcls
        else:
            from mnstrmap import Flappy as mcls
        mdef = mcls(centerx // CELL - 1, -7, go_left)
        mdef.left_weapon = mdef.right_weapon = [Fire.drop]
        monsters.Monster.__init__(self, mdef)

        self.path = [(None, centerx + (dx*centerx)*2//3, boards.bheight//3),
                     (None, centerx - (dx*centerx)*4//5, boards.bheight//6),
                     (galaga, -dx*relativex, -relativey)]
        self.gen = [self.waiting(rank * 20)]
        self.in_place = 0
        galaga.nbmonsters += 1

    def default_mode(self, angle=ANGLES//4):
        self.touchable = 1
        speed = self.SPEED
        relative, tx, ty = self.path[0]
        fx = self.x
        fy = self.y
        ymax = boards.bheight - self.ico.h
        cont = 1
        if relative:
            shoot_prob = 0.0085
        else:
            shoot_prob = 0.021
        while cont:
            if self.angry:
                self.kill()   # never getting out of a bubble
                return
            if relative:
                dx = relative.globalx + tx
                dy = relative.globaly + ty
            else:
                dx = tx
                dy = ty
            dx -= self.x
            dy -= self.y
            
            tests = []
            for a1 in (-1, 0, 1):
                a1 = (angle+a1) % self.ANGLES
                testx, testy = self.ANGLE_TABLE[a1]
                testx -= dx
                testy -= dy
                tests.append((testx*testx+testy*testy, a1))
            ignored, angle = min(tests)
            if dx*dx+dy*dy > speed*speed:
                dx, dy = self.ANGLE_TABLE[angle]
            elif relative:
                self.in_place = 1
                if self.y > ymax and relative.ships:
                    for ship in relative.ships[:]:
                        ship.kill()
                        relative.builddelay[ship.bubber] = 9999
                    relative.gameover = 1
                    #x0 = self.x//CELL + 1
                    #if x0 < 2: x0 = 0
                    #if x0 >= boards.width-2: x0 = boards.width-3
                    #bubbles.FireFlame(x0, boards.height-2, None, [-1, 1],
                    #                  boards.width)
            else:
                self.path.pop(0)
                self.gen.append(self.default_mode(angle))
                cont = 0
            fx += dx
            fy += dy
            self.move(int(fx), int(fy))
            if dx and (self.dir > 0) != (dx > 0):
                self.dir = -self.dir
                self.resetimages()
            if random.random() < shoot_prob and self.y >= 0:
                monsters.DownShot(self)
            yield None


class Galaga:
    gameover = 0
    
    def bgen(self):
        self.scores = {}
        for t in boards.initsubgame(music, self.displaypoints):
            yield t

        self.ships = []
        self.builddelay = {}
        self.nbmonsters = 0
        #finish = 0
        for t in self.frame():
            t = boards.normal_frame()
            self.build_ships()
            yield t
            #if len(self.ships) == 0:
            #    finish += 1
            #    if finish == 50:
            #        break
            #else:
            #    finish = 0
            if (BubPlayer.FrameCounter & 15) == 7:
                for s in images.ActiveSprites:
                    if isinstance(s, Bubble) and not isinstance(s, Shot):
                        s.pop()

        for t in boards.result_ranking(self.scores, self.nbmonsters):
            self.build_ships()
            yield t
        for s in images.ActiveSprites[:]:
            if isinstance(s, (Alien, Ship)):
                s.kill()

    def displaypoints(self, bubber):
        return self.scores.get(bubber, 0)

    def frame(self):
        curboard.walls_by_pos.clear()
        curboard.winds = ['v' * curboard.width] * curboard.height
        for y in range(len(curboard.walls)):
            curboard.walls[y] = ' ' * len(curboard.walls[y])
        l1 = curboard.sprites['walls']
        l2 = curboard.sprites['borderwalls']
        while l1 or l2:
            for l in [l1, l2]:
                for w in l[:]:
                    w.step(0, 5)
                    if w.y >= boards.bheight:
                        l.remove(w)
                        w.kill()
            yield None

        self.globalx = boards.bwidth // 2
        self.globaly = 0
        shifter = self.shifter()
        squadrons = len([p for p in BubPlayer.PlayerList if p.isplaying()])
        squadrons = 3 + (squadrons+1)//3
        nextsquad = 0
        relativey = 0
        squadtime = 0
        while not self.gameover:
            yield None
            #if random.random() < 0.015:
            #    bubbles.sendbubble(bubbles.PlainBubble, top=0)
            in_place = {0: [], 1: [], 2: []}
            for s in BubPlayer.MonsterList:
                if isinstance(s, Alien):
                    in_place[s.in_place].append(s)
            toohigh = self.globaly - relativey < -3*CELL
            if in_place[1]:
                xbounds = [s.x for s in in_place[1]]
                self.alien_bounds = min(xbounds), max(xbounds)
                shifter.next()
            elif toohigh:
                self.globaly += 1
            squadtime -= 1
            if nextsquad >= squadrons:
                if not (in_place[0] or in_place[1]):
                    break
            elif squadtime < 0 and not toohigh:
                squadtime = 200
                try:
                    rank = 0
                    while 1:
                        Alien(self, nextsquad, rank, relativey)
                        rank += 1
                except StopIteration:
                    pass
                nextsquad += 1
                relativey += 4*CELL
        for t in range(20):
            yield None

    def shifter(self):
        while 1:
            # go right
            while self.alien_bounds[1] < boards.bwidth-5*CELL:
                self.globalx += 2
                yield None
            # go down
            for i in range(3*CELL):
                self.globaly += 1
                yield None
            # go left
            while self.alien_bounds[0] > 3*CELL:
                self.globalx -= 2
                yield None
            # go down
            for i in range(3*CELL):
                self.globaly += 1
                yield None

    def build_ships(self):
        for p in BubPlayer.PlayerList:
            dragons = [d for d in p.dragons if not isinstance(d, Ship)]
            if dragons and len(p.dragons) == len(dragons):
                if self.builddelay.get(p):
                    self.builddelay[p] -= 1
                else:
                    self.builddelay[p] = 75
                    dragon = random.choice(dragons)
                    ship = Ship(self, p, dragon.x, dragon.y)
                    ship.dcap = dragon.dcap
                    p.dragons.append(ship)
                    p.emotic(dragon, 4)
            for d in dragons:
                d.kill()

# This game is suitable for at least min_players players
min_players = 1

def run():
    global curboard
    from boards import curboard
    boards.replace_boardgen(Galaga().bgen())

def setup():
    for key, (filename, rect) in localmap.items():
        filename = os.path.join(LocalDir, filename)
        if filename.find('%d') >= 0:
            for p in BubPlayer.PlayerList:
                images.sprmap[key, p.pn] = (filename % p.pn, rect)
        else:
            images.sprmap[key] = (filename, rect)
setup()
