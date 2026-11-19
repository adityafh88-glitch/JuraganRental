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
#include "JRDelivery.h"
#include "JRFixtures.h"
#include "JRHUD.h"
#include "JRSound.h"
#include "JRStation.h"
#include "JRVisitor.h"

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
	constexpr int32 Bocil = 0, Pekerja = 3, Gamer = 4;
	// Kode karakter untuk file suara VO_<Kode>_<Situasi>_<n> (lihat Docs/Naskah_Suara.md)
	const TCHAR* VoiceCode[] = { TEXT("Bocil"), TEXT("SMA"), TEXT("Mahasiswa"), TEXT("Kantoran"), TEXT("Gamer"), TEXT("Bapak") };

	const FLinearColor Good(0.36f, 0.79f, 0.54f);
	const FLinearColor Bad(0.94f, 0.42f, 0.36f);

	const TCHAR* HypeGames[] = { TEXT("Perang Kerajaan Online"), TEXT("Tembak-Tembakan Arena"), TEXT("Legenda Naga Nusantara"), TEXT("Balap Liar Online") };

	FJRLayout MakeGarasi()
	{
		FJRLayout L;
		L.Name = TEXT("GARASI");
		L.HalfX = 700.f; L.HalfY = 1000.f; L.DoorY = 420.f;
		L.PsY = { -800.f, -550.f, -300.f, -50.f };
		L.PcX = { 500.f, 250.f, 0.f, -250.f };
		L.Queue = { FVector(-650, -40, 0), FVector(-650, 30, 0), FVector(-650, 100, 0), FVector(-575, 200, 0), FVector(-575, 275, 0) };
		return L;
	}

	FJRLayout MakeRuko()
	{
		FJRLayout L;
		L.Name = TEXT("RUKO KECIL");
		L.HalfX = 800.f; L.HalfY = 1200.f; L.DoorY = 540.f;
		L.PsY = { -1000.f, -750.f, -500.f, -250.f, 0.f, 250.f };
		L.PcX = { 600.f, 380.f, 160.f, -60.f, -280.f, -500.f };
		L.Queue = { FVector(-750, -40, 0), FVector(-750, 30, 0), FVector(-750, 100, 0),
			FVector(-675, 200, 0), FVector(-675, 280, 0), FVector(-600, 200, 0), FVector(-600, 280, 0) };
		L.Rent = 75000.f;		// GDD awal 150rb; diturunkan setelah simulasi prototype
		L.Internet = 25000.f;
		L.bRouter = true;
		return L;
	}
}

const FJRLayout& AJRGameMode::Layout() const
{
	static const FJRLayout Garasi = MakeGarasi();
	static const FJRLayout Ruko = MakeRuko();
	return Era == 2 ? Ruko : Garasi;
}

AJRGameMode::AJRGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AJRCharacter::StaticClass();
	HUDClass = AJRHUD::StaticClass();
}

APawn* AJRGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	// Pemain selalu mulai di tengah ruangan menghadap deretan TV.
	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, FTransform(FRotator::ZeroRotator, Layout().PlayerStart()));
}

/* ---------- Membangun dunia per era ---------- */

void AJRGameMode::BeginPlay()
{
	Super::BeginPlay();
	const bool bTest = FParse::Param(FCommandLine::Get(), TEXT("jrautotest"));
	if (FParse::Param(FCommandLine::Get(), TEXT("jrera2")))
	{
		// Mulai langsung di ruko (uji / playtest).
		Era = 2; Money = 1500000.f; Rep = 45.f; CapVA = 2200; OwnedPS = 4; OwnedPC = 3;
	}
	else if (bTest)
	{
		OwnedPS = 3; OwnedPC = 2; CapVA = 2200; // supaya kompor bisa jalan bersama 5 unit
	}
	BuildWorld();

	if (bTest)
	{
		bAutoTest = true;
		Phase = EJRGamePhase::Playing;
		Hour = 13.5f;
		SpawnDelivery(TEXT("ps")); // uji kurir: kardus TV di depan pintu
		if (Era == 2) { HypeName = HypeGames[0]; HypeDays = 3; bHypeInstalled = true; }
	}
}

void AJRGameMode::BuildWorld()
{
	UWorld* World = GetWorld();
	const FJRLayout& L = Layout();
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Room = World->SpawnActor<AJRRoom>(FVector::ZeroVector, FRotator::ZeroRotator, SP);
	Room->Build(L.HalfX, L.HalfY, L.DoorY, Era == 2);
	Kasir = World->SpawnActor<AJRKasir>(L.Kasir(), FRotator::ZeroRotator, SP);
	Mcb = World->SpawnActor<AJRMcb>(L.Mcb(), FRotator(0.f, 90.f, 0.f), SP);
	Stove = World->SpawnActor<AJRStove>(L.Stove(), FRotator(0.f, 90.f, 0.f), SP);
	Router = L.bRouter ? World->SpawnActor<AJRRouter>(L.Router(), FRotator(0.f, 180.f, 0.f), SP) : nullptr;
	if (!SlotMarker) SlotMarker = World->SpawnActor<AJRSlotMarker>(FVector::ZeroVector, FRotator::ZeroRotator, SP);

	QueueUsed.Init(false, L.Queue.Num());
	for (int32 i = 0; i < OwnedPS; ++i) SpawnStation(EJRUnitType::PS);
	for (int32 i = 0; i < OwnedPC; ++i) SpawnStation(EJRUnitType::PC);
}

void AJRGameMode::MoveToEra2()
{
	// Simpan jumlah unit dan isi kardus, lalu bongkar garasi.
	OwnedPS = Count(EJRUnitType::PS);
	OwnedPC = Count(EJRUnitType::PC);
	TArray<FName> BoxIds;
	for (AJRDeliveryBox* B : Boxes) { if (B) { BoxIds.Add(B->ItemId); B->Destroy(); } }
	Boxes.Reset();
	for (AJRStation* S : Stations) if (S) S->Destroy();
	Stations.Reset();
	for (AActor* A : TArray<AActor*>{ Room, Kasir, Mcb, Stove, Router }) if (A) A->Destroy();

	Era = 2;
	CapVA = FMath::Max(CapVA, 2200);
	BuildWorld();
	for (const FName& Id : BoxIds) SpawnDelivery(Id);

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn()) Pawn->SetActorLocation(Layout().PlayerStart(), false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(FRotator::ZeroRotator);
	}
	AddToast(TEXT("Selamat datang di ruko baru! Sewa dan internet ditagih tiap tutup toko."), Good);
}

