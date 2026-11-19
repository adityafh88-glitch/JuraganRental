# Bunyi aktivitas unit yang sedang dipakai (pendek, acak, pelan) + motor lewat sesekali.
# Menggantikan dengung suasana dan musik chiptune yang terus berputar.
import math, os, random, sys
sys.path.insert(0, os.path.dirname(__file__))
from make_sfx import SR, add, highpass, lowpass, noise, save, silence, tone  # noqa: E402

random.seed(53)


def click(pitch):
    c = highpass(noise(0.004, decay=0.0015), 2500)
    add(c, tone(pitch, 0.012, decay=0.004), 0.0, 0.35)
    return c


# Klik tombol stik (3 variasi): 4-8 klik plastik cepat
for v in range(1, 4):
    x = silence(0.9)
    t = 0.02
    for _ in range(random.randint(4, 8)):
        add(x, click(random.uniform(2200, 3200)), t, random.uniform(0.5, 1.0))
        t += random.uniform(0.05, 0.13)
    save("SFX_Stik_Klik_%d" % v, x[: int((t + 0.05) * SR)], peak=0.6)

# Tendangan bola dari speaker TV (teredam): "bok"
x = silence(0.4)
add(x, lowpass(noise(0.03, decay=0.01), 900), 0.0, 1.0)
add(x, tone(180, 0.15, decay=0.04), 0.0, 0.7)
save("SFX_TV_Tendang", lowpass(x, 1500), peak=0.6)

# Sorakan "GOOOL" penonton dari TV (teredam, seperti terdengar dari speaker TV tabung)
x = silence(2.0)
crowd = noise(1.8, attack=0.25, decay=0.6)
crowd = lowpass(highpass(crowd, 250), 1300)
add(x, crowd, 0.05, 1.0)
for k in range(6):  # gumam vokal samar
    add(x, lowpass(tone(random.uniform(220, 380), 1.2, attack=0.2, decay=0.5, vib=0.03), 900), 0.1 + k * 0.05, 0.08)
save("SFX_TV_Gol", x, peak=0.55)

# Bip game samar dari TV (2 variasi)
for v, notes in ((1, [988, 1319, 1568]), (2, [784, 659, 523])):
    x = silence(0.5)
    for i, f in enumerate(notes):
        add(x, tone(f, 0.09, shape="square", decay=0.05), i * 0.09, 0.4)
    save("SFX_TV_Bip_%d" % v, lowpass(x, 1400), peak=0.45)

# Ketikan keyboard (3 variasi): rentetan tuts dengan tempo manusiawi
for v in range(1, 4):
    x = silence(1.4)
    t = 0.02
    for _ in range(random.randint(7, 14)):
        k = lowpass(highpass(noise(0.012, decay=0.004), 600), 4500)
        add(k, tone(random.uniform(380, 520), 0.02, decay=0.006), 0.0, 0.3)
        add(x, k, t, random.uniform(0.55, 1.0))
        t += random.uniform(0.06, 0.16) if random.random() > 0.15 else random.uniform(0.2, 0.35)
    if v == 3:  # spasi/enter lebih berat di akhir
        add(x, lowpass(noise(0.02, decay=0.008), 1800), t, 1.0)
        t += 0.05
    save("SFX_Keyboard_%d" % v, x[: int((t + 0.05) * SR)], peak=0.55)

# Klik mouse ganda
x = silence(0.3)
for t in (0.02, 0.14):
    add(x, highpass(noise(0.004, decay=0.0012), 3000), t, 1.0)
    add(x, tone(4200, 0.008, decay=0.002), t, 0.3)
save("SFX_Mouse_Klik", x, peak=0.5)

# Motor lewat di luar (sekali, bukan loop), mendekat lalu menjauh
n = int(4.0 * SR)
motor, ph, ph2 = [], 0.0, 0.0
for i in range(n):
    u = i / n
    f = 92 + 22 * (1 - u)
    ph += 2 * math.pi * f / SR
    ph2 += 2 * math.pi * f * 2.02 / SR
    amp = math.sin(math.pi * u) ** 2.2
    saw = 2.0 * ((ph / (2 * math.pi)) % 1.0) - 1.0
    motor.append((saw * 0.7 + 0.3 * math.sin(ph2)) * amp)
motor = lowpass(motor, 650)
add(motor, lowpass(noise(4.0, attack=1.5, decay=1.2), 400), 0.0, 0.15)
save("SFX_Motor_Lewat", motor, peak=0.6)
