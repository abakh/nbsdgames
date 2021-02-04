
 ################################################
##       GTK-based implementation of xshm      ##
################################################

import os, sys, math
from modes import KeyPressed, KeyReleased
import caching

def import_trickery():
    global gtk, gdk
    argv = sys.argv[:]
    del sys.argv[1:]
    import gtk
    from gtk import gdk
    sys.argv[:] = argv
import_trickery()


class Display:
    
    def __init__(self, width, height, title, zoom="100"):
        if zoom.endswith('%'):
            zoom = zoom[:-1]
        scale = float(zoom) / 100.0
        iscale = int(scale+0.001)
        if abs(scale - iscale) < 0.002:
            scale = iscale
        self.scale = scale
        
        self.width  = int(width * scale)
        self.height = int(height * scale)
        self.tempppmfile = caching.mktemp('.ppm')

        # create a top level window
        w = self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        w.connect("destroy", lambda w: sys.exit())
        w.connect("key-press-event", self.key_press_event)
        w.connect("key-release-event", self.key_release_event)
        w.connect("motion-notify-event", self.motion_notify_event)
        w.connect("button-press-event", self.button_press_event)
        w.add_events(gdk.KEY_PRESS_MASK |
                     gdk.POINTER_MOTION_MASK |
                     gdk.BUTTON_PRESS_MASK)
        w.resize(self.width, self.height)
        w.set_title(title)
        w.show()

        self.offscreen = gtk.create_pixmap(w.window, self.width, self.height)
        self.gc = gdk.gc_new(w.window)
        self.gc.set_rgb_fg_color(gdk.color_parse('#000000'))

        self.events_key = []
        self.events_mouse = []
        self.event_motion = None

        pixel = "\x00\x00\x80"
        hole  = "\x01\x01\x01"
        pb = self.pixmap(32, 32, ((pixel+hole)*16 + (hole+pixel)*16) * 16, 0x010101)
        self.taskbkgnd = self.renderpixbuf(pb)

    def taskbar(self, (x, y, w, h)):
        scale = self.scale
        x2 = x+w
        y2 = y+h
        x, y, x2, y2 = int(x*scale), int(y*scale), int(x2*scale), int(y2*scale)
        pixmap, gc, ignored = self.taskbkgnd
        for j in range(y, y2, 32):
            for i in range(x, x2, 32):
                gc.set_clip_origin(i, j)
                self.offscreen.draw_drawable(gc, pixmap, 0, 0,
                                             i, j, x2-i, y2-j)

    def pixmap(self, w, h, data, colorkey=-1):
        filename = self.tempppmfile
        f = open(filename, 'wb')
        print >> f, 'P6'
        print >> f, w, h
        print >> f, 255
        f.write(data)
        f.close()
        pb = gdk.pixbuf_new_from_file(filename)
        if colorkey >= 0:
            pb = pb.add_alpha(1, chr(colorkey >> 16),
                              chr((colorkey >> 8) & 0xFF),
                              chr(colorkey & 0xFF))
        if self.scale == 1:
            return self.renderpixbuf((pb,))
        else:
            return (pb,)

    def renderpixbuf(self, input):
        if len(input) == 3:
            return input
        pb, = input
        pixmap, mask = pb.render_pixmap_and_mask()
        if mask is not None:
            gc = gdk.gc_new(self.window.window)
            gc.set_clip_mask(mask)
            return (pixmap, gc, mask)
        else:
            return (pixmap, self.gc, None)

    def getopticon(self, input, (x, y, w, h), ignored_alpha=255):
        if len(input) == 3:
            return None
        pb, = input
        scale = self.scale
        newpb = gdk.Pixbuf("rgb", 1, 8, w, h)
        newpb.fill(0)
        pb.copy_area(x, y, w, h, newpb, 0, 0)
        newpb = newpb.scale_simple(int(w*scale), int(h*scale),
                                   gdk.INTERP_HYPER)
        if newpb is None:
            return None, None, None
        else:
            return self.renderpixbuf((newpb,))

    def getppm(self, (x, y, w, h), int=int, ceil=math.ceil):
        scale = self.scale
        if isinstance(scale, int):
            x *= scale
            y *= scale
            w *= scale
            h *= scale
        else:
            w = int(ceil((x+w)*scale))
            h = int(ceil((y+h)*scale))
            x = int(x*scale)
            y = int(y*scale)
            w -= x
            h -= y
        bkgnd = gtk.create_pixmap(self.window.window, w, h)
        bkgnd.draw_drawable(self.gc, self.offscreen, x, y, 0, 0, w, h)
        return bkgnd, self.gc, None

    def putppm(self, x, y, (pixmap, gc, ignored), rect=None, int=int):
        if pixmap is None:
            return
        scale = self.scale
        if rect is None:
            srcx = srcy = 0
            w = h = 4095
        else:
            srcx, srcy, w, h = rect
        x = int(x*scale)
        y = int(y*scale)
        if gc is not self.gc:
            gc.set_clip_origin(x-srcx, y-srcy)
        self.offscreen.draw_drawable(gc, pixmap, srcx, srcy, x, y, w, h)

    def flip(self):
        self.window.window.draw_drawable(self.gc, self.offscreen,
                                         0, 0, 0, 0, self.width, self.height)
        gdk.flush()
        self.events_poll()

    def close(self):
        self.window.destroy()

    def clear(self):
        self.offscreen.draw_rectangle(self.gc, 1,
                                      0, 0, self.width, self.height)

    def events_poll(self):
        while gtk.events_pending():
            gtk.main_iteration()

    def key_press_event(self, window, event):
        self.events_key.append((event.keyval, KeyPressed))

    def key_release_event(self, window, event):
        self.events_key.append((event.keyval, KeyReleased))

    def motion_notify_event(self, window, event):
        self.event_motion = (int(event.x/self.scale), int(event.y/self.scale))

    def button_press_event(self, window, event):
        self.events_mouse.append((int(event.x/self.scale), int(event.y/self.scale)))

    def keyevents(self):
        self.events_poll()
        result = self.events_key
        self.events_key = []
        return result

    def pointermotion(self):
        result = self.event_motion
        self.event_motion = None
        return result

    def mouseevents(self):
        self.events_poll()
        result = self.events_mouse
        self.events_mouse = []
        return result
    
    def selectlist(self):
        return []


def htmloptionstext(nameval):
    return 'Scale image by <%s size=5>%%' % (
        nameval('text', 'zoom', default='100'))