/* ---------- Mode uji otomatis ---------- */

void AJRGameMode::TickAutoTest(float DeltaSeconds)
{
	AutoTestTime += DeltaSeconds;

	// Pemain otomatis: nyalakan unit, perbaiki, terima bayaran, naikkan MCB, restart router.
	for (AJRStation* S : Stations)
	{
		if (S->bBroken) { S->bBroken = false; S->bVirus = false; }
		if (S->Customer && S->Customer->State == EJRCustomerState::WaitStart) StartSession(S);
	}
	if (bMcbTripped) ResetMcb();
	if (bNetSlow && AutoTestTime > 12.f) RestartRouter();
	// Kantin otomatis: masak, angkat, antar ke pemesan pertama.
	for (AJRStation* S : Stations)
	{
		if (!S->Customer || !S->Customer->bOrder) continue;
		if (bCarryMie) { DeliverMie(S); AutoTestMieDelivered++; }
		else if (StoveState == EJRStoveState::Idle && CarryBox.IsNone()) StartCook();
		else if (StoveState == EJRStoveState::Ready) TakeMie();
		break;
	}
	if (NextPayer() && FMath::FRand() < 0.02f) CollectNextPayer();
	// Kurir otomatis: angkat kardus lalu pasang.
	if (!CarryBox.IsNone()) { InstallBox(); AutoTestInstalled++; }
	else if (Boxes.Num() > 0 && !bCarryMie && AutoTestTime > 3.f) PickBox(Boxes[0]);
	if (Customers.Num() < 7 && FMath::FRand() < 0.05f) SpawnCustomer();
	// Era 2: picu internet lemot dan Pak RT agar ikut teruji.
	if (Era == 2 && AutoTestTime > 8.f && AutoTestTime - DeltaSeconds <= 8.f) { bNetSlow = true; AddToast(TEXT("Internet lemot! (uji)"), Bad); }
	if (Era == 2 && AutoTestTime > 10.f && AutoTestTime - DeltaSeconds <= 10.f) PakRtEvent();

	struct FShot { float Time; FVector Loc; FRotator Rot; const TCHAR* Name; };
	static const FShot ShotsGarasi[] = {
		{ 20.f, FVector(-560.f, -880.f, 250.f), FRotator(-18.f, 38.f, 0.f), TEXT("JR_Shot_RentalPS") },
		{ 23.f, FVector(-500.f, 300.f, 240.f), FRotator(-15.f, 15.f, 0.f), TEXT("JR_Shot_Warnet") },
		{ 26.f, FVector(-280.f, -230.f, 190.f), FRotator(-14.f, 148.f, 0.f), TEXT("JR_Shot_Bangku") },
	};
	static const FShot ShotsRuko[] = {
		{ 20.f, FVector(-700.f, -1100.f, 260.f), FRotator(-16.f, 32.f, 0.f), TEXT("JR_Shot_Ruko_PS") },
		{ 23.f, FVector(-650.f, 300.f, 250.f), FRotator(-14.f, 22.f, 0.f), TEXT("JR_Shot_Ruko_Warnet") },
		{ 26.f, FVector(700.f, -900.f, 240.f), FRotator(-12.f, 150.f, 0.f), TEXT("JR_Shot_Ruko_Pintu") },
	};
	const FShot* Shots = Era == 2 ? ShotsRuko : ShotsGarasi;
	const int32 NumShots = 3;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ACharacter* Pawn = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
	const int32 i = AutoTestShot;
	if (i < NumShots && AutoTestTime >= Shots[i].Time - 1.f)
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
	if (AutoTestShot == NumShots && AutoTestTime > 27.f)
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
		UE_LOG(LogTemp, Display, TEXT("JR_AUTOTEST_DONE era=%d customers=%d income=%.0f mie=%d stock=%.0f events=%d installed=%d stations=%d netslow=%d"),
			Era, Stats.Customers, Stats.Income, AutoTestMieDelivered, Stats.Stock, AutoTestEvents, AutoTestInstalled, Stations.Num(), bNetSlow ? 1 : 0);
		FGenericPlatformMisc::RequestExit(false);
	}
}

/* ---------- Unit ---------- */

