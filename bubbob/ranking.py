from __future__ import generators
import random
import boards, images, gamesrv
from boards import CELL, HALFCELL
from mnstrmap import DigitsMisc, Flood, GreenAndBlue
from bubbles import Bubble
from bonuses import Points
from player import BubPlayer

MARGIN = 22
VMARGIN = 12

class RPicture:
    def __init__(self):
        self.icons = []
    def put(self, ico, dx=0, dy=0):
        self.icons.append((dx, dy, ico))
    def getsize(self):
        if self.icons:
            return (max([dx+ico.w for dx, dy, ico in self.icons]) + MARGIN,
                    max([dy+ico.h for dx, dy, ico in self.icons]))
        else:
            return 0, 0
    def render(self, x, y):
        return [gamesrv.Sprite(ico, x+dx, y+dy) for dx, dy, ico in self.icons]

class RPoints:
    def __init__(self, bubber, nbpoints):
        self.bubber = bubber
        self.nbpoints = nbpoints
    def getsize(self):
        return 0, 0
    def render(self, x, y):
        Points(x, y, self.bubber.pn, self.nbpoints)
        return []

class RNumber(RPicture):
    map = {'%': 'percent'}
    for digit in range(10):
        map[str(digit)] = DigitsMisc.digits_white[digit]
    
    def __init__(self, text):
        RPicture.__init__(self)
        x = 0
        for c in text:
            ico = images.sprget(self.map[c])
            self.put(ico, dx=x)
            x += ico.w+1

class RText(RPicture):
    def __init__(self, text, margin=VMARGIN):
        RPicture.__init__(self)
        x = 0
        for c in text:
            ico = images.sprcharacterget(c)
            if ico is not None:
                self.put(ico, dx=x)
                x += 7
        self.margin = margin
    def getsize(self):
        w, h = RPicture.getsize(self)
        h -= (VMARGIN-self.margin)
        return w, h


def linesize(line):
    width = MARGIN
    height = 0
    for item in line:
        w, h = item.getsize()
        width += w
        if h > height:
            height = h
    return width, height

