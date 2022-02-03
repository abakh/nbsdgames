from __future__ import generators
from socket import *
from select import select
from struct import pack, unpack
import zlib, os, random, struct, md5, sys
from time import time, ctime
from msgstruct import *
from errno import EWOULDBLOCK


SERVER_TIMEOUT = 7200   # 2 hours without any connection or port activity


def protofilepath(filename):
  dirpath = filename
  path = []
  while dirpath:
    dirpath, component = os.path.split(dirpath)
    assert component, "invalid file path %r" % (filename,)
    path.insert(0, component)
  path.insert(0, game.FnBasePath)
  return '/'.join(path)


class Icon:
  count = 0

  def __init__(self, bitmap, code, x,y,w,h, alpha=255):
    self.w = w
    self.h = h
    self.origin = (bitmap, x, y)
    self.code = code
    if alpha == 255:
      self.msgdef = message(MSG_DEF_ICON, bitmap.code, code, x,y,w,h)
    else:
      self.msgdef = message(MSG_DEF_ICON, bitmap.code, code, x,y,w,h, alpha)
    framemsgappend(self.msgdef)

  def getimage(self):
    import pixmap
    bitmap, x, y = self.origin
    image = pixmap.decodepixmap(bitmap.read())
    return pixmap.cropimage(image, (x, y, self.w, self.h))

  def getorigin(self):
    bitmap, x, y = self.origin
    return bitmap, (x, y, self.w, self.h)


class DataChunk:
  
  def __init__(self):
    for c in clients:
      if c.initialized == 2:
        self.defall(c)
    if recording and game:
      self.defall(recording)

  def read(self, slice=None):
    f = open(self.filename, "rb")
    data = f.read()
    f.close()
    if slice:
      start, length = slice
      data = data[start:start+length]
    return data

  def defall(self, client):
    if client.proto == 1 or not self.filename:
      # protocol 1
      try:
        msgdef = self.msgdef
      except AttributeError:
        data = zlib.compress(self.read())
        msgdef = self.msgdef = self.getmsgdef(data)
    else:
      # protocol >= 2
      try:
        msgdef = self.sendmsgdef
      except AttributeError:
        fileid = len(filereaders)
        filereaders[fileid] = self.read
        data = self.read()
        msgdef = self.sendmsgdef = (self.getmd5def(fileid, data) +
                                    self.getmsgdef(fileid))
    client.msgl.append(msgdef)

  def getmd5def(self, fileid, data, offset=0):
    checksum = md5.new(data).digest()
    return message(MSG_MD5_FILE, fileid, protofilepath(self.filename),
                   offset, len(data), checksum)


class Bitmap(DataChunk):

  def __init__(self, code, filename, colorkey=None):
    self.code = code
    self.filename = filename
    self.icons = {}
    self.colorkey = colorkey
    DataChunk.__init__(self)

  def geticon(self, x,y,w,h, alpha=255):
    rect = (x,y,w,h)
    try:
      return self.icons[rect]
    except:
      ico = Icon(self, Icon.count, x,y,w,h, alpha)
      Icon.count += 1
      self.icons[rect] = ico
      return ico

  def geticonlist(self, w, h, count):
    return map(lambda i, fn=self.geticon, w=w, h=h: fn(i*w, 0, w, h), range(count))

  def getmsgdef(self, data):
    if self.colorkey is not None:
      return message(MSG_DEF_BITMAP, self.code, data, self.colorkey)
    else:
      return message(MSG_DEF_BITMAP, self.code, data)

  def defall(self, client):
    DataChunk.defall(self, client)
    for i in self.icons.values():
      client.msgl.append(i.msgdef)


class MemoryBitmap(Bitmap):
  
  def __init__(self, code, data, colorkey=None):
    self.data = data
    Bitmap.__init__(self, code, None, colorkey)

  def read(self, slice=None):
    data = self.data
    if slice:
      start, length = slice
      data = data[start:start+length]
    return data


class Sample(DataChunk):

  def __init__(self, code, filename, freqfactor=1):
    self.code = code
    self.filename = filename
    self.freqfactor = freqfactor
    DataChunk.__init__(self)

  def defall(self, client):
    if client.has_sound > 0:
      DataChunk.defall(self, client)

  def getmsgdef(self, data):
    return message(MSG_DEF_SAMPLE, self.code, data)

  def read(self, slice=None):
    f = open(self.filename, "rb")
    data = f.read()
    f.close()
    if self.freqfactor != 1:
      freq, = unpack("<i", data[24:28])
      freq = int(freq * self.freqfactor)
      data = data[:24] + pack("<i", freq) + data[28:]
    if slice:
      start, length = slice
      data = data[start:start+length]
    return data

  def getmd5def(self, fileid, data):
    if self.freqfactor == 1:
      return DataChunk.getmd5def(self, fileid, data)
    else:
      datahead = data[:28]
      datatail = data[28:]
      return (message(MSG_PATCH_FILE, fileid, 0, datahead) +
              DataChunk.getmd5def(self, fileid, datatail, offset=28))

  def play(self, lvolume=1.0, rvolume=None, pad=0.5, singleclient=None):
    if rvolume is None:
      rvolume = lvolume
    lvolume *= 2.0*(1.0-pad)
    rvolume *= 2.0*pad
    if lvolume < 0.0:
      lvolume = 0.0
    elif lvolume > 1.0:
      lvolume = 1.0
    if rvolume < 0.0:
      rvolume = 0.0
    elif rvolume > 1.0:
      rvolume = 1.0
    message = pack("!hBBh", self.code, int(lvolume*255.0),
                                       int(rvolume*255.0), -1)
    if singleclient is None:
      clist = clients[:]
    else:
      clist = [singleclient]
    for c in clist:
      if c.has_sound:
        c.sounds.setdefault(message, 4)


