from __future__ import generators
import cStringIO

def decodepixmap(data):
    f = cStringIO.StringIO(data)
    sig = f.readline().strip()
    assert sig == "P6"
    while 1:
        line = f.readline().strip()
        if not line.startswith('#'):
            break
    wh = line.split()
    w, h = map(int, wh)
    sig = f.readline().strip()
    assert sig == "255"
    data = f.read()
    f.close()
    return w, h, data

def encodepixmap(w, h, data):
    return 'P6\n%d %d\n255\n%s' % (w, h, data)

def cropimage((w, h, data), (x1, y1, w1, h1)):
    assert 0 <= x1 <= x1+w1 <= w
    assert 0 <= y1 <= y1+h1 <= h
    scanline = w*3
    lines = [data[p:p+w1*3]
             for p in range(y1*scanline + x1*3,
                            (y1+h1)*scanline + x1*3,
                            scanline)]
    return w1, h1, ''.join(lines)

def vflip(w, h, data):
    scanline = w*3
    lines = [data[p:p+scanline] for p in range(0, len(data), scanline)]
    lines.reverse()
    return ''.join(lines)

def hflip(w, h, data):
    scanline = w*3
    lines = [''.join([data[p:p+3] for p in range(p1+scanline-3, p1-3, -3)])
             for p1 in range(0, len(data), scanline)]
    return ''.join(lines)

def rotate_cw(w, h, data):
    scanline = w*3
    lastline = len(data) - scanline
    lines = [''.join([data[p:p+3] for p in range(lastline + p1, -1, -scanline)])
             for p1 in range(0, scanline, 3)]
    return ''.join(lines)

def rotate_ccw(w, h, data):
    scanline = w*3
    lines = [''.join([data[p:p+3] for p in range(p1, len(data), scanline)])
             for p1 in range(scanline-3, -3, -3)]
    return ''.join(lines)

def rotate_180(w, h, data):
    scanline = w*3
    lines = [''.join([data[p:p+3] for p in range(p1+scanline-3, p1-3, -3)])
             for p1 in range(0, len(data), scanline)]
    lines.reverse()
    return ''.join(lines)

def makebkgnd(w, h, data):
    scanline = 3*w
    result = []
    for position in range(0, scanline*h, scanline):
        line = []
        for p in range(position, position+scanline, 3):
            line.append(2 * (chr(ord(data[p  ]) >> 3) +
                             chr(ord(data[p+1]) >> 3) +
                             chr(ord(data[p+2]) >> 3)))
        line = ''.join(line)
        result.append(line)
        result.append(line)
    return w*2, h*2, ''.join(result)

translation_darker = ('\x00\x01' + '\x00'*126 +
                      ''.join([chr(n//4) for n in range(0,128)]))
translation_dragon = translation_darker[:255] + '\xC0'

def make_dark((w, h, data), translation):
    return w, h, data.translate(translation)

def col((r, g, b)):
    r = ord(r)
    g = ord(g)
    b = ord(b)
    return ((g>>2 + r>>3) << 24) | (b << 16) | (g << 8) | r

def imagezoomer(w, h, data):
    "Zoom a cartoon image by a factor of three, progressively."
    scale = 3
    scanline = 3*w
    rw = (w-1)*scale+1
    rh = (h-1)*scale+1
    pixels = []
    colcache = {}
    revcache = {}
    for base in range(0, scanline*h, scanline):
        line = []
        for x in range(w):
            key = data[base + 3*x : base + 3*(x+1)]
            try:
                c = colcache[key]
            except KeyError:
                c = colcache[key] = col(key)
                revcache[c] = key
            line.append(c)
        pixels.append(line)
        yield None

    Pairs = {
        (0, 0): [(0, 0, 0, 0),
                 (-1,0, 1, 0),
                 (0,-1, 0, 1)],
        (1, 0): [(0, 0, 1, 0),
                 (0, 1, 1,-1),
                 (0,-1, 1, 1)],
        (2, 0): [(0, 0, 1, 0),
                 (0, 1, 1,-1),
                 (0,-1, 1, 1)],
        (0, 1): [(0, 0, 0, 1),
                 (-1,0, 1, 1),
                 (1, 0,-1, 1)],
        (1, 1): [(0, 0, 1, 1),
                 (0, 1, 1, -1),
                 (1, 0,-1, 1)],
        (2, 1): [(1, 0, 0, 1),
                 (0,-1, 1, 1),
                 (0, 0, 2, 1)],
        (0, 2): [(0, 0, 0, 1),
                 (-1,0, 1, 1),
                 (1, 0,-1, 1)],
        (1, 2): [(0, 1, 1, 0),
                 (-1,0, 1, 1),
                 (0, 0, 1, 2)],
        (2, 2): [(0, 0, 1, 1),
                 (0, 1, 2, 0),
                 (1, 0, 0, 2)],
        }
    result = []
    for y in range(rh):
        yield None
        for x in range(rw):
            # ______________________________

            i = x//scale
            j = y//scale
            ps = []
            for dx1, dy1, dx2, dy2 in Pairs[x%scale, y%scale]:
                if (0 <= i+dx1 < w and 0 <= i+dx2 < w and
                    0 <= j+dy1 < h and 0 <= j+dy2 < h):
                    p1 = pixels[j+dy1][i+dx1]
                    p2 = pixels[j+dy2][i+dx2]
                    ps.append(max(p1, p2))
            p1 = min(ps)

            # ______________________________
            result.append(revcache[p1])
    data = ''.join(result)
    yield (rw, rh, data)
