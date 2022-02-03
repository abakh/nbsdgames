#! /usr/bin/env python



# __________
import os, sys
if __name__ == '__main__':
    LOCALDIR = sys.argv[0]
else:
    LOCALDIR = __file__
try:
    LOCALDIR = os.readlink(LOCALDIR)
except:
    pass
LOCALDIR = os.path.dirname(os.path.abspath(LOCALDIR))
# ----------

import random, time

sys.path.insert(0, os.path.join(os.path.dirname(LOCALDIR), 'common'))
sys.path.insert(0, LOCALDIR)
import gamesrv

PROFILE = 0


class BubBobGame(gamesrv.Game):

    FnDesc = "Bub & Bob"
    FnBasePath = "bubbob"
    Quiet = 0
    End = 0

    def __init__(self, levelfile,
                 beginboard  = 1,
                 stepboard   = 1,
                 finalboard  = 100,
                 limitlives  = None,
                 extralife   = 50000,
                 lifegainlimit = None,
                 autoreset   = 0,
                 metaserver  = 0,
                 monsters    = 0):
        gamesrv.Game.__init__(self)
        self.game_reset_gen = None
        self.levelfile  = levelfile
        self.beginboard = beginboard
        self.finalboard = finalboard
        self.stepboard  = stepboard
        self.limitlives = limitlives
        self.lifegainlimit = lifegainlimit
        self.extralife  = extralife
        self.autoreset  = autoreset
        self.metaserver = metaserver
        self.f_monsters = monsters
        self.updatemetaserver()
        levelsname, ext = os.path.splitext(os.path.basename(levelfile))
        self.FnDesc     = BubBobGame.FnDesc + ' ' + levelsname
        self.reset()
        self.openserver()

    def openboard(self, num=None):
        self.End = 0
        if num is None:
            num = self.beginboard-1
        import boards
        boards.loadmodules(force=1)
        import boards   # possibly re-import
        self.width = boards.bwidth + 9*boards.CELL
        self.height = boards.bheight
        boards.curboard = None
        boards.BoardGen = [boards.next_board(num)]
        self.updatemetaserver()

    def reset(self):
        import player
        self.openboard()
        for p in player.BubPlayer.PlayerList:
            p.reset()

    def FnPlayers(self):
        from player import BubPlayer
        result = {}
        for p in BubPlayer.PlayerList:
            result[p.pn] = p
        return result

    def FnFrame(self):
        if self.metaregister:
            self.do_updatemetaserver()
        frametime = 0.0
        for i in range(500):
            import boards
            for gen in boards.BoardGen[:]:
                try:
                    frametime += next(gen)
                except StopIteration:
                    try:
                        boards.BoardGen.remove(gen)
                    except ValueError:
                        pass
            if frametime >= 1.1:
                break
        else:
            # should normally never occur
            boards.BoardGen[:] = [boards.next_board()]
            frametime = 1.0
        if self.game_reset_gen is None:
            if self.End and self.autoreset:
                self.game_reset_gen = boards.game_reset()
        else:
            try:
                next(self.game_reset_gen)
            except StopIteration:
                self.game_reset_gen = None
        return frametime * boards.FRAME_TIME

    def FnServerInfo(self, msg):
        try:
            from images import writestr
            writestr(50, 50, msg)
            self.sendudpdata()
        except:
            pass

    def FnExcHandler(self, kbd):
        if kbd:
            self.FnServerInfo("Server was Ctrl-C'ed!")
        else:
            self.FnServerInfo('Ooops -- server crash!')
        from player import BubPlayer
        if kbd and not [p for p in BubPlayer.PlayerList if p.isplaying()]:
            return 0
        import traceback
        print("-"*60)
        traceback.print_exc()
        print("-"*60)
        if not kbd:
            try:
                if self.metaserver:
                    try:
                        import metaclient
                    except ImportError:
                        pass
                    else:
                        if metaclient.metaclisrv:
                            metaclient.metaclisrv.send_traceback()
            except Exception as e:
                print('! %s: %s' % (e.__class__.__name__, e))
        import boards
        num = getattr(boards.curboard, 'num', None)
        if self.Quiet:
            print("Crash recovery! Automatically restarting board %s" % num)
            import time; time.sleep(2)
        else:
            print("Correct the problem and leave pdb to restart board %s..."%num)
            import pdb; pdb.post_mortem(sys.exc_info()[2])
        self.openboard(num)
        return 1

    def FnListBoards():
        import boards
        result = []
        for fn in os.listdir('levels'):
            base, ext = os.path.splitext(fn)
            if ext in ('.py', '.bin'):
                result.append((base, os.path.join('levels', fn)))
        return result
    FnListBoards = staticmethod(FnListBoards)

    def FnExtraDesc(self):
        import boards
        s = gamesrv.Game.FnExtraDesc(self)
        if boards.curboard and self.End != 'gameover':
            s = '%s, board %d' % (s, boards.curboard.num+1)
        return s

    def do_updatemetaserver(self):
        self.metaregister -= 1
        if self.metaregister > 0:
            return
        if self.metaserver and (self.autoreset or self.End != 'gameover'):
            setuppath('metaserver')
            import metaclient
            metaclient.meta_register(self)
            print('.')
        else:
            try:
                import metaclient
            except ImportError:
                pass
            else:
                metaclient.meta_unregister(self)

    def updatemetaserver(self):
        self.metaregister = 2

    updateboard = updateplayers = updatemetaserver


def setuppath(dirname):
    dir = os.path.abspath(os.path.join(LOCALDIR, os.pardir, dirname))
    if not os.path.isdir(dir):
        print((
            '../%s: directory not found ("cvs update -d" ?)' % dirname), file=sys.stderr)
        sys.exit(1)
    if dir not in sys.path:
        sys.path.insert(0, dir)

