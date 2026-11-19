# Sintesis efek suara orisinal Juragan Rental (tanpa sampel luar, bebas lisensi).
# Jalankan: python Tools/make_sfx.py  ->  ArtSource/Audio/*.wav (44.1 kHz, 16-bit, mono)

import math
import os
import random
import struct
import wave

SR = 44100
OUT = os.path.join(os.path.dirname(__file__), "..", "ArtSource", "Audio")
random.seed(17)


def silence(sec):
    return [0.0] * int(sec * SR)


def add(buf, src, at=0.0, gain=1.0):
    i0 = int(at * SR)
    need = i0 + len(src)
    if need > len(buf):
        buf.extend([0.0] * (need - len(buf)))
    for i, v in enumerate(src):
        buf[i0 + i] += v * gain
    return buf


def env(n, attack, decay_sec, hold=0.0):
    a = max(1, int(attack * SR))
    h = int(hold * SR)
    out = []
    for i in range(n):
        if i < a:
            out.append(i / a)
        elif i < a + h:
            out.append(1.0)
        else:
            t = (i - a - h) / SR
            out.append(math.exp(-t / max(decay_sec, 1e-4)))
    return out


def tone(freq, sec, shape="sine", decay=None, attack=0.002, hold=0.0, vib=0.0):
    n = int(sec * SR)
    e = env(n, attack, decay if decay else sec, hold)
    out = []
    ph = 0.0
    for i in range(n):
        f = freq * (1.0 + vib * math.sin(2 * math.pi * 5.5 * i / SR))
        ph += 2 * math.pi * f / SR
        if shape == "sine":
            v = math.sin(ph)
        elif shape == "square":
            v = 1.0 if math.sin(ph) >= 0 else -1.0
        elif shape == "saw":
            v = 2.0 * ((ph / (2 * math.pi)) % 1.0) - 1.0
        else:  # triangle
            x = (ph / (2 * math.pi)) % 1.0
            v = 4 * abs(x - 0.5) - 1.0
        out.append(v * e[i])
    return out


def sweep(f0, f1, sec, shape="sine", decay=None):
    n = int(sec * SR)
    e = env(n, 0.003, decay if decay else sec)
    out, ph = [], 0.0
    for i in range(n):
        f = f0 + (f1 - f0) * i / n
        ph += 2 * math.pi * f / SR
        v = math.sin(ph) if shape == "sine" else (2.0 * ((ph / (2 * math.pi)) % 1.0) - 1.0)
        out.append(v * e[i])
    return out


def noise(sec, decay=None, attack=0.001):
    n = int(sec * SR)
    e = env(n, attack, decay if decay else sec)
    return [random.uniform(-1, 1) * e[i] for i in range(n)]


def lowpass(x, cutoff):
    a = math.exp(-2 * math.pi * cutoff / SR)
    y, prev = [], 0.0
    for v in x:
        prev = (1 - a) * v + a * prev
        y.append(prev)
    return y


def highpass(x, cutoff):
    lp = lowpass(x, cutoff)
    return [a - b for a, b in zip(x, lp)]


def bell(freq, sec, decay):
    out = tone(freq, sec, decay=decay)
    add(out, tone(freq * 2.76, sec, decay=decay * 0.5), 0, 0.35)
    add(out, tone(freq * 5.4, sec, decay=decay * 0.25), 0, 0.15)
    return out


def loopify(x, fade=0.25):
    """Buat loop mulus: ekor dicampur ke kepala."""
    f = int(fade * SR)
    body = x[: len(x) - f]
    for i in range(f):
        w = i / f
        body[i] = body[i] * w + x[len(x) - f + i] * (1 - w)
    return body


def save(name, x, peak=0.85):
    m = max(1e-9, max(abs(v) for v in x))
    x = [v / m * peak for v in x]
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name + ".wav")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1, min(1, v)) * 32767)) for v in x))
    print("ok", name, round(len(x) / SR, 2), "s")


# 1. TV tabung menyala: klik saklar, "dung" degauss, desing tipis
x = silence(1.0)
add(x, highpass(noise(0.012), 2000), 0.0, 0.9)
add(x, tone(55, 0.45, decay=0.18), 0.02, 0.9)
add(x, tone(110, 0.45, decay=0.12, vib=0.02), 0.02, 0.4)
whine = tone(9000, 0.9, attack=0.25, decay=2.0)
add(x, whine, 0.08, 0.05)
save("SFX_TV_Nyala", x)

# 2. Stik rusak: plastik retak
x = silence(0.45)
add(x, highpass(noise(0.03, decay=0.01), 1500), 0.0, 1.0)
add(x, tone(1800, 0.12, decay=0.03), 0.0, 0.4)
add(x, highpass(noise(0.03, decay=0.01), 2500), 0.07, 0.7)
add(x, tone(2400, 0.1, decay=0.025), 0.07, 0.3)
save("SFX_Stik_Rusak", x)

# 3. Stik dibanting (rage quit)
x = silence(0.8)
add(x, tone(85, 0.4, decay=0.12), 0.0, 1.0)
add(x, lowpass(noise(0.2, decay=0.05), 900), 0.0, 0.9)
for k, t in enumerate((0.09, 0.16, 0.26)):
    add(x, highpass(noise(0.02, decay=0.006), 2500), t, 0.5 / (k + 1))
save("SFX_Stik_Banting", x)

# 4. Laci kasir "ka-ching"
x = silence(1.1)
add(x, lowpass(noise(0.14, attack=0.05, decay=0.08), 1200), 0.0, 0.5)
add(x, bell(2093, 0.9, 0.35), 0.12, 0.8)
for t in (0.18, 0.22, 0.27, 0.31):
    add(x, tone(random.uniform(5200, 7400), 0.06, decay=0.02), t, 0.12)
