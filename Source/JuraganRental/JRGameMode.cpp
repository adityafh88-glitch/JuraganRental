#include "JRGameMode.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
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
		const TCHAR* Mesh; // aset di /Game/Characters
		float Scale;
		int32 Want; // 0 = PS, 1 = PC, 2 = bebas
	};

	// Urutan: bocil, pelajar, mahasiswa, pekerja, gamer, bapak
	const FTypeDef Types[] = {
		{ TEXT("Bocil berseragam"), TEXT("SK_Bocil"),     0.80f, 0 },
		{ TEXT("Anak SMA"),         TEXT("SK_Anak_SMA"),  0.95f, 0 },
		{ TEXT("Mahasiswa"),        TEXT("SK_Mahasiswa"), 1.00f, 1 },
		{ TEXT("Mas-mas kantoran"), TEXT("SK_Kantoran"),  1.00f, 2 },
		{ TEXT("Gamer online"),     TEXT("SK_Gamer"),     1.00f, 1 },
		{ TEXT("Bapak-bapak"),      TEXT("SK_Bapak"),     1.03f, 0 },
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
	Mcb = World->SpawnActor<AJRMcb>(FVector(280.f, -993.f, 130.f), FRotator(0.f, 90.f, 0.f), SP);
	Stove = World->SpawnActor<AJRStove>(FVector(-90.f, -966.f, 0.f), FRotator(0.f, 90.f, 0.f), SP);

	QueueSpots = { P(-1.5f, 5.9f), P(-0.5f, 5.9f), P(0.5f, 5.9f), P(1.5f, 5.9f), P(2.5f, 5.2f) };
	QueueUsed.Init(false, QueueSpots.Num());
	DoorLocation = FVector(-900.f, 420.f, 0.f);

	SpawnStation(EJRUnitType::PS);
	SpawnStation(EJRUnitType::PS);
	SpawnStation(EJRUnitType::PC);

	// Mode uji otomatis: -jrautotest
	if (FParse::Param(FCommandLine::Get(), TEXT("jrautotest")))
	{
		bAutoTest = true;
		Phase = EJRGamePhase::Playing;
		Hour = 13.5f;
		CapVA = 2200; // supaya kompor bisa jalan bersama 5 unit
		SpawnStation(EJRUnitType::PS);
		SpawnStation(EJRUnitType::PC);
	}
}