FVector AJRGameMode::SlotLocation(EJRUnitType Type, int32 Index, FRotator& OutRot) const
{
	const FJRLayout& L = Layout();
	OutRot = FRotator(0.f, Type == EJRUnitType::PS ? 180.f : -90.f, 0.f); // menghadap ke dalam ruangan
	return Type == EJRUnitType::PS ? FVector(L.HalfX - 80.f, L.PsY[Index], 0.f) : FVector(L.PcX[Index], L.HalfY - 70.f, 0.f);
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
	if (I >= Layout().MaxUnits()) return;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FRotator Rot;
	const FVector Loc = SlotLocation(Type, I, Rot);
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

	// Mode uji: potret dialog pertama lalu pilih opsi pertama.
	if (bAutoTest && Choices.Num() > 0)
	{
		AutoChoiceTimer += DeltaSeconds;
		if (AutoTestEvents == 0 && AutoChoiceTimer > 0.6f && AutoChoiceTimer - DeltaSeconds <= 0.6f)
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoTest/JR_Shot_Event.png")), true, false);
		}
		if (AutoChoiceTimer > 1.5f) { AutoTestEvents++; AutoChoiceTimer = 0.f; ResolveChoice(0); }
		AutoTestTime += DeltaSeconds;
	}

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
			JRSound::PlayAt(this, TEXT("SFX_Ding"), Layout().Stove() + FVector(0, 0, 100));
			AddToast(TEXT("Mi matang! Angkat di kompor."), Good);
		}
	}

	// Era 2: internet lemot dan Pak RT
	if (Era == 2 && !bNetSlow && !bAutoTest && FMath::FRand() < (bFastNet ? 0.04f : 0.12f) * DtH)
	{
		bNetSlow = true;
		JRSound::Play2D(this, TEXT("SFX_Modem"), 0.6f);
		for (AJRCustomer* X : Customers)
		{
			if (X->State == EJRCustomerState::Playing && X->Used == EJRUnitType::PC && JRSound::Voice(this, VoiceCode[X->TypeIndex], TEXT("Lemot"), X->GetActorLocation())) break;
		}
		AddToast(TEXT("Internet lemot! Gamer mulai ngomel. Tahan E di router (dinding belakang) untuk restart."), Bad);
	}
	if (RtAt >= 0.f && Hour >= RtAt)
	{
		RtAt = -1.f;
		PakRtEvent();
	}

	// Suasana: motor lewat di depan sesekali (pengganti dengung terus-menerus)
	AmbientTimer -= DeltaSeconds;
	if (AmbientTimer <= 0.f)
	{
		AmbientTimer = FMath::FRandRange(25.f, 55.f);
		JRSound::PlayAt(this, TEXT("SFX_Motor_Lewat"), Layout().DoorOutside() + FVector(0, FMath::FRandRange(-600.f, 600.f), 80.f), 0.5f);
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
	float Base = H < 13.f ? 1.6f : H < 17.f ? 3.2f : H < 19.f ? 2.0f : H < 22.f ? 3.0f : 1.2f;
	if (Era == 2 && bPaketMalam && H >= 21.f) Base *= 1.8f;
	if (bQuiet && H >= 22.f) Base *= 0.5f;
	const float RepMul = 0.5f + Rep / 100.f;
	const float PriceMul = FMath::Pow((float(FairFor(EJRUnitType::PS)) / PricePS + 3000.f / PricePC) / 2.f, 1.5f);
	float R = Base * RepMul * PriceMul * FMath::Min(Era == 2 ? 2.6f : 1.8f, 0.55f + Stations.Num() * 0.18f);
	if (Era == 2)
	{
		R *= 1.3f * (1.f + 0.04f * Regulars); // ruko di jalan besar
		if (bHypeInstalled && !HypeName.IsEmpty()) R *= 1.3f;
	}
	return R;
}

int32 AJRGameMode::PickType(float H) const
{
	const float HypeMul = (Era == 2 && bHypeInstalled && !HypeName.IsEmpty()) ? 2.f : 1.f;
	const float GamerDay = Era == 2 ? HypeMul : 0.f;
	float W[6] = { 0, 1, 0, 3, (Era == 2 ? 4.f : 3.f) * HypeMul, 2 };
	if (H < 13.f)      { const float X[6] = { 4, 0, 3, 1, 0, 1 }; FMemory::Memcpy(W, X, sizeof(W)); }
	else if (H < 17.f) { const float X[6] = { 1, 5, 2, 0, GamerDay, 1 }; FMemory::Memcpy(W, X, sizeof(W)); }
	else if (H < 19.f) { const float X[6] = { 0, 2, 1, 3, GamerDay, 2 }; FMemory::Memcpy(W, X, sizeof(W)); }
	float Total = 0.f;
	for (float V : W) Total += V;
	float R = FMath::FRand() * Total;
	for (int32 i = 0; i < 6; ++i)
	{
		R -= W[i];
		if (R <= 0.f && W[i] > 0.f) return i;
	}
	return Pekerja;
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
	AJRCustomer* C = GetWorld()->SpawnActor<AJRCustomer>(Layout().DoorOutside(), FRotator::ZeroRotator, SP);
	C->Setup(TI, T.Name, T.Mesh, T.Scale, Want, T.Want == 2);
	Customers.Add(C);
	Stats.Customers++;
	JRSound::PlayAt(this, TEXT("SFX_Bel_Pintu"), FVector(-(Layout().HalfX - 40.f), Layout().DoorY, 220.f), 0.6f);
	if (!Assign(C)) ToQueue(C);
}

