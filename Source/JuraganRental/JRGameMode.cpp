#include "JRGameMode.h"

#include "Engine/World.h"
#include "JRBlockout.h"
#include "JRCharacter.h"
#include "JRCustomer.h"
#include "JRFixtures.h"
#include "JRHUD.h"
#include "JRStation.h"

namespace
{
	struct FTypeDef
	{
		const TCHAR* Name;
		uint32 Top;
		uint32 Bottom;
		float Scale;
		int32 Want; // 0 = PS, 1 = PC, 2 = bebas
	};

	// Urutan: bocil, pelajar, mahasiswa, pekerja, gamer, bapak
	const FTypeDef Types[] = {
		{ TEXT("Bocil berseragam"), 0xffffff, 0x2c4a8a, 0.80f, 0 },
		{ TEXT("Anak SMA"),         0xffffff, 0x777777, 0.92f, 0 },
		{ TEXT("Mahasiswa"),        0x4f8c6a, 0x2d3a55, 1.00f, 1 },
		{ TEXT("Mas-mas kantoran"), 0x5a7fbf, 0x222222, 1.00f, 2 },
		{ TEXT("Gamer online"),     0x262626, 0x262626, 1.00f, 1 },
		{ TEXT("Bapak-bapak"),      0xb5653a, 0x3a3a3a, 1.05f, 0 },
	};
	constexpr int32 Bocil = 0;

	int32 PickType(float H)
	{
		float W[6] = { 0, 1, 0, 3, 3, 2 };
		if (H < 13.f)      { const float X[6] = { 4, 0, 3, 1, 0, 1 }; FMemory::Memcpy(W, X, sizeof(W)); }
		else if (H < 17.f) { const float X[6] = { 1, 5, 2, 0, 0, 1 }; FMemory::Memcpy(W, X, sizeof(W)); }
		else if (H < 19.f) { const float X[6] = { 0, 2, 1, 3, 0, 2 }; FMemory::Memcpy(W, X, sizeof(W)); }
		float Total = 0.f;
		for (float V : W) Total += V;
		float R = FMath::FRand() * Total;
		for (int32 i = 0; i < 6; ++i)
		{
			R -= W[i];
			if (R <= 0.f && W[i] > 0.f) return i;
		}
		return 3;
	}

	// Koordinat prototype (x kanan, z ke arah pintu, dalam meter) -> Unreal (cm).
	FVector P(float X, float Z) { return FVector(-Z * 100.f, X * 100.f, 0.f); }

	const FLinearColor Good(0.36f, 0.79f, 0.54f);
	const FLinearColor Bad(0.94f, 0.42f, 0.36f);

	const float PsX[4] = { -8.f, -5.5f, -3.f, -0.5f };
	const float PcZ[4] = { -5.f, -2.5f, 0.f, 2.5f };
	constexpr int32 MaxPerType = 4;
}

AJRGameMode::AJRGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AJRCharacter::StaticClass();
	HUDClass = AJRHUD::StaticClass();
}

APawn* AJRGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	// Pemain selalu mulai di tengah garasi menghadap deretan TV.
	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, FTransform(FRotator::ZeroRotator, FVector(-250.f, 0.f, 110.f)));
}

void AJRGameMode::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	World->SpawnActor<AJRRoom>(FVector::ZeroVector, FRotator::ZeroRotator, SP);
	Kasir = World->SpawnActor<AJRKasir>(P(-6.5f, 4.9f), FRotator::ZeroRotator, SP);
	Mcb = World->SpawnActor<AJRMcb>(FVector(280.f, -994.f, 160.f), FRotator(0.f, 90.f, 0.f), SP);

	QueueSpots = { P(-1.5f, 5.9f), P(-0.5f, 5.9f), P(0.5f, 5.9f), P(1.5f, 5.9f), P(2.5f, 5.2f) };
	QueueUsed.Init(false, QueueSpots.Num());
	DoorLocation = FVector(-900.f, 420.f, 0.f);

	SpawnStation(EJRUnitType::PS);
	SpawnStation(EJRUnitType::PS);
	SpawnStation(EJRUnitType::PC);
}

