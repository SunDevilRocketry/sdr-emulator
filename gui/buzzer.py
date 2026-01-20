import numpy as np
import sounddevice as sd

# FC buzzer stand-in
class Buzzer:
    def beep_buzzer(self, freq=880, duration=1.0, sample_rate=44100, volume=0.3):
        t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
    
        # Sawtooth wave from -1 to 1
        wave = 2.0 * (freq * t - np.floor(0.5 + freq * t))

        sd.play(volume * wave, sample_rate)
        sd.wait()