bool AJRGameMode::Assign(AJRCustomer* C)
{
	AJRStation* S = FindFree(C->Want);
	if (!S && C->bFlex) S = FindFree(C->Want == EJRUnitType::PS ? EJRUnitType::PC : EJRUnitType::PS);
	if (!S) return false;
	if (C->QueueIdx >= 0) { QueueUsed[C->QueueIdx] = false; C->QueueIdx = -1; }
	C->bBenchSeat = false;
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
	C->bBenchSeat = I < 3;
	C->Patience = 1.25f;
	C->WalkTo(Layout().Queue[I], EJRCustomerState::Queue);
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
	C->WalkTo(Layout().DoorOutside(), EJRCustomerState::Gone);
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
	JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Marah_Pulang"), C->GetActorLocation());
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
	C->Used = S->Type;
	C->State = EJRCustomerState::Playing;

	// Paket malam warnet (Era 2): flat sampai tutup.
	if (S->Type == EJRUnitType::PC && Era == 2 && bPaketMalam && Hour >= 21.f && (C->TypeIndex == Gamer || C->TypeIndex == Pekerja))
	{
		C->bPaket = true;
		C->HoursLeft = 24.f - Hour;
		C->Bill = 10000.f;
		JRSound::PlayAt(this, TEXT("SFX_TV_Nyala"), S->GetActorLocation(), 0.6f);
		JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Paket"), C->GetActorLocation());
		AddToast(FString::Printf(TEXT("%s: %s ambil paket malam sampai tutup (Rp 10.000)"), *S->DisplayName, *C->DisplayName));
		return;
	}

	static const float OptionsGarasi[] = { 1.f, 1.5f, 2.f };
	static const float OptionsRuko[] = { 1.f, 1.5f, 2.f, 2.5f, 3.f };
	const float Hours = C->TypeIndex == Bocil ? 1.f
		: (Era == 2 ? OptionsRuko[FMath::RandRange(0, 4)] : OptionsGarasi[FMath::RandRange(0, 2)]);
	C->HoursLeft = Hours;
	C->Bill = Hours * PriceFor(S->Type);
	if (C->TypeIndex == Bocil && FMath::FRand() < (bAutoTest ? 1.f : 0.35f))
	{
		C->EmakAt = bAutoTest ? 0.05f : FMath::FRandRange(0.25f, 0.7f);
	}
	JRSound::PlayAt(this, S->Type == EJRUnitType::PS ? TEXT("SFX_Konsol_Boot") : TEXT("SFX_TV_Nyala"), S->GetActorLocation() + FVector(0, 0, 100), S->Type == EJRUnitType::PS ? 0.8f : 0.6f);
	AddToast(FString::Printf(TEXT("%s menyala: %s main %s jam (%s)"), *S->DisplayName, *C->DisplayName,
		*FString::SanitizeFloat(Hours), *JRFormatRp(C->Bill)));

	if (Era == 2 && S->Type == EJRUnitType::PC && Hour >= 19.f && FMath::FRand() < 0.15f) JokiEvent(C);
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
	if (C->bRaging) return;
	C->HoursLeft -= DtH;
	C->Played += DtH;
	if (C->EmakAt >= 0.f && C->Played >= C->EmakAt)
	{
		C->EmakAt = -1.f;
		EmakEvent(C);
		return;
	}
	// Internet lemot: gamer di PC tidak puas
	if (S->Type == EJRUnitType::PC && bNetSlow)
	{
		C->WaitMinutes += DtH * 30.f;
		if (!C->bLagged) { C->bLagged = true; C->SatAdj -= 10.f; }
	}

	const float BreakPerHour = S->Type == EJRUnitType::PS ? (bStikOri ? 0.07f : 0.22f) : 0.18f;
	if (FMath::FRand() < BreakPerHour * DtH)
	{
		S->bBroken = true;
		C->Breakdowns++;
		if (S->Type == EJRUnitType::PC && Era == 2 && FMath::FRand() < (bAntivirus ? 0.1f : 0.45f))
		{
			S->bVirus = true;
			JRSound::PlayAt(this, TEXT("SFX_Error_PC"), S->GetActorLocation() + FVector(0, 0, 100));
			AddToast(FString::Printf(TEXT("%s kena virus! Tahan E lebih lama di sana untuk install ulang."), *S->DisplayName), Bad);
		}
		else
		{
			JRSound::PlayAt(this, S->Type == EJRUnitType::PS ? TEXT("SFX_Stik_Rusak") : TEXT("SFX_Error_PC"), S->GetActorLocation() + FVector(0, 0, 100));
			AddToast(S->Type == EJRUnitType::PS
				? FString::Printf(TEXT("Stik di %s rusak! Tahan E di sana untuk memperbaiki."), *S->DisplayName)
				: FString::Printf(TEXT("%s hang! Tahan E di sana untuk restart."), *S->DisplayName), Bad);
		}
		JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Rusak"), C->GetActorLocation());
	}
	// Pesan mi instan
	if (!C->bOrder && !C->bAte && FMath::FRand() < (bAutoTest ? 6.f : 0.4f) * DtH)
	{
		C->bOrder = true;
		C->OrderHours = 1.f;
		JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Pesan_Mi"), C->GetActorLocation());
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
	if (C->HoursLeft <= 0.f)
	{
		if (S->Type == EJRUnitType::PS && C->TypeIndex != Bocil && FMath::FRand() < (bAutoTest ? 0.5f : 0.1f))
		{
			RageQuitEvent(C);
			return;
		}
		EndSession(C);
	}
}

/* ---------- Kejadian ---------- */

void AJRGameMode::PushChoice(const FString& Title, const FString& Body, TArray<FString> Labels, TArray<TFunction<void()>> Actions)
{
	FJRChoice C;
	C.Title = Title;
	C.Body = Body;
	C.Labels = MoveTemp(Labels);
	C.Actions = MoveTemp(Actions);
	Choices.Add(MoveTemp(C));
}

void AJRGameMode::ResolveChoice(int32 Index)
{
	if (Choices.Num() == 0) return;
	FJRChoice C = MoveTemp(Choices[0]);
	Choices.RemoveAt(0);
	if (C.Actions.IsValidIndex(Index) && C.Actions[Index]) C.Actions[Index]();
}

void AJRGameMode::EmakEvent(AJRCustomer* C)
{
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FJRLayout& L = Layout();
	AJRVisitor* Emak = GetWorld()->SpawnActor<AJRVisitor>(FVector(-(L.HalfX - 100.f), L.DoorY, 0.f), FRotator::ZeroRotator, SP);
	if (Emak)
	{
		Emak->Setup(TEXT("SK_Emak"), TEXT("A_Marah"));
		JRSound::Voice(this, TEXT("Emak"), TEXT("Datang"), Emak->GetActorLocation());
	}
	const FString Where = C->Station ? C->Station->DisplayName : FString(TEXT("TV"));
	TWeakObjectPtr<AJRCustomer> WC(C);
	TWeakObjectPtr<AJRVisitor> WE(Emak);
	const FVector Door = L.DoorOutside();
	PushChoice(TEXT("Emak datang!"),
		FString::Printf(TEXT("Seorang ibu berdiri di pintu membawa sapu: \"Bang, lihat anak saya? Seragam putih biru, harusnya masih di sekolah!\" Bocil di %s pura-pura tidak dengar."), *Where),
		{ TEXT("Serahkan bocilnya"), TEXT("Bilang tidak ada") },
		{
			[this, WC, WE, Door]()
			{
				if (WE.IsValid()) WE->LeaveTo(Door);
				AJRCustomer* B = WC.Get();
				if (!B || B->State != EJRCustomerState::Playing) return;
				const float Paid = FMath::RoundToFloat(B->Bill * 0.5f);
				Money += Paid;
				Stats.Income += Paid;
				Stats.Satisfaction.Add(50);
				Stats.RepDelta += 2.f;
				B->bOrder = false;
				B->Mood = TEXT("T_T");
				if (WE.IsValid()) JRSound::Voice(this, TEXT("Emak"), TEXT("Jewer"), WE->GetActorLocation());
				JRSound::Voice(this, TEXT("Bocil"), TEXT("Ketahuan"), B->GetActorLocation());
				AddToast(FString::Printf(TEXT("Bocil dijewer pulang. Emak bayar setengah: %s. Reputasi warga +2"), *JRFormatRp(Paid)), Good);
				Leave(B);
			},
			[this, WE, Door]()
			{
				if (WE.IsValid())
				{
					JRSound::Voice(this, TEXT("Emak"), TEXT("Pergi"), WE->GetActorLocation());
					WE->LeaveTo(Door);
				}
				Stats.RepDelta -= 3.f;
				AddToast(TEXT("Emak pergi sambil ngomel. Reputasi warga -3"), Bad);
			}
		});
}

void AJRGameMode::RageQuitEvent(AJRCustomer* C)
{
	AJRStation* S = C->Station;
	C->bRaging = true;
	if (S) S->bBroken = true;
	JRSound::PlayAt(this, TEXT("SFX_Stik_Banting"), C->GetActorLocation());
	JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Rage"), C->GetActorLocation());
	TWeakObjectPtr<AJRCustomer> WC(C);
	PushChoice(TEXT("Rage quit!"),
		FString::Printf(TEXT("%s kalah adu penalti lalu membanting stik di %s. Stiknya rusak."), *C->DisplayName, S ? *S->DisplayName : TEXT("TV")),
		{ TEXT("Tagih ganti rugi Rp 15.000"), TEXT("Sudahlah, maafkan") },
		{
			[this, WC]()
			{
				AJRCustomer* X = WC.Get();
				if (!X) return;
				X->Bill += 15000.f;
				X->bFined = true;
				X->SatAdj -= 15.f;
				Stats.RepDelta -= 2.f;
				AddToast(TEXT("Ganti rugi masuk tagihan. Reputasi -2"), Bad);
				X->bRaging = false;
				EndSession(X);
			},
			[this, WC]()
			{
				AJRCustomer* X = WC.Get();
				if (!X) return;
				Stats.RepDelta += 1.f;
				AddToast(TEXT("Warga menilai kamu sabar. Reputasi +1"), Good);
				X->bRaging = false;
				EndSession(X);
			}
		});
}

void AJRGameMode::PakRtEvent()
{
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FJRLayout& L = Layout();
	AJRVisitor* Rt = GetWorld()->SpawnActor<AJRVisitor>(FVector(-(L.HalfX - 100.f), L.DoorY, 0.f), FRotator::ZeroRotator, SP);
	if (Rt)
	{
		Rt->Setup(TEXT("SK_Pak_RT"), TEXT("A_Marah"));
		JRSound::Voice(this, TEXT("PakRT"), TEXT("Datang"), Rt->GetActorLocation());
	}
	TWeakObjectPtr<AJRVisitor> WR(Rt);
	const FVector Door = L.DoorOutside();
	auto Bye = [this, WR, Door](const TCHAR* Line)
	{
		if (!WR.IsValid()) return;
		JRSound::Voice(this, TEXT("PakRT"), Line, WR->GetActorLocation());
		WR->LeaveTo(Door);
	};
	PushChoice(TEXT("Pak RT datang"),
		TEXT("\"Mas, sudah jam sepuluh lewat. Warga komplain suara teriakan main game sampai ke gang sebelah.\""),
		{ TEXT("Kecilkan suara"), TEXT("Kasih kopi gratis (Rp 5.000)"), TEXT("Abaikan") },
		{
			[this, Bye]() { Bye(TEXT("Kecil")); bQuiet = true; AddToast(TEXT("Suara dikecilkan. Malam ini pelanggan lebih sepi")); },
			[this, Bye]() { Bye(TEXT("Kopi")); Money -= 5000.f; Stats.Stock += 5000.f; Stats.RepDelta += 1.f; AddToast(TEXT("Pak RT ngopi dulu, urusan beres. Reputasi +1"), Good); },
			[this, Bye]() { Bye(TEXT("Abai")); Stats.RepDelta -= 3.f; AddToast(TEXT("Pak RT tersinggung. Reputasi warga -3"), Bad); }
		});
}

void AJRGameMode::JokiEvent(AJRCustomer* C)
{
	TWeakObjectPtr<AJRCustomer> WC(C);
	JRSound::Voice(this, TEXT("Gamer"), TEXT("Joki"), C->GetActorLocation());
	PushChoice(TEXT("Joki rank datang"),
		FString::Printf(TEXT("\"Bang, saya joki rank. Pakai %s, kasih diskon dong. Nanti saya bawa satu tim ke sini.\""), C->Station ? *C->Station->DisplayName : TEXT("PC")),
		{ TEXT("Kasih diskon 20%"), TEXT("Harga normal") },
		{
			[this, WC]()
			{
				AJRCustomer* X = WC.Get();
				if (!X) return;
				X->Bill = FMath::RoundToFloat(X->Bill * 0.8f);
				Regulars = FMath::Min(10, Regulars + 1);
				Stats.RepDelta += 1.f;
				AddToast(TEXT("Joki jadi langganan. Gamer online makin sering datang"), Good);
			},
			[WC]()
			{
				if (AJRCustomer* X = WC.Get()) X->SatAdj -= 10.f;
			}
		});
}

float AJRGameMode::TotalDebt() const
{
	float Sum = 0.f;
	for (float D : Debts) Sum += D;
	return Sum;
}

/* ---------- Kantin ---------- */

void AJRGameMode::StartCook()
{
	if (StoveState != EJRStoveState::Idle || IsCarrying()) return;
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
	if (StoveState != EJRStoveState::Ready || IsCarrying()) return;
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
	JRSound::PlayAt(this, TEXT("SFX_Notif"), S->GetActorLocation(), 0.7f);
	AddToast(FString::Printf(TEXT("Mi diantar ke %s. +%s masuk tagihan"), *S->DisplayName, *JRFormatRp(MiePrice)), Good);
}

/* ---------- Kasir ---------- */

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
	const FVector K = Layout().Kasir();
	C->WalkTo(FVector(K.X - 100.f, K.Y + FMath::Min(N, 4) * 85.f, 0.f), EJRCustomerState::WaitPay);
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
	if (C->TypeIndex != Bocil && !C->bFined && FMath::FRand() < 0.12f)
	{
		static const TCHAR* Excuses[] = { TEXT("Besok bayar, bang. Sumpah."), TEXT("Gajian minggu depan, bang."), TEXT("Dompet ketinggalan di motor, bang.") };
		TWeakObjectPtr<AJRCustomer> WC(C);
		JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Ngutang"), C->GetActorLocation());
		PushChoice(TEXT("Bang, ngutang dulu ya"),
			FString::Printf(TEXT("\"%s\" Tagihannya %s."), Excuses[FMath::RandRange(0, 2)], *JRFormatRp(C->Bill)),
			{ TEXT("Catat di buku utang"), TEXT("Tolak, harus bayar") },
			{
				[this, WC]()
				{
					AJRCustomer* X = WC.Get();
					if (!X || X->State != EJRCustomerState::WaitPay) return;
					Debts.Add(X->Bill);
					Stats.Utang += X->Bill;
					AddToast(FString::Printf(TEXT("%s dicatat di buku utang"), *JRFormatRp(X->Bill)));
					FinishPayment(X, 0.f);
				},
				[this, WC]()
				{
					AJRCustomer* X = WC.Get();
					if (!X || X->State != EJRCustomerState::WaitPay) return;
					X->SatAdj -= 20.f;
					Stats.RepDelta -= 0.5f;
					FinishPayment(X, X->Bill);
				}
			});
		return;
	}
	FinishPayment(C, C->Bill);
}

