# Bungkus teks yang tampil ke pemain dengan JRT(...) agar bisa diterjemahkan, dan ganti
# FString::Printf(JRT(TEXT("..."))) menjadi JRPrintf(...) (Printf UE hanya menerima literal).
# Aman dijalankan ulang: literal yang sudah dibungkus dilewati.
# Pakai: python Tools/wrap_strings.py [--dry]
import glob, os, re, sys, json

SRC = os.path.join(os.path.dirname(__file__), "..", "Source", "JuraganRental")
LIT = re.compile(r'(?<!JRT\()TEXT\("((?:[^"\\]|\\.)*)"\)')
SKIP_PREFIX = re.compile(r'^(/|SFX_|VO_|MUS_|LOOP_|A_|SK_|SM_|M_|JR|jr|Engine)')
# Kata yang dipakai sebagai nama file suara / kunci logika, jangan diterjemahkan
SKIP_EXACT = {
    "Juragan Rental", "Foto", "HighResShot 1920x1080", "Raka", "JuraganRental_%Y%m%d_%H%M%S.png",
    "Datang", "Bayar", "Rusak", "Pesan_Mi", "Marah_Pulang", "Ngutang", "Paket", "Rage", "Lemot", "Joki",
    "Ketahuan", "Jewer", "Pergi", "Kecil", "Kopi", "Abai", "Bocil", "SMA", "Mahasiswa", "Kantoran", "Gamer",
    "Bapak", "Emak", "PakRT", "Color", "Root", "Label", "Surface", "Landscape", "Juragan", "Berkah Jaya", "Sumber Rejeki", "Bang Jago", "Maju Mapan", "Music", "Body", "Cone", "Bold", "Regular",
    "Perang Kerajaan Online", "Tembak-Tembakan Arena", "Legenda Naga Nusantara", "Balap Liar Online",
}
SKIP_LINE = ("UE_LOG", "FParse", "RequestScreenshot", "LoadObject", "FObjectFinder", "GConfig",
             "CreateDefaultSubobject", "GetDefaultFontStyle", "SetVectorParameterValue", "SetMaterialByName",
             "JRSound::", "StartsWith(TEXT(\"VO_", "Voice(", "Bye(", "MakeLoop", "PlayAt(", "Play2D(", "PlayNear(",
             "LaunchURL", "ReplaceInline", "JRNOWRAP", "TEXT(\"%s %d\")", "TEXT(\"SK_")
SKIP_FILES = {"JRLang.cpp", "JRSettings.cpp", "JRSaveGame.cpp", "JRBlockout.cpp"}


def user_facing(t):
    if t in SKIP_EXACT or SKIP_PREFIX.match(t):
        return False
    # Nama file/path (mis. "Capture/promo%d_%05d.png") bukan teks pemain
    if re.search(r"\.(png|txt|wav|json|ini|log)", t) or re.match(r"^[\w%]+/", t):
        return False
    has_letters = re.search(r"[A-Za-z]{2}", t) is not None
    if not has_letters:
        return False
    if " " in t:
        return True
    # Satu kata: hanya jika diawali huruf besar dan ada huruf kecil (bukan id seperti "stik", "ps2")
    return re.match(r"^[A-Z][a-z]{2,}", t) is not None


def process(path, dry, found):
    src = open(path, encoding="utf-8").read()
    out_lines = []
    changed = False
    for line in src.split("\n"):
        if any(k in line for k in SKIP_LINE):
            out_lines.append(line)
            continue

        def rep(m):
            t = m.group(1)
            if not user_facing(t):
                return m.group(0)
            found.add(t)
            return 'JRT(TEXT("%s"))' % t

        new = LIT.sub(rep, line)
        new = new.replace("FString::Printf(JRT(TEXT(", "JRPrintf(JRT(TEXT(")
        if new != line:
            changed = True
        out_lines.append(new)
    if changed and not dry:
        text = "\n".join(out_lines)
        if '#include "JRLang.h"' not in text:
            # sisipkan setelah include pertama milik modul ini
            text = re.sub(r'(#include "JR[A-Za-z]+\.h"\n)', r'\1#include "JRLang.h"\n', text, count=1)
        open(path, "w", encoding="utf-8").write(text)
    return changed


if __name__ == "__main__":
    dry = "--dry" in sys.argv
    found = set()
    for f in sorted(glob.glob(os.path.join(SRC, "*.cpp"))):
        if os.path.basename(f) in SKIP_FILES:
            continue
        if process(f, dry, found):
            print("diubah:", os.path.basename(f))
    # Simpan juga teks yang sudah dibungkus sebelumnya
    for f in sorted(glob.glob(os.path.join(SRC, "*.cpp"))):
        for m in re.finditer(r'JRT\(TEXT\("((?:[^"\\]|\\.)*)"\)\)', open(f, encoding="utf-8").read()):
            found.add(m.group(1))
    json.dump(sorted(found), open(os.path.join(os.path.dirname(__file__), "strings_id.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=0)
    print(len(found), "teks unik ->", "Tools/strings_id.json")
