
import random, os, sys, math
import gamesrv
import images

CELL = 16    # this constant is inlined at some places, don't change
HALFCELL = CELL//2
FRAME_TIME = 0.025
#DEFAULT_LEVEL_FILE = 'levels/scratch.py'

BOARD_BKGND = 1    # 0 = black, 1 = darker larger wall tiles


class Copyable:
    pass   # see bonuses.py, class Clock


class Board(Copyable):
    letter    = 0
    fire      = 0
    lightning = 0
    water     = 0
    top       = 0
    
    WIND_DELTA = HALFCELL

    def __init__(self, num):
        # the subclasses should define 'walls', 'winds', 'monsters'
        self.walls = walls = [line for line in self.walls.split('\n') if line]
        self.winds = winds = [line for line in self.winds.split('\n') if line]
        self.num = num
        self.width = len(walls[0])
        self.height = len(walls)
        for line in walls:
            assert len(line) == self.width, "some wall lines are longer than others"
        for line in winds:
            assert len(line) == self.width, "some wind lines are longer than others"
        #assert walls[0] ==  walls[-1], "first and last lines must be identical"
        assert len(winds) == self.height, "wall and wind heights differ"
        self.walls_by_pos = {}
        self.sprites = {}
        if self.top:
            testline = self.walls[0]
        else:
            testline = self.walls[-1]
        self.holes = testline.find('  ') >= 0
        self.playingboard = 0
        self.bonuslevel = not self.monsters or (gamesrv.game.finalboard is not None and self.num >= gamesrv.game.finalboard)
        self.cleaning_gen_state = 0

    def set_musics(self, prefix=[]):
        if (self.num+1) % 20 < 10:
            gamesrv.set_musics(prefix + [images.music_intro], [images.music_game],
                               reset=0)
        else:
            gamesrv.set_musics(prefix + [], [images.music_game2], reset=0)

    def writesprites(self, name, xyicolist):
        sprlist = self.sprites.setdefault(name, [])
        xyicolist = xyicolist[:]
        xyicolist.reverse()
        for s in sprlist[:]:
            if xyicolist:
                s.move(*xyicolist.pop())
            else:
                s.kill()
                sprlist.remove(s)
        while xyicolist:
            x, y, ico = xyicolist.pop()
            sprlist.append(gamesrv.Sprite(ico, x, y))

    def enter(self, complete=1, inplace=0, fastreenter=False):
        global curboard
        if inplace:
            print("Re -", end=' ')
        print("Entering board", self.num+1)
        self.set_musics()
        # add board walls
        l = self.sprites.setdefault('walls', [])
        bl = self.sprites.setdefault('borderwalls', [])
        if inplace:
            deltay = 0
        else:
            deltay = bheight
        wnx = wny = 1
        while haspat((self.num, wnx, 0)):
            wnx += 1
        while haspat((self.num, 0, wny)):
            wny += 1
        self.wnx = wnx
        self.wny = wny

        if haspat((self.num, 'l')):
            lefticon = patget((self.num, 'l'))
            if haspat((self.num, 'r')):
                righticon = patget((self.num, 'r'))
            else:
                righticon = lefticon
            xrange = list(range(2, self.width-2))
        else:
            xrange = list(range(self.width))
            lefticon = righticon = None

        if BOARD_BKGND == 1:
            gl = self.sprites.setdefault('background', [])
            xmax = (self.width-2)*CELL
            ymax = self.height*CELL
            y = -HALFCELL
            ystep = 0
            firstextra = 1
            while y < ymax:
                x = 2*CELL+HALFCELL
                xstep = 0
                while x < xmax:
                    bitmap, rect = loadpattern((self.num, xstep, ystep),
                                               images.KEYCOL)
                    bitmap, rect = images.makebkgndpattern(bitmap, rect)
                    if firstextra:
                        # special position where a bit of black might show up
                        x -= rect[2]
                        xstep = (xstep-1) % wnx
                        firstextra = 0
                        continue
                    bkgndicon = bitmap.geticon(*rect)
                    w = gamesrv.Sprite(bkgndicon, x, y + deltay)
                    gl.append(w)
                    x += rect[2]
                    xstep = (xstep+1) % wnx
                y += rect[3]
                ystep = (ystep+1) % wny
        else:
            gl = []

        if lefticon is not None:
            for y in range(0, self.height, lefticon.h // CELL):
                bl.append(gamesrv.Sprite(lefticon, 0, y*CELL + deltay))

        for y in range(self.height):
            for x in xrange:
                c = self.walls[y][x]
                if c == '#':
                    wallicon = patget((self.num, x%wnx, y%wny), images.KEYCOL)
                    w = gamesrv.Sprite(wallicon, x*CELL, y*CELL + deltay)
                    l.append(w)
                    self.walls_by_pos[y,x] = w

        if not l:
            # self.sprites['walls'] must not be empty, for putwall
            wallicon = patget((self.num, 0, 0), images.KEYCOL)
            w = gamesrv.Sprite(wallicon, 0, -wallicon.h)
            l.append(w)

        if righticon is not None:
            for y in range(0, self.height, lefticon.h // CELL):
                bl.append(gamesrv.Sprite(righticon, (self.width-2)*CELL, y*CELL + deltay))

        while deltay:
            dy = -min(deltay, 8)
            for w in gl:
                w.step(0, dy)
            for w in l:
                w.step(0, dy)
            for w in bl:
                w.step(0, dy)
            deltay += dy
            yield 1

        if inplace:
            for w in images.ActiveSprites:
                w.to_front()

        curboard = self
        if gamesrv.game:
            gamesrv.game.updateboard()
        if not complete:
            return
        # add players
        from player import BubPlayer, scoreboard
        if not inplace:
            random.shuffle(BubPlayer.PlayerList)
        scoreboard(1, inplace=inplace)
        if not fastreenter:
            random.shuffle(BubPlayer.PlayerList)
            playing = []
            for p in BubPlayer.PlayerList:
                if p.isplaying():
                    p.enterboard(playing)
                    p.zarkon()
                    playing.append(p)
            for d in BubPlayer.DragonList:
                d.enter_new_board()
        else:
            # kill stuff left over from leave(inplace=1) (Big Clock bonus only)
            import bonuses
            keepme = bonuses.Points
            dragons = {}
            playing = []
            for p in BubPlayer.PlayerList:
                if p.isplaying():
                    for d in p.dragons:
                        if hasattr(d, 'dcap'):
                            d.dcap['shield'] = 90
                        dragons[d] = True
                    playing.append(p)
            for s in images.ActiveSprites[:]:
                if isinstance(s, keepme) or s in dragons:
                    pass
                else:
                    s.kill()
        # add monsters
        if not self.bonuslevel:
            import monsters
            f_monsters = gamesrv.game.f_monsters
            if f_monsters < 0.1:
                f_monsters = max(1.0, min(2.0, (len(playing)-2)/2.2+1.0))
            for mdef in self.monsters:
                if not fastreenter:
                    yield 2
                cls = getattr(monsters, mdef.__class__.__name__)
                dir = mdef.dir
                i = random.random()
                while i < f_monsters:
                    cls(mdef, dir=dir)
                    dir = -dir
                    i += 1.0
        self.playingboard = 1

    def putwall(self, x, y, w=None):
        wallicon = patget((self.num, x%self.wnx, y%self.wny), images.KEYCOL)
        if w is None:
            w = gamesrv.Sprite(wallicon, 0, bheight)
            l = self.sprites['walls']
            w.to_back(l[-1])
            l.append(w)
        self.walls_by_pos[y,x] = w
        if y >= 0:
            line = self.walls[y]
            self.walls[y] = line[:x] + '#' + line[x+1:]

    def killwall(self, x, y, kill=1):
        w = self.walls_by_pos[y,x]
        if kill:
            l = self.sprites['walls']
            if len(l) > 1:
                l.remove(w)
                w.kill()
            else:
                # self.sprites['walls'] must never be empty
                # or putwall will crash!
                w.move(0, -bheight)
        del self.walls_by_pos[y,x]
        line = self.walls[y]
        self.walls[y] = line[:x] + ' ' + line[x+1:]
        return w

    def reorder_walls(self):
        walls_by_pos = self.walls_by_pos
        items = [(yx, w1.ico) for yx, w1 in list(walls_by_pos.items())]
        if not items:
            return   # otherwise self.sprites['walls'] would be emptied
        items.sort()
        l = self.sprites['walls']
        while len(l) > len(items):
            l.pop().kill()
        assert len(items) == len(l)
        for ((y,x), ico), w2 in zip(items, l):
            w2.move(x*CELL, y*CELL, ico)
            walls_by_pos[y,x] = w2

    def leave(self, inplace=0):
        global curboard
        if not gamesrv.has_loop_music():
            gamesrv.fadeout(1.5)
        from player import BubPlayer
        for p in BubPlayer.PlayerList:
            if p.isplaying():
                p.savecaps()
        if BubPlayer.LeaveBonus:
            for t in BubPlayer.LeaveBonus:
                yield t
            BubPlayer.LeaveBonus = None
        curboard = None
        if inplace:
            i = -1
        else:
            while images.ActiveSprites:
                s = random.choice(images.ActiveSprites)
                s.kill()
                yield 0.9
            i = 0
        sprites = []
        for l in list(self.sprites.values()):
            sprites += l
        self.sprites.clear()
        self.walls_by_pos.clear()
        random.shuffle(sprites)
        for s in sprites:
            s.kill()
            if i:
                i -= 1
            else:
                yield 0.32
                i = 3
        if not inplace:
            for p in BubPlayer.PlayerList:
                if p.isplaying():
                    p.zarkoff()
            yield 4

    def clean_gen_state(self):
        self.cleaning_gen_state = 1
        while len(BoardGen) > 1:
            #yield force_singlegen()
            #if 'flood' in self.sprites:
            #    for s in self.sprites['flood']:
            #        s.kill()
            #    del self.sprites['flood']
            yield normal_frame()
        self.cleaning_gen_state = 0

def bget(x, y):
    if 0 <= x < curboard.width:
        if y < 0 or y >= curboard.height:
            y = 0
        return curboard.walls[y][x]
    else:
        return '#'

def wget(x, y):
    delta = curboard.WIND_DELTA
    x = (x + delta) // 16
    y = (y + delta) // 16
    if 0 <= x < curboard.width:
        if y < 0:
            y = 0
        elif y >= curboard.height:
            y = -1
        return curboard.winds[y][x]
    elif x < 0:
        return '>'
    else:
        return '<'

def onground(x, y):
    if y & 15:
        return 0
    x0 = (x+5) // 16
    x1 = (x+16) // 16
    x2 = (x+27) // 16
    y0 = y // 16 + 2

    if x0 < 0 or x2 >= curboard.width:
        return 0
    y1 = y0 - 1
    if not (0 < y0 < curboard.height):
        if y0 != curboard.height:
            y1 = 0
        y0 = 0
    y0 = curboard.walls[y0]
    y1 = curboard.walls[y1]
    return (' ' == y1[x0] == y1[x1] == y1[x2] and
            not (' ' == y0[x0] == y0[x1] == y0[x2]))
    #return (' ' == bget(x0,y0-1) == bget(x1,y0-1) == bget(x2,y0-1) and
    #        not (' ' == bget(x0,y0) == bget(x1,y0) == bget(x2,y0)))
    #return (bget(x1,y0-1)==' ' and
    #        ((bget(x1,y0)=='#') or
    #         (bget(x0,y0)=='#' and bget(x0,y0-1)==' ') or
    #         (bget(x2,y0)=='#' and bget(x2,y0-1)==' ')))

def onground_nobottom(x, y):
    return onground(x, y) and y+32 < bheight

def underground(x, y):
    if y % CELL:
        return 0
    x0 = (x+5) // CELL
    x1 = (x+CELL) // CELL
    x2 = (x+2*CELL-5) // CELL
    y0 = y // CELL

    if x0 < 0 or x2 >= curboard.width:
        return 0
    y1 = y0 - 1
    if not (0 < y0 < curboard.height):
        if y0 != curboard.height:
            y1 = 0
        y0 = 0
    y0 = curboard.walls[y0]
    y1 = curboard.walls[y1]
    return (' ' == y0[x0] == y0[x1] == y0[x2] and
            not (' ' == y1[x0] == y1[x1] == y1[x2]))

def x2bounds(x):
    if x < 32:
        return 32
    elif x > bwidth - 64:
        return bwidth - 64
    else:
        return x

def vertical_warp(nx, ny):
    if ny >= bheight:
        ny -= bheightmod
    elif ny < -32:
        ny += bheightmod
    return nx, ny

def vertical_warp_sprite(spr):
    if spr.y >= bheight:
        spr.step(0, -bheightmod)
    elif spr.y < -32:
        spr.step(0, bheightmod)

##def vertical_warp(nx, ny):
##    if ny >= bheight:
##        ny -= bheightmod
##    elif ny < -32:
##        ny += bheightmod
##    else:
##        return (nx, ny), 0
##    from player import BubPlayer
##    if BubPlayer.Moebius:
##        nx = bwidth - 2*CELL - nx
##        return (nx, ny), 1
##    else:
##        return (nx, ny), 0


MODULES = ['boards', 'bonuses', 'bubbles', 'images',
           'mnstrmap', 'monsters', 'player', 'ranking',
           'binboards', 'macbinary', 'boarddef',
           'ext1', 'ext2', 'ext3', 'ext4', 'ext5', 'ext6', 'ext7']

def loadmodules(force=0):
    levelfilename = gamesrv.game.levelfile
    modulefiles = {None: levelfilename}
    for m in MODULES:
        if os.path.isfile(m+'.py'):
            modulefiles[m] = m+'.py'
        elif os.path.isfile(os.path.join(m, '__init__.py')):
            modulefiles[m] = os.path.join(m, '__init__.py')
    mtimes = {}
    for m, mfile in list(modulefiles.items()):
        mtimes[m] = os.stat(mfile).st_mtime
    reload = force or (mtimes != getattr(sys, 'ST_MTIMES', None))
    import player
    playerlist = player.BubPlayer.PlayerList
    if reload:
        delete = hasattr(sys, 'ST_MTIMES')
        sys.ST_MTIMES = mtimes
        if delete:
            print("Reloading modules.")
            for m, mfile in list(modulefiles.items()):
                if m is not None and m in sys.modules:
                    del sys.modules[m]

    # Clear
    clearallsprites()

    import player
    for p in playerlist:
        player.upgrade(p)
    for n in range(len(playerlist), images.MAX):
        playerlist.append(player.BubPlayer(n))
    player.BubPlayer.PlayerList = playerlist
    if reload:
        import boards
        from images import haspat, loadpattern
        boards.haspat = haspat
        boards.loadpattern = loadpattern
        del boards.BoardList[:]
        if levelfilename.lower().endswith('.py'):
            levels = {}
            print('Source level file:', levelfilename)
            exec(compile(open(levelfilename, "rb").read(), levelfilename, 'exec'), levels)
            if 'GenerateLevels' in levels:
                levels = levels['GenerateLevels']()
                if isinstance(levels, list):
                    levels = dict(list(zip(list(range(len(levels))), levels)))
        else:
            import binboards
            levels = binboards.load(levelfilename)
        boards.register(levels)
    return reload

def clearallsprites():
    gamesrv.clearsprites()
    import images
    del images.ActiveSprites[:]
    images.SpritesByLoc.clear()

def wait_for_one_player():
    from player import BubPlayer
    clearallsprites()
    nimages = None
    while not [p for p in BubPlayer.PlayerList if p.isplaying()]:
        yield 3
        if not nimages:
            desc = getattr(gamesrv.game, 'FnDesc', '?')
            host, port = getattr(gamesrv.game, 'address', ('?', '?'))
            images.writestrlines([
                "Welcome to",
                desc.upper(),
                "at %s:%s" % (host.lower(), port),
                None,
                "Click on your Favorite Color's Dragon",
                "Choose four keys: Right, Left, Jump, Shoot",
                "and Let's Go!",
                None,
                "Click again for more than one player",
                "on the same machine.",
                ])
            
            from mnstrmap import PlayerBubbles
            nimages = [PlayerBubbles.bubble[0],
                       PlayerBubbles.bubble[1],
                       PlayerBubbles.bubble[1],
                       PlayerBubbles.bubble[0],
                       PlayerBubbles.bubble[2],
                       PlayerBubbles.bubble[2]]
            screenwidth = bwidth + 9*CELL
            screenheight = bheight

            def welcomebubbling(self):
                fx = self.x
                dx = (random.random() - 0.5) * 1.9
                for y in range(self.y-3, -self.ico.h, -3):
                    fx += dx
                    self.move(int(fx), y)
                    yield None
                    if y == self.ypop:
                        from mnstrmap import PlayerBubbles
                        self.setimages(None)
                        self.gen.append(self.die(PlayerBubbles.explosion))
                self.kill()

            yield 10
            gamesrv.set_musics([], [images.music_game2], reset=0)
        
        if ((not images.ActiveSprites or random.random() < 0.05678) and
            gamesrv.clients):
            # make sure the extension's images are loaded too
            # NB. this is also needed for import auto-detection
            import ext1; import ext2; import ext3
            import ext4; import ext5; import ext6
            import ext7
            
            ico = images.sprget(nimages[0])
            s = images.ActiveSprite(ico,
                                    random.randrange(0, screenwidth-ico.w),
                                    screenheight)
            s.ypop = random.randrange(-ico.h, screenheight)
            s.gen = [welcomebubbling(s)]
            s.setimages(s.cyclic(nimages, speed=1))
            if random.random() > 0.4321:
                try:
                    key, (filename, (x, y, w, h)) = random.choice(
                        list(images.sprmap.items()))
                except:
                    w = h = 0
                if w == h == 32:
                    s2 = images.ActiveSprite(images.sprget(key), -32, 0)
                    s2.gen = [s2.following(s, (s.ico.w-32)//2, (s.ico.h-32)//2)]
                    s.ypop = None
        images.action(images.ActiveSprites[:])

def patget(n, keycol=None):
    bitmap, rect = loadpattern(n, keycol)
    return bitmap.geticon(*rect)

def get_lives():
    return gamesrv.game.limitlives

def do_nothing():
    while True:
        yield 5

BoardList = []
curboard = None
BoardGen = [do_nothing()]

def next_board(num=0, complete=1, fastreenter=False):
    yield force_singlegen()
    set_frametime(1.0)
    brd = curboard
    inplace = 0
    if brd:
        inplace = brd.bonuslevel or fastreenter
        num = brd.num
        if not inplace:
            num += gamesrv.game.stepboard
            if num >= len(BoardList):
                num = len(BoardList)-1
            if (gamesrv.game.finalboard is not None and num > gamesrv.game.finalboard):
                num = gamesrv.game.finalboard
        for t in brd.leave(inplace=inplace):
            yield t

    # reset global board state
    from player import BubPlayer, reset_global_board_state
    reset_global_board_state()
    if not inplace:
        del BubPlayer.MonsterList[:]
        # wait for at least one player
        for t in wait_for_one_player():
            yield t
        # reload modules if changed
        if loadmodules():
            import boards
            boards.BoardGen = [boards.next_board(num)]
            return

    if num < 0:
        num = 0
    elif num >= len(BoardList):
        num = len(BoardList)-1
    brd = BoardList[num](num)
    for t in brd.enter(complete, inplace=inplace, fastreenter=fastreenter):
        yield t

    if brd.bonuslevel:
        gen = bonus_play
    else:
        gen = normal_play
    BoardGen[0] = gen()

def set_frametime(ft, privtime=100):
    from player import BubPlayer
    BubPlayer.BaseFrametime = ft
    BubPlayer.PlayersPrivateTime = privtime
    images.loadsounds(1.0 / ft)

def extra_boardgen(gen, at_end=0):
    if curboard.playingboard:
        if at_end or not BoardGen:
            BoardGen.append(gen)
        else:
            BoardGen.insert(1, gen)

def replace_boardgen(gen, force=0):
    if curboard.playingboard or force:
        curboard.playingboard = 0
        BoardGen[0] = gen

def force_singlegen():
    del BoardGen[1:]
    return 0

def has_singlegen():
    return len(BoardGen) <= 1

def display_hat(p, d):
    if p.team == -1 or getattr(d,'isdying',0) or hasattr(d,'no_hat'):
        return
    try:
        bottom_up = d.bottom_up()
    except AttributeError:
        bottom_up = 0
    try:
        image = ('hat', p.team, d.dir, d.hatangle)
    except AttributeError:
        image = ('hat', p.team)
    if bottom_up:
        image = 'vflip', image
        y = d.y
    else:
        y = d.y - 16
    ico = images.sprget(image)
    if (getattr(d,'hatsprite',None) is None or
        not d.hatsprite.alive):
        d.hatsprite = images.ActiveSprite(ico, d.x, y)
    else:
        d.hatsprite.to_front()
        d.hatsprite.move(d.x, y, ico)
    d.hatsprite.gen = [d.hatsprite.die([None])]

def normal_frame():
    from player import BubPlayer
    BubPlayer.FrameCounter += 1

    # main generator dispatch loop
    images.action(images.ActiveSprites[:])
    
    frametime = 10
    for p in BubPlayer.PlayerList:
        if p.isplaying():
            frametime = BubPlayer.BaseFrametime
            p.zarkon()
            for d in p.dragons:
                d.to_front()
                display_hat(p, d)
                d.prefix(p.pn)
    if not (BubPlayer.FrameCounter & 31):
        gamesrv.compactsprites()
        reset = getattr(BubPlayer, 'MultiplyerReset', 0)
        if reset and BubPlayer.FrameCounter >= reset:
            BubPlayer.MultiplyerReset = 0
            set_frametime(1.0)
    return frametime

def normal_play():
    from player import BubPlayer
    import bonuses
    framecounter = 0
    bonus_callback = bonuses.start_normal_play()
    while BubPlayer.MonsterList:
        bonus_callback()
        yield normal_frame()
        if not BubPlayer.DragonList:
            continue
        framecounter += 1
        BASE = 500
        if not (framecounter % BASE):
            if framecounter == 4*BASE:
                from monsters import Monster
                from mnstrmap import BigImages
                ico = images.sprget(BigImages.hurryup[1])
                s = images.ActiveSprite(ico, (bwidth-ico.w)//2, (bheight-ico.h)//2)
                s.setimages(s.die(BigImages.hurryup * 12, 2))
                images.Snd.Hurry.play()
                mlist = [s for s in images.ActiveSprites
                         if (isinstance(s, Monster) and s.regular() and
                             not s.angry)]
                if mlist:
                    s = random.choice(mlist)
                    s.angry = [s.genangry()]
                    s.resetimages()
            if framecounter >= 6*BASE:
                mlist = [s for s in images.ActiveSprites
                         if isinstance(s, Monster) and s.regular() and s.angry]
                if mlist:
                    images.Snd.Hell.play()
                    gamesrv.set_musics([], [])
                    s = random.choice(mlist)
                    s.become_ghost()
                    framecounter = -200
                else:
                    framecounter = 2*BASE
            if framecounter == 0:
                curboard.set_musics()
    replace_boardgen(last_monster_killed(), 1)

##def normal_play():
##    # TESTING!!
##    from player import BubPlayer
##    for p in BubPlayer.PlayerList:
##        if not p.icons:
##            p.loadicons(p.icons, images.sprget)
##    results = {BubPlayer.PlayerList[0]: 100,
##               BubPlayer.PlayerList[1]: 200,
##               BubPlayer.PlayerList[2]: 300,
##               BubPlayer.PlayerList[3]: 400,
##               BubPlayer.PlayerList[4]: 100,
##               BubPlayer.PlayerList[5]: 200,
##               BubPlayer.PlayerList[6]: 300,
##               BubPlayer.PlayerList[7]: 400,
##               BubPlayer.PlayerList[8]:1000,
##               BubPlayer.PlayerList[9]:1000,
##               }
##    maximum = None
##    for t in result_ranking(results, maximum):
##        yield t

def last_monster_killed(end_delay=390, music=None):
    from player import BubPlayer
    for t in exit_board(music=music):
        yield t
    if curboard.bonuslevel:
        curboard.playingboard = 1
        for t in bonus_play():
            yield t
            end_delay -= 1
            if end_delay <= 0:
                replace_boardgen(next_board(), 1)
                break
    else:
        for i in range(end_delay):
            yield normal_frame()
        replace_boardgen(next_board(), 1)

##def bonus_play():
##    from player import BubPlayer
##    import bubbles
##    while BubPlayer.LimitScoreColor is None:
##        yield normal_frame()
##        players = [(p.points, p.pn) for p in BubPlayer.PlayerList
##                   if p.isplaying()]
##        if players:
##            players.sort()
##            points, BubPlayer.LimitScoreColor = players[-1]
##            BubPlayer.LimitScore = ((points + limit) // 100000) * 100000
##    for p in BubPlayer.PlayerList:
##        if p.isplaying():
##            p.givepoints(0)  # check LimitScore and update scoreboard()
##    while not (BubPlayer.BubblesBecome or BubPlayer.MegaBonus):
##        if random.random() < 0.06:
##            bubbles.newbonusbubble()
##        yield normal_frame()
##    # special board end
##    import monsters
##    monsters.argh_em_all()
##    replace_boardgen(last_monster_killed(), 1)

class TimeCounter(Copyable):
    def __init__(self, limittime, blink=0):
        from player import BubPlayer
        self.saved_time = BubPlayer.LimitTime
        self.time = limittime / FRAME_TIME
        self.prev = None
        self.blink = blink
    def update(self, t):
        from player import BubPlayer, scoreboard
        self.time -= t
        if self.time < 0.0:
            self.time = 0.0
        BubPlayer.LimitTime = self.time * FRAME_TIME
        next = int(BubPlayer.LimitTime)
        if self.blink and BubPlayer.LimitTime - next >= 0.5:
            BubPlayer.LimitTime = next = None
        if self.prev != next:
            scoreboard(compresslimittime=1)
            self.prev = next
    def restore(self):
        from player import BubPlayer
        BubPlayer.LimitTime = self.saved_time

def bonus_play():
    from player import BubPlayer
    import bubbles
    BubPlayer.MegaBonus = None
    BubPlayer.BubblesBecome = None
    Time0 = 5.0 / FRAME_TIME  # when to slow down time
    tc = TimeCounter(BubPlayer.LimitTime or 180.9)   # 3:00
    prev = None
    while not (BubPlayer.BubblesBecome or BubPlayer.MegaBonus):
        if random.random() < 0.099:
            bubbles.newbonusbubble()
        t = normal_frame()
        tc.update(t)
        if tc.time < Time0:
            if tc.time <= 0.5:
                tc.time = 0.5
                BubPlayer.LimitTime = 0.0
            t *= math.sqrt(Time0 / tc.time)
        yield t
        if tc.time == 0.5:
            gamesrv.game.End = 'gameover'
            gamesrv.game.updateboard()
            replace_boardgen(game_over(), 1)
            return
    # special board end
    import monsters
    monsters.argh_em_all()
    replace_boardgen(last_monster_killed(), 1)

def game_over():
    yield force_singlegen()
    from player import scoreboard
    import ranking
    images.Snd.Extralife.play()
    gamesrv.set_musics([], [images.music_potion])
    scoreboard()
    for t in ranking.game_over():
        yield t

def game_reset():
    import time
    from player import BubPlayer
    t1 = time.time()
    while 1:
        yield 0
        if BubPlayer.LimitTime and BubPlayer.LimitTime >= 1.0:
            # someone else ticking the clock, try again later
            return
        if abs(time.time() - t1) > 2.0:
            break
    # anyone playing ?
    if not gamesrv.game.End:
        return  # yes -> cancel game_reset()
    # let's tick the clock !
    tc = TimeCounter(60.9, blink=1)   # 1:00
    t1 = time.time()
    while tc.time:
        yield 0
        # anyone playing now ?
        if not gamesrv.game.End:
            tc.restore()
            return  # yes -> cancel game_reset()
        t = time.time()  # use real time
        deltat = (t-t1)/FRAME_TIME
        if deltat < 1.0:
            deltat = 1.0
        elif deltat > 100.0:
            deltat = 100.0
        tc.update(deltat)
        t1 = t
    gamesrv.game.reset()

##def wasting_play():
##    from player import BubPlayer, scoreboard
##    import bubbles
##    curboard.wastingplay = {}
##    for p in BubPlayer.PlayerList:
##        if p.isplaying():
##            p.letters = {}
##            p.bonbons = p.points // 50000
##    scoreboard()
    
##    while len(BubPlayer.DragonList) > 1:
##        if random.random() < 0.03:
##            bubbles.newbubble(1)
##        yield normal_frame()
##    for d in BubPlayer.DragonList:
##        curboard.wastingplay[d.bubber] = len(curboard.wastingplay)
##    for i in range(50):
##        yield normal_frame()

##    total = len(curboard.wastingplay)
##    results = [(total-n, p) for p, n in curboard.wastingplay.items()]
##    results.sort()
##    results = [(p, str(n)) for n, p in results]
##    for t in display_ranking(results):
##        yield t
##    # never ending

def skiplevels(blink, skip):
    # (not used any more)
    saved = BoardGen[:]
    while skip:
        skip -= 1
        BoardGen[:] = saved
        for i in range(10):  # frozen pause
            yield 3
            if blink:
                blink.step(-bwidth, 0)
                yield 3.33
                blink.step(bwidth, 0)
        blink = None
        for t in next_board(complete=(skip==0)):
            yield t

def exit_board(delay=8, music=None, repeatmusic=[]):
    from bubbles import Bubble
    from bonuses import RandomBonus, end_normal_play
    from player import BubPlayer
    from monsters import Monster
    end_normal_play()
    curboard.playingboard = 0
    actives = images.ActiveSprites[:]
    for s in actives:
        if ((isinstance(s, Monster) and s.still_playing())
            or isinstance(s, RandomBonus)):
            s.kill()
    music = music or []
    if BubPlayer.MegaBonus:
        music[:1] = [images.music_modern]
    if music or repeatmusic:
        gamesrv.set_musics(music, repeatmusic)
    for i in range(delay):
        yield normal_frame()
    bubble_outcome = BubPlayer.BubblesBecome or Bubble.pop
    for s in actives:
        if isinstance(s, Bubble):
            bubble_outcome(s)
            yield normal_frame()
    if BubPlayer.MegaBonus:
        BubPlayer.MegaBonus()

def potion_fill(blist, big=0):
    from player import BubPlayer
    from bonuses import Bonus
    #timeleft = 1680.0
    for t in exit_board(0, music=[images.music_potion]):
        #timeleft -= t
        yield t
    notes = all_notes = []
    y = 1
    while y < 11 or (y < height-2 and (len(all_notes) < 10 or big)):
        for x in range(2, width-3, 2):
            if ' ' == bget(x,y) == bget(x+1,y) == bget(x,y+1) == bget(x+1,y+1):
                b = Bonus(x*CELL, y*CELL, falling=0, *blist[((x+y)//2)%len(blist)])
                b.timeout = (444,666)[big]
                all_notes.append(b)
        for i in range(2):
            t = normal_frame()
            #timeleft -= t
            yield t
        y += 2
    while notes: #and timeleft > 0.0:
        notes = [b for b in notes if b.alive]
        t = normal_frame()
        #timeleft -= t
        yield t
    for i in range(10):
        t = normal_frame()
        #timeleft -= t
        yield t
    results = {}
    for b in all_notes:
        for d in b.taken_by:
            bubber = d.bubber
            results[bubber] = results.get(bubber, 0) + 1
    for t in result_ranking(results, len(all_notes)):
        yield t
    #fadeouttime = 3.33
    #fullsoundframes = bonusframes - 10 - int(fadeouttime / FRAME_TIME)
    #for i in range(fullsoundframes):
    #    yield normal_frame()
    #gamesrv.fadeout(fadeouttime)
    #for i in range(fullsoundframes, 490):
    #    yield normal_frame()

def result_ranking(results, maximum=None, timeleft=200):
    import ranking
    results = ranking.ranking_picture(results, maximum, timeleft is not None)
    if curboard.bonuslevel and timeleft is not None:
        play_again = bonus_play()
    else:
        play_again = None
    for t in ranking.display(results, timeleft, play_again):
        yield t
    if gamesrv.game.End != 'gameover':
        gamesrv.set_musics([], [])
        replace_boardgen(next_board(), 1)

def extra_water_flood():
    from mnstrmap import Flood
    from monsters import Monster
    waves_icons = [images.sprget(n) for n in Flood.waves]
    fill_icon = images.sprget(Flood.fill)
    bspr = []
    if 'flood' in curboard.sprites:
        return    # only one flooding at a time
    curboard.sprites['flood'] = bspr
    waves_sprites = [gamesrv.Sprite(waves_icons[0], x, bheight-CELL)
                     for x in range(0, bwidth, CELL)]
    bspr += waves_sprites
    fill_by_line = []
    poplist = [None]
    while waves_sprites[0].y > 0:
        yield 0
        waves_icons.insert(0, waves_icons.pop())
        for s in waves_sprites:
            s.seticon(waves_icons[0])
        yield 0
        sprites = [gamesrv.Sprite(fill_icon, s.x, s.y) for s in waves_sprites]
        bspr += sprites
        fill_by_line.append(sprites)
        for s in waves_sprites:
            s.step(0, -16)
        for s in images.touching(0, waves_sprites[0].y, bwidth, bheight):
            if isinstance(s, Monster):
                s.argh(poplist)
    while 1:
        for i in range(2):
            yield 0
            waves_icons.insert(0, waves_icons.pop())
            for s in waves_sprites:
                s.seticon(waves_icons[0])
        if not fill_by_line:
            break
        for s in fill_by_line.pop():
            s.kill()
        for s in waves_sprites:
            s.step(0, 16)
    for s in waves_sprites:
        s.kill()
    del curboard.sprites['flood']

def extra_aquarium():
    from mnstrmap import Flood
    from player import BubPlayer
    for i in range(200):
        if 'flood' not in curboard.sprites:  # only one flooding at a time
            break
        yield 0
        if curboard.cleaning_gen_state:
            return
    else:
        return
    curboard.sprites['flood'] = []
    gl = curboard.sprites.setdefault('background', [])
    curboard.holes = True     # so that random PlainBubbles show up anyway
    walls = curboard.sprites['walls']
    seen = {}

    def newsprite(ico, x, y):
        s = gamesrv.Sprite(ico, x, y)
        s.to_back(walls[0])
        gl.append(s)
        return s

    def fishplayers(ymin):
        for d in BubPlayer.DragonList:
            if d not in seen and d.y >= ymin:
                seen[d] = True
                d.become_fish()
                d.bubber.emotic(d, 4)

    waves_icons = [images.sprget(n) for n in Flood.waves]
    fill_icon = images.sprget(Flood.fill)
    waves_sprites = [newsprite(waves_icons[0], x, bheight-CELL)
                     for x in range(2*CELL + HALFCELL, bwidth - 2*CELL, CELL)]
    while waves_sprites[0].y > -fill_icon.h:
        fishplayers(waves_sprites[0].y)
        yield 0
        waves_icons.append(waves_icons.pop(0))
        for s in waves_sprites:
            s.seticon(waves_icons[0])
        fishplayers(waves_sprites[0].y)
        yield 0
        for s in waves_sprites:
            newsprite(fill_icon, s.x, s.y)
        for s in waves_sprites:
            s.step(0, -16)
    for s in waves_sprites:
        s.kill()
    BubPlayer.SuperFish = True
    fishplayers(-sys.maxsize)

def extra_walls_falling():
    walls_by_pos = curboard.walls_by_pos
    moves = 1
    while moves and not curboard.cleaning_gen_state:
        moves = 0
        for y in range(height-3, -1, -1):
            for x in range(2, width-2):
                if ((y,x) in walls_by_pos and
                    (y+1,x) not in walls_by_pos and
                    (y+2,x) not in walls_by_pos):
                    y0 = y
                    while (y0-1,x) in walls_by_pos:
                        y0 -= 1
                    w = curboard.killwall(x, y0, 0)
                    curboard.putwall(x, y+1, w)
                    moves = 1
        curboard.reorder_walls()
        for y in range(6):
            yield 0

def single_blocks_falling(xylist):
    walls_by_pos = curboard.walls_by_pos
    while xylist:
        newlist = []
        for x, y in xylist:
            if ((y,x) in walls_by_pos and (y+1,x) not in walls_by_pos and
                y < curboard.height-1):
                newlist.append((x, y+1))
        for x, y in newlist:
            w = curboard.killwall(x, y-1, 0)
            curboard.putwall(x, y, w)
        xylist = newlist
        curboard.reorder_walls()
        for i in range(7):
            yield 0

def extra_display_repulse(cx, cy, dlimit=5000, dfactor=1000):
    offsets = {}
    for s in list(gamesrv.sprites_by_n.values()):
        x, y = s.getdisplaypos()
        if x is not None:
            dx = x - cx
            dy = y - cy
            d = dx*dx + dy*dy + 100
            if d <= dlimit:
                dx = (dx*dfactor)//d
                dy = (dy*dfactor)//d
                offsets[s] = dx, dy
                s.setdisplaypos(int(x+dx), int(y+dy))
    yield 0
    yield 0
    while offsets:
        prevoffsets = offsets
        offsets = {}
        for s, (dx, dy) in list(prevoffsets.items()):
            if s.alive:
                if dx < 0:
                    dx += max(1, (-dx)//5)
                elif dx:
                    dx -= max(1, dx//5)
                if dy < 0:
                    dy += max(1, (-dy)//5)
                elif dy:
                    dy -= max(1, dy//5)
                if dx or dy:
                    offsets[s] = dx, dy
                s.setdisplaypos(int(s.x+dx), int(s.y+dy))
        yield 0

def extra_bkgnd_black(cx, cy):
    gl = curboard.sprites.get('background')
    dist = 0
    while gl:
        dist += 17
        dist2 = dist * dist
        gl2 = []
        for s in gl:
            if (s.x-cx)*(s.x-cx) + (s.y-cy)*(s.y-cy) < dist2:
                s.kill()
            else:
                gl2.append(s)
        gl[:] = gl2
        yield 0

def extra_light_off(timeout, icocache={}):
    for i in range(timeout):
        if curboard.cleaning_gen_state:
            break
        dragons = {}
        import player
        playerlist = player.BubPlayer.PlayerList
        for bubber in playerlist:
            for dragon in bubber.dragons:
                dragons[dragon] = True
        for s in list(gamesrv.sprites_by_n.values()):
            try:
                ico = icocache[s.ico, s in dragons]
            except KeyError:
                ico = images.make_darker(s.ico, s in dragons)
                icocache[s.ico, s in dragons] = ico
            s.setdisplayicon(ico)
        yield 0
    for s in list(gamesrv.sprites_by_n.values()):
        s.setdisplayicon(s.ico)

def extra_swap_up_down(N=27):
    # unregister all walls
    walls = list(curboard.walls_by_pos.items())
    walls.sort()
    if not walls:
        return
    curboard.walls_by_pos.clear()
    emptyline = '##' + ' '*(width-4) + '##'
    curboard.walls = [emptyline] * height
    l = curboard.sprites['walls']
    wallicon = l[0].ico
    wallpool = l[:]
    l[:] = [gamesrv.Sprite(wallicon, 0, -wallicon.h)]
    
    # force the top half of the walls on front
    #for (y,x), w in walls:
    #    if y*2 < height:
    
    # show the walls swapping up/down
    ycenter = ((height-1)*CELL) // 2
    for i in range(N):
        alpha = math.cos((math.pi*(i+1))/N)
        ymap = {}
        for y in range(height):
            ymap[y] = int(alpha*(y*CELL-ycenter)) + ycenter
        for (y,x), w in walls:
            if y in ymap:
                w.move(x*CELL, ymap[y])
        yield 0
        if i == (N+1)//2:
            # reorder the wall sprites in the middle of the swap
            walls = [((-y,x), w) for (y,x), w in walls]
            walls.sort()
            for i in range(len(walls)):
                (y,x), w = walls[i]
                walls[i] = (y,x), wallpool[i]
            walls = [((-y,x), w) for (y,x), w in walls]
            walls.sort()
            # reverse all dragons!
            from player import BubPlayer
            for dragon in BubPlayer.DragonList:
                dragon.dcap['gravity'] *= -1.0
    
    # freeze the walls in their new position
    i = 0
    for (y,x), w in walls:
        y = height-1 - y
        if 0 <= y < height and (y,x) not in curboard.walls_by_pos:
            w = wallpool[i]
            i += 1
            curboard.putwall(x, y, w)
    l[:0] = wallpool[:i]
    for w in wallpool[i:]:
        w.kill()
    curboard.reorder_walls()

def extra_catch_all_monsters(dragons=[], everything=False):
    from monsters import Monster
    from bubbles import BigBubbleCatcher
    from bonuses import Bonus, BonusMaker
    if not dragons:
        from player import BubPlayer
        dragons = BubPlayer.DragonList
    i = 0
    give_up = 33
    for s in images.ActiveSprites[:]:
        if curboard.cleaning_gen_state:
            break
        while not dragons:
            give_up -= 1
            if give_up == 0:
                return
            yield 0
        if not s.alive or not s.touchable:
            continue
        if isinstance(s, Bonus):
            ok = s.bubblable
        elif isinstance(s, BonusMaker):
            ok = everything
        else:
            ok = isinstance(s, Monster)
        if ok:
            dragon = dragons[i%len(dragons)]
            BigBubbleCatcher(dragon, s, 542 + 17*i)
            i += 1
            yield 0
            yield 0

def extra_make_random_level(cx=None, cy=None, repeat_delay=200):
    from bonuses import DustStar
    # generate any random level
    localdir = os.path.dirname(__file__)
    filename = os.path.join(localdir, 'levels', 'RandomLevels.py')
    d = {}
    exec(compile(open(filename, "rb").read(), filename, 'exec'), d)
    Level = d['GenerateSingleLevel'](curboard.width, curboard.height)
    lvl = Level(curboard.num)
    walllist = []
    if cx is None: cx = bwidth // 2
    if cy is None: cy = bheight // 2
    for y in range(curboard.height):
        dy = cy - (y*16+8)
        dy2 = dy*dy*0.75
        for x in range(2, curboard.width-2):
            dx = cx - (x*16+8)
            d2 = dx*dx + dy2
            walllist.append((d2, x, y))
    walllist.sort()

    # dynamically replace the current level's walls with the new level's
    dist = 0
    speedf = 15.0
    added = 0
    for d2, x, y in walllist:
        while d2 > dist*dist:
            if added:
                curboard.reorder_walls()
                added = 0
            yield 0
            dist += 4.1
            speedf *= 0.99
        if curboard.walls[y][x] == ' ':
            if lvl.walls[y][x] == ' ':
                continue
            else:
                curboard.putwall(x, y)
                added = 1
                big = 1
        else:
            if lvl.walls[y][x] == ' ':
                curboard.killwall(x, y)
                big = 1
            else:
                big = 0
        sx = x*16
        sy = y*16
        DustStar(sx - 8*big,
                 sy - 8*big,
                 speedf * (sx-cx) / dist,
                 speedf * (sy-cy) / dist,
                 big=big)
    # patch the winds too
    curboard.winds = lvl.winds
    curboard.reorder_walls()
    yield 0
    # wait a bit and restart
    if repeat_delay < 1000:
        for i in range(repeat_delay):
            yield 0
            if curboard.cleaning_gen_state:
                return
        extra_boardgen(extra_make_random_level(
            repeat_delay = repeat_delay * 3 // 2))

def extra_bubbles(timeout):
    from bubbles import newforcedbubble
    falloff = 0.25
    L = math.log(0.965)     # same speed falloff rate as in throwing_bubble()
    cx = (bwidth - CELL) // 2
    cy = (bheight - CELL) // 2
    for i in range(timeout):
        if curboard.cleaning_gen_state:
            return
        if random.random() < falloff:
            bubble = newforcedbubble()
            if bubble:
                tx = random.randrange(CELL, bwidth - 2*CELL) - cx
                ty = random.randrange(CELL, bheight - 2*CELL) - cy
                if ty == 0:
                    ty = 1
                dist = math.sqrt(tx * tx + ty * ty)
                acos = tx / dist
                asin = ty / dist
                hspeed = 4 - dist * L
                bubble.thrown_bubble(cx, cy, hspeed, (acos, asin))
        falloff *= 0.998
        yield 0


def initsubgame(music, displaypoints):
    from player import BubPlayer, scoreboard
    for t in exit_board(0, repeatmusic=[music]):
        yield t
    BubPlayer.DisplayPoints = displaypoints
    scoreboard()
    for t in curboard.clean_gen_state():
        yield t

def register(dict):
    global width, height, bwidth, bheight, bheightmod
    items = list(dict.items())
    items.sort()
    for name, board in items:
        try:
            if not issubclass(board, Board) or board is Board:
                continue
        except TypeError:
            continue
        BoardList.append(board)
    # check sizes
    assert BoardList, "board file does not define any board"
    B = BoardList[0]
    try:
        test = B(-1)
        width = test.width
        height = test.height
        for B in BoardList[1:]:
            test = B(-1)
            assert test.width == width, "some boards have a different width"
            assert test.height == height, "some boards have a different height"
    except Exception as e:
        print('Caught "%s" in level "%s":' % (e, B.__name__))
        raise e
    bwidth = width*CELL
    bheight = height*CELL
    bheightmod = (height+2)*CELL

##def define_boards(filename):
##    global curboard, boards, width, height, bwidth, bheight, bheightmod
##    curboard = None
##    boards = []
##    def board((wallfile, wallrect), shape):
##        lines = shape.strip().split('\n')
##        bmp = gamesrv.getbitmap(wallfile)
##        wallicon = bmp.geticon(*wallrect)
##        boards.append(Board(lines, wallicon))
##    d = {'board': board}
##    execfile(filename, d)
##    assert boards, "board file does not define any board"
##    width = boards[0].width
##    height = boards[0].height
##    for b in boards[1:]:
##        assert b.width == width, "some boards have a different width"
##        assert b.height == height, "some boards have a different height"
##    bwidth = width*CELL
##    bheight = height*CELL
##    bheightmod = len(boards[0].lines)*CELL


#try:
#    import psyco
#except ImportError:
#    pass
#else:
#    psyco.bind(normal_frame)
