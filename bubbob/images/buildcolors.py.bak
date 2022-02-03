#! /usr/bin/env python
import sys, os

if __name__ == '__main__':
    ThisDir = sys.argv[0]
else:
    ThisDir = __file__
ThisDir = os.path.dirname(os.path.abspath(ThisDir))

### rotate colors
import colorsys
COLORS = [#(0, 0.0, 1.0, 1, 1),      # vert
          #(1, 0.0, 1.0, 1, 1),      # bleu
          (1, -0.7, 1.0, 1, 1),      # rose
          (0, -0.2, 1.0, 1, 1),      # brun
          (1,  0.72,1.0,-1, 1),      # jaune
          (0, -0.35,0.85,1, 1),      # rouge
          (0,   0,  0.0, 1, 1),      # gris
          (0, -0.85,  0.9, 1, 1),      # cyan (was mauve)
          #(0, 0.2,  1.0, 1, 1),      # turquoise
          (0, 0.925, 0.95,-1, 1),       # bleu fonce
          #(0, 0.45, 0.5, -0.5, 0.75), # hum
          (1, 'specialpixelmap'),    # vert fonce
          ]
MAX = 2 + len (COLORS)

## By ION:
#
# Here's the new palette-based method.
# 
# It's an array [N][320] of 24bit unsigned integers 
# (where N is the total number of color sets including the original one.)
# 
# That is, you access it like
#
# Palettes[(PALETTESIZE * palettenumber)+paletteindex]
#
# Activate it by passing a palette file as a cmdline argument.
#
# The color mapping could be further sped up 
# by making Palettes an array of bytes rather than ints,
# at the cost of increased complexity (Palettes [ (PALETTESIZE * 3 * palettenumber) + paletteindex + component])
#

Palettes = None # currently there is no 'internal' palette since this is experimental.

PALETTESIZE = 960

PaletteIndex = None
# generate the string:paletteindex lookup table
def initpalettelut ():
    global PaletteIndex
    global COLORS, COLORMAPS
    # palette 0 is the base palette (green dragon, blue tiger)
    #
    # Palette 0 must contain NO duplicate colors.
    PaletteIndex = {}
    for i in range (PALETTESIZE):
	v = Palettes[i]
        #if v & 0xff == 0 and (v >> 8) & 0xff == 0x87 and (v >> 16) & 0xff == 0:
        #    print 'FOUND'
	s = "".join ([chr ((v >> shift) & 0xff) for shift in (0,8,16)])
	PaletteIndex[s] = i
    # invalidate COLORS, but match the length to the number of alt palettes.
    COLORS = range ((len (Palettes) / PALETTESIZE) - 1)
    #print 'COLORS',COLORS
    COLORMAPS = [{} for n in COLORS]
    #print 'COLORMAPS',COLORMAPS

def loadpalettesets (filename):
    global Palettes
    #import array
    #Palettes = array.array ('I')
    Palettes = []
    assert ((os.path.getsize (filename) % (PALETTESIZE * 3)) == 0)
    #print os.path.getsize (filename)
    f = open (filename, 'rb')
    for i in range (os.path.getsize(filename) / (PALETTESIZE * 3)):
        for j in range (PALETTESIZE):
            tmp = f.read (3)
            val = ord (tmp[0]) | (ord (tmp[1]) << 8) | (ord (tmp[2]) << 16)
            Palettes.append (val)
    #debuggest
    #print len(Palettes)
    #print len(Palettes) % PALETTESIZE
    assert (len (Palettes) % PALETTESIZE) == 0
    #print "Palettes len:",len (Palettes)

def inputfiles ():
    InputFiles = {
        os.path.join (ThisDir, os.pardir, 'ext1', 'image1-%d.ppm'): 1,
        os.path.join (ThisDir, os.pardir, 'ext3', 'image1-%d.ppm'): 1,
        os.path.join (ThisDir, os.pardir, 'ext4', 'image1-%d.ppm'): 1,
        os.path.join (ThisDir, os.pardir, 'ext6', 'image1-%d.ppm'): 1,
        os.path.join (ThisDir, os.pardir, 'ext7', 'image1-%d.ppm'): 1,
        }
    d = {}
    execfile (os.path.join(ThisDir, os.pardir, 'sprmap.py'), d)
    sprmap = d['sprmap']
    for key, (filename, rect) in sprmap.items ():
        if filename.find('%d') >= 0:
            InputFiles[os.path.join (ThisDir, filename)] = 1
    return InputFiles.keys ()

# ____________________________________________________________

def pixelmap (r, g, b):
    r /= 255.0
    g /= 255.0
    b /= 255.0
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    h = (h*sign + delta) % 1.0
    s *= sat
    v *= lumen
    r, g, b = colorsys.hsv_to_rgb(h, s, v)
    return r*255.1, g*255.1, b*255.1

def specialpixelmap (r, g, b):
    return r * 0.1, g * 0.7, r * 0.5

usingpalette = 0

def palettepixelmap (r, g, b):
#    print max(r,g,b)
    packed = chr(r) + chr(g) + chr(b)
    try:
	index = PaletteIndex[packed]
        #print 'index %r' % index
#        print 'USING', usingpalette
	v = thispalette[index] #Palettes[(PALETTESIZE * (usingpalette + 1)) + index]
#        print 'hit! %r' % packed
#        print '-> %r' % (chr(v & 0xff) + chr ((v >> 8) & 0xff) + chr((v >> 16) & 0xff))
#        print '%r : %r' % (Palettes[index], Palettes[PALETTESIZE + index])
	return v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff
    except KeyError:
	return r,g,b

