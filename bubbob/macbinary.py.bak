import struct


def padto(n, m):
    return (n+m-1) & ~(m-1)

def resourceclass(rtype):
    return globals().get(rtype.strip() + 'Resource', Resource)


class TypeList:

    def __init__(self, type, fmap, fdata, namebase, start, count):
        self.type = type
        self.fmap = fmap
        self.fdata = fdata
        self.namebase = namebase
        self.start = start
        self.count = count
        self.ids = None

    def resources(self):
        if self.ids is None:
            ResourceCls = resourceclass(self.type)
            d = {}
            self.fmap.seek(self.start)
            for resid, resname, resattr, resofshi, resofslo in [
                        struct.unpack(">HHBBHxxxx", self.fmap.read(12))
                        for i in range(self.count)]:
                if resname == 0xffff:
                    name = None
                else:
                    self.fmap.seek(self.namebase + resname)
                    namelen, = struct.unpack(">B", self.fmap.read(1))
                    name = self.fmap.read(namelen)
                assert resid not in d
                d[resid] = ResourceCls(self.type, resid, name, resattr,
                                       self.fdata, resofslo + (resofshi<<16))
            self.ids = d
        return self.ids

    def __getitem__(self, id):
        return self.resources()[id]

    def keys(self):
        return self.resources().keys()

    def values(self):
        return self.resources().values()

    def items(self):
        return self.resources().items()

    def namedict(self):
        return dict([(r.name, r) for r in self.resources().values() if r.name is not None])


class MacBinary:

    def __init__(self, f):
        if type(f) is type(''):
            f = open(f, 'rb')
        self.f = f
        self.f.seek(0x53)
        self.dataforksize, self.resforksize = struct.unpack(">ll", self.f.read(8))
        self.loadresources()

    def getdata(self):
        self.f.seek(0x80)
        return self.f.read(self.dataforksize)

    def loadresources(self):
        f = Subfile(self.f, padto(0x80 + self.dataforksize, 0x80), self.resforksize)
        ofsdata, ofsmap, lendata, lenmap = struct.unpack(">llll", f.read(16))
        fdata = Subfile(f, ofsdata, lendata)
        fmap = Subfile(f, ofsmap, lenmap)
        fmap.seek(24)
        ofstype, ofsname = struct.unpack(">HH", fmap.read(4))
        self.dtypes = {}
        fmap.seek(ofstype)
        numtypes, = struct.unpack(">H", fmap.read(2))
        numtypes = numtypes + 1
        for rtype, num, ofsref in [struct.unpack(">4sHH", fmap.read(8))
                                   for i in range(numtypes)]:
            assert rtype not in self.dtypes
            self.dtypes[rtype] = TypeList(rtype, fmap, fdata, ofsname,
                                         ofstype + ofsref, num + 1)

    def __getitem__(self, rtype):
        return self.dtypes[rtype]

    def types(self):
        return self.dtypes

    def keys(self):
        return self.dtypes.keys()

    def values(self):
        return self.dtypes.values()

    def items(self):
        return self.dtypes.items()


class Subfile:
    def __init__(self, f, start, length):
        if start < 0:
            raise ValueError, 'negative position'
        if isinstance(f, Subfile):
            if start + length > f.length:
                raise ValueError, 'subfile out of bounds'
            f, start = f.f, f.start+start
        self.f = f
        self.start = start
        self.length = length
        self.position = 0
    def read(self, size=None):
        if size is None or self.position + size > self.length:
            size = self.length - self.position
        if size <= 0:
            return ''
        self.f.seek(self.start + self.position)
        self.position = self.position + size
        return self.f.read(size)
    def seek(self, npos):
        if npos < 0:
            raise ValueError, 'negative position'
        self.position = npos


class Resource:
    
    def __init__(self, type, id, name, attr, srcfile, srcofs):
        self.type = type
        self.id = id
        self.name = name
        self.attr = attr
        self.srcfile = srcfile
        self.srcofs = srcofs
        
    def subfile(self):
        self.srcfile.seek(self.srcofs)
        length, = struct.unpack(">l", self.srcfile.read(4))
        return Subfile(self.srcfile, self.srcofs + 4, length)
    
    def load(self):
        return self.subfile().read()


