from __future__ import generators
import os, math, random
import images, gamesrv
from images import ActiveSprite
from boards import CELL, HALFCELL
from mnstrmap import GreenAndBlue
from bubbles import BubblingEyes, Bubble
from bonuses import Bonus

LocalDir = os.path.basename(os.path.dirname(__file__))


localmap = {
    'ark-paddle':  ('image1-%d.ppm', (0, 0, 80, 32)),
    }

music = gamesrv.getmusic(os.path.join(LocalDir, 'music.wav'))
snd_wall  = gamesrv.getsample(os.path.join(LocalDir, 'wall.wav'))
snd_brick = gamesrv.getsample(os.path.join(LocalDir, 'brick.wav'))


def aget(x, y):
    if 0 <= x < curboard.width and y >= 0:
        if y >= curboard.height:
            return ' '
        return curboard.walls[y][x]
    else:
        return '#'

def sign(x):
    if x >= 0.0:
        return 1
    else:
        return -1


class PaddleEyes(BubblingEyes):

    def __init__(self, bubber, saved_caps, paddle):
        BubblingEyes.__init__(self, bubber, saved_caps, paddle)
        self.deltax = (paddle.ico.w - self.ico.w) // 2
        self.deltay = (paddle.ico.h - self.ico.h) // 2
        self.step(self.deltax, self.deltay)

    def playing_bubble(self, paddle, accel=0.75, vmax=4.5):
        import boards
        dx = self.deltax
        dy = self.deltay
        bubber = paddle.bubber
        vx = 0.0
        fx = paddle.x
        while paddle.alive:
            wannago = bubber.wannago(self.dcap)
            if paddle.timeleft is None:
                keydy = 0
            else:
                keydy = -1
            key = ('eyes', wannago, keydy)
            if fx < 2*CELL:
                if vx < 0.0:
                    vx = -vx * 0.45
                wannago = 1
            elif fx + paddle.ico.w > boards.bwidth - 2*CELL:
                if vx > 0.0:
                    vx = -vx * 0.45
                wannago = -1
            if not wannago:
                if -accel <= vx <= accel:
                    vx = 0
                elif vx < 0.0:
                    wannago = 0.7
                else:
                    wannago = -0.7
            vx += accel * wannago
            if vx < -vmax:
                vx = -vmax
            elif vx > vmax:
                vx = vmax
            fx += vx
            paddle.move(int(fx), paddle.y)
            self.move(paddle.x+dx, paddle.y+dy, images.sprget(key))
            yield None
        self.kill()

    def bottom_up(self):
        return 0


