#
# Second try at automatically generating levels that are
# a bit more related to each other instead of being completely independent.
#


import sys, random, math
from random import uniform, choice, randrange


class Parameter(object):
    def __init__(self, name, rng):
        self.name = name
        self.rng = rng
    def __get__(self, instance, cls):
        assert self.name not in instance.__dict__
        value = self.rng()
        setattr(instance, self.name, value)
        return value

class ChoiceParameter(Parameter):
    def __init__(self, name, list):
        Parameter.__init__(self, name, lambda : choice(list))

class BoolParameter(Parameter):
    def __init__(self, name, prob=0.5):
        Parameter.__init__(self, name, lambda : random.random() < prob)

def flat(mean,var):
    return randrange(mean-var,mean+var+1)

def dice(n,sides,orig=1):
    result = 0
    for i in range(n):
        result += orig+randrange(sides)
    return result

def fork(choice1, prob, choice2):
    if random.random() < prob:
        return choice1()
    else:
        return choice2()

MnstrCategory = {
    "Nasty": 0,
    "Monky": 0,
    "Ghosty": 1,
    "Flappy": 1,
    "Springy": 2,
    "Orcy": 0,
    "Gramy": 0,
    "Blitzy": 2}
MnstrNames = list(MnstrCategory.keys())
Bonuses = ['letter', 'fire', 'lightning', 'water', 'top']

def mnstrclslist(name):
    import boarddef
    classname1 = 'L' + name
    classname2 = 'R' + name
    return [getattr(boarddef, classname1), getattr(boarddef, classname2)]

