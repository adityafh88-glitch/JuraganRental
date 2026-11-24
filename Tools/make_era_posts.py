# Carousel Threads "Panduan 3 era" (1080x1350) dari foto Tools\rekam_era.ps1 (Build\Promo\Era\raw).
# Hasil: Build\Promo\Era\01_panduan.png ... 05_tanya_jawab.png
import os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.join(os.path.dirname(__file__), "..")
RAW = os.path.join(ROOT, "Build", "Promo", "Era", "raw")
OUT = os.path.join(ROOT, "Build", "Promo", "Era")
LOGO = os.path.join(ROOT, "ArtSource", "Logo", "Logo_Judul.png")
DISPLAY = os.path.join(ROOT, "ArtSource", "Fonts", "LuckiestGuy-Regular.ttf")
F = r"C:\Windows\Fonts"
W, H = 1080, 1350
YELLOW, BLUE, BROWN, CREAM, RED, TEAL = (253, 199, 58), (49, 125, 245), (58, 32, 18), (255, 246, 226), (232, 72, 52), (40, 170, 150)


def disp(size): return ImageFont.truetype(DISPLAY, size)
def body(size, bold=True): return ImageFont.truetype(os.path.join(F, "segoeuib.ttf" if bold else "segoeui.ttf"), size)


def photo(name, w, h):
    im = Image.open(os.path.join(RAW, name)).convert("RGB")
    s = max(w / im.width, h / im.height)
    im = im.resize((round(im.width * s), round(im.height * s)), Image.LANCZOS)
    x, y = (im.width - w) // 2, (im.height - h) // 2
    return im.crop((x, y, x + w, y + h))


def framed(canvas, img, x, y, r=26):
    d = ImageDraw.Draw(canvas)
    w, h = img.size
    d.rounded_rectangle((x + 12, y + 14, x + w + 12, y + h + 14), radius=r, fill=BROWN)            # bayangan tebal
    d.rounded_rectangle((x - 8, y - 8, x + w + 8, y + h + 8), radius=r, fill=BROWN)
    m = Image.new("L", img.size, 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, w, h), radius=r - 6, fill=255)
    canvas.paste(img, (x, y), m)


def text_w(d, t, f): return d.textlength(t, font=f)


