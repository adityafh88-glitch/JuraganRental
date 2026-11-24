# Carousel Threads ajakan uji tertutup Google Play + daftar perubahan sejak 0.9.5 (1080x1350).
# Hasil: Build\Promo\UjiTertutup\01_ajakan.png, 02_yang_baru.png, 03_cara_ikut.png
import os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.join(os.path.dirname(__file__), "..")
OUT = os.path.join(ROOT, "Build", "Promo", "UjiTertutup")
STORE_RAW = os.path.join(ROOT, "Build", "Promo", "Store", "raw")
ERA_RAW = os.path.join(ROOT, "Build", "Promo", "Era", "raw")
LOGO = os.path.join(ROOT, "ArtSource", "Logo", "Logo_Judul.png")
DISPLAY = os.path.join(ROOT, "ArtSource", "Fonts", "LuckiestGuy-Regular.ttf")
F = r"C:\Windows\Fonts"
W, H = 1080, 1350
YELLOW, BLUE, BROWN, CREAM, RED, TEAL = (253, 199, 58), (49, 125, 245), (58, 32, 18), (255, 246, 226), (232, 72, 52), (40, 170, 150)
os.makedirs(OUT, exist_ok=True)


def disp(size): return ImageFont.truetype(DISPLAY, size)
def body(size, bold=True): return ImageFont.truetype(os.path.join(F, "segoeuib.ttf" if bold else "segoeui.ttf"), size)
def tw(d, t, f): return d.textlength(t, font=f)


def photo(path, w, h):
    im = Image.open(path).convert("RGB")
    s = max(w / im.width, h / im.height)
    im = im.resize((round(im.width * s), round(im.height * s)), Image.LANCZOS)
    x, y = (im.width - w) // 2, (im.height - h) // 2
    return im.crop((x, y, x + w, y + h))


def framed(canvas, img, x, y, r=26):
    d = ImageDraw.Draw(canvas)
    w, h = img.size
    d.rounded_rectangle((x + 12, y + 14, x + w + 12, y + h + 14), radius=r, fill=BROWN)
    d.rounded_rectangle((x - 8, y - 8, x + w + 8, y + h + 8), radius=r, fill=BROWN)
    m = Image.new("L", img.size, 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, w, h), radius=r - 6, fill=255)
    canvas.paste(img, (x, y), m)


def wrap(d, text, f, maxw):
    lines, cur = [], ""
    for wd in text.split():
        t = (cur + " " + wd).strip()
        if tw(d, t, f) > maxw and cur: lines.append(cur); cur = wd
        else: cur = t
    lines.append(cur)
    return lines


def center(d, y, text, f, fill):
    d.text(((W - tw(d, text, f)) / 2, y), text, font=f, fill=fill)
    return y + f.size


def page(n, bg):
    im = Image.new("RGB", (W, H), bg)
    d = ImageDraw.Draw(im)
    d.text((W - 120, 52), "%d/3" % n, font=disp(40), fill=CREAM if bg == BLUE else BROWN)
    return im, d


