#! /usr/bin/env python

from distutils.core import setup
from distutils.extension import Extension

setup ( name="xshm",
        version="0.2",
        description="X window system Shared Memory extension",
        author="Armin & Odie",
        author_email="arigo@tunes.org",
        ext_modules=[Extension(name = 'xshm',
                               sources = ['xshm.c'],
                               include_dirs = ['/usr/X11R6/include'],
                               library_dirs = ['/usr/X11R6/lib'],
                               libraries = ['X11', 'Xext'])]
        )
