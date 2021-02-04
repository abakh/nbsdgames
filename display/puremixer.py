import sys, audioop


class PureMixer:
    #
    #  An audio mixer in Python based on audioop
    #
    #  Note that opening the audio device itself is outside the scope of
    #  this module.  Anything else could also be done with the mixed data,
    #  e.g. stored on disk, for all this module knows.

    def __init__(self, freq=44100, bits=8, signed=0,
                       channels=1, byteorder=None):
        """Open the mixer and set its parameters."""
        self.freq = freq
        self.bytes = bits/8
        self.signed = signed
        self.channels = channels
        self.byteorder = byteorder or sys.byteorder
        self.parameters = (freq, self.bytes, signed, channels, self.byteorder)
        self.bytespersample = channels*self.bytes
        self.queue = '\x00' * self.bytes

    def resample(self, data, freq=44100, bits=8, signed=0,
                             channels=1, byteorder=None):
        "Convert a sample to the mixer's own format."
        bytes = bits/8
        byteorder = byteorder or sys.byteorder
        if (freq, bytes, signed, channels, byteorder) == self.parameters:
            return data
        # convert to native endianness
        if byteorder != sys.byteorder:
            data = byteswap(data, bytes)
            byteorder = sys.byteorder
        # convert unsigned -> signed for the next operations
        if not signed:
            data = audioop.bias(data, bytes, -(1<<(bytes*8-1)))
            signed = 1
        # convert stereo -> mono
        while channels > self.channels:
            assert channels % 2 == 0
            data = audioop.tomono(data, bytes, 0.5, 0.5)
            channels /= 2
        # resample to self.freq
        if freq != self.freq:
            data, ignored = audioop.ratecv(data, bytes, channels,
                                           freq, self.freq, None)
            freq = self.freq
        # convert between 8bits and 16bits
        if bytes != self.bytes:
            data = audioop.lin2lin(data, bytes, self.bytes)
            bytes = self.bytes
        # convert mono -> stereo
        while channels < self.channels:
            data = audioop.tostereo(data, bytes, 1.0, 1.0)
            channels *= 2
        # convert signed -> unsigned
        if not self.signed:
            data = audioop.bias(data, bytes, 1<<(bytes*8-1))
            signed = 0
        # convert to mixer endianness
        if byteorder != self.byteorder:
            data = byteswap(data, bytes)
            byteorder = self.byteorder
        # done
        if (freq, bytes, signed, channels, byteorder) != self.parameters:
            raise ValueError, 'sound sample conversion failed'
        return data

    def wavesample(self, file):
        "Read a sample from a .wav file (or file-like object)."
        import wave
        w = wave.open(file, 'r')
        return self.resample(w.readframes(w.getnframes()),
                             freq = w.getframerate(),
                             bits = w.getsampwidth() * 8,
                             signed = w.getsampwidth() > 1,
                             channels = w.getnchannels(),
                             byteorder = 'little')

    def mix(self, mixer_channels, bufsize):
        """Mix the next batch buffer.
        Each object in the mixer_channels list must be a file-like object
        with a 'read(size)' method."""
        data = ''
        already_seen = {}
        channels = mixer_channels[:]
        channels.reverse()
        for c in channels:
            if already_seen.has_key(c):
                data1 = ''
            else:
                data1 = c.read(bufsize)
                already_seen[c] = 1
            if data1:
                l = min(len(data), len(data1))
                data = (audioop.add(data[:l], data1[:l], 1) +
                        (data1[l:] or data[l:]))
            else:
                try:
                    mixer_channels.remove(c)
                except ValueError:
                    pass
        data += self.queue * ((bufsize - len(data)) / self.bytes)
        self.queue = data[-self.bytes:]
        return data


def byteswap(data, byte):
    if byte == 1:
        return
    if byte == 2:
        typecode = 'h'
    elif byte == 4:
        typecode = 'i'
    else:
        raise ValueError, 'cannot convert endianness for samples of %d bytes' % byte
    import array
    a = array.array(typecode, data)
    if a.itemsize != byte:
        raise ValueError, 'endianness convertion failed'
    a.byteswap()
    return a.tostring()
