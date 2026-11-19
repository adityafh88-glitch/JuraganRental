# Suara boot konsol "PlayStasiun" orisinal (bukan rekaman/tiruan melodi konsol asli).
# Susunan: klik saklar -> dentuman dalam -> desir naik -> akor berkilau megah -> gema.
import math, os, random, sys
sys.path.insert(0, os.path.dirname(__file__))
from make_sfx import SR, add, bell, highpass, lowpass, noise, save, silence, tone  # noqa: E402

random.seed(31)
dur = 3.2
x = silence(dur)

# 1) Klik saklar + dentuman sub dalam
add(x, highpass(noise(0.01, decay=0.003), 1500), 0.0, 0.6)
add(x, tone(48, 1.4, attack=0.01, decay=0.5), 0.02, 1.0)
add(x, tone(72, 1.2, attack=0.02, decay=0.4), 0.03, 0.5)

# 2) Desir udara yang naik (noise yang makin terang)
n = int(1.0 * SR)
wh = noise(1.0, attack=0.6, decay=0.35)
bright = highpass(wh, 600)
add(x, [v * (i / n) for i, v in enumerate(bright)], 0.15, 0.35)

# 3) Akor megah (Bb mayor add9, nada orisinal) dengan pad lembut berlapis
chord = [233.1, 293.7, 349.2, 466.2, 523.3]
for k, f in enumerate(chord):
    pad = tone(f, 2.3, attack=0.35, decay=1.1, vib=0.003)
    add(x, pad, 0.75 + k * 0.03, 0.22)
    add(x, tone(f * 2.0, 2.0, attack=0.4, decay=0.9), 0.78 + k * 0.03, 0.06)

# 4) Kilau: lonceng tinggi berurutan cepat, lalu satu nada terakhir yang panjang
for k, f in enumerate([1396.9, 1864.7, 2093.0, 2793.8]):
    add(x, bell(f, 1.4, 0.45), 0.85 + k * 0.07, 0.18)
add(x, bell(3136.0, 1.8, 0.8), 1.2, 0.12)

# 5) Gema halus: salinan tertunda dan diredam
echo = lowpass(x, 3000)
add(x, echo, 0.18, 0.25)
x = x[: int(dur * SR)]
save("SFX_Konsol_Boot", x, peak=0.8)
