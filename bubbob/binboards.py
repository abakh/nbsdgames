import macbinary
import boards
import gamesrv
from mnstrmap import Nasty, Monky, Ghosty, Flappy
from mnstrmap import Springy, Orcy, Gramy, Blitzy
from images import KEYCOL

keycol = (KEYCOL & 0xFF,
          (KEYCOL>>8) & 0xFF,
          (KEYCOL>>16) & 0xFF)


def meancolor(img):
    r1 = g1 = b1 = 0
    count = float(len(img) * len(img[0]))
    for line in img:
        for r, g, b in line:
            r1 += r
            g1 += g
            b1 += b
    return r1/count, g1/count, b1/count

def addshadow(img, (r1, g1, b1), depth=8):
    w = len(img[0])
    h = len(img)
    pad = depth * [keycol]
    result = [line + pad for line in img] + [
        (w+depth) * [keycol] for d in range(depth)]
    for d in range(depth):
        f = 1.0 - float(d)/depth
        color = (r1 * f, g1 * f, b1 * f)
        for i in range(w):
            result[h+d][1+d+i] = color
        for i in range(h):
            result[1+d+i][w+d] = color
    return result

def addrshadow(img, (r1, g1, b1), depth=8):
    w = len(img[0])
    h = len(img)
    pad = depth * [keycol]
    result = [line + pad for line in img]
    for d in range(depth):
        f = 1.0 - float(d)/depth
        color = (r1 * f, g1 * f, b1 * f)
        for i in range(h):
            result[i][w+d] = color
    return result


def load(filename):
    print "Loading %s..." % filename
    Bin = macbinary.MacBinary(filename)
    levels = {}
    mnstrlist = [Nasty, Monky, Ghosty, Flappy,
                 Springy, Orcy, Gramy, Blitzy]
    
    for key, lvl in Bin['LEVL'].items():
        d = lvl.getlevel(mnstrlist)
        class BinBoard(boards.Board):
            pass
        for key1, value1 in d.items():
            setattr(BinBoard, key1, value1)
        levels[key] = BinBoard

    def loader(code, rsrc=Bin['ppat'], cache={}):
        try:
            return cache[code]
        except KeyError:
            pass
        keycol1 = None
        bid = code[0]
        result = None
        if code[1] == 'l':
            # left border wall
            img = rsrc[bid + 228].getimage()
            color = meancolor(img)
            img = [line[:32] for line in img]
            result = addrshadow(img, color)
        elif code[1] == 'r':
            # right border wall
            img = rsrc[bid + 228].getimage()
            w = len(img[0])
            assert w in (32, 64), bid
            if w == 64:
                color = meancolor(img)
                img = [line[32:64] for line in img]
                result = addrshadow(img, color)
        else:
            # normal wall
            dx, dy = code[1:]
            img = rsrc[bid + 128].getimage()
            w = len(img[0])
            h = len(img)
            assert w & 15 == h & 15 == 0, bid
            dx *= 16
            dy *= 16
            if dx < w and dy < h:
                color = meancolor(img)
                img = [line[dx:dx+16] for line in img[dy:dy+16]]
                result = addshadow(img, color)
                keycol1 = KEYCOL
        if result is not None:
            w, h, data = macbinary.image2rgb(result)
            ppmdata = "P6\n%d %d\n255\n%s" % (w, h, data)
            result = gamesrv.newbitmap(ppmdata, keycol1), (0, 0, w, h)
        cache[code] = result
        return result

    def bin_haspat(code, loader=loader):
        try:
            return loader(code) is not None
        except KeyError:
            return 0
    def bin_loadpattern(code, keycol=None, loader=loader):
        result = loader(code)
        assert result is not None, code
        return result

    boards.haspat = bin_haspat
    boards.loadpattern = bin_loadpattern
    return levels
