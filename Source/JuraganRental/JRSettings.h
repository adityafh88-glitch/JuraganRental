// Pengaturan pemain (disimpan ke slot "JRSettings"): suara dan kontrol layar sentuh.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "JRSettings.generated.h"

UCLASS()
class JURAGANRENTAL_API UJRSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool bSfx = true;			// efek suara (unit, kasir, motor, dll.)

	UPROPERTY()
	bool bVoice = true;			// suara karakter

	UPROPERTY()
	bool bMusic = true;			// musik radio

	UPROPERTY()
	bool bJoystickFixed = true;	// joystick layar diam di tempat (false = pindah ke posisi jari)

	UPROPERTY()
	bool bInvertLook = false;	// geser ke atas = lihat ke bawah

	UPROPERTY()
	float LookSpeed = 1.f;		// kepekaan mouse / joystick lihat

	UPROPERTY()
	float UIScale = 1.8f;		// ukuran tampilan di layar sentuh (HP kecil butuh teks lebih besar)

	UPROPERTY()
	float JoystickSize = 0.85f;	// ukuran joystick layar (1 = bawaan Unreal; layar HP kecil cocok sedikit lebih kecil)

	UPROPERTY()
	bool bMissionsCompact = false;	// daftar misi diciutkan jadi satu baris

	UPROPERTY()
	bool bToastsHidden = false;	// notifikasi kanan atas disembunyikan (tinggal label jumlah)

	UPROPERTY()
	int32 Language = 0;			// 0 otomatis (ikut Windows), 1 Indonesia, 2 English

	UPROPERTY()
	int32 GfxPreset = -1;		// HP: 0 Hemat, 1 Seimbang, 2 Bagus (-1 = belum dipilih, pakai Seimbang)

	UPROPERTY()
	int32 MaxFps = 60;			// HP: batas FPS 30 atau 60

	UPROPERTY()
	bool bShowFps = false;		// angka FPS di pojok layar (untuk penguji)

	// Tujuan jangka panjang (JRBadges.cpp): lencana, ending, dan hitungan sepanjang permainan, per pemain.
	UPROPERTY() TArray<FString> Badges;
	UPROPERTY() TArray<FString> Endings;
	UPROPERTY() int32 LifePaid = 0;
	UPROPERTY() int32 LifeUpsell = 0;
	UPROPERTY() int32 LifeMie = 0;
	UPROPERTY() int32 LifeExact = 0;
	UPROPERTY() int32 LifeTours = 0;
};

namespace JRSettings
{
	/** Pengaturan aktif (dimuat dari disk saat pertama dipakai). */
	JURAGANRENTAL_API UJRSettingsSave* Get();
	JURAGANRENTAL_API void Save();

	/** Ukuran tampilan layar sentuh: Kecil 1,2 / Sedang 1,5 / Besar 1,8 / Sangat besar 2,1. */
	JURAGANRENTAL_API float NextUIScale(float Current);
	JURAGANRENTAL_API FString UIScaleName(float Scale);

	/** Ukuran joystick: Kecil 0,7 / Sedang 0,85 / Besar 1,0 / Sangat besar 1,15. */
	JURAGANRENTAL_API float NextJoystickSize(float Current);
	JURAGANRENTAL_API FString JoystickSizeName(float Size);

	/** HP: terapkan kualitas grafis (skala resolusi 3D, bayangan, efek) dan batas FPS. Dipanggil saat game mulai dan saat diubah. */
	JURAGANRENTAL_API void ApplyGraphics();
	JURAGANRENTAL_API int32 GfxPreset();		// preset aktif (yang belum dipilih = Seimbang)
	JURAGANRENTAL_API FString GfxPresetName(int32 Preset);
}
