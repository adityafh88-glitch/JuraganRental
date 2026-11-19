// Pemutar suara: efek (/Game/Audio/SFX_*, LOOP_*) dan suara karakter opsional (/Game/Audio/Voice/VO_*).
// File yang belum ada dilewati diam-diam, jadi suara karakter bisa diisi bertahap.

#pragma once

#include "CoreMinimal.h"

class UObject;
class USoundBase;
class UAudioComponent;
class USceneComponent;

namespace JRSound
{
	/** Muat suara dari /Game/Audio (atau /Game/Audio/Voice untuk nama berawalan VO_). Null jika tidak ada. */
	JURAGANRENTAL_API USoundBase* Find(const FString& Name);

	/** Efek tanpa posisi (UI, MCB). */
	JURAGANRENTAL_API void Play2D(const UObject* Context, const TCHAR* Name, float Volume = 1.f);

	/** Efek 3D di lokasi, mengecil dengan jarak. */
	JURAGANRENTAL_API void PlayAt(const UObject* Context, const TCHAR* Name, const FVector& Location, float Volume = 1.f);

	/** Bunyi kecil yang hanya terdengar dari dekat (aktivitas unit: klik stik, ketikan). */
	JURAGANRENTAL_API void PlayNear(const UObject* Context, const TCHAR* Name, const FVector& Location, float Volume = 1.f);

	/** Suara karakter: memilih acak VO_<Char>_<Situasi>_1..3 yang tersedia. Mengembalikan false jika belum ada rekaman. */
	JURAGANRENTAL_API bool Voice(const UObject* Context, const FString& Char, const TCHAR* Situasi, const FVector& Location);

	/** Komponen suara loop yang menempel di Parent (mis. musik TV, desis kompor). Belum diputar. */
	JURAGANRENTAL_API UAudioComponent* MakeLoop(USceneComponent* Parent, const TCHAR* Name, float Volume, bool b3D = true);
}