class Music(DataChunk):

  def __init__(self, filename, filerate=44100):
    self.filename = filename
    self.filerate = filerate
    self.f = open(filename, 'rb')
    self.f.seek(0, 2)
    filesize = self.f.tell()
    self.endpos = max(self.filerate, filesize - self.filerate)
    self.fileid = len(filereaders)
    filereaders[self.fileid] = self.read
    self.md5msgs = {}
    DataChunk.__init__(self)

  def read(self, (start, length)):
    self.f.seek(start)
    return self.f.read(length)

  def msgblock(self, position, limited=1):
    blocksize = self.filerate
    if limited and position+blocksize > self.endpos:
      blocksize = self.endpos-position
      if blocksize <= 0:
        return ''
    #self.f.seek(position)
    #return message(MSG_DEF_MUSIC, self.code, position, self.f.read(blocksize))
    try:
      msg = self.md5msgs[position]
    except KeyError:
      data = self.read((position, blocksize))
      checksum = md5.new(data).digest()
      msg = message(MSG_MD5_FILE, self.fileid, protofilepath(self.filename),
                    position, blocksize, checksum)
      self.md5msgs[position] = msg
    return msg

  def clientsend(self, clientpos):
    msg = self.msgblock(clientpos)
    #print 'clientsend:', self.code, len(msg), clientpos
    if msg:
      return [msg], clientpos + self.filerate
    else:
      return [], None

  def initialsend(self, c):
    return [self.msgblock(0), self.msgblock(self.endpos, 0)], self.filerate

  def defall(self, client):
    pass


def clearsprites():
  sprites_by_n.clear()
  sprites[:] = ['']

def compactsprites(insert_new=None, insert_before=None):
  global sprites, sprites_by_n
  if insert_before is not None:
    if insert_new.alive:
      insert_before = insert_before.alive
    else:
      insert_before = None
  newsprites = ['']
  newd = {}
  l = sprites_by_n.items()
  l.sort()
  for n, s in l:
    if n == insert_before:
      prevn = insert_new.alive
      newn = insert_new.alive = len(newsprites)
      newsprites.append(sprites[prevn])
      newd[newn] = insert_new
      l.remove((prevn, insert_new))
    newn = s.alive = len(newsprites)
    newsprites.append(sprites[n])
    newd[newn] = s
  sprites = newsprites
  sprites_by_n = newd


class Sprite:

##  try:
##    import psyco.classes
##  except ImportError:
##    pass
##  else:
##    __slots__ = ['x', 'y', 'ico', 'alive']
##    __metaclass__ = psyco.classes.psymetaclass

  def __init__(self, ico, x,y):
    self.x = x
    self.y = y
    self.ico = ico
    self.alive = len(sprites)
    if (-ico.w < x < game.width and
        -ico.h < y < game.height):
      sprites.append(pack("!hhh", x, y, ico.code))
    else:
      sprites.append('')  # starts off-screen
    sprites_by_n[self.alive] = self

  def move(self, x,y, ico=None):
    self.x = x
    self.y = y
    if ico is not None:
      self.ico = ico
    sprites[self.alive] = pack("!hhh", x, y, self.ico.code)

  def setdisplaypos(self, x, y):
    # special use only (self.x,y are not updated)
    s = sprites[self.alive]
    if len(s) == 6:
      sprites[self.alive] = pack("!hh", x, y) + s[4:]

  def setdisplayicon(self, ico):
    # special use only (self.ico is not updated)
    s = sprites[self.alive]
    if len(s) == 6:
      sprites[self.alive] = s[:4] + pack("!h", ico.code)

  #sizeof_displaypos = struct.calcsize("!hh")
  def getdisplaypos(self):
    # special use only (normally, read self.x,y,ico directly)
    s = sprites[self.alive]
    if self.alive and len(s) == 6:
      return unpack("!hh", s[:4])
    else:
      return None, None

  def step(self, dx,dy):
    x = self.x = self.x + dx
    y = self.y = self.y + dy
    sprites[self.alive] = pack("!hhh", x, y, self.ico.code)

  def seticon(self, ico):
    self.ico = ico
    sprites[self.alive] = pack("!hhh", self.x, self.y, ico.code)

  def hide(self):
    sprites[self.alive] = ''

  def kill(self):
    if self.alive:
      del sprites_by_n[self.alive]
      sprites[self.alive] = ''
      self.alive = 0

  def prefix(self, n, m=0):
    pass #sprites[self.alive] = pack("!hhh", n, m, 32767) + sprites[self.alive]

  def to_front(self):
    if self.alive and self.alive < len(sprites)-1:
      self._force_to_front()

  def _force_to_front(self):
    info = sprites[self.alive]
    sprites[self.alive] = ''
    del sprites_by_n[self.alive]
    self.alive = len(sprites)
    sprites_by_n[self.alive] = self
    sprites.append(info)

  def to_back(self, limit=None):
    assert self is not limit
    if limit:
      n1 = limit.alive + 1
    else:
      n1 = 1
    if self.alive > n1:
      if n1 in sprites_by_n:
        keys = sprites_by_n.keys()
        keys.remove(self.alive)
        keys.sort()
        keys = keys[keys.index(n1):]
        reinsert = [sprites_by_n[n] for n in keys]
        for s1 in reinsert:
          s1._force_to_front()
        assert n1 not in sprites_by_n
      info = sprites[self.alive]
      sprites[self.alive] = ''
      del sprites_by_n[self.alive]
      self.alive = n1
      sprites_by_n[n1] = self
      sprites[n1] = info

  def __repr__(self):
    if self.alive:
      return "<sprite %d at %d,%d>" % (self.alive, self.x, self.y)
    else:
      return "<killed sprite>"


