// Aturan inti Juragan Rental: waktu dan hari, pelanggan, billing, listrik, kantin, kejadian,
// kurir, laporan, toko, dan perpindahan era (Garasi -> Ruko kecil).
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
class AJRRoom;
class AJRRouter;
class AJRDeliveryBox;
class AJRSlotMarker;

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
	float Utang = 0.f;	// utang baru dicatat hari ini
	float RepDelta = 0.f;
	int32 Customers = 0;
	TArray<int32> Satisfaction;
};

/** Tata letak satu era: ukuran ruangan dan posisi semua perabot (cm). */
struct FJRLayout
{
	FString Name;
	float HalfX = 700.f;	// belakang = +X, pintu = -X
	float HalfY = 1000.f;	// kiri = -Y, kanan (warnet) = +Y
	float DoorY = 420.f;
	TArray<float> PsY;		// posisi Y deretan TV (di X = HalfX - 80)
	TArray<float> PcX;		// posisi X deretan PC (di Y = HalfY - 70)
	TArray<FVector> Queue;	// 3 pertama = duduk di bangku antre
	float Rent = 0.f;
	float Internet = 0.f;
	bool bRouter = false;

	FVector Kasir() const { return FVector(-(HalfX - 210.f), -0.65f * HalfY, 0.f); }
	FVector Mcb() const { return FVector(0.4f * HalfX, -(HalfY - 7.f), 130.f); }
	FVector Stove() const { return FVector(-0.13f * HalfX, -(HalfY - 34.f), 0.f); }
	FVector Router() const { return FVector(HalfX - 12.f, 0.55f * HalfY, 175.f); }
	FVector DoorOutside() const { return FVector(-(HalfX + 200.f), DoorY, 0.f); }
	FVector PlayerStart() const { return FVector(-0.36f * HalfX, 0.f, 110.f); }
	int32 MaxUnits() const { return PsY.Num(); }
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
	int32 Era = 1;
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
	TArray<FString> ReportLines;
	const FJRLayout& Layout() const;

	/* ---------- Era 2: ruko ---------- */
	bool bPS2 = false;			// semua TV pakai PlayStasiun 2 (harga wajar Rp 3.000)
	bool bAntivirus = false;
	bool bFastNet = false;
	bool bPaketMalam = false;
	bool bNetSlow = false;
	FString HypeName;			// game online yang sedang hype (kosong = tidak ada)
	int32 HypeDays = 0;
	bool bHypeInstalled = false;
	int32 Regulars = 0;			// joki/gamer langganan
	void TogglePaketMalam();
	void RestartRouter();

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

	/** Waktu berjalan hanya jika toko buka dan tidak ada kejadian yang menunggu pilihan. */
	bool IsPlaying() const { return Phase == EJRGamePhase::Playing && Choices.Num() == 0; }
	int32 PriceFor(EJRUnitType Type) const { return Type == EJRUnitType::PS ? PricePS : PricePC; }
	int32 FairFor(EJRUnitType Type) const { return Type == EJRUnitType::PS ? (bPS2 ? 3000 : 2000) : 3000; }
	const FJRDayStats& GetStats() const { return Stats; }

	/* ---------- Kejadian dengan pilihan ---------- */
	TArray<FJRChoice> Choices;
	void PushChoice(const FString& Title, const FString& Body, TArray<FString> Labels, TArray<TFunction<void()>> Actions);
	void ResolveChoice(int32 Index);

	/* ---------- Buku utang ---------- */
	TArray<float> Debts;
	float TotalDebt() const;

	/* ---------- Kurir ---------- */
	TArray<FName> PendingOrders;	// dipesan, datang besok pagi
	FName CarryBox;					// kardus yang sedang dibawa
	bool IsCarrying() const { return bCarryMie || !CarryBox.IsNone(); }
	void PickBox(AJRDeliveryBox* Box);
	void InstallBox();
	bool GetInstallSlot(FVector& OutLoc, FRotator& OutRot) const;
	FString ItemName(FName Id) const;

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
	void BuildWorld();
	void MoveToEra2();
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
	int32 PickType(float H) const;
	void ComputePower();
	void Trip();
	void EndDay();
	void StartDay();
	void SpawnDelivery(FName Id);
	void FinishPayment(AJRCustomer* C, float Paid);
	void EmakEvent(AJRCustomer* C);
	void RageQuitEvent(AJRCustomer* C);
	void PakRtEvent();
	void JokiEvent(AJRCustomer* C);
	int32 InPipeline(FName Id) const;
	int32 Count(EJRUnitType Type) const;
	bool Gate1Open() const { return Money >= 1500000.f && Rep >= 40.f; }
	FVector SlotLocation(EJRUnitType Type, int32 Index, FRotator& OutRot) const;

	UPROPERTY()
	TArray<TObjectPtr<AJRStation>> Stations;

	UPROPERTY()
	TArray<TObjectPtr<AJRCustomer>> Customers;

	UPROPERTY()
	TObjectPtr<AJRRoom> Room;

	UPROPERTY()
	TObjectPtr<AJRKasir> Kasir;

	UPROPERTY()
	TObjectPtr<AJRMcb> Mcb;

	UPROPERTY()
	TObjectPtr<AJRStove> Stove;

	UPROPERTY()
	TObjectPtr<AJRRouter> Router;

	UPROPERTY()
	TObjectPtr<AJRSlotMarker> SlotMarker;

	UPROPERTY()
	TArray<TObjectPtr<AJRDeliveryBox>> Boxes;

	int32 OwnedPS = 2;
	int32 OwnedPC = 1;
	TArray<bool> QueueUsed;
	FJRDayStats Stats;
	float RtAt = -1.f;		// jam Pak RT datang malam ini (-1 = tidak)
	float AmbientTimer = 12.f;	// detik sampai motor lewat berikutnya
	bool bQuiet = false;	// suara dikecilkan setelah ditegur Pak RT

	// Mode uji otomatis (-jrautotest, -jrera2): main sendiri, ambil screenshot, lalu keluar.
	void TickAutoTest(float DeltaSeconds);
	bool bAutoTest = false;
	float AutoTestTime = 0.f;
	int32 AutoTestShot = 0;
	int32 AutoTestMieDelivered = 0;
	int32 AutoTestEvents = 0;
	int32 AutoTestInstalled = 0;
	float AutoChoiceTimer = 0.f;
};