void AJRGameMode::FinishPayment(AJRCustomer* C, float Paid)
{
	Money += Paid;
	Stats.Income += Paid;
	const int32 S = CalcSatisfaction(C);
	Stats.Satisfaction.Add(S);
	C->Mood = Paid <= 0.f ? TEXT(";)") : S >= 60 ? TEXT(":)") : S >= 35 ? TEXT(":|") : TEXT(">:(");
	if (Paid > 0.f)
	{
		JRSound::PlayAt(this, TEXT("SFX_Kaching"), Layout().Kasir() + FVector(0, 0, 110));
		if (FMath::FRand() < 0.5f) JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Bayar"), C->GetActorLocation());
		AddToast(FString::Printf(TEXT("+%s dari %s"), *JRFormatRp(Paid), *C->DisplayName), Good);
	}
	Leave(C);
}

int32 AJRGameMode::CalcSatisfaction(const AJRCustomer* C) const
{
	const float PriceRatio = C->bPaket ? 1.f : float(PriceFor(C->Used)) / FairFor(C->Used);
	const float S = 70.f - 0.4f * C->WaitMinutes - 15.f * C->Breakdowns - 40.f * (PriceRatio - 1.f)
		+ (C->bAte ? 5.f : 0.f) - 10.f * C->MissedOrders + C->SatAdj;
	return FMath::RoundToInt(FMath::Clamp(S, 0.f, 100.f));
}

