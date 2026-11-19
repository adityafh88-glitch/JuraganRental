# Kumpulkan teks yang tampil ke pemain dari kode C++ (TEXT("...")), untuk terjemahan.
# Pakai: python Tools/extract_strings.py [--list]  -> jumlah per file, atau daftar lengkap (JSON) ke Tools/strings_id.json
import glob, json, os, re, sys

SRC = os.path.join(os.path.dirname(__file__), "..", "Source", "JuraganRental")
LIT = re.compile(r'TEXT\("((?:[^"\\]|\\.)*)"\)')
SKIP = re.compile(r'^(/|SFX_|VO_|MUS_|LOOP_|A_|SK_|SM_|M_|JR|jr|Color$|Root$|%s$|%d$|\s*$)')
SKIP_LINE = ("UE_LOG", "FParse", "RequestScreenshot", "LoadObject", "FObjectFinder", "TEXT(\"Color\")", "GConfig", "Find(FString::Printf(TEXT(\"VO_", "SlotName")


def user_facing(t):
    if SKIP.match(t):
        return False
    return re.search(r"[a-z]{3}", t) is not None


def collect():
    out = []
    for f in sorted(glob.glob(os.path.join(SRC, "*.cpp"))):
        for no, line in enumerate(open(f, encoding="utf-8"), 1):
            if any(k in line for k in SKIP_LINE):
                continue
            for m in LIT.finditer(line):
                if user_facing(m.group(1)):
                    out.append((os.path.basename(f), no, m.group(1)))
    return out


if __name__ == "__main__":
    items = collect()
    uniq = sorted(set(t for _, _, t in items))
    if "--list" in sys.argv:
        json.dump(uniq, open(os.path.join(os.path.dirname(__file__), "strings_id.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=0)
    per = {}
    for f, _, _ in items:
        per[f] = per.get(f, 0) + 1
    print(len(items), "kemunculan,", len(uniq), "unik")
    print(sorted(per.items(), key=lambda x: -x[1]))