class Shape:
    basemnstr = ChoiceParameter('basemnstr', MnstrNames)
    extramnstr = ChoiceParameter('extramnstr', list(range(4)))
    samemnstr = BoolParameter('samemnstr')
    baseshape = ChoiceParameter('baseshape', '   ODBGMPRWZS')
    rooms = BoolParameter('rooms')
    holes = BoolParameter('holes')
    lines = ChoiceParameter('lines', '   -/|')
    platforms = BoolParameter('platforms')
    platholes = BoolParameter('platholes')
    platfull  = BoolParameter('platfull')
    mess = ChoiceParameter('mess', '        ....!')
    closed = BoolParameter('closed', 0.95)
    bonuses = ChoiceParameter('bonuses', range(3**len(Bonuses)))
    smooth = ChoiceParameter('smooth', list(range(4)))
    startplats = BoolParameter('startplats', 0.98)
    makespace = BoolParameter('makespace', 0.8)
    straightfall = BoolParameter('straightfall', 0.8)
    mirrored = BoolParameter('mirrored', 0.4)
    enlargeholes = BoolParameter('enlargeholes', 0.9)

    all_parameters = [name for name in list(locals().keys())
                      if not name.startswith('_')]

    def __init__(self, shape=None):
        if shape:
            self.__dict__.update(shape.__dict__)
        self.modified = 0

    def set_gens(self, rooms=0, platforms=0, holes=0, smooth=0, mess=' ', lines=' '):
        self.rooms = rooms
        self.platforms = platforms
        self.holes = holes
        self.smooth = smooth
        self.mess = mess
        self.lines = lines

    def reset(self, attrname=None):
        if attrname:
            try:
                del self.__dict__[attrname]
            except KeyError:
                pass
        else:
            self.__dict__.clear()
        self.modified = 1

    def __eq__(self, other):
        return (self.__class__ is other.__class__ and
                self.__dict__ == other.__dict__)

    def test_similar_parameters(self, prevlist):
        similarity = 0
        rprevlist = prevlist[:]
        rprevlist.reverse()
        for param in Shape.all_parameters:
            accum = 0
            for prev in rprevlist:
                if getattr(self, param) != getattr(prev, param):
                    break
                else:
                    accum += 1
                    similarity += accum
        minimum = min(4*len(prevlist), 7)
        if not (minimum <= similarity <= 17):
            self.reset()

    def test_not_too_often(self, prevlist):
        for param, bad_value, delay in [
            ('mess', '.', 2),
            ('mess', '!', 11),
            ('holes', 1,  1),
            ]:
            if getattr(self, param) == bad_value:
                for prev in prevlist[-delay:]:
                    if getattr(prev, param) == bad_value:
                        self.reset(param)

    def test_mess_hole(self, prevlist):
        if self.mess == '!':
            self.holes = 1

    all_tests = [value for (name, value) in list(locals().items())
                 if name.startswith('test_')]

    def accept(self, lvl):
        f = lambda d=self.difficulty : randrange(3, 4+int(9*d))
        lvl.mlist = [(mnstrclslist(self.basemnstr), f)]
        repeat = choice([2,2,3]) - self.extramnstr
        if repeat > 1:
            lvl.mlist *= repeat
        if self.extramnstr:
            othermnstr = [name for name in MnstrNames if name!=self.basemnstr]
            if self.samemnstr:
                othermnstr = [name for name in othermnstr
                              if MnstrCategory[name]==MnstrCategory[self.basemnstr]]
            random.shuffle(othermnstr)
            for name in othermnstr[:self.extramnstr]:
                lvl.mlist.append((mnstrclslist(name), f))

        lvl.genwalls = []

        if self.baseshape == 'G':
            lvl.genwalls.append((RandomLevel.grids,
                                 uniform(0.7,0.8),
                                 uniform(0.7,0.8)))
            self.set_gens()
        if self.baseshape == 'P':
            lvl.genwalls.append((RandomLevel.pegs,
                                  uniform(0.1,0.2),
                                  uniform(0.45,0.7),
                                  choice([0,1,1,1])))
            self.set_gens(smooth=3)
            self.closed = random.random() < 0.80
        if self.baseshape == 'B':
            nr = choice([0,0,1])
            lvl.genwalls.append((RandomLevel.bouncers,
                                 dice(1, 100) + 250 - nr*200, # length
                                 uniform(0.7, 1.7),
                                 nr))
            self.set_gens(smooth=3)
        if self.baseshape == 'W':
            nr = dice(1, 3) + 2
            lvl.genwalls.append((RandomLevel.walkers,
                                 dice(2, 100) + 100, # length
                                 nr, nr + dice(2, 3),
                                 choice([0,1])))
            self.set_gens()
        if self.baseshape == 'R':
            lvl.genwalls.append((RandomLevel.rivers,
                                 randrange(3,(lvl.WIDTH-4)/4), # the number of rivers
                                 uniform(0.3, 1.4), # the side stepping threshold
                                 10))                # the max side stepping size
            self.set_gens()
        if self.baseshape == 'Z':
            lvl.genwalls.append((RandomLevel.zigzag,))
            self.set_gens()
        if self.baseshape == 'M':
            lvl.genwalls.append((RandomLevel.mondrian,))
            self.set_gens()
        if self.baseshape == 'O':
            lvl.genwalls.append((RandomLevel.remove_joined_blocks,))
            self.set_gens()
        if self.baseshape == 'S':
            lvl.genwalls.append((RandomLevel.platforms_reg,))
            self.set_gens()
            self.makespace = random.random() < 0.1
            if self.closed:
                self.startplats = 0
        if self.baseshape == 'D':
            lvl.genwalls.append((RandomLevel.discrete_blocks,))
            self.set_gens()
            self.makespace = random.random() < 0.1
            if self.closed:
                self.startplats = 0

        if self.rooms:
            nr = dice(2, 6)
            lvl.genwalls.append((RandomLevel.rooms,
                                 lambda : flat(9-nr,2),    # the half size of the room
                                 lambda : uniform(0.8,1.2), # the excentricity of the room
                                 nr))                       # the number of rooms
        if self.lines != ' ':
            rng_angle = {
                '-': lambda : 0,
                '/': None,    # default
                '|': lambda : math.pi/2,
                }
            lvl.genwalls.append((RandomLevel.lines,
                                  lambda : dice(8,3), # line length
                                  dice(2,4),          # number of lines
                                  rng_angle[self.lines]))
        if self.platforms:
            nplat  = dice(2,4,0)
            if nplat: space  = flat((lvl.HEIGHT-1)/nplat/2,(lvl.HEIGHT-1)/nplat/2-1)
            else:     space = 1
            if self.platholes:
                nholes = lambda : dice(1,3)
            else:
                nholes = lambda : 0
            wholes = lambda : dice(2,3)
            full = self.platfull
            lvl.genwalls.append((RandomLevel.platforms,
                                  (nplat,space),   # number of platform and spacing
                                  (nholes,wholes), # number of holes and width
                                  full))           # full width platform
        if self.mess != ' ':
            threshold = {
                '.': 0.02 + 0.08*random.random(),   # normal
                '!': 0.25 + 0.2 *random.random(),   # super-filled
                }
            lvl.genwalls.append((RandomLevel.mess, threshold[self.mess]))
        if self.holes:
            nh = choice([1,1,2,2,2,3,3,3,4,5])
            lvl.genwalls.append((RandomLevel.holes,
                                 lambda : flat(9-nh,2),       # radius of the holes
                                 lambda : uniform(0.9,1.1),   # excentricity
                                 nh,                          # number of holes
                                 lambda : choice([0,0,0,1]))) # circle or rectangle
        if self.closed:
            lvl.genwalls.append((RandomLevel.close,))
        if self.smooth > 0:
            # smooth away all lone empty spaces
            lvl.genwalls.append((RandomLevel.smooth, 1.0, 1))
            # possibly smooth away some lone bricks
            if self.smooth == 2:
                lvl.genwalls.append((RandomLevel.smooth, 0.25, 0))
            elif self.smooth == 3:
                lvl.genwalls.append((RandomLevel.smooth, 0.75, 0))
        if self.startplats:
            lvl.genwalls.append((RandomLevel.startplatform, ))
            lvl.genwalls.append((RandomLevel.openstartway, ))

        if self.makespace:
            lvl.genwalls.append((RandomLevel.make_space, ))

        if self.straightfall:
            lvl.genwalls.append((RandomLevel.prevent_straight_fall, ))

	if self.mirrored:
	    lvl.genwalls.append((RandomLevel.mirror, ))

        if self.enlargeholes:
            lvl.genwalls.append((RandomLevel.enlarge_tiny_holes, ))

        lvl.genwalls.append((RandomLevel.generate_wind, ))
        b = self.bonuses
        for name in Bonuses:
            setattr(lvl, name, (b % 3) == 1)
            b = b // 3
        lvl.autogen_shape = self