class Player:
  standardplayericon = None

  def playerjoin(self):
    pass

  def playerleaves(self):
    pass

  def _playerleaves(self):
    if self.isplaying():
      self._client.killplayer(self)
      del self._client
    self.playerleaves()

  def isplaying(self):
    return hasattr(self, "_client")


class Client:
  SEND_BOUND_PER_FRAME = 0x6000   # bytes
  KEEP_ALIVE           = 2.2      # seconds

  def __init__(self, socket, addr):
    socket.setblocking(0)
    self.socket = socket
    self.addr = addr
    self.udpsocket = None
    self.udpsockcounter = 0
    self.initialdata = MSG_WELCOME
    self.initialized = 0
    self.msgl = [message(MSG_PING)]
    self.buf = ""
    self.players = { }
    self.sounds = None
    self.has_sound = 0
    self.has_music = 0
    self.musicpos = { }
    self.proto = 1
    self.dyncompress = None
    addsocket('CLIENT', self.socket, self.input_handler)
    clients.append(self)
    self.log('connected')
    self.send_buffer(self.initialdata)

  def opengame(self, game):
    if self.initialized == 0:
      self.initialdata += game.FnDesc + '\n'
      self.initialized = 1
    if self.initialized == 1:
      if game.broadcast_port:
        self.initialdata += message(MSG_BROADCAST_PORT, game.broadcast_port)
        game.trigger_broadcast()
      self.initialdata += game.deffieldmsg()
    else:
      self.msgl.append(game.deffieldmsg())
    self.activity = self.last_ping = time()
    self.force_ping_delay = 0.6
    for c in clients:
      for id in c.players.keys():
        self.msgl.append(message(MSG_PLAYER_JOIN, id, c is self))

  def emit(self, udpdata, broadcast_extras):
    if self.initialdata:
      self.send_buffer(self.initialdata)
    elif self.initialized == 2:
      buffer = ''.join(self.msgl)
      if buffer:
        self.send_buffer(buffer)
      if self.udpsocket is not None:
        if self.sounds:
          if broadcast_extras is None or self not in broadcast_clients:
            udpdata = ''.join(self.sounds.keys() + [udpdata])
          else:
            broadcast_extras.update(self.sounds)
          for key, value in self.sounds.items():
            if value:
              self.sounds[key] = value-1
            else:
              del self.sounds[key]
        if broadcast_extras is None or self not in broadcast_clients:
          if self.dyncompress is not None:
            udpdatas = self.dynamic_compress(udpdata)
          else:
            udpdatas = [udpdata]
          for udpdata in udpdatas:
            try:
              self.udpsockcounter += self.udpsocket.send(udpdata)
            except error, e:
              print >> sys.stderr, 'ignored:', str(e)
              pass  # ignore UDP send errors (buffer full, etc.)
      if self.has_music > 1 and NOW >= self.musicstreamer:
        self.musicstreamer += 0.99
        self.sendmusicdata()
      if not self.msgl:
        if abs(NOW - self.activity) <= self.KEEP_ALIVE:
          if abs(NOW - self.last_ping) <= self.force_ping_delay:
            return
          if self.udpsockcounter < 1024:
            return
          self.force_ping_delay += 0.2
        self.msgl.append(message(MSG_PING, self.udpsockcounter>>10))
        self.last_ping = NOW

  def setup_dyncompress(self):
    def dyncompress():
      # See comments in pclient.Playfield.dynamic_decompress().
      threads = []
      for t in range(3):
        co = zlib.compressobj(6)
        threads.append((chr(0x88 + t) + chr(t), co))
      frame = 0
      globalsync = 0

      while 1:
        # write three normal packets, one on each thread
        for t in range(3):
          head, co = threads.pop(0)
          yield head + chr(frame), co
          threads.append((chr(ord(head[0]) & 0x87) + chr(frame), co))
          yield None, None
          frame = (frame + 1) & 0xFF

        # sync frame, write two packets (on two threads)
        # and restart compression at the current frame for these threads
        head, co = threads.pop(0)
        yield head + chr(frame), co
        co1 = zlib.compressobj(6)
        co2 = zlib.compressobj(6)
        globalsync += 1
        if globalsync == 4:
          # next on this thread will be a global sync packet
          nextframe = (frame + 2) & 0xFF
          globalsync = 0
        else:
          # next of this thread will be a local sync packet
          yield None, co1
          nextframe = frame
        threads.append((chr(ord(head[0]) | 8) + chr(nextframe), co1))

        # 2nd packet of the current frame
        head, co = threads.pop(0)
        yield head + chr(frame), co
        yield None, co2
        threads.append((chr(ord(head[0]) | 8) + chr(frame), co2))
        
        yield None, None
        frame = (frame + 1) & 0xFF
    
    self.dyncompress = dyncompress()

  def dynamic_compress(self, framedata):
    result = []
    for head, co in self.dyncompress:
      if not co:
        return result
      data = [head, co.compress(framedata), co.flush(zlib.Z_SYNC_FLUSH)]
      if head:
        result.append(''.join(data))

  def send_can_mix(self):
    return not self.msgl and self.socket is not None

  def send_buffer(self, buffer):
    try:
      count = self.socket.send(buffer[:self.SEND_BOUND_PER_FRAME])
    except error, e:
      if e.args[0] != EWOULDBLOCK:
        self.msgl = []
        self.initialdata = ""
        self.disconnect(e, 'emit')
        return
    else:
      #g = open('log', 'ab'); g.write(buffer[:count]); g.close()
      buffer = buffer[count:]
      self.activity = NOW
    if self.initialdata:
      self.initialdata = buffer
    elif buffer:
      self.msgl = [buffer]
    else:
      self.msgl = []

  def receive(self, data):
    #print "receive:", `data`
    try:
      data = self.buf + data
      while data:
        values, data = decodemessage(data)
        if not values:
          break  # incomplete message
        fn = self.MESSAGES.get(values[0])
        if fn:
          fn(self, *values[1:])
        else:
          print "unknown message from", self.addr, ":", values
      self.buf = data
    except struct.error:
      import traceback
      traceback.print_exc()
      self.socket.send('\n\n<h1>Protocol Error</h1>\n')
      hs = findsocket('HTTP')
      if hs is not None:
        url = 'http://%s:%s' % (HOSTNAME, displaysockport(hs))
        self.socket.send('''
If you meant to point your web browser to this server,
then use the following address:

<a href="%s">%s</a>
''' % (url, url))
      self.disconnect('protocol error', 'receive')

  def input_handler(self):
    try:
      data = self.socket.recv(2048)
    except error, e:
      self.disconnect(e, "socket.recv")
    else:
      if data:
        self.activity = NOW
        self.receive(data)
      elif not hasattr(self.socket, 'RECV_CAN_RETURN_EMPTY'):
        # safecheck that this means disconnected
        iwtd, owtd, ewtd = select([self.socket], [], [], 0.0)
        if self.socket in iwtd:
          self.disconnect('end of data', 'socket.recv')

  def disconnect(self, err=None, infn=None):
    removesocket('CLIENT', self.socket)
    if err:
      extra = ": " + str(err)
    else:
      extra = ""
    if infn:
      extra += " in " + infn
    print 'Disconnected by', self.addr, extra
    self.log('disconnected' + extra)
    for p in self.players.values():
      p._playerleaves()
    try:
      del broadcast_clients[self]
    except KeyError:
      pass
    clients.remove(self)
    try:
      self.socket.close()
    except:
      pass
    self.socket = None
    if not clients and game is not None:
      game.FnDisconnected()

  def killplayer(self, player):
    for id, p in self.players.items():
      if p is player:
        framemsgappend(message(MSG_PLAYER_KILL, id))
        del self.players[id]
        if game:
          game.updateplayers()

  def joinplayer(self, id, *rest):
    if self.players.has_key(id):
      print "Note: player %s is already playing" % (self.addr+(id,),)
      return
    if game is None:
      return   # refusing new player before the game starts
    p = game.FnPlayers()[id]
    if p is None:
      print "Too many players. New player %s refused." % (self.addr+(id,),)
      self.msgl.append(message(MSG_PLAYER_KILL, id))
    elif p.isplaying():
      print "Note: player %s is already played by another client" % (self.addr+(id,),)
    else:
      print "New player %s" % (self.addr+(id,),)
      p._client = self
      p.playerjoin()
      p.setplayername('')
      self.players[id] = p
      game.updateplayers()
      for c in clients:
        c.msgl.append(message(MSG_PLAYER_JOIN, id, c is self))

  def remove_player(self, id, *rest):
    try:
      p = self.players[id]
    except KeyError:
      print "Note: player %s is not playing" % (self.addr+(id,),)
    else:
      p._playerleaves()

  def set_player_name(self, id, name, *rest):
    p = game.FnPlayers()[id]
    p.setplayername(name)

  def set_udp_port(self, port, addr=None, *rest):
    if port == MSG_BROADCAST_PORT:
      self.log('set_udp_port: broadcast')
      broadcast_clients[self] = 1
      #print "++++ Broadcasting ++++ to", self.addr
    else:
      try:
        del broadcast_clients[self]
      except KeyError:
        pass
      if port == MSG_INLINE_FRAME or port == 0:
        # client requests data in-line on the TCP stream
        self.dyncompress = None
        import udpovertcp
        self.udpsocket = udpovertcp.SocketMarshaller(self.socket, self)
        s = self.udpsocket.tcpsock
        self.log('set_udp_port: udp-over-tcp')
      else:
        try:
          if hasattr(self.socket, 'udp_over_udp_mixer'):
            # for SocketOverUdp
            self.udpsocket = self.socket.udp_over_udp_mixer()
          else:
            self.udpsocket = socket(AF_INET, SOCK_DGRAM)
            self.udpsocket.setblocking(0)
            addr = addr or self.addr[0]
            self.udpsocket.connect((addr, port))
        except error, e:
          print >> sys.stderr, "Cannot set UDP socket to", addr, str(e)
          self.udpsocket = None
          self.udpsockcounter = sys.maxint
        else:
          if self.proto >= 3:
            self.setup_dyncompress()
        s = self.udpsocket
        self.log('set_udp_port: %s:%d' % (addr, port))
      if s:
        try:
          s.setsockopt(SOL_IP, IP_TOS, 0x10)  # IPTOS_LOWDELAY
        except error, e:
          print >> sys.stderr, "Cannot set IPTOS_LOWDELAY:", str(e)

  def enable_sound(self, sound_mode=1, *rest):
    if sound_mode != self.has_sound:
      self.sounds = {}
      self.has_sound = sound_mode
      if self.has_sound > 0:
        for snd in samples.values():
          snd.defall(self)
      #self.log('enable_sound %s' % sound_mode)

  def enable_music(self, mode, *rest):
    if mode != self.has_music:
      self.has_music = mode
      self.startmusic()
      #self.log('enable_music')

  def startmusic(self):
    if self.has_music:
      self.musicstreamer = time()
      for cde in currentmusics[1:]:
        if cde not in self.musicpos:
          msgl, self.musicpos[cde] = music_by_id[cde].initialsend(self)
          self.msgl += msgl
      if self.has_music > 1:
        self.sendmusicdata()
        self.msgl.append(message(MSG_PLAY_MUSIC, *currentmusics))

  def sendmusicdata(self):
    for cde in currentmusics[1:]:
      if self.musicpos[cde] is not None:
        msgl, self.musicpos[cde] = music_by_id[cde].clientsend(self.musicpos[cde])
        self.msgl += msgl
        return

  def ping(self, *rest):
    if self.initialized < 2:
      # send all current bitmap data
      self.initialized = 2
      for b in bitmaps.values():
        b.defall(self)
      self.finishinit(game)
      for id, p in game.FnPlayers().items():
        if p.standardplayericon is not None:
          self.msgl.append(message(MSG_PLAYER_ICON, id, p.standardplayericon.code))
    self.msgl.append(message(MSG_PONG, *rest))

  def finishinit(self, game):
    pass

  def pong(self, *rest):
    pass

  def log(self, message):
    print self.addr, message

  def protocol_version(self, version, *rest):
    self.proto = version

  def md5_data_request(self, fileid, position, size, *rest):
    data = filereaders[fileid]((position, size))
    data = zlib.compress(data)
    self.msgl.append(message(MSG_ZPATCH_FILE, fileid, position, data))