def parse_cmdline(argv):
    # parse command-line
    def usage():
        print('usage:', file=sys.stderr)
        print('  python bb.py', file=sys.stderr)
##        print >> sys.stderr, '  python bb.py [-w/--webbrowser=no]'
##        print >> sys.stderr, 'where:'
##        print >> sys.stderr, '  -w  --webbrowser=no  don''t automatically start web browser'
        print('or:', file=sys.stderr)
        print('  python bb.py [level-file.bin] [-m] [-b#] [-s#] [-l#] [-M#]', file=sys.stderr)
        print('with options:', file=sys.stderr)
        print('  -m  --metaserver  register the server on the Metaserver so anyone can join', file=sys.stderr)
        print('  -b#  --begin #    start at board number # (default 1)', file=sys.stderr)
        print('       --start #    synonym for --begin', file=sys.stderr)
        print('       --final #    end at board number # (default 100)', file=sys.stderr)
        print('  -s#  --step #     advance board number by steps of # (default 1)', file=sys.stderr)
        print('  -l#  --lives #    limit the number of lives to #', file=sys.stderr)
        print('       --extralife #    gain extra life every # points', file=sys.stderr)
        print('       --limitlives #    max # of lives player can gain in one board', file=sys.stderr)
        print('  -M#  --monsters # multiply the number of monsters by #', file=sys.stderr)
        print('                      (default between 1.0 and 2.0 depending on # of players)', file=sys.stderr)
        print('  -i   --infinite   restart the server at the end of the game', file=sys.stderr)
        print('  --port LISTEN=#   set fixed tcp port for game server', file=sys.stderr)
        print('  --port HTTP=#     set fixed tcp port for http server', file=sys.stderr)
        print('  -h   --help       display this text', file=sys.stderr)
        #print >> sys.stderr, '  -rxxx record the game in file xxx'
        sys.exit(1)

    try:
        from getopt import gnu_getopt as getopt
    except ImportError:
        from getopt import getopt
    from getopt import error
    try:
        opts, args = getopt(argv, 'mb:s:l:M:ih',
                            ['metaserver', 'start=', 'step=',
                             'lives=', 'monsters=', 'infinite', 'help',
                             'extralife=', 'limitlives=', 'final=',
                             'saveurlto=', 'quiet', 'port=', 'makeimages'])
    except error as e:
        print('bb.py: %s' % str(e), file=sys.stderr)
        print(file=sys.stderr)
        usage()
        
    options = {}
    #webbrowser = 1
    save_url_to = None
    quiet = 0
    for key, value in opts:
        if key in ('-m', '--metaserver'):
            options['metaserver'] = 1
        elif key in ('-b', '--start', '--begin'):
            options['beginboard'] = int(value)
        elif key in ('-s', '--step'):
            options['stepboard'] = int(value)
        elif key in ('-l', '--lives'):
            options['limitlives'] = int(value)
        elif key in ('--limitlives'):
            options['lifegainlimit'] = int(value)
        elif key in ('--final'):
            options['finalboard'] = int(value)
            if options['finalboard'] < options['beginboard']:
                print('bb.py: final board value must be larger than begin board.', file=sys.stderr)
                sys.exit(1)
        elif key in ('--extralife'):
            options['extralife'] = int(value)
        elif key in ('-M', '--monsters'):
            options['monsters'] = float(value)
        elif key in ('-i', '--infinite'):
            options['autoreset'] = 1
        elif key in ('-h', '--help'):
            usage()
        elif key == '--saveurlto':
            save_url_to = value
        elif key == '--quiet':
            quiet = 1
        elif key == '--port':
            portname, portvalue = value.split('=')
            portvalue = int(portvalue)
            import msgstruct
            msgstruct.PORTS[portname] = portvalue
        elif key == '--makeimages':
            import images
            sys.exit(0)
        #elif key in ('-w', '--webbrowser'):
        #    webbrowser = value.startswith('y')
    if args:
        if len(args) > 1:
            print('bb.py: multiple level files specified', file=sys.stderr)
            sys.exit(1)
        levelfile = os.path.abspath(args[0])
        os.chdir(LOCALDIR)
        BubBobGame(levelfile, **options)
    else:
        if options:
            print('bb.py: command-line options ignored', file=sys.stderr)
        start_metaserver(save_url_to, quiet)

def start_metaserver(save_url_to, quiet):
    os.chdir(LOCALDIR)
    setuppath('http2')
    import httppages
    httppages.main(BubBobGame, save_url_to, quiet)


def setup():
    keybmp = gamesrv.getbitmap(os.path.join('images', 'keys.ppm'))
    def keybmplist(x):
        return [keybmp.geticon(x, y, 32, 32) for y in range(0, 128, 32)]
    BubBobGame.FnKeys = [
        ("right",  keybmplist(0),   "kRight"),
        ("left",   keybmplist(32),  "kLeft"),
        ("jump",   keybmplist(64),  "kJump"),
        ("fire",   keybmplist(96),  "kFire"),
        ("-right", [],              "kmRight"),
        ("-left",  [],              "kmLeft"),
        ("-jump",  [],              "kmJump"),
        ("-fire",  [],              "kmFire"),
        ]

setup()

def main():
    parse_cmdline(sys.argv[1:])
    if not PROFILE:
        gamesrv.mainloop()
    else:
        import profile
        prof = profile.Profile()
        try:
            prof = prof.run('gamesrv.mainloop()')
        finally:
            prof.dump_stats('profbb')

if __name__ == '__main__':
    main()