int32 AJRGameMode::Count(EJRUnitType Type) const
{
	int32 N = 0;
	for (const AJRStation* S : Stations) if (S->Type == Type) ++N;
	return N;
}

void AJRGameMode::SpawnStation(EJRUnitType Type)
{
	const int32 I = Count(Type);
	if (I >= MaxPerType) return;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Loc = Type == EJRUnitType::PS ? P(PsX[I], -6.2f) : P(9.3f, PcZ[I]);
	const FRotator Rot(0.f, Type == EJRUnitType::PS ? 180.f : -90.f, 0.f); // menghadap ke dalam ruangan
	AJRStation* S = GetWorld()->SpawnActor<AJRStation>(Loc, Rot, SP);
	S->Setup(Type, I);
	Stations.Add(S);
}

/* ---------- Loop utama ---------- */

void AJRGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (FJRToast& T : Toasts) T.TimeLeft -= DeltaSeconds;
	Toasts.RemoveAll([](const FJRToast& T) { return T.TimeLeft <= 0.f; });

	if (!IsPlaying()) return;

	const float DtH = DeltaSeconds * HoursPerSecond();
	Hour += DtH;

	if (Hour < 23.f && FMath::FRand() < SpawnRate(Hour) * DtH)
	{
		SpawnCustomer();
	}
	for (AJRCustomer* C : TArray<TObjectPtr<AJRCustomer>>(Customers))
	{
		if (C->State == EJRCustomerState::Queue || (C->State == EJRCustomerState::Walk && C->NextState == EJRCustomerState::Queue))
		{
			Assign(C);
		}
	}

	ComputePower();
	if (!bMcbTripped) Stats.Elec += PowerVA * 2.f * DtH; // Rp 2 per VA per jam

	if (Hour >= 24.f)
	{
		Hour = 24.f;
		EndDay();
	}
}

float AJRGameMode::SpawnRate(float H) const
{
	const float Base = H < 13.f ? 1.6f : H < 17.f ? 3.2f : H < 19.f ? 2.0f : H < 22.f ? 3.0f : 1.2f;
	const float RepMul = 0.5f + Rep / 100.f;
	const float PriceMul = FMath::Pow((2000.f / PricePS + 3000.f / PricePC) / 2.f, 1.5f);
	return Base * RepMul * PriceMul * FMath::Min(1.8f, 0.55f + Stations.Num() * 0.18f);
}

/* ---------- Pelanggan ---------- */

AJRStation* AJRGameMode::FindFree(EJRUnitType Type) const
{
	for (AJRStation* S : Stations) if (S->Type == Type && S->IsFree()) return S;
	return nullptr;
}

void AJRGameMode::SpawnCustomer()
{
	const int32 TI = PickType(Hour);
	const FTypeDef& T = Types[TI];
	EJRUnitType Want = T.Want == 2 ? (FMath::RandBool() ? EJRUnitType::PS : EJRUnitType::PC) : (T.Want == 0 ? EJRUnitType::PS : EJRUnitType::PC);
	if (Want == EJRUnitType::PC && Count(EJRUnitType::PC) == 0) Want = EJRUnitType::PS;
	if (Want == EJRUnitType::PS && Count(EJRUnitType::PS) == 0) Want = EJRUnitType::PC;
	if (!FindFree(Want) && !QueueUsed.Contains(false)) return; // toko penuh, tidak jadi masuk

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AJRCustomer* C = GetWorld()->SpawnActor<AJRCustomer>(DoorLocation, FRotator::ZeroRotator, SP);
	C->Setup(TI, T.Name, JRBlockout::Hex(T.Top), JRBlockout::Hex(T.Bottom), T.Scale, Want, T.Want == 2);
	Customers.Add(C);
	Stats.Customers++;
	if (!Assign(C)) ToQueue(C);
}

bool AJRGameMode::Assign(AJRCustomer* C)
{
	AJRStation* S = FindFree(C->Want);
	if (!S && C->bFlex) S = FindFree(C->Want == EJRUnitType::PS ? EJRUnitType::PC : EJRUnitType::PS);
	if (!S) return false;
	if (C->QueueIdx >= 0) { QueueUsed[C->QueueIdx] = false; C->QueueIdx = -1; }
	S->Customer = C;
	C->Station = S;
	C->Patience = 1.f;
	C->WalkTo(S->GetSeatLocation(), EJRCustomerState::WaitStart);
	return true;
}

