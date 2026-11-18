// Aturan inti Juragan Rental: waktu dan hari, pelanggan, billing, listrik, laporan, dan toko.
// Angka-angka mengikuti prototype web (lihat GDD bagian Ekonomi).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "JRTypes.h"
#include "JRGameMode.generated.h"

class AJRStation;
class AJRCustomer;
class AJRKasir;
class AJRMcb;
class AJRStove;

struct FJRToast
{
	FString Text;
	FLinearColor Color;
	float TimeLeft = 0.f;
};

struct FJRDayStats
{
	float Income = 0.f;
	float Elec = 0.f;
	float Kabur = 0.f;
	float Stock = 0.f;	// modal bahan kantin
	float RepDelta = 0.f;
	int32 Customers = 0;
	TArray<int32> Satisfaction;
};

UCLASS()
class JURAGANRENTAL_API AJRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJRGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

	/* ---------- Tuning ---------- */
	static constexpr float DaySeconds = 360.f;		// 08.00-24.00 = 6 menit nyata
	float HoursPerSecond() const { return 16.f / DaySeconds; }

	/* ---------- State yang dibaca HUD ---------- */
	EJRGamePhase Phase = EJRGamePhase::Intro;
	int32 Day = 1;
	float Hour = 8.f;
	float Money = 100000.f;
	float Rep = 30.f;
	int32 CapVA = 1300;
	int32 PowerVA = 0;
	bool bMcbTripped = false;
	int32 PricePS = 2000;
	int32 PricePC = 3000;
	bool bStikOri = false;
	TArray<FJRToast> Toasts;

	/* ---------- Kantin ---------- */
	static constexpr float CookSeconds = 4.f;
	static constexpr int32 MieCost = 1500;
	static constexpr int32 MiePrice = 4000;
	EJRStoveState StoveState = EJRStoveState::Idle;
	float CookProgress = 0.f;	// detik
	bool bCarryMie = false;
	void StartCook();
	void TakeMie();
	void DeliverMie(AJRStation* Station);
	TArray<FString> ReportLines;

	bool IsPlaying() const { return Phase == EJRGamePhase::Playing; }
	int32 PriceFor(EJRUnitType Type) const { return Type == EJRUnitType::PS ? PricePS : PricePC; }
	int32 FairFor(EJRUnitType Type) const { return Type == EJRUnitType::PS ? 2000 : 3000; }
	const FJRDayStats& GetStats() const { return Stats; }

	/* ---------- HP jadul: toko dan tarif ---------- */
	TArray<FJRShopItem> GetShopItems() const;
	bool BuyItem(FName Id);
	void AdjustPrice(EJRUnitType Type, int32 Delta);

	/* ---------- Dipanggil pemain ---------- */
	void Confirm();				// Enter: buka toko / mulai hari berikutnya
	void StartSession(AJRStation* Station);
	void CollectNextPayer();
	void CollectPayer(AJRCustomer* C);
	void ResetMcb();
	AJRCustomer* NextPayer() const;

	/* ---------- Dipanggil pelanggan ---------- */
	void TickPlaying(AJRCustomer* C, float DtH);
	void LeaveAngry(AJRCustomer* C, const FString& Message);
	void RunAway(AJRCustomer* C);
	void CustomerGone(AJRCustomer* C);

	void AddToast(const FString& Text, const FLinearColor& Color = FLinearColor(0.37f, 0.70f, 1.f));

private:
	void SpawnStation(EJRUnitType Type);
	void SpawnCustomer();
	AJRStation* FindFree(EJRUnitType Type) const;
	bool Assign(AJRCustomer* C);
	bool ToQueue(AJRCustomer* C);
	void Release(AJRCustomer* C);
	void Leave(AJRCustomer* C);
	void GoPay(AJRCustomer* C);
	void EndSession(AJRCustomer* C);
	int32 CalcSatisfaction(const AJRCustomer* C) const;
	float SpawnRate(float H) const;
	void ComputePower();
	void Trip();
	void EndDay();
	int32 Count(EJRUnitType Type) const;

	UPROPERTY()
	TArray<TObjectPtr<AJRStation>> Stations;

	UPROPERTY()
	TArray<TObjectPtr<AJRCustomer>> Customers;

	UPROPERTY()
	TObjectPtr<AJRKasir> Kasir;

	UPROPERTY()
	TObjectPtr<AJRMcb> Mcb;

	UPROPERTY()
	TObjectPtr<AJRStove> Stove;

	TArray<FVector> QueueSpots;
	TArray<bool> QueueUsed;
	FVector DoorLocation = FVector::ZeroVector;
	FJRDayStats Stats;

	// Mode uji otomatis (-jrautotest): main sendiri, ambil screenshot, lalu keluar.
	void TickAutoTest(float DeltaSeconds);
	bool bAutoTest = false;
	float AutoTestTime = 0.f;
	int32 AutoTestShot = 0;
	int32 AutoTestMieDelivered = 0;
};
