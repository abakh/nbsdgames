#
# This script outputs the random levels in a format you can
# save into a file in the levels-directory and bub'n'bros
# will be able to use it.
#
# this accepts the following parameters:
# -seed N  use random seed N for the generation
#

import sys
import random
import string
sys.path.append('..')
sys.path.append('../common')

n_lvls = 1

idx = 0
while idx < len(sys.argv):
    arg = sys.argv[idx]
    idx += 1
    if arg == '-seed':
        arg = sys.argv[idx]
        idx += 1
        print("# Using seed: " + arg + "\n")
        random.seed(arg)

def printlvl(level):
    mons = {}
    monconv = {}
    tmpmons = {}

    # Populate monster tables
    for y in range(0,level.HEIGHT):
        for x in range(0,level.WIDTH):
            wm = level.wmap[y][x]
            if wm >= 'a':
                m = getattr(level, wm)
                if m.dir == 1:
                    dir = 'L'
                else:
                    dir = 'R'
                s = dir + m.cls.__name__
                if s in tmpmons:
                    tmpmons[s].append(wm)
                else:
                    tmpmons[s] = [wm]
    # Build monster character conversion tables
    lettr = 'a'
    for m in tmpmons:
        for n in tmpmons[m]:
            monconv[n] = lettr
        mons[lettr] = m
        lettr = chr(ord(lettr) + 1)

    # Build walls, replacing monsters from mons[]
    walls = ""

    for y in range(0,level.HEIGHT-1):
        walls += "##"
        for x in range(0,level.WIDTH):
            wm = level.wmap[y][x]
            if wm >= 'a':
                if wm in monconv:
                    walls += monconv[wm]
                else:
                    walls += '?'
            else:
                walls += wm
        walls += "##\n"
    walls += "##"
    for x in range(0,level.WIDTH):
        if level.wmap[0][x] == '#' or level.wmap[level.HEIGHT-1][x] == '#':
            walls += "#"
        else:
            walls += " "
    walls += "##\n"

    # Build winds
    winds = ""
    for y in range(0,level.HEIGHT):
        for x in range(0,level.WIDTH+4):
            winds += level.winds[y][x]
        winds += "\n"

    for m in mons:
        print("    " + m + " = " + mons[m])

    if level.letter:
        print("    letter = 1")
    if level.fire:
        print("    fire = 1")
    if level.lightning:
        print("    lightning = 1")
    if level.water:
        print("    water = 1")
    if level.top:
        print("    top = 1")

    print("    walls = \"\"\"\n" + walls + "\"\"\"")
    print("    winds = \"\"\"\n" + winds + "\"\"\"")


for i in range(n_lvls):
    print("""
import boarddef, mnstrmap, random
from boarddef import LNasty, LMonky, LGhosty, LFlappy
from boarddef import LSpringy, LOrcy, LGramy, LBlitzy
from boarddef import RNasty, RMonky, RGhosty, RFlappy
from boarddef import RSpringy, ROrcy, RGramy, RBlitzy
""")

    d = {'__name__': 'RandomLevels'}
    exec(compile(open('levels/RandomLevels.py', "rb").read(), 'levels/RandomLevels.py', 'exec'), d)

    for i, Lvl in enumerate(d['GenerateLevels']()):
        level = Lvl(i)

        if level.monsters:
            print("\n\nclass level%02d(boarddef.Level):" % (i+1))
        else:
            print("\n\nclass levelFinal(boarddef.Level):")

        printlvl(level)
	print()