def generate_shape(prevlist):
    tests = Shape.all_tests
    s = Shape()
    for i in range(100):
        s1 = Shape(s)
        random.shuffle(tests)
        for test in tests:
            test(s1, prevlist)
        if not s1.modified and s1 == s:
            break
        s = s1
#    else:
#        sys.stdout.write('*')
    del s.modified
    return s

def makeshapes(nblevels=25):
    shapelist = []
    for i in range(nblevels):
        s = generate_shape(shapelist)
        s.difficulty = float(i+1)/nblevels
        yield s
        shapelist.append(s)
        if len(shapelist) == 10:
            del shapelist[:]

def GenerateLevels():
#    print 'generating levels',
    Levels = []
    for s in makeshapes():
        class level(RandomLevel):
            WIDTH  = 28
            HEIGHT = 23
            def enter(self, *args, **kw):
                result = RandomLevel.enter(self, *args, **kw)
                params = list(self.autogen_shape.__dict__.items())
                params.sort()
#                for keyvalue in params:
#                    print '%20s: %s' % keyvalue
                return result
        s.accept(level)
        Levels.append(level)
#        sys.stdout.write('.')
#        sys.stdout.flush()
#    print
    class levelfinal(RandomLevel):
        WIDTH = level.WIDTH
        HEIGHT = level.HEIGHT
        genwalls = [(RandomLevel.platforms,(5,3),(lambda:flat(2,1),lambda:flat(6,2)),1),
                    (RandomLevel.close,)]
    Levels.append(levelfinal)
    return Levels

def GenerateSingleLevel(width, height):
    [s] = makeshapes(1)
    class level(RandomLevel):
        WIDTH = width
        HEIGHT = height
    s.accept(level)
    return level

if __name__ == '__main__':
    for s in makeshapes():
        print(s.__dict__)
else:
    rnglevel = {}
    exec(compile(open('levels/rnglevel', "rb").read(), 'levels/rnglevel', 'exec'), rnglevel)
    RandomLevel = rnglevel['RandomLevel']
