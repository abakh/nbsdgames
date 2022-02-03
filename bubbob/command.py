import os, sys

levels, ext = os.path.splitext(os.path.basename(sys.argv[1]))
for ext in ['.py', '.bin']:
    levelfile = 'levels/%s%s' % (levels, ext)
    if os.path.exists(levelfile):
        break
sys.argv[1] = levelfile

exec(compile(open('bb.py', "rb").read(), 'bb.py', 'exec'))