def ppmbreak (f):
    sig = f.readline ().strip ()
    assert sig == "P6"
    while 1:
        line = f.readline ().strip ()
        if not line.startswith('#'):
            break
    wh = line.split ()
    w, h = map (int, wh)
    sig = f.readline ().strip()
    assert sig == "255"
    data = f.read ()
    return w, h, data

COLORMAPS = [{} for n in COLORS]
del n

def paletterotate (imglist, chr=chr, int=int, ord=ord):
    global thispalette
    gw, gh, green = imglist[0]
#    assert bw == gw and bh == gh
    n = 0
    (_, _, fromimage) = imglist[0]
    for reserved in COLORS:
        # is not being entered, the fool.
#        lut = {}
#        for         
        thispalette = Palettes[(PALETTESIZE * (reserved + 1)):(PALETTESIZE * (reserved + 2))]
	# wot is this? _ means unused?
#       (_, _, otherimage) = imglist[1-n]
        image = []
        colormap = COLORMAPS[reserved]
        append = image.append

        for i in range (0, len(fromimage), 3):
            rgb1 = fromimage[i:i+3]
#            rgb2 = otherimage[i:i+3]
#            if rgb1 == rgb2:
#               append (rgb1)
            if rgb1 in colormap:
                append (colormap[rgb1])
            else:
#                print 'HI!'
                r, g, b = ord(rgb1[0]), ord(rgb1[1]), ord(rgb1[2])
#                print '%d,%d,%d ->' % (r,g,b)
                r, g, b = palettepixelmap (r, g, b)
#                print '%d,%d,%d.' % (r,g,b)
                newrgb = chr (int (r))+chr (int (g))+chr (int (b))
                append (newrgb)
                colormap[rgb1] = newrgb
        imglist.append((gw, gh, ''.join (image)))


def rotate (imglist, chr=chr, int=int, ord=ord):
    global delta, sat, sign, lumen
    (bw, bh, blue), (gw, gh, green) = imglist
    assert bw == gw and bh == gh
    for reserved in range (len (COLORS)):
        if len (COLORS[reserved]) == 2:
            n, fn = COLORS[reserved]
            fn = globals ()[fn]
        else:
            n, delta, sat, sign, lumen = COLORS[reserved]
            fn = pixelmap
        (_, _, fromimage) = imglist[n]
        (_, _, otherimage) = imglist[1-n]
        image = []
        colormap = COLORMAPS[reserved]
        append = image.append
        for i in range (0, len(fromimage), 3):
            rgb1 = fromimage[i:i+3]
            rgb2 = otherimage[i:i+3]
            if rgb1 == rgb2:
                append (rgb1)
            elif rgb1 in colormap:
                append (colormap[rgb1])
            else:
                r, g, b = fn(ord(rgb1[0]), ord(rgb1[1]), ord(rgb1[2]))
                newrgb = chr(int(r))+chr(int(g))+chr(int(b))
                append(newrgb)
                colormap[rgb1] = newrgb
        imglist.append((bw, bh, ''.join(image)))

def writeout (imglist, namepattern, paletted = False):
    start = 2
    if paletted:
        start = 1
    for i in range (start, len (imglist)):
        w, h, data = imglist[i]
        fn = namepattern % i
        f = open (fn, 'wb')
        print >> f, 'P6'
        print >> f, w, h
        print >> f, 255
        f.write (data)
        f.close ()


def convert (name):
    print >> sys.stderr, 'generating colors for %s...' % name
    imglist = [ppmbreak (open (name % 0, 'rb'))]
    paletted = False
    if Palettes:
        paletterotate (imglist)
        paletted = True
    else:
        imglist.append(ppmbreak (open (name % 1, 'rb')))
	rotate (imglist)
    writeout (imglist, name, paletted)

def updatecheck ():
    myself = os.path.join (ThisDir, 'buildcolors.py')

    def older (list1, list2):
        def mtime (name):
            try:
                st = os.stat (name)
            except OSError:
                return None
            else:
                return st.st_mtime
        list2 = [mtime (name) for name in list2]
        if None in list2:
            return 0
        else:
            list1 = [mtime(name) for name in list1]
            list1 = [t for t in list1 if t is not None]
            return list1 and list2 and max (list1) < min (list2)

    rebuild = {}
    for filename in inputfiles ():
        distfiles = [myself, filename % 0]
        genfiles = [filename % n for n in range (1, MAX)]
        rebuild[filename] = not older (distfiles, genfiles)
    return rebuild


#try to load palettes first
tmp = os.path.join (ThisDir, os.pardir, 'images', 'palettes.dat')
if os.path.exists (tmp):
    #print 'loading palettes'
    loadpalettesets (tmp)
    initpalettelut ()
else:
    # from now on we should always use the palette approach;
    # comment out the following line to restore the old color-rotation code.
    raise IOError("cannot find the palette file %r" % (tmp,))


if __name__ == '__auto__':    # when execfile'd from images.py
    rebuild = updatecheck ().items ()
    rebuild.sort ()
    for fn, r in rebuild:
        if r:
            convert(fn)
    
#try:
#    import psyco
#    psyco.bind(rotate)
#except:
#    pass

if __name__ == '__main__':
    if sys.argv[1:2] == ['-f']:
        files = inputfiles ()
    elif sys.argv[1:2] == ['-c']:
        for filename in inputfiles ():
            for n in range (1, MAX):
                try:
                    os.unlink (filename % n)
                except OSError:
                    pass
                else:
                    print 'rm', filename % n
        sys.exit()
    else:
        rebuild = updatecheck ()
        if 0 in rebuild.values ():
            print >> sys.stderr, ('%d images up-to-date. '
                                  'Use -f to force a rebuild or -c to clean.' %
                                  rebuild.values ().count(0))
        files = [fn for fn, r in rebuild.items () if r]

    files.sort ()
    for filename in files:
        convert (filename)

