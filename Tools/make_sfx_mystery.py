# Bunyi untuk cerita "Misteri TV 1": sengatan misterius (pelan, tidak mengagetkan) dan bunyi kaset emas.
import math, os, random, sys
sys.path.insert(0, os.path.dirname(__file__))
from make_sfx import SR, add, lowpass, noise, save, silence, tone  # noqa: E402

random.seed(13)

# Misteri: dengung rendah naik pelan + kotak musik sumbang 3 nada
x = silence(3.2)
add(x, lowpass(tone(55, 3.0, attack=1.2, decay=1.0), 300), 0.0, 0.6)
add(x, lowpass(tone(58.3, 3.0, attack=1.2, decay=1.0), 300), 0.0, 0.5)  # sedikit sumbang = gelisah
add(x, lowpass(noise(3.0, attack=1.5, decay=1.0), 500), 0.0, 0.08)
for i, f in enumerate((1318.5, 1174.7, 1244.5)):  # E6, D6, D#6: melodi kotak musik yang "salah"
    add(x, tone(f, 0.9, decay=0.35), 0.9 + i * 0.45, 0.22)
    add(x, tone(f * 2.01, 0.6, decay=0.2), 0.9 + i * 0.45, 0.06)
save("SFX_Misteri", x, peak=0.6)

# Kaset emas: kilau naik (hadiah)
x = silence(1.4)
for i, f in enumerate((784, 988, 1175, 1568, 1976)):
    add(x, tone(f, 0.5, decay=0.25), i * 0.09, 0.35)
save("SFX_Kaset_Emas", x, peak=0.6)