def display(lines, timeleft, bgen=None, black=0):
    waves = []
    if lines:
        totalwidth = 0
        totalheight = 0
        for line in lines:
            w, h = linesize(line)
            if w > totalwidth:
                totalwidth = w
            totalheight += h
        heightmargin = (boards.bheight-2*CELL - totalheight) // (len(lines)+1)
        if heightmargin > VMARGIN:
            heightmargin = VMARGIN
        totalheight += heightmargin * (len(lines)+1)
        
        # size in number of CELLs
        cwidth = (totalwidth+CELL-1) // CELL
        cheight = (totalheight+CELL-1) // CELL
        
        x0 = ((boards.width - cwidth) // 2) * CELL + HALFCELL
        y0 = ((boards.height - cheight) // 2) * CELL + HALFCELL
        extras = boards.curboard.sprites.setdefault('ranking', [])
        #while extras:
        #    extras.pop().kill()
        #    yield 0.12
        vspeed = -4
        while extras:
            nextras = []
            for s in extras:
                s.step(0, vspeed)
                if s.y + s.ico.h <= 0:
                    s.kill()
                else:
                    nextras.append(s)
            extras[:] = nextras
            yield 1
            vspeed -= 1

        # draw the box filled with water
        original_y0 = y0
        wallicon = boards.patget((boards.curboard.num, 0, 0), images.KEYCOL)
        if black:
            fillicon = images.sprget('gameoverbkgnd')
            waveicons = [wallicon]
            y0 = boards.bheight+CELL
        else:
            fillicon = images.sprget(Flood.fill)
            waveicons = [images.sprget(n) for n in Flood.waves]
        for y in range(y0-CELL, y0+cheight*CELL+CELL, CELL):
            w = gamesrv.Sprite(wallicon, x0-CELL, y)
            extras.append(w)
        for x in range(x0, x0+cwidth*CELL, CELL):
            w = gamesrv.Sprite(wallicon, x, y0+cheight*CELL)
            extras.append(w)
            w = gamesrv.Sprite(waveicons[-1], x, y0-CELL)
            extras.append(w)
            waves.append(w)
            for y in range(y0, y0+cheight*CELL, CELL):
                w = gamesrv.Sprite(fillicon, x, y)
                extras.append(w)
        for y in range(y0-CELL, y0+cheight*CELL+CELL, CELL):
            w = gamesrv.Sprite(wallicon, x0+cwidth*CELL, y)
            extras.append(w)

        # draw the individual items inside
        y = y0 + totalheight
        lines.reverse()
        for line in lines:
            linew, lineh = linesize(line)
            x = x0 + MARGIN
            y -= (lineh + heightmargin)
            for item in line:
                w, h = item.getsize()
                extras += item.render(x, y+(lineh-h)//2)
                x += w

        vspeed = 0
        while y0 > original_y0:
            vspeed = max(vspeed-1, original_y0 - y0)
            y0 += vspeed
            for s in extras:
                s.step(0, vspeed)
            yield 1
    
    while timeleft > 0.0:
        if waves:
            ico = waveicons.pop(0)
            waveicons.append(ico)
            for w in waves:
                w.seticon(ico)
        for i in range(2):
            if bgen is None:
                t = boards.normal_frame()
            else:
                try:
                    t = bgen.next()
                except StopIteration:
                    timeleft = 0.0
                    break
            timeleft -= t
            yield t

# ____________________________________________________________

def ranking_picture(results, maximum, givepoints):
    if maximum is None:
        maximum = 0
        for n in results.values():
            maximum += n
    maximum = maximum or 1
    ranking = []
    teamrank = [0, 0]
    teamplayers = [[], []]
    for p, n in results.items():
        if p.team != -1:
            teamrank[p.team] += n
            teamplayers[p.team].append((n,p))
        else:
            ranking.append((n, random.random(), p))
    teamplayers[0].sort()
    teamplayers[0].reverse()
    teamplayers[1].sort()
    teamplayers[1].reverse()
    if teamplayers[0] != []:
        ranking.append((teamrank[0], random.random(), teamplayers[0]))
    if teamplayers[1] != []:
        ranking.append((teamrank[1], random.random(), teamplayers[1]))
    ranking.sort()
    ranking.reverse()

    nbpoints = givepoints and ((len(ranking)+1)//2)*10000
    lines = []
    for (n, dummy, bubber), i in zip(ranking, range(len(ranking))):
        pic = RPicture()
        if isinstance(bubber, list):
            fraction = (nbpoints//(10*len(bubber))) * 10
            total = fraction * len(bubber)
            for n, bub in bubber:
                bub.givepoints(fraction)
            bubber = bubber[0][1]
            pic.put(images.sprget(('hat', bubber.team)))
        else:
            if len(ranking) == 1:
                icon = 0
            elif i == 0:
                icon = 10
            elif i == len(ranking) - 1:
                icon = 9
            else:
                icon = 0
            pic.put(bubber.icons[icon, +1])
            total = 0
        line = []
        if nbpoints > 0:
            line.append(RPoints(bubber, nbpoints))
            bubber.givepoints(nbpoints - total)
            nbpoints -= 10000
        line.append(pic)
        line.append(RNumber(str(int(n*100.00001/maximum)) + '%'))
        lines.append(line)
    return lines


def just_wait():
    while 1:
        yield 2

def screen_scores():
    results = {}
    for p in BubPlayer.PlayerList:
        if p.points:
            results[p] = p.points
    lines = ranking_picture(results, None, 0)
    lines.insert(0, [RText("    THE END")])
    return lines

def screen_monster():
    pairs = []
    for p in BubPlayer.PlayerList:
        catch = p.stats.get('monster', {})
        for p2, count in catch.items():
            if count:
                pairs.append((count, p, p2))
    random.shuffle(pairs)
    pairs.sort()
    pairs.reverse()
    del pairs[5:]
    lines = []
    if pairs:
        lines.append([RText('Best Monster Bubblers')])
        for count, p, p2 in pairs:
            pic = RPicture()
            pic.put(p.icons[4,+1], 0, 6)
            pic.put(images.sprget(GreenAndBlue.new_bubbles[p.pn][1]), 31, 6)
            pic.put(images.sprget(GreenAndBlue.new_bubbles[p.pn][3]), 69, 6)
            pic.put(images.sprget(GreenAndBlue.normal_bubbles[p.pn][0]), 101)
            pic.put(images.sprget(p2), 101)
            lines.append([pic, RNumber(str(count))])
    return lines

def screen_catch():
    pairs = []
    for p in BubPlayer.PlayerList:
        catch = p.stats.get('catch', {})
        for p2, count in catch.items():
            if count:
                pairs.append((count, p, p2))
    random.shuffle(pairs)
    pairs.sort()
    pairs.reverse()
    del pairs[5:]
    lines = []
    if pairs:
        lines.append([RText('Best Dragon Bubblers')])
        for count, p, p2 in pairs:
            pic = RPicture()
            pic.put(p.icons[4,+1], 0, 6)
            pic.put(images.sprget(GreenAndBlue.new_bubbles[p.pn][1]), 31, 6)
            pic.put(images.sprget(GreenAndBlue.new_bubbles[p.pn][3]), 69, 6)
            pic.put(images.sprget(GreenAndBlue.normal_bubbles[p2.pn][0]), 101)
            pic.put(images.sprget(('eyes', 0,0)), 101)
            lines.append([pic, RNumber(str(count))])
    return lines

def screen_bonus():
    pairs = []
    for p in BubPlayer.PlayerList:
        catch = p.stats.get('bonus', {})
        for p2, count in catch.items():
            if count > 1:
                pairs.append((count, p, p2))
    random.shuffle(pairs)
    pairs.sort()
    pairs.reverse()
    seen = {}
    npairs = []
    for count, p, p2 in pairs:
        if p2 not in seen:
            npairs.append((count, p, p2))
            seen[p2] = 1
    pairs = npairs
    del pairs[5:]
    lines = []
    if pairs:
        lines.append([RText('Best Bonus Catchers')])
        for count, p, p2 in pairs:
            pic = RPicture()
            pic.put(p.icons[1,+1], 0)
            pic.put(images.sprget(p2), 44)
            lines.append([pic, RNumber(str(count))])
    return lines

def screen_bubble():
    pairs = []
    for p in BubPlayer.PlayerList:
        count = p.stats['bubble']
        if count:
            pairs.append((count, p))
    random.shuffle(pairs)
    pairs.sort()
    pairs.reverse()
    del pairs[5:]
    lines = []
    if pairs:
        lines.append([RText('Best Bubble Exploders')])
        for count, p in pairs:
            pic = RPicture()
            pic.put(p.icons[1,+1], 0)
            pic.put(images.sprget(Bubble.exploding_bubbles[1]), 27)
            lines.append([pic, RNumber(str(count))])
    return lines

def screen_die():
    pairs = []
    for p in BubPlayer.PlayerList:
        count = p.stats['die']
        if count:
            pairs.append((count, p))
    random.shuffle(pairs)
    pairs.sort()
    pairs.reverse()
    del pairs[5:]
    lines = []
    if pairs:
        lines.append([RText('Top Deaths')])
        n = 0
        for count, p in pairs:
            pic = RPicture()
            pic.put(p.icons[6+(n%3),+1], 0)
            lines.append([pic, RNumber(str(count))])
            n += 1
    return lines

def screen_authors():
    return [
        [RText('programming', 6)],
        [RText('     Armin & Odie')],
        [RText('art', 6)],
        [RText('     David Gowers, based on McSebi')],
        [RText('levels', 6)],
        [RText('     Gio & Odie & MS & Armin')],
        [RText('special thanks', 6)],
        [RText('     Odie & Brachamutanda')],
        [RText('beta-testers', 6)],
        [RText('     IMA Connection')],
        ]

def game_over():
    while 1:
        for screen in [screen_scores, screen_monster, screen_catch,
                       screen_bonus, screen_bubble, screen_die,
                       screen_authors]:
            lines = screen()
            if lines:
                for t in display(lines, 300, just_wait(), 1):
                    yield t
