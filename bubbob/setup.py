#! /usr/bin/env python

from distutils.core import setup
from distutils.extension import Extension

##setup (       name="gencopy",
##              version="0.1",
##              description="generator and iterator state",
##              author="Armin",
##        author_email="arigo@tunes.org",
##              ext_modules=[Extension(name = 'gencopy',
##                               sources = ['gencopy.c'])]
##        )

setup ( name="statesaver",
        version="0.1",
        description="object duplicator working on generators and iterators",
        author="Armin",
        author_email="arigo@tunes.org",
        ext_modules=[Extension(name = 'statesaver',
                               sources = ['statesaver.c'])]
        )