bool AJRGameMode::ToQueue(AJRCustomer* C)
{
	const int32 I = QueueUsed.Find(false);
	if (I == INDEX_NONE) return false;
	QueueUsed[I] = true;
	C->QueueIdx = I;
	C->Patience = 1.25f;
	C->WalkTo(QueueSpots[I], EJRCustomerState::Queue);
	return true;
}

void AJRGameMode::Release(AJRCustomer* C)
{
	if (C->Station) { C->Station->Customer = nullptr; C->Station = nullptr; }
}

void AJRGameMode::Leave(AJRCustomer* C)
{
	Release(C);
	if (C->QueueIdx >= 0) { QueueUsed[C->QueueIdx] = false; C->QueueIdx = -1; }
	C->WalkTo(DoorLocation, EJRCustomerState::Gone);
}

void AJRGameMode::CustomerGone(AJRCustomer* C)
{
	Customers.Remove(C);
	C->Destroy();
}

void AJRGameMode::LeaveAngry(AJRCustomer* C, const FString& Message)
{
	Stats.Satisfaction.Add(12);
	C->Mood = TEXT(">:(");
	AddToast(Message, Bad);
	Leave(C);
}

void AJRGameMode::RunAway(AJRCustomer* C)
{
	Stats.Kabur += C->Bill;
	Stats.RepDelta -= 0.5f;
	Stats.Satisfaction.Add(30);
	C->Mood = TEXT("KABUR!");
	C->Speed = 450.f;
	AddToast(FString::Printf(TEXT("%s kabur tanpa bayar %s! Kasir terlalu lama kosong."), *C->DisplayName, *JRFormatRp(C->Bill)), Bad);
	Leave(C);
}

void AJRGameMode::StartSession(AJRStation* S)
{
	AJRCustomer* C = S->Customer;
	if (!C) return;
	static const float Options[] = { 1.f, 1.5f, 2.f };
	const float Hours = C->TypeIndex == Bocil ? 1.f : Options[FMath::RandRange(0, 2)];
	C->HoursLeft = Hours;
	C->Used = S->Type;
	C->Bill = Hours * PriceFor(S->Type);
	C->State = EJRCustomerState::Playing;
	AddToast(FString::Printf(TEXT("%s menyala: %s main %s jam (%s)"), *S->DisplayName, *C->DisplayName,
		*FString::SanitizeFloat(Hours), *JRFormatRp(C->Bill)));
}

void AJRGameMode::TickPlaying(AJRCustomer* C, float DtH)
{
	AJRStation* S = C->Station;
	if (!S) return;
	if (S->bBroken || bMcbTripped)
	{
		C->WaitMinutes += DtH * 60.f;
		return;
	}
	C->HoursLeft -= DtH;

	const float BreakPerHour = S->Type == EJRUnitType::PS ? (bStikOri ? 0.07f : 0.22f) : 0.18f;
	if (FMath::FRand() < BreakPerHour * DtH)
	{
		S->bBroken = true;
		C->Breakdowns++;
		AddToast(S->Type == EJRUnitType::PS
			? FString::Printf(TEXT("Stik di %s rusak! Tahan E di sana untuk memperbaiki."), *S->DisplayName)
			: FString::Printf(TEXT("%s hang! Tahan E di sana untuk restart."), *S->DisplayName), Bad);
	}
	if (C->HoursLeft <= 0.f) EndSession(C);
}

void AJRGameMode::EndSession(AJRCustomer* C)
{
	Release(C);
	GoPay(C);
}

void AJRGameMode::GoPay(AJRCustomer* C)
{
	int32 N = 0;
	for (const AJRCustomer* X : Customers)
	{
		if (X != C && (X->State == EJRCustomerState::WaitPay || (X->State == EJRCustomerState::Walk && X->NextState == EJRCustomerState::WaitPay))) ++N;
	}
	const FVector Spot = P(-6.5f + FMath::Min(N, 4) * 0.85f, 5.9f);
	C->WalkTo(Spot, EJRCustomerState::WaitPay);
}

