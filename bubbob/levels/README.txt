===============================
How to make your own level sets
===============================

The .bin files are MacBinary files from the original MacOS9 game.  The
.py files are the levels that we did ourselves.  To make new levels, you
need to make a new .py file in the current directory (bubbob/levels/).

For an example, open CompactLevels.py with any text editor.  The
structure should be fairly simple to understand even without knowledge
about Python.  (Just don't try to look at RandomLevels.py first: it is a
full Python program that generates the levels completely randomly.)

To get started, copy CompactLevels.py to a new name, like MyLevels.py,
and start editing it.  You can remove or change levels, but the last one
should always be called LevelFinal and have no monster in it.  All other
levels should have monsters, otherwise they'll be considered as the
final level.

Also note that all levels need to have the same size (width and height),
but different level sets can have different sizes.  For example, the
levels in CompactLevels.py are a bit smaller than the ones in the .bin
files, and the levels in scratch.py are a bit larger.
