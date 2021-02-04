
 ################################################
##     pygame-based implementation of xshm     ##
################################################

import os, sys
from Xlib import X, display


# -*-*- SLOOWWW -*-*-
import psyco; psyco.full()


class Display:
    
    def __init__(self, width, height, title):
        self.dpy = display.Display()
        self.default_scr = self.dpy.screen()
        self.root = self.default_scr.root
        self.width = width
        self.height = height
        self.depth = self.default_scr.root_depth
        
        self.backpixmap = self.root.create_pixmap(width, height, self.depth)
        self.win = self.root.create_window(
            0, 0, width, height, 0, self.depth,
            override_redirec = 0,
            background_pixel = self.default_scr.black_pixel,
            backing_store = X.NotUseful,
            )
        self.win.map()
        
        self.gc     = self.win.create_gc()
        self.gc_and = self.win.create_gc()
        self.gc_or  = self.win.create_gc()

        self.gc.change(foreground = self.default_scr.black_pixel)
        self.gc_and.change(function = X.GXand)
        self.gc_or .change(function = X.GXor)

        self.selectinput = 0
        self.keyev = []
        self.mouseev = []
        self.motionev = None
        self.dpy.flush()

        pixel = "\x00\x00\x80"
        hole  = "\x01\x01\x01"
        self.taskbkgnd = self.pixmap(32, 32,
                                     ((pixel+hole)*16 + (hole+pixel)*16) * 16,
                                     0x010101)

    def pixmap(self, w, h, data, colorkey=-1):
        print >> sys.stderr, '.',
        extent = w*h
        depth = self.depth
        if depth >= 24:
            bitmap_pad = 32
        else:
            bitmap_pad = 16
        scanline = ((w+bitmap_pad-1) & ~(bitmap_pad-1)) / 8;
        if colorkey >= 0:
            key = (chr(colorkey >> 16) +
                   chr((colorkey>>8) & 0xFF) +
                   chr(colorkey & 0xFF))
        else:
            key = None
        if depth == 15:
            p_size = 5, 5, 5
        elif depth == 16:
            p_size = 5, 6, 5
        elif depth == 24 or depth == 32:
            p_size = 8, 8, 8
        else:
            raise ValueError, 'unsupported screen depth %d' % depth

        imgdata = []
        maskdata = []

        for color in range(3):
            plane = 128
            while plane >= (1<<(8-p_size[color])):
                src = 0
                for y in range(h):
                    imgline = 0L
                    maskline = 0L
                    shifter = 1L
                    for x in range(w):
                        if data[src:src+3] == key:
                            # transparent
                            maskline |= shifter
                        elif ord(data[src+color]) & plane:
                            imgline |= shifter
                        shifter <<= 1
                        src += 3
                    imgdata.append(long2string(imgline, scanline))
                    maskdata.append(long2string(maskline, scanline))
                plane /= 2

        imgdata = ''.join(imgdata)
        if colorkey >= 0:
            maskdata = ''.join(maskdata)
            mask = self.win.create_pixmap(w, h, depth)
            mask.put_image(self.gc, 0, 0, w, h, X.XYPixmap, depth, 0, maskdata)
        else:
            mask = None
        imgdata = ''.join(imgdata)
        image = self.win.create_pixmap(w, h, depth)
        image.put_image(self.gc, 0, 0, w, h, X.XYPixmap, depth, 0, imgdata)
        image.mask = mask
        image.size = w, h
        return image

    def getppm(self, (x, y, w, h), bkgnd=None):
        if bkgnd is None:
            bkgnd = self.win.create_pixmap(w, h, self.depth)
            bkgnd.mask = None
            bkgnd.size = w, h
        bkgnd.copy_area(self.gc, self.backpixmap, x, y, w, h, 0, 0)
        return bkgnd

    def putppm(self, x, y, image, rect=None):
        if rect:
            x1, y1, w1, h1 = rect
        else:
            x1 = y1 = 0
            w1, h1 = image.size
        if image.mask is None:
            self.backpixmap.copy_area(self.gc, image, x1, y1, w1, h1, x, y)
        else:
            self.backpixmap.copy_area(self.gc_and, image.mask,
                                      x1, y1, w1, h1, x, y)
            self.backpixmap.copy_area(self.gc_or, image,
                                      x1, y1, w1, h1, x, y)

    def flip(self):
        self.win.copy_area(self.gc, self.backpixmap,
                           0, 0, self.width, self.height, 0, 0)
        self.dpy.flush()
        self.readXevents()

    def close(self):
        self.dpy.close()

    def clear(self):
        self.backpixmap.fill_rectangle(self.gc, 0, 0, self.width, self.height)

    def readXevents(self):
        n = self.dpy.pending_events()
        if n:
            for i in range(n):
                event = self.dpy.next_event()
                if event.type == X.KeyPress or event.type == X.KeyRelease:
                    self.keyev.append((event.detail, event.type))
                elif event.type == X.ButtonPress:
                    self.mouseev.append((event.event_x, event.event_y))
                elif event.type == X.MotionNotify:
                    self.motionev = event.event_x, event.event_y
                elif event.type == X.DestroyNotify:
                    raise SystemExit
            self.readXevents()

    def enable_event(self, mask):
        self.selectinput |= mask
        self.win.change_attributes(event_mask=self.selectinput)

    def keyevents(self):
        if not (self.selectinput & X.KeyReleaseMask):
            self.enable_event(X.KeyPressMask | X.KeyReleaseMask)
        self.readXevents()
        result = self.keyev
        self.keyev = []
        return result

    def mouseevents(self):
        if not (self.selectinput & X.ButtonPressMask):
            self.enable_event(X.ButtonPressMask)
        result = self.mouseev
        self.mouseev = []
        return result

    def pointermotion(self):
        result = self.motionev
        self.motionev = None
        return result

    def has_sound(self):
        return 0

    def selectlist(self):
        from socket import fromfd, AF_INET, SOCK_STREAM
        return [fromfd(self.dpy.fileno(), AF_INET, SOCK_STREAM)]

    def taskbar(self, (x, y, w, h)):
        for j in range(y, y+h, 32):
            for i in range(x, x+w, 32):
                self.putppm(i, j, self.taskbkgnd,
                            (0, 0, x+w-i, y+h-j))


def long2string(bits, strlen):
    return ''.join([chr((bits>>n)&0xFF) for n in range(0, 8*strlen, 8)])