##  def def_file(self, filename, md5sum):
##    fnp = []
##    while filename:
##      filename, tail = os.path.split(filename)
##      fnp.insert(0, tail)
##    if fnp[:len(FnBasePath)] == FnBasePath:
##      filename = os.path.join(*fnp[len(FnBasePath):])
##      self.known_files[filename] = md5sum

  MESSAGES = {
    CMSG_PROTO_VERSION: protocol_version,
    CMSG_ADD_PLAYER   : joinplayer,
    CMSG_REMOVE_PLAYER: remove_player,
    CMSG_UDP_PORT     : set_udp_port,
    CMSG_ENABLE_SOUND : enable_sound,
    CMSG_ENABLE_MUSIC : enable_music,
    CMSG_PING         : ping,
    CMSG_PONG         : pong,
    CMSG_DATA_REQUEST : md5_data_request,
    CMSG_PLAYER_NAME  : set_player_name,
##    CMSG_DEF_FILE     : def_file,
    }


class SimpleClient(Client):

  def finishinit(self, game):
    num = 0
    for keyname, icolist, fn in game.FnKeys:
      self.msgl.append(message(MSG_DEF_KEY, keyname, num,
                               *[ico.code for ico in icolist]))
      num += 1
  
  def cmsg_key(self, pid, keynum):
    if game is not None:
      try:
        player = self.players[pid]
        fn = game.FnKeys[keynum][2]
      except (KeyError, IndexError):
        game.FnUnknown()
      else:
        getattr(player, fn) ()

  MESSAGES = Client.MESSAGES.copy()
  MESSAGES.update({
    CMSG_KEY: cmsg_key,
    })


