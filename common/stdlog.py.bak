import sys, os
from time import localtime, ctime


class LogFile:
    
    def __init__(self, filename=None, limitsize=32768):
        if filename is None:
            filename = sys.argv[0]
            if filename.endswith('.py'):
                filename = filename[:-3]
            filename += '.log'
        self.limitsize = limitsize
        if not self._open(filename):
            import tempfile
            filename = os.path.join(tempfile.gettempdir(),
                                    os.path.basename(filename))
            if not self._open(filename):
                self.f = self.filename = None
        self.lasttime = None

    def close(self):
        if self.f is not None:
            self.f.close()
            self.f = None

    def __nonzero__(self):
        return self.f is not None

    def _open(self, filename):
        try:
            self.f = open(filename, 'r+', 1)
            self.f.seek(0, 2)
        except IOError:
            # The open r+ might have failed simply because the file
            # does not exist. Try to create it.
            try:
                self.f = open(filename, 'w+', 1)
            except (OSError, IOError):
                return 0
        except OSError:
            return 0
        self.filename = filename
        if self.f.tell() > 0:
            print >> self.f
            print >> self.f, '='*44
        return 1

    def _check(self):
        if self.f is None:
            return 0
        lt = localtime()
        if lt[:4] != self.lasttime:
            self.lasttime = lt[:4]
            if self.f.tell() >= self.limitsize:
                self.f.seek(-(self.limitsize>>1), 1)
                data = self.f.read()
                self.f.seek(0)
                self.f.write('(...)' + data)
                self.f.truncate()
            self.f.write('========= %s =========\n' % ctime())
        return 1

    def write(self, data):
        if self._check():
            self.f.write(data)

    def writelines(self, data):
        if self._check():
            self.f.writelines(data)

    def flush(self):
        if self._check():
            self.f.flush()


class Logger:
    stdout_captured = 0
    stderr_captured = 0
    
    def __init__(self, f):
        self.targets = [f]

    def capture_stdout(self):
        if not Logger.stdout_captured:
            self.targets.append(sys.stdout)
            sys.stdout = self
            Logger.stdout_captured = 1

    def capture_stderr(self):
        if not Logger.stderr_captured:
            self.targets.append(sys.stderr)
            sys.stderr = self
            Logger.stderr_captured = 1

    def write(self, data):
        for f in self.targets:
            f.write(data)

    def writelines(self, data):
        for f in self.targets:
            f.writelines(data)

    def flush(self):
        for f in self.targets:
            f.flush()