AJRCustomer* AJRGameMode::NextPayer() const
{
	AJRCustomer* Best = nullptr;
	for (AJRCustomer* C : Customers)
	{
		if (C->State == EJRCustomerState::WaitPay && (!Best || C->PayOrder < Best->PayOrder)) Best = C;
	}
	return Best;
}

void AJRGameMode::CollectNextPayer()
{
	AJRCustomer* C = NextPayer();
	if (!C) return;
	Money += C->Bill;
	Stats.Income += C->Bill;
	const int32 S = CalcSatisfaction(C);
	Stats.Satisfaction.Add(S);
	C->Mood = S >= 60 ? TEXT(":)") : S >= 35 ? TEXT(":|") : TEXT(">:(");
	AddToast(FString::Printf(TEXT("+%s dari %s"), *JRFormatRp(C->Bill), *C->DisplayName), Good);
	Leave(C);
}

int32 AJRGameMode::CalcSatisfaction(const AJRCustomer* C) const
{
	const float PriceRatio = float(PriceFor(C->Used)) / FairFor(C->Used);
	const float S = 70.f - 0.4f * C->WaitMinutes - 15.f * C->Breakdowns - 40.f * (PriceRatio - 1.f);
	return FMath::RoundToInt(FMath::Clamp(S, 0.f, 100.f));
}

/* ---------- Listrik ---------- */

void AJRGameMode::ComputePower()
{
	int32 U = 100; // lampu
	if (!bMcbTripped)
	{
		for (const AJRStation* S : Stations)
		{
			if (S->Customer && S->Customer->State == EJRCustomerState::Playing && !S->bBroken)
			{
				U += S->Type == EJRUnitType::PS ? 150 : 300;
			}
		}
	}
	PowerVA = U;
	if (!bMcbTripped && U > CapVA) Trip();
}

void AJRGameMode::Trip()
{
	bMcbTripped = true;
	for (AJRCustomer* C : Customers)
	{
		if (C->State == EJRCustomerState::Playing) C->Breakdowns++;
	}
	AddToast(TEXT("JEGLEK! Daya lewat batas, MCB turun. Naikkan MCB di dinding kiri."), Bad);
}

void AJRGameMode::ResetMcb()
{
	if (!bMcbTripped) return;
	bMcbTripped = false;
	AddToast(TEXT("Listrik menyala lagi"), Good);
}

/* ---------- Hari, laporan, toko ---------- */

void AJRGameMode::EndDay()
{
	Phase = EJRGamePhase::Report;
	for (AJRCustomer* C : Customers)
	{
		const bool bOwes = C->State == EJRCustomerState::Playing || C->State == EJRCustomerState::WaitPay
			|| (C->State == EJRCustomerState::Walk && C->NextState == EJRCustomerState::WaitPay);
		if (bOwes)
		{
			Money += C->Bill;
			Stats.Income += C->Bill;
			Stats.Satisfaction.Add(CalcSatisfaction(C));
		}
		C->Destroy();
	}
	Customers.Empty();
	for (bool& Used : QueueUsed) Used = false;
	for (AJRStation* S : Stations) S->Customer = nullptr;
	bMcbTripped = false;
	Money -= Stats.Elec;

	float Avg = 50.f;
	if (Stats.Satisfaction.Num() > 0)
	{
		float Sum = 0.f;
		for (int32 V : Stats.Satisfaction) Sum += V;
		Avg = Sum / Stats.Satisfaction.Num();
	}
	const float OldRep = Rep;
	Rep = FMath::Clamp(Rep + (Avg - 60.f) / 10.f + Stats.RepDelta, 0.f, 100.f);

	ReportLines.Reset();
	ReportLines.Add(FString::Printf(TEXT("Pelanggan datang: %d"), Stats.Customers));
	ReportLines.Add(FString::Printf(TEXT("Pemasukan sewa: %s"), *JRFormatRp(Stats.Income)));
	ReportLines.Add(FString::Printf(TEXT("Tagihan listrik: %s"), *JRFormatRp(-Stats.Elec)));
	ReportLines.Add(FString::Printf(TEXT("Untung bersih: %s"), *JRFormatRp(Stats.Income - Stats.Elec)));
	ReportLines.Add(FString::Printf(TEXT("Kabur tanpa bayar: %s"), *JRFormatRp(Stats.Kabur)));
	ReportLines.Add(FString::Printf(TEXT("Kepuasan rata-rata: %d/100"), FMath::RoundToInt(Avg)));
	ReportLines.Add(FString::Printf(TEXT("Reputasi: %d -> %d"), FMath::RoundToInt(OldRep), FMath::RoundToInt(Rep)));
	ReportLines.Add(Money >= 1500000.f && Rep >= 40.f
		? FString(TEXT("Gerbang 1 terbuka! Ruko kecil menyusul di versi berikutnya."))
		: FString::Printf(TEXT("Target Gerbang 1: Rp 1.500.000 dan reputasi 40. Sekarang %s, reputasi %d."), *JRFormatRp(Money), FMath::RoundToInt(Rep)));
}

