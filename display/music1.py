class Music:
    def __init__(self, filename):
        self.filename = filename
        self.w = None
        self.sampledata = ''
    def openchannel(self):
        if self.w is not None:
            self.w.close()
        import wave
        self.w = w = wave.open(open(self.filename, 'rb'), 'r')
        self.w_params = (w.getnchannels(),
                         w.getsampwidth(),
                         w.getframerate())
        chan, width, freq = self.w_params
        self.dataleft = w.getnframes() * (chan*width)
        self.sampledata = ''
    def decode(self, mixer, bytecount):
        result = self.sampledata
        if not result and self.dataleft > 0:
            # decode and convert some more data
            chan, width, freq = self.w_params
            #framecount = bytecount / (chan*width)
            inputdata = self.w.readframes(bytecount)  #(framecount)
            self.dataleft -= len(inputdata)
            result = mixer.resample(inputdata,
                                    freq = freq,
                                    bits = width * 8,
                                    signed = width > 1,
                                    channels = chan,
                                    byteorder = 'little')
            #print len(result)
        self.sampledata = result[bytecount:]
        return result[:bytecount]
