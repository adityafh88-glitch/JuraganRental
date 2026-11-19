# Buat Source/JuraganRental/JRLangTable.inc dari Tools/strings_en.tsv (Indonesia<TAB>English, satu baris per teks).
# Memeriksa format %s/%d agar sama persis di kedua bahasa (urutan dan jumlah), supaya tidak crash.
import json, os, re, sys

HERE = os.path.dirname(__file__)
TSV = os.path.join(HERE, "strings_en.tsv")
OUT = os.path.join(HERE, "..", "Source", "JuraganRental", "JRLangTable.inc")
SPEC = re.compile(r"%(?:%|[-+ 0#]*\d*(?:\.\d+)?[sdifxu])")

pairs, errors = [], []
for no, line in enumerate(open(TSV, encoding="utf-8"), 1):
    line = line.rstrip("\n")
    if not line.strip():
        continue
    if "\t" not in line:
        errors.append("baris %d tanpa TAB: %s" % (no, line[:60]))
        continue
    src, en = line.split("\t", 1)
    if SPEC.findall(src) != SPEC.findall(en):
        errors.append("baris %d format beda: %s | %s" % (no, SPEC.findall(src), SPEC.findall(en)))
    pairs.append((src, en))

known = set(json.load(open(os.path.join(HERE, "strings_id.json"), encoding="utf-8")))
missing = sorted(known - set(s for s, _ in pairs))
if errors:
    print("\n".join(errors))
    sys.exit(1)
with open(OUT, "w", encoding="utf-8") as f:
    f.write("// Dibuat otomatis oleh Tools/make_lang_table.py dari Tools/strings_en.tsv - jangan diedit manual.\n")
    for s, e in pairs:
        f.write('\t\t{ TEXT("%s"), TEXT("%s") },\n' % (s, e))
print(len(pairs), "terjemahan ditulis.", len(missing), "teks belum diterjemahkan:")
for m in missing:
    print("  -", m)
