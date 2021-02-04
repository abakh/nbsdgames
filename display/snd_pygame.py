import sys
from modes import musichtmloptiontext as htmloptionstext
from pygame.locals import *
import pygame.mixer

if pygame.mixer is None:
    raise ImportError


#ENDMUSICEVENT = USEREVENT


class Sound:
    has_sound = has_music = 0   # until initialized

    def __init__(self):
        try:
            pygame.mixer.init()
        except pygame.error, e:
            print >> sys.stderr, "sound disabled: %s" % str(e)
        else:
            self.has_sound = 1
            try:
                from pygame.mixer import music
            except ImportError:
                pass
            else:
                self.has_music = music is not None
        self.cmusics = None

    def close(self):
        try:
            pygame.mixer.stop()
        except pygame.error:
            pass
        if self.has_music:
            try:
                pygame.mixer.music.stop()
            except pygame.error:
                pass

    def sound(self, f):
        return pygame.mixer.Sound(f.freezefilename())

    def flop(self):
        # the events are not processed if pygame is not also the display,
        # so ENDMUSICEVENT will not arrive -- poll for end of music
        if self.cmusics and not pygame.mixer.music.get_busy():
            self.next_music()

    def play(self, sound, lvolume, rvolume):
        channel = pygame.mixer.find_channel(1)
        channel.stop()
        try:
            channel.set_volume(lvolume, rvolume)
        except TypeError:
            channel.set_volume(0.5 * (lvolume+rvolume))
        channel.play(sound)

    def play_musics(self, musics, loop_from):
        #dpy_pygame.EVENT_HANDLERS[ENDMUSICEVENT] = self.next_music
        #pygame.mixer.music.set_endevent(ENDMUSICEVENT)
        self.cmusics = musics, loop_from, 0
        self.next_music()

    def next_music(self, e=None):
        if self.cmusics:
            musics, loop_from, c = self.cmusics
            if c >= len(musics):  # end
                c = loop_from
                if c >= len(musics):
                    pygame.mixer.music.stop()
                    self.cmusics = None
                    return
            pygame.mixer.music.load(musics[c].freezefilename())
            pygame.mixer.music.play()
            self.cmusics = musics, loop_from, c+1

    def fadeout(self, millisec):
        #print "fadeout:", millisec
        pygame.mixer.music.fadeout(millisec)
        self.cmusics = None