MAX_CLIENTS = 32

clients = []
FnClient = SimpleClient
broadcast_clients = {}
filereaders = {}
bitmaps = {}
samples = {}
music_by_id = {}
currentmusics = [0]
sprites = ['']
sprites_by_n = {}
recording = None
game = None
serversockets = {}
socketsbyrole = {}
socketports   = {}

def framemsgappend(msg):
  for c in clients:
    c.msgl.append(msg)
  if recording:
    recording.write(msg)

##def sndframemsgappend(msg):
##  for c in clients:
##    if c.has_sound:
##      c.msgl.append(msg)

def set_udp_port(port):
  hostchooser.UDP_PORT = port

def has_loop_music():
  return currentmusics[0] < len(currentmusics)-1

def finalsegment(music1, music2):
  intro1 = music1[1:1+music1[0]]
  intro2 = music2[1:1+music2[0]]
  loop1 = music1[1+music1[0]:]
  loop2 = music2[1+music2[0]:]
  return loop1 == loop2 and intro1 == intro2[len(intro2)-len(intro1):]

def set_musics(musics_intro, musics_loop, reset=1):
  mlist = []
  loop_from = len(musics_intro)
  mlist.append(loop_from)
  for m in musics_intro + musics_loop:
    mlist.append(m.fileid)
  reset = reset or not finalsegment(mlist, currentmusics)
  currentmusics[:] = mlist
  if reset:
    for c in clients:
      c.startmusic()