/* ---------- Listrik dan internet ---------- */

void AJRGameMode::ComputePower()
{
	int32 U = Era == 2 ? 250 : 100; // lampu (+ kulkas di ruko)
	if (!bMcbTripped)
	{
		for (const AJRStation* S : Stations)
		{
			if (S->Customer && S->Customer->State == EJRCustomerState::Playing && !S->bBroken)
			{
				U += S->Type == EJRUnitType::PS ? (bPS2 ? 180 : 150) : 300;
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
	JRSound::Play2D(this, TEXT("SFX_MCB_Turun"));
	AddToast(TEXT("JEGLEK! Daya lewat batas, MCB turun. Naikkan MCB di dinding kiri."), Bad);
}

void AJRGameMode::ResetMcb()
{
	if (!bMcbTripped) return;
	bMcbTripped = false;
	JRSound::Play2D(this, TEXT("SFX_MCB_Naik"));
	AddToast(TEXT("Listrik menyala lagi"), Good);
}

void AJRGameMode::RestartRouter()
{
	if (!bNetSlow) return;
	bNetSlow = false;
	JRSound::Play2D(this, TEXT("SFX_Notif"), 0.7f);
	AddToast(TEXT("Internet lancar lagi"), Good);
}

void AJRGameMode::TogglePaketMalam()
{
	bPaketMalam = !bPaketMalam;
}

/* ---------- Hari, laporan ---------- */

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
	bNetSlow = false;
	StoveState = EJRStoveState::Idle;
	bCarryMie = false;
	if (!CarryBox.IsNone()) { SpawnDelivery(CarryBox); CarryBox = NAME_None; }
	Choices.Reset();
	const FJRLayout& L = Layout();
	Money -= Stats.Elec + L.Rent + L.Internet;

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
	if (Era == 2)
	{
		ReportLines.Add(FString::Printf(TEXT("Sewa ruko + internet: %s"), *JRFormatRp(-(L.Rent + L.Internet))));
	}
	ReportLines.Add(FString::Printf(TEXT("Untung bersih: %s"), *JRFormatRp(Stats.Income - Stats.Elec - Stats.Stock - L.Rent - L.Internet)));
	ReportLines.Add(FString::Printf(TEXT("Kabur tanpa bayar: %s"), *JRFormatRp(Stats.Kabur)));
	ReportLines.Add(FString::Printf(TEXT("Utang baru dicatat: %s (total piutang %s)"), *JRFormatRp(Stats.Utang), *JRFormatRp(TotalDebt())));
	if (PendingOrders.Num() > 0) ReportLines.Add(FString::Printf(TEXT("Pesanan diantar kurir besok pagi: %d kardus"), PendingOrders.Num()));
	ReportLines.Add(FString::Printf(TEXT("Kepuasan rata-rata: %d/100"), FMath::RoundToInt(Avg)));
	ReportLines.Add(FString::Printf(TEXT("Reputasi: %d -> %d"), FMath::RoundToInt(OldRep), FMath::RoundToInt(Rep)));

	// Berita besok (Era 2): game hype
	if (!HypeName.IsEmpty() && --HypeDays <= 0)
	{
		ReportLines.Add(FString::Printf(TEXT("Demam %s mulai reda."), *HypeName));
		HypeName.Reset();
		bHypeInstalled = false;
	}
	if (Era == 2 && HypeName.IsEmpty() && FMath::FRand() < 0.5f)
	{
		HypeName = HypeGames[FMath::RandRange(0, UE_ARRAY_COUNT(HypeGames) - 1)];
		HypeDays = 3;
		bHypeInstalled = false;
		ReportLines.Add(FString::Printf(TEXT("Besok game baru hype: %s. Install di semua PC lewat HP."), *HypeName));
	}

	if (Era == 1)
	{
		ReportLines.Add(Gate1Open()
			? FString(TEXT("Gerbang 1 terbuka! Buka HP > Toko untuk pindah ke ruko kecil."))
			: FString::Printf(TEXT("Target Gerbang 1: Rp 1.500.000 dan reputasi 40. Sekarang %s, reputasi %d."), *JRFormatRp(Money), FMath::RoundToInt(Rep)));
	}
	else
	{
		const int32 Units = Count(EJRUnitType::PS) + Count(EJRUnitType::PC);
		ReportLines.Add(Money >= 8000000.f && Rep >= 65.f && Units >= 10
			? FString(TEXT("Gerbang 2 tercapai! Game Center 2 lantai menunggu di Era 3."))
			: FString::Printf(TEXT("Target Gerbang 2: Rp 8 juta, reputasi 65, 10 unit. Sekarang %s, reputasi %d, %d unit."), *JRFormatRp(Money), FMath::RoundToInt(Rep), Units));
	}
}

/* ---------- Toko (HP), kurir ---------- */

TArray<FJRShopItem> AJRGameMode::GetShopItems() const
{
	TArray<FJRShopItem> Out;
	const int32 Max = Layout().MaxUnits();
	const bool bFullPS = Count(EJRUnitType::PS) + InPipeline(TEXT("ps")) >= Max;
	const bool bFullPC = Count(EJRUnitType::PC) + InPipeline(TEXT("pc")) >= Max;
	auto Once = [this](FName Id, bool bOwned) { return bOwned ? TEXT("Sudah dibeli") : InPipeline(Id) > 0 ? TEXT("Sudah dipesan") : TEXT(""); };
	auto Done = [](bool bOwned) { return bOwned ? TEXT("Sudah dibeli") : TEXT(""); };

	if (Era == 1)
	{
		Out.Add({ TEXT("move"), TEXT("Pindah ke Ruko Kecil"),
			TEXT("Muat 6 TV dan 6 PC, daya 2.200 VA, router internet. Sewa Rp 75.000/hari + internet Rp 25.000/hari."), 1000000,
			!Gate1Open() ? TEXT("Syarat: kas Rp 1.500.000 dan reputasi 40") : Phase == EJRGamePhase::Playing ? TEXT("Pindahan saat toko tutup") : TEXT("") });
	}
	Out.Add({ TEXT("ps"), bPS2 ? TEXT("TV 21\" + PlayStasiun 2") : TEXT("TV tabung + PlayStasiun 1"), TEXT("Tambah 1 unit rental. Diantar kurir besok pagi."), bPS2 ? 180000 : 120000, bFullPS ? TEXT("Tempat penuh") : TEXT("") });
	Out.Add({ TEXT("pc"), TEXT("PC warnet"), TEXT("Tambah 1 bilik PC. Diantar kurir besok pagi."), 250000, bFullPC ? TEXT("Tempat penuh") : TEXT("") });
	Out.Add({ TEXT("stik"), TEXT("Stik original"), TEXT("Stik jauh lebih jarang rusak."), 50000, Once(TEXT("stik"), bStikOri) });
	if (Era == 1)
	{
		Out.Add({ TEXT("daya"), TEXT("Tambah daya ke 2.200 VA"), TEXT("MCB tidak gampang turun."), 200000, Once(TEXT("daya"), CapVA >= 2200) });
	}
	else
	{
		Out.Add({ TEXT("daya2"), TEXT("Tambah daya ke 3.500 VA"), TEXT("Cukup untuk ruko penuh + kompor."), 350000, Once(TEXT("daya2"), CapVA >= 3500) });
		Out.Add({ TEXT("ps2"), TEXT("PlayStasiun 2 untuk semua TV"), TEXT("Harga wajar rental PS naik ke Rp 3.000/jam. 180 VA per unit."), 90000 * FMath::Max(1, Count(EJRUnitType::PS)), Done(bPS2) });
		Out.Add({ TEXT("av"), TEXT("Antivirus berlisensi"), TEXT("PC yang hang jarang kena virus."), 80000, Done(bAntivirus) });
		Out.Add({ TEXT("net"), TEXT("Paket internet 20 Mbps"), TEXT("Internet lemot jauh lebih jarang."), 250000, Done(bFastNet) });
		if (!HypeName.IsEmpty())
		{
			Out.Add({ TEXT("hype"), FString::Printf(TEXT("Install %s di semua PC"), *HypeName), TEXT("Gamer online datang lebih banyak selama game ini hype."), 60000, Done(bHypeInstalled) });
		}
	}
	return Out;
}

FString AJRGameMode::ItemName(FName Id) const
{
	for (const FJRShopItem& It : GetShopItems()) if (It.Id == Id) return It.Name;
	return Id.ToString();
}

int32 AJRGameMode::InPipeline(FName Id) const
{
	int32 N = 0;
	for (const FName& P : PendingOrders) if (P == Id) ++N;
	for (const AJRDeliveryBox* B : Boxes) if (B && B->ItemId == Id) ++N;
	if (CarryBox == Id) ++N;
	return N;
}

bool AJRGameMode::BuyItem(FName Id)
{
	if (Phase == EJRGamePhase::Intro) return false;
	for (const FJRShopItem& It : GetShopItems())
	{
		if (It.Id != Id) continue;
		if (!It.Lock.IsEmpty() || Money < It.Cost) return false;
		Money -= It.Cost;
		// Layanan langsung berlaku; barang fisik diantar kurir besok pagi.
		if (Id == TEXT("move")) { MoveToEra2(); return true; }
		if (Id == TEXT("ps2")) { bPS2 = true; PricePS = FMath::Max(PricePS, 3000); }
		else if (Id == TEXT("av")) bAntivirus = true;
		else if (Id == TEXT("net")) bFastNet = true;
		else if (Id == TEXT("hype")) bHypeInstalled = true;
		else
		{
			PendingOrders.Add(Id);
			AddToast(FString::Printf(TEXT("Dipesan: %s (%s). Kurir datang besok pagi."), *It.Name, *JRFormatRp(It.Cost)), Good);
			return true;
		}
		AddToast(FString::Printf(TEXT("Dibeli: %s (%s)"), *It.Name, *JRFormatRp(It.Cost)), Good);
		return true;
	}
	return false;
}

void AJRGameMode::SpawnDelivery(FName Id)
{
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FJRLayout& L = Layout();
	const int32 K = Boxes.Num();
	const FVector Loc(-(L.HalfX - 70.f) + (K / 5) * 80.f, L.DoorY + 140.f + (K % 5) * 85.f, 0.f); // berjajar di kanan pintu
	AJRDeliveryBox* Box = GetWorld()->SpawnActor<AJRDeliveryBox>(Loc, FRotator(0.f, 0.f, 0.f), SP);
	Box->Setup(Id, ItemName(Id));
	Boxes.Add(Box);
}

void AJRGameMode::PickBox(AJRDeliveryBox* Box)
{
	if (!Box || IsCarrying()) return;
	const FName Id = Box->ItemId;
	Boxes.Remove(Box);
	JRSound::PlayAt(this, TEXT("SFX_Kardus"), Box->GetActorLocation());
	Box->Destroy();
	if (Id == TEXT("stik"))
	{
		bStikOri = true;
		AddToast(TEXT("Stik original dibagikan ke semua TV"), Good);
	}
	else if (Id == TEXT("daya") || Id == TEXT("daya2"))
	{
		CapVA = Id == TEXT("daya") ? 2200 : 3500;
		AddToast(FString::Printf(TEXT("Petugas listrik datang. Daya naik ke %d VA"), CapVA), Good);
	}
	else
	{
		CarryBox = Id;
		AddToast(FString::Printf(TEXT("Bawa %s ke tempat bertanda hijau"), *ItemName(Id)));
	}
}

bool AJRGameMode::GetInstallSlot(FVector& OutLoc, FRotator& OutRot) const
{
	if (CarryBox.IsNone()) return false;
	const EJRUnitType Type = CarryBox == TEXT("pc") ? EJRUnitType::PC : EJRUnitType::PS;
	const int32 I = Count(Type);
	if (I >= Layout().MaxUnits()) return false;
	OutLoc = SlotLocation(Type, I, OutRot);
	return true;
}

void AJRGameMode::InstallBox()
{
	if (CarryBox.IsNone()) return;
	const EJRUnitType Type = CarryBox == TEXT("pc") ? EJRUnitType::PC : EJRUnitType::PS;
	if (Count(Type) >= Layout().MaxUnits()) return;
	SpawnStation(Type);
	JRSound::PlayAt(this, TEXT("SFX_Pasang"), Stations.Last()->GetActorLocation() + FVector(0, 0, 80));
	CarryBox = NAME_None;
	AddToast(FString::Printf(TEXT("%s terpasang dan siap dipakai"), *Stations.Last()->DisplayName), Good);
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
		StartDay();
	}
}

void AJRGameMode::StartDay()
{
	Day++;
	Hour = 8.f;
	Stats = FJRDayStats();
	bQuiet = false;
	RtAt = (Era == 2 && FMath::FRand() < 0.5f) ? FMath::FRandRange(22.f, 23.5f) : -1.f;
	Phase = EJRGamePhase::Playing;

	// Kurir datang
	if (PendingOrders.Num() > 0)
	{
		for (const FName& Id : PendingOrders) SpawnDelivery(Id);
		AddToast(FString::Printf(TEXT("Kurir datang! %d kardus di depan pintu."), PendingOrders.Num()), Good);
		PendingOrders.Reset();
	}
	if (bHypeInstalled && !HypeName.IsEmpty()) AddToast(FString::Printf(TEXT("%s terpasang. Gamer online mulai antre!"), *HypeName), Good);

	// Yang ngutang datang membayar (atau menghilang)
	TArray<float> Keep;
	float Repaid = 0.f, Lost = 0.f;
	int32 N = 0;
	for (float D : Debts)
	{
		if (FMath::FRand() < 0.6f) { Repaid += FMath::RoundToFloat(D * 1.2f); ++N; }
		else if (FMath::FRand() < 0.6f) Keep.Add(D);
		else Lost += D;
	}
	Debts = Keep;
	Money += Repaid;
	if (Repaid > 0.f) AddToast(FString::Printf(TEXT("%d orang datang bayar utang plus bonus: +%s"), N, *JRFormatRp(Repaid)), Good);
	if (Lost > 0.f) AddToast(FString::Printf(TEXT("Utang %s sepertinya tidak akan kembali"), *JRFormatRp(Lost)), Bad);
}

void AJRGameMode::AddToast(const FString& Text, const FLinearColor& Color)
{
	Toasts.Insert({ Text, Color, 5.f }, 0);
	if (Toasts.Num() > 5) Toasts.SetNum(5);
}
