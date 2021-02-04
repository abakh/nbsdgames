import boards, mnstrmap

class left:
    dir = 1
    def __init__(self, cls):
        self.cls = cls
    def build(self, x, y):
        return self.cls(x=x, y=y-1, dir=self.dir)

class right(left):
    dir = 0

LNasty = left(mnstrmap.Nasty)
RNasty = right(mnstrmap.Nasty)

LMonky = left(mnstrmap.Monky)
RMonky = right(mnstrmap.Monky)

LGhosty = left(mnstrmap.Ghosty)
RGhosty = right(mnstrmap.Ghosty)

LFlappy = left(mnstrmap.Flappy)
RFlappy = right(mnstrmap.Flappy)

LSpringy = left(mnstrmap.Springy)
RSpringy = right(mnstrmap.Springy)

LOrcy = left(mnstrmap.Orcy)
ROrcy = right(mnstrmap.Orcy)

LGramy = left(mnstrmap.Gramy)
RGramy = right(mnstrmap.Gramy)

LBlitzy = left(mnstrmap.Blitzy)
RBlitzy = right(mnstrmap.Blitzy)


# Sugar around the Board class
class Level(boards.Board):

    WIND_DELTA = boards.CELL
    winds = None
    monsters = []

    def __init__(self, num):
        walls = [line for line in self.walls.split('\n') if line]
        self.monsters = list(self.monsters)
        for y in range(len(walls)):
            line = walls[y]
            for x in range(len(line)):
                c = line[x]
                if c != ' ' and c != '#':
                    deflist = getattr(self, c)
                    if isinstance(deflist, left):
                        deflist = (deflist,)
                    for builder in deflist:
                        self.monsters.append(builder.build(x,y))
                    self.walls = self.walls.replace(c, ' ')
        if self.winds is None:
            width = len(walls[0])
            height = len(walls)
            spaces = " " * (width-6)
            lbar = '>'*(width/2-2)
            rbar = '<'*(width/2-2)
            winds = ['>> ' + spaces + ' <<',
                          lbar + 'x'*(width-len(lbar)-len(rbar)) + rbar]
            winds += ['>>^' + spaces + '^<<'] * (height-2)
            self.winds = '\n'.join(winds)
        boards.Board.__init__(self, num)