def fadeout(time=1.0):
  msg = message(MSG_FADEOUT, int(time*1000))
  for c in clients:
    if c.has_music > 1:
      c.msgl.append(msg)
  currentmusics[:] = [0]


def getbitmap(filename, colorkey=None):
  try:
    return bitmaps[filename]
  except:
    bmp = Bitmap(len(bitmaps), filename, colorkey)
    bitmaps[filename] = bmp
    return bmp

def getsample(filename, freqfactor=1):
  try:
    return samples[filename, freqfactor]
  except:
    snd = Sample(len(samples), filename, freqfactor)
    samples[filename, freqfactor] = snd
    return snd

def getmusic(filename, filerate=44100):
  try:
    return samples[filename]
  except:
    mus = Music(filename, filerate)
    samples[filename] = mus
    music_by_id[mus.fileid] = mus
    return mus

def newbitmap(data, colorkey=None):
  bmp = MemoryBitmap(len(bitmaps), data, colorkey)
  bitmaps[bmp] = bmp
  return bmp


def addsocket(role, socket, handler=None, port=None):
  if port is None:
    host, port = socket.getsockname()
  if handler is not None:
    serversockets[socket] = handler
  socketsbyrole.setdefault(role, []).append(socket)
  socketports[socket] = port

def findsockets(role):
  return socketsbyrole.get(role, [])

def findsocket(role):
  l = findsockets(role)
  if l:
    return l[-1]
  else:
    return None

def removesocket(role, socket=None):
  if socket is None:
    for socket in socketsbyrole.get(role, [])[:]:
      removesocket(role, socket)
    return
  try:
    del serversockets[socket]
  except KeyError:
    pass
  try:
    socketsbyrole.get(role, []).remove(socket)
  except ValueError:
    pass
  try:
    del socketports[socket]
  except KeyError:
    pass

def opentcpsocket(port=None):
  port = port or PORTS.get('LISTEN', INADDR_ANY)
  s = findsocket('LISTEN')
  if s is None:
    s = socket(AF_INET, SOCK_STREAM)
    try:
      s.bind(('', port))
      s.listen(1)
    except error:
      if port == INADDR_ANY:
        for i in range(10):
          port = random.choice(xrange(8000, 12000))
          try:
            s.bind(('', port))
            s.listen(1)
          except error:
            pass
          else:
            break
        else:
          raise error, "server cannot find a free TCP socket port"
      else:
        raise

    def tcpsocket_handler(s=s):
      conn, addr = s.accept()
      game.newclient(conn, addr)
    
    addsocket('LISTEN', s, tcpsocket_handler)
  return s

def openpingsocket(only_port=None):
  only_port = only_port or PORTS.get('PING', None)
  s = findsocket('PING')
  if s is None:
    import hostchooser
    s = hostchooser.serverside_ping(only_port)
    if s is None:
      return None
    def pingsocket_handler(s=s):
      global game
      import hostchooser
      if game is not None:
        args = game.FnDesc, ('', game.address[1]), game.FnExtraDesc()
      else:
        ts = findsocket('LISTEN')
        if ts:
          address = '', displaysockport(ts)
        else:
          address = '', ''
        args = 'Not playing', address, ''
      hs = findsocket('HTTP')
      args = args + (displaysockport(hs),)
      hostchooser.answer_ping(s, *args)
    addsocket('PING', s, pingsocket_handler)
  return s