def pill(d, x, y, t, f, bg, fg, pad=22):
    w = text_w(d, t, f) + 2 * pad
    d.rounded_rectangle((x, y, x + w, y + f.size + 26), radius=(f.size + 26) // 2, fill=bg, outline=BROWN, width=4)
    d.text((x + pad, y + 10), t, font=f, fill=fg)
    return x + w


def bullets(d, x, y, items, f, maxw, color=BROWN, gap=8, dry=False):
    for it in items:
        lines, cur = [], ""
        for wd in it.split():
            t = (cur + " " + wd).strip()
            if text_w(d, t, f) > maxw - 34 and cur: lines.append(cur); cur = wd
            else: cur = t
        lines.append(cur)
        if not dry: d.ellipse((x, y + f.size * 0.42, x + 14, y + f.size * 0.42 + 14), fill=RED)
        for i, ln in enumerate(lines):
            if not dry: d.text((x + 30, y), ln, font=f, fill=color)
            y += f.size + 6
        y += gap
    return y


def page(n, bg=YELLOW):
    im = Image.new("RGB", (W, H), bg)
    d = ImageDraw.Draw(im)
    d.text((W - 120, 52), "%d/5" % n, font=disp(40), fill=BROWN if bg == YELLOW else CREAM)
    return im, d


def era_slide(n, era, title, sub, main, side, buy, gate, note=None):
    im, d = page(n)
    d.text((70, 52), "ERA %d" % era, font=disp(54), fill=RED)
    d.text((70, 112), title, font=disp(84), fill=BLUE)
    d.text((72, 212), sub, font=body(30), fill=BROWN)
    framed(im, photo(main, 640, 400), 70, 280)
    framed(im, photo(side, 290, 400), 730, 280)
    d.text((70, 712), "YANG BISA DIBELI", font=disp(40), fill=BROWN)
    half = (len(buy) + 1) // 2
    # Huruf daftar mengecil sampai daftar + catatan muat di atas pita syarat (era 2 paling panjang)
    limit = H - 150 - 16 - (90 if note else 0)
    size = 31
    while size > 22:
        f = body(size)
        if max(bullets(d, 70, 772, buy[:half], f, 470, gap=10, dry=True), bullets(d, 560, 772, buy[half:], f, 470, gap=10, dry=True)) + 10 <= limit: break
        size -= 1
    y1 = bullets(d, 70, 772, buy[:half], body(size), 470, gap=10)
    y2 = bullets(d, 560, 772, buy[half:], body(size), 470, gap=10)
    y = max(y1, y2) + 10
    if note:
        d.rounded_rectangle((70, y, W - 70, y + 70), radius=20, fill=CREAM, outline=BROWN, width=4)
        d.text((96, y + 16), note, font=body(28), fill=RED)
        y += 90
    # Pita syarat naik era: selebar halaman, ukuran huruf menyesuaikan agar tidak terpotong
    size = 32
    while size > 20 and text_w(d, gate, body(size)) > W - 200: size -= 1
    d.rounded_rectangle((70, H - 150, W - 70, H - 64), radius=22, fill=BROWN)
    d.text((100, H - 150 + (86 - size) / 2 - 6), gate, font=body(size), fill=YELLOW)
    im.save(os.path.join(OUT, "%02d_era%d.png" % (n, era)))


def cover():
    im, d = page(1, BLUE)
    lg = Image.open(LOGO).convert("RGBA")
    lg = lg.resize((560, round(lg.height * 560 / lg.width)), Image.LANCZOS)
    im.paste(lg, ((W - 560) // 2, 50), lg)
    d.text(((W - text_w(d, "PANDUAN 3 ERA", disp(96))) / 2, 380), "PANDUAN 3 ERA", font=disp(96), fill=CREAM)
    t2 = "dari garasi sampai Game Center"
    d.text(((W - text_w(d, t2, body(40))) / 2, 492), t2, font=body(40), fill=YELLOW)
    shots = [("era1_Fasad_Era1.png", "1 GARASI"), ("era2_Fasad_Era2.png", "2 RUKO KECIL"), ("era3_Fasad_Era3.png", "3 GAME CENTER")]
    y = 580
    for name, label in shots:
        framed(im, photo(name, 700, 190), 190, y)
        pill(d, 70, y + 66, label.split()[0], disp(46), YELLOW, BROWN, pad=26)
        d.text((190 + 20, y + 140), " ".join(label.split()[1:]), font=disp(34), fill=CREAM)
        y += 230
    im.save(os.path.join(OUT, "01_panduan.png"))


def faq():
    im, d = page(5)
    d.text((70, 60), "KENAPA BILLING", font=disp(80), fill=BROWN)
    d.text((70, 150), "BELUM ADA DI GARASI?", font=disp(80), fill=BLUE)
    y = bullets(d, 70, 270, [
        "Di garasi kamu masih juragan sendirian: nyalakan TV, nagih di kasir, masak mi, semua sendiri. Itu justru serunya era 1.",
        "Billing (komputer di samping kasir untuk menyalakan semua unit dari satu tempat) baru dijual di RUKO, mulai Level Juragan 5.",
        "Karyawan (kasir, teknisi, koki, OB) juga baru bisa direkrut di ruko lewat HP > Kelola.",
        "Barang yang tertulis \"Butuh Level Juragan\" akan terbuka sendiri. XP didapat dari pelanggan yang bayar, mi, misi, dan turnamen.",
        "Di Game Center, saklar ON/OFF di billing bisa mematikan unit sementara supaya MCB tidak turun saat semua penuh.",
    ], body(33), W - 140, gap=18)
    d.rounded_rectangle((70, y + 10, W - 70, y + 140), radius=24, fill=CREAM, outline=BROWN, width=5)
    d.text((100, y + 30), "Masih bingung atau ketemu bug?", font=body(34), fill=BROWN)
    d.text((100, y + 80), "Tulis di kolom komentar ya, warga rental!", font=body(34, False), fill=BROWN)
    im.save(os.path.join(OUT, "05_tanya_jawab.png"))


if __name__ == "__main__":
    cover()
    era_slide(2, 1, "GARASI RUMAH", "Modal awal: 2 TV tabung, 1 PC, MCB 1.300 VA. Muat 4 TV + 4 PC.",
              "era1_Shot_RentalPS.png", "era1_Shot_Bangku.png",
              ["TV tabung + PS1 (Rp 90rb)", "PC warnet (Rp 250rb)", "Stik original (Rp 50rb)", "Tambah daya 2.200 VA (Rp 150rb, Lv 2)",
               "Dekor: poster, tanaman, kipas, lampu neon RGB", "Bantal & kursi busa (Rp 100rb, Lv 3)", "Barang loak bekas (kadang muncul)",
               "Impian: HP adik, dapur Emak"],
              "Naik ke ruko: kas Rp 600rb + reputasi 40 (HP > Toko)",
              "Belum ada billing & karyawan: semua dikerjakan sendiri!")
    era_slide(3, 2, "RUKO KECIL", "Muat 6 TV + 6 PC, daya 2.200 VA, router internet, sewa Rp 75rb/hari.",
              "era2_Shot_Ruko_PS.png", "era2_Shot_Ruko_Warnet.png",
              ["SISTEM BILLING (Rp 300rb, Lv 5)", "Karyawan: kasir, teknisi, koki, OB (HP > Kelola)", "TV 21\" + PS2, PS2 untuk semua TV (Lv 4)",
               "Tambah daya 4.400 VA (Lv 4)", "Upgrade PC RAM + VGA (Lv 5)", "AC split & bean bag (Lv 6)", "Antivirus & install game hype",
               "Internet: NetRakyat, KilatNet, FiberNusa", "Turnamen PS tiap Sabtu", "Impian: motor juragan"],
              "Naik ke Game Center: kas Rp 3 juta + reputasi 60 + 10 unit",
              "Billing mulai dijual di sini!")
    era_slide(4, 3, "GAME CENTER", "2 lantai: 8 TV + 8 PC di bawah, 6 booth VIP di atas.",
              "era3_Shot_GC_Lantai1.png", "era3_Shot_GC_VIP.png",
              ["Booth VIP lantai 2 + PS3 (Lv 6)", "Kulkas minuman (Lv 7)", "Konter pulsa & top-up (Lv 8)", "Printer & fotokopi (Lv 10)",
               "Tambah daya 7.700 VA (Lv 8)", "Saklar ON/OFF unit di billing", "Impian: renovasi rumah Emak", "Impian terbesar: umrahkan Emak"],
              "Target akhir: Rp 10 juta + reputasi 85 (Juragan Sejati)",
              None)
    faq()
    print("ok")
