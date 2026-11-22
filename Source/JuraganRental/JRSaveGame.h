// Simpanan permainan: disimpan otomatis setiap toko tutup (akhir hari) dan di tengah hari
// (tiap 2 jam dalam game, saat keluar, saat aplikasi ke latar belakang). Dimuat dari menu utama.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "JRSaveGame.generated.h"

UCLASS()
class JURAGANRENTAL_API UJRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Version = 1;
	UPROPERTY() FDateTime SavedAt;

	UPROPERTY() int32 Era = 1;
	UPROPERTY() int32 Day = 1;
	UPROPERTY() float Money = 0.f;
	UPROPERTY() float Rep = 0.f;
	UPROPERTY() int32 CapVA = 1300;
	UPROPERTY() int32 PricePS = 2000;
	UPROPERTY() int32 PricePC = 3000;
	UPROPERTY() int32 OwnedPS = 2;
	UPROPERTY() int32 OwnedPC = 1;
	UPROPERTY() int32 OwnedVIP = 0;
	UPROPERTY() int32 PriceVIP = 8000;
	UPROPERTY() bool bBilling = false;
	// Misi: jenis, target, progres, hadiah, selesai, mingguan (disimpan berpasangan per indeks)
	UPROPERTY() TArray<int32> MissionKind;
	UPROPERTY() TArray<int32> MissionTarget;
	UPROPERTY() TArray<int32> MissionProgress;
	UPROPERTY() TArray<int32> MissionReward;
	UPROPERTY() TArray<bool> MissionDone;
	UPROPERTY() TArray<bool> MissionWeekly;
	UPROPERTY() TArray<FVector> TrashPos;
	UPROPERTY() TArray<int32> TrashKind;

	UPROPERTY() bool bStikOri = false;
	UPROPERTY() bool bPS2 = false;
	UPROPERTY() bool bAntivirus = false;
	UPROPERTY() bool bFastNet = false;
	UPROPERTY() bool bPaketMalam = false;
	UPROPERTY() FString HypeName;
	UPROPERTY() int32 HypeDays = 0;
	UPROPERTY() bool bHypeInstalled = false;
	UPROPERTY() int32 Regulars = 0;

	UPROPERTY() TArray<float> Debts;
	UPROPERTY() TArray<FName> PendingOrders;	// diantar kurir besok pagi
	UPROPERTY() TArray<FName> Boxes;			// kardus yang sudah ada di depan pintu
	UPROPERTY() TArray<FString> ReportLines;	// laporan hari terakhir, ditampilkan lagi saat dimuat
	UPROPERTY() int32 MysteryStep = 0;
	UPROPERTY() int32 MysteryDay = 3;
	UPROPERTY() bool bGoldCassette = false;
	UPROPERTY() bool bNagaServer = false;
	UPROPERTY() TArray<bool> Staff;
	UPROPERTY() int32 SeatTier = 0;
	UPROPERTY() bool bFastPC = false;
	UPROPERTY() TArray<FName> Decor;
	UPROPERTY() int32 GuestsDone = 0;
	UPROPERTY() int32 ViralDays = 0;
	UPROPERTY() int32 NetPlan = 0;
	UPROPERTY() int32 PendingNetPlan = -1;
	UPROPERTY() int32 UsedPS = 0;	// berapa TV terakhir yang barang bekas
	UPROPERTY() int32 UsedPC = 0;

	// Paket 1: Level Juragan, upgrade unit, bisnis sampingan.
	UPROPERTY() int32 Xp = -1;			// -1 = simpanan lama (XP diberikan sesuai era saat dimuat)
	UPROPERTY() TArray<int32> LvPS;		// level unit per indeks
	UPROPERTY() TArray<int32> LvPC;
	UPROPERTY() TArray<int32> LvVIP;
	UPROPERTY() bool bKulkas = false;
	UPROPERTY() bool bKonter = false;
	UPROPERTY() bool bPrinter = false;
	UPROPERTY() int32 Drinks = 0;
	UPROPERTY() int32 Paper = 0;
	UPROPERTY() float Saldo = 0.f;

	// Nama rental dan Viralin
	UPROPERTY() FString RentalName;
	UPROPERTY() TArray<FString> PostAuthor;
	UPROPERTY() TArray<FString> PostText;
	UPROPERTY() TArray<int32> PostLikes;
	UPROPERTY() TArray<int32> PostDay;

	// Simpanan tengah hari: dimuat lagi pada jam yang sama (pelanggan yang sedang main tidak ikut).
	UPROPERTY() bool bMidDay = false;
	UPROPERTY() float Hour = 24.f;
	UPROPERTY() int32 TutStep = -1;
	UPROPERTY() TArray<float> DayFloats;	// Income, Elec, Kabur, Stock, Utang, RepDelta, Bonus, Side
	UPROPERTY() TArray<int32> DayInts;		// lihat AJRGameMode::SaveProgress
	UPROPERTY() TArray<int32> DaySat;
	UPROPERTY() TArray<int32> DayMoments;
	UPROPERTY() FName LoakOffer;
	UPROPERTY() float GuestAt = -1.f;
	UPROPERTY() int32 GuestWhich = -1;
	UPROPERTY() float RtAt = -1.f;
	UPROPERTY() bool bQuiet = false;
	UPROPERTY() bool bFightToday = false;
	UPROPERTY() bool bTourAccepted = false;
	UPROPERTY() bool bTourDone = false;
};