def openhttpsocket(ServerClass=None, HandlerClass=None,
                   port=None):
  port = port or PORTS.get('HTTP', None)
  s = findsocket('HTTP')
  if s is None:
    if ServerClass is None:
      from BaseHTTPServer import HTTPServer as ServerClass
    if HandlerClass is None:
      import javaserver
      from httpserver import MiniHandler as HandlerClass
    server_address = ('', port or 8000)
    try:
      httpd = ServerClass(server_address, HandlerClass)
    except error:
      if port is None:
        server_address = ('', INADDR_ANY)
        try:
          httpd = ServerClass(server_address, HandlerClass)
        except error, e:
          print >> sys.stderr, "cannot start HTTP server", str(e)
          return None
      else:
        raise
    s = httpd.socket
    addsocket('HTTP', s, httpd.handle_request)
  return s

BROADCAST_PORT_RANGE = xrange(18000, 19000)
#BROADCAST_MESSAGE comes from msgstruct
BROADCAST_DELAY      = 0.6180
BROADCAST_DELAY_INCR = 2.7183

def openbroadcastsocket(broadcastport=None):
  s = findsocket('BROADCAST')
  if s is None:
    try:
      s = socket(AF_INET, SOCK_DGRAM)
      s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
    except error, e:
      print >> sys.stderr, "Cannot broadcast", str(e)
      return None
    port = broadcastport or random.choice(BROADCAST_PORT_RANGE)
    addsocket('BROADCAST', s, port=port)
  return s

def displaysockport(s):
  return socketports.get(s, 'off')


class Game:
  width     = 640
  height    = 480
  backcolor = 0x000000

  FnDesc    = "NoName"
  FnFrame   = lambda self: 1.0
  FnExcHandler=lambda self, k: 0
  FnServerInfo=lambda self, s: None
  FnPlayers = lambda self: {}
  FnKeys    = []
  FnUnknown = lambda self: None
  FnDisconnected = lambda self: None

  def __init__(self):
    global game
    s = opentcpsocket()
    self.address = HOSTNAME, socketports[s]
    bs = self.broadcast_s = openbroadcastsocket()
    self.broadcast_port = socketports.get(bs)
    self.broadcast_next = None
    self.nextframe = time()
    clearsprites()
    game = self
    if recording:
      for b in bitmaps.values():
        b.defall(recording)
    self.pendingclients = []

  def openserver(self):
    ps = openpingsocket()
    print '%s server at %s:%d, Broadcast %d, UDP %d' % (
      self.FnDesc, self.address[0], self.address[1],
      displaysockport(self.broadcast_s), displaysockport(ps))

    hs = openhttpsocket()
    if hs:
      print 'HTTP server: http://%s:%d' % (
        self.address[0], displaysockport(hs))

    try:
      from localmsg import autonotify
    except ImportError:
      pass
    else:
      autonotify(self.FnDesc, *self.address)

    if clients:
      for c in clients:
        c.opengame(self)
    if recording:
      recording.start()
      recording.write(self.deffieldmsg())

  def trigger_broadcast(self):
    assert self.broadcast_s is not None
    game.broadcast_delay = BROADCAST_DELAY
    game.broadcast_next  = time() + self.broadcast_delay

  def deffieldmsg(self):
    return message(MSG_DEF_PLAYFIELD,
                   self.width, self.height, self.backcolor,
                   self.FnDesc)

  def socketerrors(self, ewtd):
    for c in clients[:]:
      if c.socket in ewtd:
        del ewtd[c.socket]
        c.disconnect("error", "select")

  def mainstep(self):
    global NOW
    if self.pendingclients:
      self.newclient(*self.pendingclients.pop())
    NOW = time()
    delay = self.nextframe - NOW
    if delay<=0.0:
      self.nextframe += self.FnFrame()
      self.sendudpdata()
      NOW = time()
      delay = self.nextframe - NOW
      if delay<0.0:
        self.nextframe = NOW
        delay = 0.0
    if self.broadcast_next is not None and NOW >= self.broadcast_next:
      if not clients:
        self.broadcast_next = None
      else:
        try:
          self.broadcast_s.sendto(BROADCAST_MESSAGE,
                                  ('<broadcast>', self.broadcast_port))
          #print "Broadcast ping"
        except error:
          pass  # ignore failed broadcasts
        self.broadcast_next = time() + self.broadcast_delay
        self.broadcast_delay *= BROADCAST_DELAY_INCR
    return delay

  def sendudpdata(self):
    sprites[0] = ''
    udpdata = ''.join(sprites)
    if len(broadcast_clients) >= 2:
      broadcast_extras = {}
    else:
      broadcast_extras = None
    for c in clients[:]:
      c.emit(udpdata, broadcast_extras)
    if recording:
      recording.udpdata(udpdata)
    if broadcast_extras is not None:
      udpdata = ''.join(broadcast_extras.keys() + [udpdata])
      try:
        self.broadcast_s.sendto(udpdata,
                                ('<broadcast>', self.broadcast_port))
        #print "Broadcast UDP data"
      except error:
        pass  # ignore failed broadcasts

  def FnExtraDesc(self):
    players = 0
    for c in clients:
      players += len(c.players)
    if players == 0:
      return 'no player'
    elif players == 1:
      return 'one player'
    else:
      return '%d players' % players

  def updateplayers(self):
    pass

  def updateboard(self):
    pass

  def newclient(self, conn, addr):
    if len(clients)==MAX_CLIENTS:
      print "Too many connections; refusing new connection from", addr
      conn.close()
    else:
      try:
        addrname = (gethostbyaddr(addr[0])[0],) + addr[1:]
      except:
        addrname = addr
      print 'Connected by', addrname
      try:
        c = FnClient(conn, addrname)
      except error, e:
        print 'Connexion already lost!', e
      else:
        if game is not None:
          c.opengame(game)

  def newclient_threadsafe(self, conn, addr):
    self.pendingclients.insert(0, (conn, addr))