save("SFX_Kaching", x)

# 5. MCB turun: klak + dengung listrik padam
x = silence(0.7)
add(x, highpass(noise(0.006), 1000), 0.0, 1.0)
add(x, tone(120, 0.12, decay=0.04), 0.0, 0.8)
hum = [a + 0.5 * b for a, b in zip(tone(100, 0.6, decay=0.15), tone(200, 0.6, decay=0.1))]
add(x, hum, 0.0, 0.35)
save("SFX_MCB_Turun", x)

# 6. MCB naik: klik + dengung menyala
x = silence(0.5)
add(x, highpass(noise(0.005), 1500), 0.0, 0.9)
add(x, tone(100, 0.45, attack=0.2, decay=0.6), 0.02, 0.25)
save("SFX_MCB_Naik", x)

# 7. Desis memasak (loop)
x = highpass(noise(3.4, attack=0.0, decay=99), 1800)
x = [v * 0.5 for v in x]
for _ in range(60):
    add(x, highpass(noise(0.008, decay=0.002), 3000), random.uniform(0, 3.3), random.uniform(0.3, 0.9))
save("LOOP_Masak", loopify(x, 0.3), peak=0.5)

# 8. Ding mi matang
x = silence(1.3)
add(x, bell(1318, 1.2, 0.45), 0.0, 1.0)
save("SFX_Ding", x)

# 9. Tombol HP jadul
save("SFX_HP_Tombol", tone(1000, 0.07, shape="square", decay=0.04), peak=0.4)
x = silence(0.3)
add(x, tone(1320, 0.08, shape="square", decay=0.05), 0.0)
add(x, tone(1760, 0.1, shape="square", decay=0.06), 0.09)
save("SFX_HP_Buka", x, peak=0.4)

# 10. Notifikasi lembut
x = silence(0.4)
add(x, tone(880, 0.2, decay=0.1), 0.0)
add(x, tone(1175, 0.25, decay=0.12), 0.1)
save("SFX_Notif", x, peak=0.5)

# 11. Kardus diangkat/diletakkan
x = silence(0.4)
add(x, lowpass(noise(0.1, decay=0.03), 700), 0.0, 1.0)
add(x, tone(150, 0.2, decay=0.06), 0.0, 0.6)
save("SFX_Kardus", x)

# 12. Pasang unit: ratchet obeng
x = silence(0.9)
for k in range(7):
    t = 0.05 + k * 0.11
    add(x, highpass(noise(0.01, decay=0.003), 2000), t, 0.8)
    add(x, tone(3100, 0.05, decay=0.015), t, 0.25)
save("SFX_Pasang", x)

# 13. Bel pintu "ting-tong" (pelanggan datang)
x = silence(1.6)
add(x, bell(784, 1.0, 0.5), 0.0, 1.0)
add(x, bell(622, 1.2, 0.6), 0.35, 1.0)
save("SFX_Bel_Pintu", x, peak=0.55)

# 14. Modem dial-up (internet lemot)
x = silence(2.2)
for k, (f1, f2) in enumerate([(697, 1209), (770, 1336), (852, 1477), (941, 1336)]):
    t = k * 0.13
    add(x, tone(f1, 0.1, decay=1), t, 0.5)
    add(x, tone(f2, 0.1, decay=1), t, 0.5)
add(x, sweep(1200, 2400, 0.35), 0.6, 0.5)
scr = highpass(noise(1.1, decay=1.5), 1500)
add(x, [v * (0.6 + 0.4 * math.sin(i / 30)) for i, v in enumerate(scr)], 1.0, 0.6)
add(x, tone(2100, 1.0, shape="square", decay=0.8), 1.05, 0.15)
save("SFX_Modem", x, peak=0.6)

# 15. Error PC / virus
x = silence(0.6)
for k in range(3):
    add(x, tone(150, 0.12, shape="square", decay=0.2), k * 0.17, 0.6)
save("SFX_Error_PC", x, peak=0.5)

# 16. Musik chiptune dari TV (loop orisinal, 2 bar)
bpm, step = 140, 60 / 140 / 2
melody = [659, 784, 880, 784, 659, 587, 523, 587, 659, 880, 988, 880, 784, 659, 587, 659]
bass = [131, 131, 165, 165, 175, 175, 196, 196]
x = silence(len(melody) * step + 0.3)
for i, f in enumerate(melody):
    add(x, tone(f, step * 0.9, shape="square", decay=step * 0.8), i * step, 0.18)
for i, f in enumerate(bass):
    add(x, tone(f, step * 1.9, shape="triangle", decay=step * 3), i * step * 2, 0.35)
save("LOOP_Game_PS", loopify(x, 0.05), peak=0.5)

# 17. Suasana garasi: dengung ruangan + motor lewat sesekali (loop 14 dtk)
x = lowpass(noise(14.4, attack=0.0, decay=999), 180)
x = [v * 0.8 for v in x]
for t0 in (2.0, 8.5):
    n = int(3.0 * SR)
    motor, ph = [], 0.0
    for i in range(n):
        u = i / n
        f = 95 + 20 * (1 - u)                      # efek doppler sederhana
        ph += 2 * math.pi * f / SR
        amp = math.sin(math.pi * u) ** 2           # mendekat lalu menjauh
        motor.append((2.0 * ((ph / (2 * math.pi)) % 1.0) - 1.0) * amp)
    add(x, lowpass(motor, 700), t0, 0.5)
save("LOOP_Suasana", loopify(x, 0.4), peak=0.35)
