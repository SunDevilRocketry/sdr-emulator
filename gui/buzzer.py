import sounddevice as sd
import numpy as np

# FC buzzer stand-in
class Buzzer:
    # Configuration
    def __init__(self):
        self.samplerate = 44100  # Sampling rate
        self.frequency = 880.0 
        self.start_idx = 0
        self.stream = None

    # Generate a new chunk of the wave
    def __new_chunk(self, outdata, frames, time, status):
        t = (self.start_idx + np.arange(frames)) / self.samplerate
        t = t.reshape(-1, 1)
        outdata[:] = 0.5 * np.sin(2 * np.pi * self.frequency * t)
        self.start_idx += frames

    # Start playing the tone
    def start_tone(self):
        if self.stream is None:
            self.stream = sd.OutputStream(channels=1, callback=self.__new_chunk, samplerate=self.samplerate)
            self.stream.start()

    # Stop playing the tone
    def stop_tone(self):
        if self.stream is not None:
            self.stream.stop()
            self.stream.close()
            self.stream = None