def slide1():
    im, d = page(1, BLUE)
    lg = Image.open(LOGO).convert("RGBA")
    lg = lg.resize((520, round(lg.height * 520 / lg.width)), Image.LANCZOS)
    im.paste(lg, ((W - 520) // 2, 40), lg)
    y = center(d, 340, "UJI TERTUTUP", disp(110), CREAM) + 6
    y = center(d, y, "GOOGLE PLAY", disp(110), YELLOW) + 26
    center(d, y, "Versi 1.0.2 siap dimainkan, langsung dari Play Store", body(34), CREAM)
    framed(im, photo(os.path.join(ERA_RAW, "era3_Shot_GC_Lantai1.png"), 940, 420), 70, 650)
    d.rounded_rectangle((70, 1130, W - 70, 1240), radius=28, fill=YELLOW, outline=BROWN, width=6)
    t = 'Komen "JURAGAN" atau DM kami!'
    d.text(((W - tw(d, t, disp(56))) / 2, 1152), t, font=disp(56), fill=BROWN)
    d.text((70, H - 80), "Gratis  -  Android  -  iPhone nyusul", font=body(30), fill=CREAM)
    im.save(os.path.join(OUT, "01_ajakan.png"))


def slide2():
    im, d = page(2, YELLOW)
    d.text((70, 52), "YANG BARU", font=disp(84), fill=BROWN)
    d.text((70, 140), "SEJAK VERSI 0.9.5", font=disp(60), fill=BLUE)
    framed(im, photo(os.path.join(STORE_RAW, "garasi_JR_Shot_Event.png"), 455, 256), 70, 240)
    framed(im, photo(os.path.join(STORE_RAW, "ruko_JR_Event_1.png"), 455, 256), 555, 240)
    groups = [
        ("CERITA & GAMEPLAY", [
            "13 kejadian keluarga & kampung: adik minta SPP, atap bocor, Bapak pinjam TV",
            "Impian juragan sampai umrahkan Emak + ending baru (total 5)",
            "Jumat Berkah, sewa PS bawa pulang, CD macet: balik PS-nya!",
            "Mulai era 2, billing bisa ON/OFF tiap unit pelanggan (hemat daya, MCB aman)",
        ]),
        ("PERBAIKAN", [
            "Pesanan mi sudah diperbaiki: pelanggan nunggu mi-nya dulu sebelum pulang",
            "Bahasa bisa langsung diganti Indonesia / Inggris, tanpa restart",
            "Simpan otomatis: keluar game, lalu \"Lanjutkan\" kembali ke permainan terakhir",
            "Garasi & ruko lebih kecil, jalan lebih dekat",
            "Hujan sungguhan, UI baru, balon emosi pelanggan",
            "Papan nama & barang tidak berantakan lagi",
        ]),
    ]

    def draw(size, dry):
        f = body(size)
        y = 560
        for title, items in groups:
            if not dry: d.text((70, y), title, font=disp(40), fill=RED)
            y += 54
            for it in items:
                if not dry: d.ellipse((74, y + 12, 88, y + 26), fill=BROWN)
                for ln in wrap(d, it, f, W - 180):
                    if not dry: d.text((104, y), ln, font=f, fill=BROWN)
                    y += f.size + 6
                y += 8
            y += 14
        return y

    size = 29
    while size > 21 and draw(size, True) > H - 70: size -= 1	# daftar lebih panjang: huruf menyesuaikan
    draw(size, False)
    im.save(os.path.join(OUT, "02_yang_baru.png"))


def slide3():
    im, d = page(3, YELLOW)
    d.text((70, 52), "CARA IKUT", font=disp(92), fill=BROWN)
    d.text((72, 158), "Uji tertutup lewat Google Play", font=body(34), fill=BLUE)
    steps = [
        ("Kirim email Gmail kamu", "Komen \"JURAGAN\" lalu DM email Gmail yang dipakai di Play Store."),
        ("Kami daftarkan", "Email kamu masuk daftar penguji, lalu kami kirim link undangan."),
        ("Buka link & pasang", "Ketuk \"Jadi penguji\", lalu pasang Juragan Rental dari Play Store."),
        ("Main & kasih masukan", "Bug, saran, keluhan: tulis di komentar atau DM. Semua kami baca!"),
    ]
    y = 250
    for i, (title, desc) in enumerate(steps, 1):
        d.rounded_rectangle((70, y, W - 70, y + 190), radius=28, fill=CREAM, outline=BROWN, width=5)
        d.ellipse((100, y + 50, 190, y + 140), fill=BLUE, outline=BROWN, width=5)
        n = str(i)
        d.text((145 - tw(d, n, disp(60)) / 2, y + 60), n, font=disp(60), fill=CREAM)
        d.text((220, y + 26), title, font=disp(44), fill=BROWN)
        yy = y + 84
        for ln in wrap(d, desc, body(29, False), W - 330):
            d.text((220, yy), ln, font=body(29, False), fill=BROWN)
            yy += 38
        y += 215
    d.rounded_rectangle((70, y + 4, W - 70, y + 130), radius=24, fill=BROWN)
    for k, ln in enumerate(wrap(d, "Sudah pasang versi 0.9.5 dari link alpha? Hapus dulu versi lama, lalu pasang dari Play Store.", body(30), W - 200)):
        d.text((100, y + 22 + k * 42), ln, font=body(30), fill=YELLOW)
    im.save(os.path.join(OUT, "03_cara_ikut.png"))


if __name__ == "__main__":
    slide1(); slide2(); slide3()
    print("ok")