void AJRGameMode::TickAutoTest(float DeltaSeconds)
{
	AutoTestTime += DeltaSeconds;

	// Pemain otomatis: nyalakan unit, perbaiki, terima bayaran, naikkan MCB.
	for (AJRStation* S : Stations)
	{
		if (S->bBroken) S->bBroken = false;
		if (S->Customer && S->Customer->State == EJRCustomerState::WaitStart) StartSession(S);
	}
	if (bMcbTripped) ResetMcb();
	// Kantin otomatis: masak, angkat, antar ke pemesan pertama.
	for (AJRStation* S : Stations)
	{
		if (!S->Customer || !S->Customer->bOrder) continue;
		if (bCarryMie) { DeliverMie(S); AutoTestMieDelivered++; }
		else if (StoveState == EJRStoveState::Idle) StartCook();
		else if (StoveState == EJRStoveState::Ready) TakeMie();
		break;
	}
	if (NextPayer() && FMath::FRand() < 0.02f) CollectNextPayer();
	if (Customers.Num() < 6 && FMath::FRand() < 0.05f) SpawnCustomer();

	struct FShot { float Time; FVector Loc; FRotator Rot; const TCHAR* Name; };
	static const FShot Shots[] = {
		{ 20.f, FVector(-560.f, -880.f, 250.f), FRotator(-18.f, 38.f, 0.f), TEXT("JR_Shot_RentalPS") },
		{ 23.f, FVector(-500.f, 300.f, 240.f), FRotator(-15.f, 15.f, 0.f), TEXT("JR_Shot_Warnet") },
		{ 26.f, FVector(120.f, -700.f, 190.f), FRotator(-18.f, -132.f, 0.f), TEXT("JR_Shot_Kantin") },
	};
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ACharacter* Pawn = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
	const int32 i = AutoTestShot;
	if (i < UE_ARRAY_COUNT(Shots) && AutoTestTime >= Shots[i].Time - 1.f)
	{
		if (Pawn)
		{
			Pawn->GetCharacterMovement()->DisableMovement();
			Pawn->SetActorEnableCollision(false);
			Pawn->SetActorLocation(Shots[i].Loc - FVector(0, 0, 64.f));
			PC->SetControlRotation(Shots[i].Rot);
		}
		if (AutoTestTime >= Shots[i].Time)
		{
			const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoTest") / FString(Shots[i].Name) + TEXT(".png"));
			FScreenshotRequest::RequestScreenshot(File, true, false);
			AutoTestShot = i + 1;
		}
	}
	// Buka HP dan potret layarnya juga.
	if (AutoTestShot == UE_ARRAY_COUNT(Shots) && AutoTestTime > 27.f)
	{
		if (AJRHUD* Hud = PC ? Cast<AJRHUD>(PC->GetHUD()) : nullptr)
		{
			if (!Hud->IsPhoneOpen()) Hud->SetPhoneOpen(true);
			if (AutoTestTime > 28.5f)
			{
				const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoTest/JR_Shot_HP.png"));
				FScreenshotRequest::RequestScreenshot(File, true, false);
				AutoTestShot++;
			}
		}
	}
	if (AutoTestTime > 30.f)
	{
		UE_LOG(LogTemp, Display, TEXT("JR_AUTOTEST_DONE customers=%d income=%.0f mie=%d stock=%.0f"), Stats.Customers, Stats.Income, AutoTestMieDelivered, Stats.Stock);
		FGenericPlatformMisc::RequestExit(false);
	}
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
	if (bAutoTest) TickAutoTest(DeltaSeconds);

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

	if (StoveState == EJRStoveState::Cooking && !bMcbTripped)
	{
		CookProgress += DeltaSeconds;
		if (CookProgress >= CookSeconds)
		{
			StoveState = EJRStoveState::Ready;
			AddToast(TEXT("Mi matang! Angkat di kompor."), FLinearColor(0.36f, 0.79f, 0.54f));
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
	C->Setup(TI, T.Name, T.Mesh, T.Scale, Want, T.Want == 2);
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
	// Pesan mi instan
	if (!C->bOrder && !C->bAte && FMath::FRand() < (bAutoTest ? 6.f : 0.4f) * DtH)
	{
		C->bOrder = true;
		C->OrderHours = 1.f;
		AddToast(FString::Printf(TEXT("%s di %s pesan mi instan"), *C->DisplayName, *S->DisplayName));
	}
	if (C->bOrder)
	{
		C->OrderHours -= DtH;
		if (C->OrderHours <= 0.f)
		{
			C->bOrder = false;
			C->MissedOrders++;
			AddToast(FString::Printf(TEXT("Pesanan mi di %s kelamaan, batal"), *S->DisplayName), Bad);
		}
	}
	if (C->HoursLeft <= 0.f) EndSession(C);
}

void AJRGameMode::StartCook()
{
	if (StoveState != EJRStoveState::Idle || bCarryMie) return;
	if (Money < MieCost)
	{
		AddToast(TEXT("Kas tidak cukup untuk beli mi"), Bad);
		return;
	}
	Money -= MieCost;
	Stats.Stock += MieCost;
	StoveState = EJRStoveState::Cooking;
	CookProgress = 0.f;
}

void AJRGameMode::TakeMie()
{
	if (StoveState != EJRStoveState::Ready || bCarryMie) return;
	StoveState = EJRStoveState::Idle;
	bCarryMie = true;
}

void AJRGameMode::DeliverMie(AJRStation* S)
{
	AJRCustomer* C = S ? S->Customer.Get() : nullptr;
	if (!C || !C->bOrder || !bCarryMie) return;
	bCarryMie = false;
	C->bOrder = false;
	C->bAte = true;
	C->Bill += MiePrice;
	AddToast(FString::Printf(TEXT("Mi diantar ke %s. +%s masuk tagihan"), *S->DisplayName, *JRFormatRp(MiePrice)), Good);
}

void AJRGameMode::EndSession(AJRCustomer* C)
{
	C->bOrder = false;
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
	CollectPayer(NextPayer());
}

void AJRGameMode::CollectPayer(AJRCustomer* C)
{
	if (!C || C->State != EJRCustomerState::WaitPay) return;
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
	const float S = 70.f - 0.4f * C->WaitMinutes - 15.f * C->Breakdowns - 40.f * (PriceRatio - 1.f)
		+ (C->bAte ? 5.f : 0.f) - 10.f * C->MissedOrders;
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
	if (!bMcbTripped && StoveState == EJRStoveState::Cooking) U += 600; // kompor listrik
	PowerVA = U;
	if (!bMcbTripped && U > CapVA) Trip();
}

void AJRGameMode::Trip()
{
	bMcbTripped = true;
	if (StoveState == EJRStoveState::Cooking)
	{
		StoveState = EJRStoveState::Idle;
		AddToast(TEXT("Mi di kompor gagal matang karena listrik mati"), Bad);
	}
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
	StoveState = EJRStoveState::Idle;
	bCarryMie = false;
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
	ReportLines.Add(FString::Printf(TEXT("Pemasukan sewa + jajan: %s"), *JRFormatRp(Stats.Income)));
	ReportLines.Add(FString::Printf(TEXT("Belanja bahan kantin: %s"), *JRFormatRp(-Stats.Stock)));
	ReportLines.Add(FString::Printf(TEXT("Tagihan listrik: %s"), *JRFormatRp(-Stats.Elec)));
	ReportLines.Add(FString::Printf(TEXT("Untung bersih: %s"), *JRFormatRp(Stats.Income - Stats.Elec - Stats.Stock)));
	ReportLines.Add(FString::Printf(TEXT("Kabur tanpa bayar: %s"), *JRFormatRp(Stats.Kabur)));
	ReportLines.Add(FString::Printf(TEXT("Kepuasan rata-rata: %d/100"), FMath::RoundToInt(Avg)));
	ReportLines.Add(FString::Printf(TEXT("Reputasi: %d -> %d"), FMath::RoundToInt(OldRep), FMath::RoundToInt(Rep)));
	ReportLines.Add(Money >= 1500000.f && Rep >= 40.f
		? FString(TEXT("Gerbang 1 terbuka! Ruko kecil menyusul di versi berikutnya."))
		: FString::Printf(TEXT("Target Gerbang 1: Rp 1.500.000 dan reputasi 40. Sekarang %s, reputasi %d."), *JRFormatRp(Money), FMath::RoundToInt(Rep)));
}

TArray<FJRShopItem> AJRGameMode::GetShopItems() const
{
	TArray<FJRShopItem> Out;
	const bool bFullPS = Count(EJRUnitType::PS) >= MaxPerType, bFullPC = Count(EJRUnitType::PC) >= MaxPerType;
	Out.Add({ TEXT("ps"), TEXT("TV tabung + PlayStasiun 1"), TEXT("Tambah 1 unit rental. 150 VA saat menyala."), 120000, bFullPS ? TEXT("Tempat penuh") : TEXT("") });
	Out.Add({ TEXT("pc"), TEXT("PC warnet"), TEXT("Tambah 1 bilik PC. 300 VA saat menyala."), 250000, bFullPC ? TEXT("Tempat penuh") : TEXT("") });
	Out.Add({ TEXT("stik"), TEXT("Stik original"), TEXT("Stik jauh lebih jarang rusak."), 50000, bStikOri ? TEXT("Sudah dibeli") : TEXT("") });
	Out.Add({ TEXT("daya"), TEXT("Tambah daya ke 2.200 VA"), TEXT("MCB tidak gampang turun."), 200000, CapVA >= 2200 ? TEXT("Sudah dibeli") : TEXT("") });
	return Out;
}

bool AJRGameMode::BuyItem(FName Id)
{
	if (Phase == EJRGamePhase::Intro) return false;
	for (const FJRShopItem& It : GetShopItems())
	{
		if (It.Id != Id) continue;
		if (!It.Lock.IsEmpty() || Money < It.Cost) return false;
		Money -= It.Cost;
		if (Id == TEXT("ps")) SpawnStation(EJRUnitType::PS);
		else if (Id == TEXT("pc")) SpawnStation(EJRUnitType::PC);
		else if (Id == TEXT("stik")) bStikOri = true;
		else if (Id == TEXT("daya")) CapVA = 2200;
		AddToast(FString::Printf(TEXT("Dibeli: %s (%s)"), *It.Name, *JRFormatRp(It.Cost)), FLinearColor(0.36f, 0.79f, 0.54f));
		return true;
	}
	return false;
}

void AJRGameMode::AdjustPrice(EJRUnitType Type, int32 Delta)
{
	int32& Price = Type == EJRUnitType::PS ? PricePS : PricePC;
	Price = FMath::Clamp(Price + Delta, 1000, 8000);
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
