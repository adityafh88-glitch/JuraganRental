// Terjemahan sederhana: teks Indonesia di kode adalah kunci; versi Inggris diambil dari JRLangTable.inc.
// Bahasa dipilih di Pengaturan dan langsung berlaku (JRLang::Reset + HUD membangun ulang widget); awalnya mengikuti bahasa perangkat.

#pragma once

#include "CoreMinimal.h"

namespace JRLang
{
	/** true jika bahasa aktif adalah Inggris. */
	JURAGANRENTAL_API bool IsEnglish();
	/** Baca ulang pilihan bahasa (dipanggil setelah pemain mengganti bahasa). */
	JURAGANRENTAL_API void Reset();
}

/** Terjemahkan teks Indonesia ke bahasa aktif (kembali ke teks asli jika belum ada terjemahannya). */
JURAGANRENTAL_API const TCHAR* JRT(const TCHAR* Indonesian);

/** Seperti FString::Printf, tetapi format boleh berasal dari terjemahan (bukan literal). */
JURAGANRENTAL_API FString JRPrintf(const TCHAR* Fmt, ...);
