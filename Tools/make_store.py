# Buat aset Play Store dari bahan Tools\rekam_store.ps1 (Build\Promo\Store\raw):
#   Build\Promo\Store\01..08_*.png  screenshot 1920x1080 dengan judul (gaya warna logo)
#   Build\Promo\Store\feature_1024x500.png  feature graphic
# Butuh Pillow (pip install pillow) dan font Windows Segoe UI.
import os
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.join(os.path.dirname(__file__), "..")
RAW = os.path.join(ROOT, "Build", "Promo", "Store", "raw")
OUT = os.path.join(ROOT, "Build", "Promo", "Store")
LOGO = os.path.join(ROOT, "ArtSource", "Logo", "Logo_Judul.png")
HIRES = os.path.join(ROOT, "Saved", "Screenshots", "WindowsEditor")
F = r"C:\Windows\Fonts"
BLACK, BOLD = "seguibl.ttf", "segoeuib.ttf"
YELLOW, CREAM, INK, TEAL = (255, 199, 58), (255, 246, 226), (32, 22, 14), (64, 196, 178)

SHOTS = [
    ("gc_JR_Shot_GC_Lantai1.png", "Dari garasi sampai", "Game Center 2 lantai"),
    ("ruko_JR_Shot_Ruko_PS.png", "Jaga rental PS & warnet", "ala kampung Indonesia"),
    ("garasi_JR_Shot_Event.png", "Hidup juragan penuh drama:", "adik minta SPP, atap bocor, Emak jualan"),
    ("ruko_JR_Event_1.png", "CD macet? Balik PS-nya!", "Trik legendaris anak rental"),
    ("berkah_JR_Shot_Event.png", "Jumat Berkah, turnamen,", "dan kejadian kampung tiap hari"),
    ("berkah_JR_Shot_HP.png", "Belanja unit dan upgrade", "langsung dari HP jadulmu"),
    ("gc_JR_Shot_GC_VIP.png", "Lantai VIP: sofa, TV besar,", "dan PlayStasiun 3"),
    ("gc_JR_Event_4.png", "Wujudkan impian terbesar:", "umrahkan Emak dari hasil rental"),
]


def font(name, size):
    return ImageFont.truetype(os.path.join(F, name), size)


def background(w, h):
    bg = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(bg)
    for y in range(h):
        t = y / h
        d.line([(0, y), (w, y)], fill=(int(58 + 30 * t), int(36 + 14 * t), int(20 + 8 * t)))
    return bg


def logo(width):
    lg = Image.open(LOGO).convert("RGBA")
    return lg.resize((width, round(lg.height * width / lg.width)), Image.LANCZOS)


def store_shot(src, line1, line2, idx):
    W, H = 1920, 1080
    im = background(W, H)
    shot = Image.open(os.path.join(RAW, src)).convert("RGB")
    sw = 1600
    sh = round(shot.height * sw / shot.width)
    shot = shot.resize((sw, sh), Image.LANCZOS)
    x, y = (W - sw) // 2, H - sh - 30
    # bingkai tebal gelap ala papan nama logo
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((x - 10, y - 10, x + sw + 10, y + sh + 10), radius=26, fill=INK)
    mask = Image.new("L", (sw, sh), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, sw, sh), radius=18, fill=255)
    im.paste(shot, (x, y), mask)
    # judul dua baris di pita atas
    f1, f2 = font(BLACK, 58), font(BOLD, 40)
    top = 22
    d.text(((W - d.textlength(line1, font=f1)) / 2 + 3, top + 4), line1, font=f1, fill=(0, 0, 0))
    d.text(((W - d.textlength(line1, font=f1)) / 2, top), line1, font=f1, fill=YELLOW)
    d.text(((W - d.textlength(line2, font=f2)) / 2, top + 70), line2, font=f2, fill=CREAM)
    lg = logo(170)
    im.paste(lg, (W - lg.width - 30, 14), lg)
    name = "%02d_%s.png" % (idx, os.path.splitext(src)[0].split("_JR_")[-1].lower())
    im.save(os.path.join(OUT, name))
    return name


def feature():
    W, H = 1024, 500
    im = background(W, H)
    src = os.path.join(HIRES, "HighresScreenshot00008.png")
    if not os.path.exists(src):
        src = os.path.join(RAW, SHOTS[0][0])
    shot = Image.open(src).convert("RGB")
    s = max(640 / shot.width, H / shot.height)
    shot = shot.resize((round(shot.width * s), round(shot.height * s)), Image.LANCZOS)
    sx = (shot.width - 640) // 2
    shot = shot.crop((sx, (shot.height - H) // 2, sx + 640, (shot.height - H) // 2 + H))
    # gambar game di kanan, memudar ke kiri
    fade = Image.new("L", (640, H))
    for x in range(640):
        a = int(255 * min(1.0, x / 220))
        ImageDraw.Draw(fade).line([(x, 0), (x, H)], fill=a)
    im.paste(shot, (W - 640, 0), fade)
    lg = logo(520)
    im.paste(lg, (24, 40), lg)
    d = ImageDraw.Draw(im)
    f = font(BLACK, 38)
    text = "Dari garasi sampai Game Center!"
    d.text((44 + 3, 360 + 4), text, font=f, fill=(0, 0, 0))
    d.text((44, 360), text, font=f, fill=CREAM)
    f2 = font(BOLD, 26)
    d.rounded_rectangle((44, 420, 44 + d.textlength("Simulator rental PS & warnet Indonesia", font=f2) + 40, 466), radius=23, fill=TEAL, outline=INK, width=3)
    d.text((64, 425), "Simulator rental PS & warnet Indonesia", font=f2, fill=INK)
    im.save(os.path.join(OUT, "feature_1024x500.png"))


if __name__ == "__main__":
    for i, (src, a, b) in enumerate(SHOTS, 1):
        print(store_shot(src, a, b, i))
    feature()
    print("feature_1024x500.png")