TArray<FString> AJRGameMode::GetShopLines() const
{
	auto Line = [this](int32 Key, const TCHAR* Name, int32 Cost, bool bAvailable, const TCHAR* Done)
	{
		if (!bAvailable) return FString::Printf(TEXT("[%d] %s  -  %s"), Key, Name, Done);
		return FString::Printf(TEXT("[%d] %s  -  %s%s"), Key, Name, *JRFormatRp(Cost), Money < Cost ? TEXT("  (kas kurang)") : TEXT(""));
	};
	TArray<FString> Out;
	Out.Add(Line(1, TEXT("TV tabung + PlayStasiun 1"), 120000, Count(EJRUnitType::PS) < MaxPerType, TEXT("tempat penuh")));
	Out.Add(Line(2, TEXT("PC warnet"), 250000, Count(EJRUnitType::PC) < MaxPerType, TEXT("tempat penuh")));
	Out.Add(Line(3, TEXT("Stik original (jarang rusak)"), 50000, !bStikOri, TEXT("sudah")));
	Out.Add(Line(4, TEXT("Tambah daya ke 2.200 VA"), 200000, CapVA < 2200, TEXT("sudah")));
	Out.Add(FString::Printf(TEXT("[5] / [6] Tarif rental PS: %s/jam (wajar Rp 2.000)"), *JRFormatRp(PricePS)));
	return Out;
}

bool AJRGameMode::TryBuy(int32 Slot)
{
	if (Phase != EJRGamePhase::Report) return false;
	auto Pay = [this](int32 Cost) { if (Money < Cost) return false; Money -= Cost; return true; };
	switch (Slot)
	{
	case 1: if (Count(EJRUnitType::PS) < MaxPerType && Pay(120000)) { SpawnStation(EJRUnitType::PS); return true; } break;
	case 2: if (Count(EJRUnitType::PC) < MaxPerType && Pay(250000)) { SpawnStation(EJRUnitType::PC); return true; } break;
	case 3: if (!bStikOri && Pay(50000)) { bStikOri = true; return true; } break;
	case 4: if (CapVA < 2200 && Pay(200000)) { CapVA = 2200; return true; } break;
	case 5: PricePS = FMath::Min(6000, PricePS + 500); return true;
	case 6: PricePS = FMath::Max(1000, PricePS - 500); return true;
	default: break;
	}
	return false;
}

void AJRGameMode::Confirm()
{
	if (Phase == EJRGamePhase::Intro)
	{
		Phase = EJRGamePhase::Playing;
	}
	else if (Phase == EJRGamePhase::Report)
	{
		Day++;
		Hour = 8.f;
		Stats = FJRDayStats();
		Phase = EJRGamePhase::Playing;
	}
}

void AJRGameMode::AddToast(const FString& Text, const FLinearColor& Color)
{
	Toasts.Insert({ Text, Color, 5.f }, 0);
	if (Toasts.Num() > 5) Toasts.SetNum(5);
}