def recursiveloop(endtime, extra_sockets):
  global game
  timediff = 1
  while timediff:
    if game is not None:
      delay = game.mainstep()
    else:
      delay = 5.0
    iwtd = extra_sockets + serversockets.keys()
    timediff = max(0.0, endtime - time())
    iwtd, owtd, ewtd = select(iwtd, [], iwtd, min(delay, timediff))
    if ewtd:
      if game:
        game.socketerrors(ewtd)
      if ewtd:
        print >> sys.stderr, "Unexpected socket error reported"
    for s in iwtd:
      if s in serversockets:
        serversockets[s]()    # call handler
      elif s in extra_sockets:
        return s
    if not extra_sockets and timediff:
      return 1
  return None

SERVER_SHUTDOWN = 0.0

def mainloop():
  global game, SERVER_SHUTDOWN
  servertimeout = None
  try:
    while serversockets:
      try:
        if game is not None:
          delay = game.mainstep()
        else:
          delay = SERVER_SHUTDOWN or 5.0
        iwtd = serversockets.keys()
        try:
          iwtd, owtd, ewtd = select(iwtd, [], iwtd, delay)
        except Exception, e:
          from select import error as select_error
          if not isinstance(e, select_error):
            raise
          iwtd, owtd, ewtd = [], [], []
        if ewtd:
          if game:
            game.socketerrors(ewtd)
          if ewtd:
            print >> sys.stderr, "Unexpected socket error reported"
          servertimeout = None
        if iwtd:
          for s in iwtd:
            if s in serversockets:
              serversockets[s]()    # call handler
          servertimeout = None
        elif SERVER_SHUTDOWN and not ewtd and not owtd:
          SERVER_SHUTDOWN -= delay
          if SERVER_SHUTDOWN <= 0.001:
            raise SystemExit, "Server shutdown requested."
        elif clients or getattr(game, 'autoreset', 0):
          servertimeout = None
        elif servertimeout is None:
          servertimeout = time() + SERVER_TIMEOUT
        elif time() > servertimeout:
          raise SystemExit, "No more server activity, timing out."
      except KeyboardInterrupt:
        if game is None or not game.FnExcHandler(1):
          raise
      except SystemExit:
        raise
      except:
        if game is None or not game.FnExcHandler(0):
          raise
  finally:
    removesocket('LISTEN')
    removesocket('PING')
    if clients:
      print "Server crash -- waiting for clients to terminate..."
      while clients:
        iwtd = [c.socket for c in clients]
        try:
          iwtd, owtd, ewtd = select(iwtd, [], iwtd, 120.0)
        except KeyboardInterrupt:
          break
        if not (iwtd or owtd or ewtd):
          break   # timeout - give up
        for c in clients[:]:
          if c.socket in ewtd:
            c.disconnect("select reported an error")
          elif c.socket in iwtd:
            try:
              data = c.socket.recv(2048)
            except error, e:
              c.disconnect(e)
            else:
              if not data and not hasattr(c.socket, 'RECV_CAN_RETURN_EMPTY'):
                c.disconnect("end of data")
    print "Server closed."

def closeeverything():
  global SERVER_SHUTDOWN
  SERVER_SHUTDOWN = 2.5
  if game is not None:
    game.FnServerInfo("Server is stopping!")

# ____________________________________________________________

try:
  from localmsg import recordfilename
except ImportError:
  pass
else:

  class RecordFile:
    proto = 2
    has_sound = 0

    def __init__(self, filename, sampling=1/7.77):
      self.filename = filename
      self.f = None
      self.sampling = sampling
      self.msgl = []
      self.write = self.msgl.append

    def start(self):
      if not self.f:
        import gzip, atexit
        self.f = gzip.open(self.filename, 'wb')
        atexit.register(self.f.close)
        self.recnext = time() + self.sampling

    def udpdata(self, udpdata):
      if self.f:
        now = time()
        if now >= self.recnext:
          while now >= self.recnext:
            self.recnext += self.sampling
          self.write(message(MSG_RECORDED, udpdata))
          self.f.write(''.join(self.msgl))
          del self.msgl[:]
  
  recording = RecordFile(recordfilename)
  del recordfilename