class Paddle(ActiveSprite):

    def __init__(self, arkanoid, bubber, px, py):
        ico = images.sprget(('ark-paddle', bubber.pn))
        ActiveSprite.__init__(self, ico, px - (ico.w-2*CELL)//2,
                                         py - (ico.h-2*CELL)//2)
        self.arkanoid = arkanoid
        self.bubber = bubber
        self.timeleft = None
        self.gen.append(self.bounce_down())
        self.gen.append(self.bkgndstuff())
        self.arkanoid.paddles.append(self)

    def bounce_down(self):
        import boards
        target_y = boards.bheight - self.ico.h
        fy = self.y
        vy = 0.0
        while fy < target_y or abs(vy) > 0.3:
            if fy < target_y:
                vy += 0.3
            elif vy > 0.0:
                vy = -vy / 3.0
            fy += vy
            self.move(self.x, int(fy))
            yield None
        while self.y > target_y:
            self.step(0, -2)
            yield None
        self.move(self.x, target_y)
        self.gen.append(self.wait_and_shoot())

    def wait_and_shoot(self):
        timeout = 30
        while timeout > 0:
            timeout -= self.arkanoid.ready
            yield None
        self.gen.append(self.catch(Ball(self)))

    def catch(self, ball):
        import boards
        while ball.alive:
            if ball.y > boards.bheight//2+1 and ball.vy > 0.0:
                deltay = self.y - Ball.Y_MARGIN - ball.y
                self.timeleft = deltay / ball.vy
                #if -1.25 <= self.timeleft <= 0.5:
                if -12 <= deltay <= 1:
                    ball.bouncepad(self.arkanoid.paddles)
            yield None
            self.timeleft = None
        if ball.missed:
            self.kill()

    def kill(self):
        images.Snd.Pop.play(1.0, pad=0.0)
        images.Snd.Pop.play(1.0, pad=1.0)
        ico = images.sprget(Bubble.exploding_bubbles[0])
        for i in range(11):
            s = ActiveSprite(ico,
                             self.x + random.randrange(self.ico.w) - CELL,
                             self.y + random.randrange(self.ico.h) - CELL)
            s.gen.append(s.die(Bubble.exploding_bubbles))
        try:
            self.arkanoid.paddles.remove(self)
        except ValueError:
            pass
        ActiveSprite.kill(self)

    def bkgndstuff(self):
        while 1:
            if self.timeleft is not None:
                self.arkanoid.order.append((self.timeleft, self))
            yield None
            touching = images.touching(self.x+1, self.y+1,
                                       self.ico.w-2, self.ico.h-2)
            touching.reverse()
            for s in touching:
                if isinstance(s, Bonus):
                    s.touched(self)

    def score(self, hits):
        bricks = self.arkanoid.bricks
        bricks[self.bubber] = bricks.get(self.bubber, 0) + hits
        self.bubber.givepoints(125*(2**hits))


class Ball(ActiveSprite):

    Y_MARGIN = 20
    SPEED = 5.8
    
    def __init__(self, paddle):
        self.paddle = paddle
        imglist1 = GreenAndBlue.new_bubbles[paddle.bubber.pn]
        ActiveSprite.__init__(self, images.sprget(imglist1[0]),
                              paddle.x + paddle.ico.w//2,
                              paddle.y - Ball.Y_MARGIN)
        self.missed = 0
        self.setimages(self.imgseq(imglist1[1:], 6))
        self.bounceangle(0.2)
        self.gen.append(self.flying())

    def bouncepad(self, paddles):
        for paddle in paddles:
            dx = (self.x + self.ico.w//2) - (paddle.x + paddle.ico.w//2)
            dxmax = paddle.ico.w//2
            angle = float(dx) / dxmax
            if 0.0 <= angle <= 1.0:
                angle = angle * 1.111 + 0.07
            elif -1.0 <= angle <= 0.0:
                angle = angle * 1.111 - 0.07
            else:
                continue
            self.bounceangle(angle)
            self.play(snd_wall)
            break

    def bounceangle(self, angle):
        self.vx = math.sin(angle) * self.SPEED
        self.vy = - math.cos(angle) * self.SPEED

    def flying(self):
        import boards
        fx = self.x
        fy = self.y
        while self.y < boards.bheight:
            fx += self.vx
            fy += self.vy
            self.move(int(fx), int(fy))
            yield None
            cx = self.x // CELL + 1
            cy = self.y // CELL + 1
            dx = sign(self.vx)
            dy = sign(self.vy)
            hits = 0.0
            if aget(cx, cy) == '#':
                hits += self.ahit(cx, cy, 0, 0)
            if aget(cx+dx, cy) == '#':
                hits += self.ahit(cx+dx, cy, 0, dy)
                self.vx = -self.vx
            if aget(cx, cy+dy) == '#':
                hits += self.ahit(cx, cy+dy, dx, 0)
                self.vy = -self.vy
            if hits:
                hits = int(hits)
                if hits:
                    self.paddle.score(hits)
                    self.play(snd_brick)
                else:
                    self.play(snd_wall)
        self.missed = 1
        self.kill()

    def ahit(self, cx, cy, dx, dy):
        total = 0.01
        for i in (-1, 0, 1):
            x = cx + i*dx
            y = cy + i*dy
            if (2 <= x < curboard.width - 2 and 0 <= y < curboard.height and
                aget(x, y) == '#'):
                curboard.killwall(x, y)
                self.paddle.arkanoid.killedbricks += 1
                total += 1.0
        return total

    def pop(self):
        self.play(images.Snd.Pop)
        self.gen = [self.die(Bubble.exploding_bubbles)]


class Arkanoid:
    
    def bgen(self, limittime = 60.1): # 0:60
        import boards
        from player import BubPlayer

        self.bricks = {}
        for t in boards.initsubgame(music, self.displaypoints):
            yield t

        tc = boards.TimeCounter(limittime)
        self.ready = 0
        self.builddelay = {}
        self.nbbricks = 0
        self.order = []
        self.paddles = []
        #finish = 0
        for t in self.frame():
            self.order_paddles()
            t = boards.normal_frame()
            self.build_paddles()
            yield t
            #if len(self.paddles) == 0:
            #    finish += 1
            #    if finish == 20:
            #        break
            #else:
            #    finish = 0
            tc.update(t)
            if tc.time == 0.0:
                break
            if (BubPlayer.FrameCounter & 15) == 7:
                for s in images.ActiveSprites:
                    if isinstance(s, Bonus):
                        s.timeout = 0   # bonuses stay
                    elif isinstance(s, Bubble):
                        s.pop()

        tc.restore()
        self.ready = 0
        for s in images.ActiveSprites[:]:
            if isinstance(s, Ball):
                s.pop()
        for t in boards.result_ranking(self.bricks, self.nbbricks):
            self.build_paddles()
            yield t
        self.remove_paddles()
        self.unframe()

    def displaypoints(self, bubber):
        return self.bricks.get(bubber, 0)

    def frame(self):
        for y in range(curboard.height-1, curboard.height//2, -1):
            yield None
            yield None
            for x in range(2, curboard.width-2):
                if aget(x, y) == '#':
                    curboard.killwall(x, y)
        brickline = curboard.width-4
        expected = (brickline * curboard.height) // 5
        y = curboard.height//2
        nbbricks = 0
        while y>=0 and nbbricks + (y+1)*brickline >= expected:
            yield None
            for x in range(2, curboard.width-2):
                if aget(x, y) == '#':
                    nbbricks += 1
            y -= 1
        while y >= -1:
            yield None
            yield None
            for x in range(2, curboard.width-2):
                if y < 0 or aget(x, y) == ' ':
                    curboard.putwall(x, y)
            nbbricks += brickline
            curboard.reorder_walls()
            y -= 1

        nbbricks -= brickline
        self.ready = 1
        self.nbbricks = nbbricks
        self.killedbricks = 0
        while self.killedbricks < self.nbbricks:
            yield None

    def unframe(self):
        for x in range(2, curboard.width-2):
            curboard.killwall(x, -1)

    def build_paddles(self):
        from player import BubPlayer
        for p in BubPlayer.PlayerList:
            dragons = [d for d in p.dragons if not isinstance(d, PaddleEyes)]
            if dragons and len(p.dragons) == len(dragons):
                if self.builddelay.get(p):
                    self.builddelay[p] -= 1
                else:
                    self.builddelay[p] = 53
                    dragon = random.choice(dragons)
                    paddle = Paddle(self, p, dragon.x, dragon.y)
                    eyes = PaddleEyes(p, dragon.dcap, paddle)
                    p.dragons.append(eyes)
                    p.emotic(dragon, 4)
            for d in dragons:
                d.kill()

    def order_paddles(self):
        self.order.sort()
        self.order.reverse()
        for timeleft, paddle in self.order:
            try:
                self.paddles.remove(paddle)
            except ValueError:
                pass
            else:
                self.paddles.insert(0, paddle)
                paddle.to_front()
        del self.order[:]

    def remove_paddles(self):
        killclasses = (Paddle, PaddleEyes, Ball, Bonus)
        for s in images.ActiveSprites[:]:
            if isinstance(s, killclasses):
                s.kill()

# This game is suitable for at least min_players players
min_players = 1

def run():
    global curboard
    import boards
    from boards import curboard
    boards.replace_boardgen(Arkanoid().bgen())

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
