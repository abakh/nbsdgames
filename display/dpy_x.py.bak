import sys
import xshm
from modes import BaseDisplay
from cStringIO import StringIO


class Display(BaseDisplay):
    
    def __init__(self, width, height, title, shm='yes'):
        use_shm = not shm.startswith('n')
        self.xdpy = xdpy = xshm.Display(width, height, use_shm)
        self.pixmap = xdpy.pixmap
        self.getppm = xdpy.getppm
        self.putppm = xdpy.putppm
        self.overlayppm = xdpy.overlayppm
        self.close  = xdpy.close
        self.clear  = xdpy.clear
        self.flip   = xdpy.flip
        self.keyevents = xdpy.keyevents
        self.mouseevents = xdpy.mouseevents
        self.pointermotion = xdpy.pointermotion
        if use_shm and not xdpy.shmmode():
            print >> sys.stderr, \
                  "Note: cannot use SHM extension (%dx%d), display will be slow." % \
                  (width, height)

    def selectlist(self):
        if hasattr(self.xdpy, 'fd'):
            from socket import fromfd, AF_INET, SOCK_STREAM
            return [fromfd(self.xdpy.fd(), AF_INET, SOCK_STREAM)]
        else:
            return []


def htmloptionstext(nameval):
    return '''
<%s> Use the shared memory extension</input><%s><br>
<font size=-1>Note: Disable it for remote connections or old X servers</font>
''' % (nameval("checkbox", "shm", "yes", default="yes"),
       nameval("hidden", "shm", "no"))