class RGBImage:
    def __init__(self, w, h, data):
        assert len(data) == 3*w*h
        self.w = w
        self.h = h
        self.data = data


def loadcolormap(f):
    size, = struct.unpack(">xxxxxxH", f.read(8))
    size = size + 1
    d = {}
    for index, r, g, b in [struct.unpack(">HHHH", f.read(8)) for i in range(size)]:
        assert index not in d, 'duplicate color index'
        d[index] = r/256.0, g/256.0, b/256.0
    return d

def image2rgb(image):
    # returns (w, h, data)
    h = len(image)
    result1 = []
    for line in image:
        for r, g, b in line:
            result1.append(chr(int(r)) + chr(int(g)) + chr(int(b)))
    return len(image[0]), len(image), ''.join(result1)


class clutResource(Resource):
    # a color table
    def gettable(self):
        return loadcolormap(self.subfile())


class ppatResource(Resource):
    # a pattern
    def getimage(self):
        f = self.subfile()
        pattype, patmap, patdata = struct.unpack(">Hll", f.read(10))
        if pattype != 1:
            raise ValueError, 'Pattern type not supported'
        f.seek(patmap)
        (rowBytes, h, w, packType, packSize,
         pixelType, pixelSize, cmpCount, cmpSize, pmTable) = (
            struct.unpack(">xxxxHxxxxHHxxHlxxxxxxxxHHHHxxxxlxxxx", f.read(50)))
        isBitmap = (rowBytes & 0x8000) != 0
        rowBytes &= 0x3FFF
        if packType != 0:
            raise ValueError, 'packed image not supported'
        if pixelType != 0 or cmpCount != 1:
            raise ValueError, 'direct RGB image not supported'
        assert cmpSize == pixelSize and pixelSize in [1,2,4,8]
        f.seek(pmTable)
        colormap = loadcolormap(f)
        bits_per_pixel = pixelSize
        pixels_per_byte = 8 // bits_per_pixel
        image = []
        f.seek(patdata)
        for y in range(h):
            line = f.read(rowBytes)
            imgline = []
            for x in range(w):
                n = x//pixels_per_byte
                idx = ((ord(line[n]) >> ((pixels_per_byte - 1 - x%pixels_per_byte) * bits_per_pixel))
                       & ((1<<bits_per_pixel)-1))
                imgline.append(colormap[idx])
            image.append(imgline)
        return image


class LEVLResource(Resource):
    # bub & bob level
    WIDTH = 32
    HEIGHT = 25
    MONSTERS = 30
    WALLS = { 1:'#', 0:' '}
    WINDS = { 0:' ', 1:'>', 2:'<', 3:'v', 4:'^', 5:'x', 0x66:' '}
    FLAGS = ['flag0', 'letter', 'fire', 'lightning', 'water', 'top', 'flag6', 'flag7']

    def getlevel(self, mnstrlist):
        f = self.subfile()
        result = {}
        
        walls = []
        for y in range(self.HEIGHT):
            line = f.read(self.WIDTH//8)
            line = [self.WALLS[(ord(line[x//8]) >> (x%8)) & 1]
                    for x in range(self.WIDTH)]
            walls.append(''.join(line))
        result['walls'] = '\n'.join(walls)

        winds = []
        for y in range(self.HEIGHT):
            line = f.read(self.WIDTH)
            line = [self.WINDS[ord(v)] for v in line]
            winds.append(''.join(line))
        result['winds'] = '\n'.join(winds)

        monsters = []
        for i in range(self.MONSTERS):
            x,y,monster_type,f1,f2,f3 = struct.unpack(">BBBBBB", f.read(6))
            if monster_type != 0:
                assert f1 == 0, f1
                cls = mnstrlist[monster_type-1]
                monsters.append(cls(x=x, y=y, dir=f2, player=f3))
        result['monsters'] = monsters

        result['level'], = struct.unpack('>H', f.read(2))
        for i in range(8):
            result[self.FLAGS[i]] = ord(f.read(1))
        
        return result
