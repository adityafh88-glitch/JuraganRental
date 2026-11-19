# Lagu radio SEMENTARA (MUS_Radio_1): gitar akustik petik santai, dibuat dengan sintesis Karplus-Strong.
# Akan diganti lagu dari ElevenLabs Music (lihat Docs/Naskah_Musik.md) dengan nama file yang sama.
import math, os, random, struct, wave

SR = 32000
BPM = 84
BEAT = 60.0 / BPM
OUT = os.path.join(os.path.dirname(__file__), "..", "ArtSource", "Audio", "Music", "MUS_Radio_1.wav")
random.seed(7)


def midi(n):
    return 440.0 * 2 ** ((n - 69) / 12.0)


def pluck(freq, sec, bright=0.5, amp=1.0):
    """Senar dipetik (Karplus-Strong) dengan sedikit peredaman agar lembut."""
    n = int(sec * SR)
    period = max(2, int(SR / freq))
    buf = [random.uniform(-1, 1) for _ in range(period)]
    # haluskan awal agar tidak terlalu cempreng
    for _ in range(int(3 * (1 - bright))):
        buf = [(buf[i] + buf[i - 1]) * 0.5 for i in range(period)]
    out = []
    decay = 0.996
    for i in range(n):
        j = i % period
        v = buf[j]
        out.append(v * amp)
        buf[j] = decay * 0.5 * (v + buf[(j + 1) % period])
    return out


def add(dst, src, at, gain):
    s = int(at * SR)
    for i, v in enumerate(src):
        k = s + i
        if k >= len(dst):
            break
        dst[k] += v * gain


# Progresi santai: C - Am - F - G (dua putaran) lalu Am - F - C - G
CHORDS = {
    "C": [48, 52, 55, 60, 64], "Am": [45, 52, 57, 60, 64], "F": [41, 48, 53, 57, 60],
    "G": [43, 50, 55, 59, 62], "Em": [40, 47, 52, 55, 59],
}
PROG = ["C", "Am", "F", "G", "C", "Am", "F", "G", "Am", "F", "C", "G", "F", "G", "C", "C"]
BARS = PROG * 2
total = len(BARS) * 4 * BEAT + 3.0
mix = [0.0] * int(total * SR)

# Pola petikan per birama (ketukan 8-an): bass, lalu senar atas bergantian
PATTERN = [0, 3, 2, 4, 1, 3, 2, 4]
for b, name in enumerate(BARS):
    notes = CHORDS[name]
    t0 = b * 4 * BEAT
    for k, idx in enumerate(PATTERN):
        at = t0 + k * BEAT / 2 + random.uniform(-0.008, 0.008)  # sedikit "manusiawi"
        gain = 0.55 if idx == 0 else 0.32 * random.uniform(0.8, 1.0)
        add(mix, pluck(midi(notes[idx]), 1.6, bright=0.4 if idx == 0 else 0.6), at, gain)

# Melodi sederhana di putaran kedua (pentatonik C), lembut
SCALE = [72, 74, 76, 79, 81, 84]
t = len(PROG) * 4 * BEAT
while t < total - 4.0:
    note = random.choice(SCALE)
    dur = random.choice([1, 1, 2, 2, 3]) * BEAT / 2
    add(mix, pluck(midi(note), 1.2, bright=0.7), t, 0.22)
    t += dur + (BEAT / 2 if random.random() < 0.25 else 0)

# Shaker pelan di ketukan 8-an
for i in range(int((total - 3.0) / (BEAT / 2))):
    at = i * BEAT / 2
    n = int(0.05 * SR)
    hs = [random.uniform(-1, 1) * math.exp(-j / (0.012 * SR)) for j in range(n)]
    hs = [hs[j] - (hs[j - 1] if j else 0) for j in range(n)]  # highpass kasar
    add(mix, hs, at, 0.035 if i % 2 else 0.05)

# Fade in/out dan normalisasi
fade = int(1.5 * SR)
for i in range(fade):
    mix[i] *= i / fade
    mix[-1 - i] *= i / fade
peak = max(abs(v) for v in mix) or 1.0
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with wave.open(OUT, "wb") as w:
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(SR)
    w.writeframes(b"".join(struct.pack("<h", int(v / peak * 0.7 * 32767)) for v in mix))
print("ok", OUT, round(total, 1), "s")
