// Iklan berhadiah (rewarded). Android: Google AdMob lewat JuraganRental_UPL.xml (Java).
// PC/editor: tidak ada iklan, kecuali -jradsim (iklan pura-pura untuk uji otomatis).

#pragma once

#include "CoreMinimal.h"

namespace JRAds
{
	/** Mulai SDK iklan dan muat iklan pertama (sekali, saat game dibuka). */
	JURAGANRENTAL_API void Init();
	/** Ada iklan yang siap ditayangkan sekarang. Tawaran iklan hanya muncul kalau ini true. */
	JURAGANRENTAL_API bool IsReady();
	/** Tayangkan iklan; OnDone(true) dipanggil di game thread bila pemain menonton sampai dapat hadiah. */
	JURAGANRENTAL_API void Show(TFunction<void(bool)> OnDone);
	/** Dipanggil dari Java (lewat JNI) saat iklan ditutup. */
	void OnClosed(bool bEarned);
}
