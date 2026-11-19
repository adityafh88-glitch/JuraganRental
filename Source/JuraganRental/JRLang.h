// Terjemahan sederhana: teks Indonesia di kode adalah kunci; versi Inggris diambil dari JRLangTable.inc.
// Bahasa dipilih di Pengaturan (berlaku setelah game dimulai ulang); awalnya mengikuti bahasa Windows.

#pragma once

#include "CoreMinimal.h"

namespace JRLang
{
	/** true jika bahasa aktif adalah Inggris. */
	JURAGANRENTAL_API bool IsEnglish();
}

/** Terjemahkan teks Indonesia ke bahasa aktif (kembali ke teks asli jika belum ada terjemahannya). */
JURAGANRENTAL_API const TCHAR* JRT(const TCHAR* Indonesian);

/** Seperti FString::Printf, tetapi format boleh berasal dari terjemahan (bukan literal). */
JURAGANRENTAL_API FString JRPrintf(const TCHAR* Fmt, ...);
