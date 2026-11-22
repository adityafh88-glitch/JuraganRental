#include "JRGameMode.h"
#include "JRLang.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
#include "JRBlockout.h"
#include "JRCharacter.h"
#include "JRCustomer.h"
#include "JRDelivery.h"
#include "JRFixtures.h"
#include "JRHUD.h"
#include "JRSound.h"
#include "JRRadio.h"
#include "JRService.h"
#include "JROpening.h"
#include "JRStation.h"
#include "JRVisitor.h"
#include "JRSaveGame.h"
#include "JRSettings.h"
#include "JRMenuWidget.h"
#include "JRPhoneWidget.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/CoreDelegates.h"
#include "RHI.h"
#include "RHIStats.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"

namespace JRLocal_GameMode
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
		L.Name = TEXT("GARASI");	// JRNOWRAP diterjemahkan saat ditampilkan (tata letak disimpan statis)
		L.HalfX = 700.f; L.HalfY = 1000.f; L.DoorY = 420.f;
		L.PsY = { -800.f, -550.f, -300.f, -50.f };
		L.PcX = { 500.f, 250.f, 0.f, -250.f };
		L.Queue = { FVector(-650, -40, 0), FVector(-650, 30, 0), FVector(-650, 100, 0), FVector(-575, 200, 0), FVector(-575, 275, 0) };
		return L;
	}

	FJRLayout MakeRuko()
	{
		FJRLayout L;
		L.Name = TEXT("RUKO KECIL");	// JRNOWRAP diterjemahkan saat ditampilkan (tata letak disimpan statis)
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

	FJRLayout MakeGameCenter()
	{
		FJRLayout L;
		L.Name = TEXT("GAME CENTER");	// JRNOWRAP diterjemahkan saat ditampilkan (tata letak disimpan statis)
		L.HalfX = 900.f; L.HalfY = 1300.f; L.DoorY = 650.f;
		L.PsY = { -1000.f, -750.f, -500.f, -250.f, 0.f, 250.f, 500.f, 750.f };
		L.PcX = { 700.f, 500.f, 300.f, 100.f, -100.f, -300.f, -500.f, -700.f };
		L.Queue = { FVector(-850, 0, 0), FVector(-850, 70, 0), FVector(-850, 140, 0),
			FVector(-775, 240, 0), FVector(-775, 320, 0), FVector(-700, 240, 0), FVector(-700, 320, 0), FVector(-625, 240, 0) };
		L.Rent = 150000.f;
		L.Internet = 25000.f;
		L.bRouter = true;
		L.FloorH = 350.f;
		L.VipY = { -700.f, -380.f, -60.f, 260.f, 580.f, 900.f };
		L.StairX0 = 60.f; L.StairX1 = 810.f;
		L.McbPos = FVector(-450.f, -(L.HalfY - 7.f), 130.f);
		return L;
	}
}
using namespace JRLocal_GameMode;

const FJRLayout& AJRGameMode::Layout() const
{
	static const FJRLayout Garasi = MakeGarasi();
	static const FJRLayout Ruko = MakeRuko();
	static const FJRLayout Center = MakeGameCenter();
	return Era >= 3 ? Center : Era == 2 ? Ruko : Garasi;
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
	if (FParse::Param(FCommandLine::Get(), TEXT("jrera3")))
	{
		// Mulai langsung di Game Center 2 lantai (uji / playtest).
		Era = 3; Money = 1500000.f; Rep = 62.f; CapVA = 4400; OwnedPS = 6; OwnedPC = 5; OwnedVIP = 3; bPS2 = true; PricePS = 3000;
		Xp = XpForLevel(7);	// kira-kira level saat tiba di Game Center
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("jrera2")))
	{
		// Mulai langsung di ruko (uji / playtest).
		Era = 2; Money = 1000000.f; Rep = 45.f; CapVA = 2900; OwnedPS = 4; OwnedPC = 3;
		Xp = XpForLevel(4);	// kira-kira level saat tiba di ruko
	}
	else if (bTest)
	{
		OwnedPS = 3; OwnedPC = 2; CapVA = 2200; // supaya kompor bisa jalan bersama 5 unit
	}
	FParse::Value(FCommandLine::Get(), TEXT("jrpromo="), PromoId);
	if (PromoId > 0) PromoSetup();
	BuildWorld();
	NewDailyMissions();
	NewWeeklyMission();
	if (GConfig)
	{
		GConfig->GetBool(TEXT("JuraganRental"), TEXT("bDemo"), bDemo, GGameIni);
		GConfig->GetString(TEXT("JuraganRental"), TEXT("DemoStoreURL"), DemoStoreURL, GGameIni);
		GConfig->GetBool(TEXT("JuraganRental"), TEXT("bDevMenu"), bDevMenu, GGameIni);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("jrdemo"))) bDemo = true;
	AppBgHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &AJRGameMode::OnAppBackground);
	AppQuitHandle = FCoreDelegates::GetApplicationWillTerminateDelegate().AddUObject(this, &AJRGameMode::OnAppBackground);
	bMenuTest = FParse::Param(FCommandLine::Get(), TEXT("jrmenutest"));
	bBalance = FParse::Param(FCommandLine::Get(), TEXT("jrbalance"));
	bMysteryTest = FParse::Param(FCommandLine::Get(), TEXT("jrmysterytest"));
	if (bMysteryTest) { Phase = EJRGamePhase::Playing; Hour = 19.f; } // 8 bab muat sebelum tengah malam
	if (bBalance)
	{
		Phase = EJRGamePhase::Playing;
		UGameplayStatics::SetGlobalTimeDilation(this, 6.f);
	}
	if (bMenuTest) SetTickableWhenPaused(true); // uji menu jeda tetap berjalan saat game dijeda
	if (FParse::Param(FCommandLine::Get(), TEXT("jropening"))) BeginNewGame();	// uji: langsung putar adegan pembuka
	if (PromoId > 0) Phase = EJRGamePhase::Playing;
	else if (!bTest && !bBalance && !bMysteryTest && !FParse::Param(FCommandLine::Get(), TEXT("jrera2")) && !FParse::Param(FCommandLine::Get(), TEXT("jrera3")) && !FParse::Param(FCommandLine::Get(), TEXT("jropening"))) Phase = EJRGamePhase::Menu;

	if (bTest)
	{
		bAutoTest = true;
		Phase = EJRGamePhase::Playing;
		Hour = 13.5f;
		SpawnDelivery(TEXT("ps")); // uji kurir: kardus TV di depan pintu
		if (Era >= 2) { HypeName = HypeGames[0]; HypeDays = 3; bHypeInstalled = true; }
		if (Era >= 3) PriceVIP = 4000; // uji: lebih banyak pelanggan VIP naik ke lantai 2
		if (FParse::Param(FCommandLine::Get(), TEXT("jrbilling"))) { bBilling = true; SpawnBillingPC(); }
		if (FParse::Param(FCommandLine::Get(), TEXT("jrtour"))) { bTourAccepted = true; Hour = 15.95f; }
		bStaffTest = Era >= 2 && FParse::Param(FCommandLine::Get(), TEXT("jrstaff"));
		if (bStaffTest) { for (bool& B : Staff) B = true; SpawnStaffActors(); }
		if (FParse::Param(FCommandLine::Get(), TEXT("jrtuttest"))) TutStep = TutStart;
		if (FParse::Param(FCommandLine::Get(), TEXT("jrdaily"))) { Day = 2; RollMorning(); }
		if (FParse::Param(FCommandLine::Get(), TEXT("jrbadges")))
		{
			// Uji lencana/ending (tidak tersimpan ke disk): ending Raja Turnamen + Gulung Tikar saat tutup
			JRSettings::Get()->LifeTours = 4; JRSettings::Get()->LifeMie = 49; NegDays = 2; Money = -1000000.f; Day = 7;
		}	// uji kabar pagi + kartu rencana (dialog dipotret, kartu pertama dipilih otomatis)
		bPaket1Test = Era >= 3 && FParse::Param(FCommandLine::Get(), TEXT("jrpaket1"));
		if (bPaket1Test)
		{
			// Level 1: barang Era 3 harus terkunci level.
			Xp = 0;
			FString LockVip, LockKulkas;
			for (const FJRShopItem& It : GetShopItems()) { if (It.Id == TEXT("vip")) LockVip = It.Lock; if (It.Id == TEXT("kulkas")) LockKulkas = It.Lock; }
			UE_LOG(LogTemp, Display, TEXT("JR_PAKET1_LOCK lv=%d vip=[%s] kulkas=[%s]"), JuraganLevel(), *LockVip, *LockKulkas);
			// Naik ke level 13 lewat AddXp (uji toast naik level), lalu beli semua bisnis sampingan.
			AddXp(XpForLevel(13));
			Money += 6000000.f;
			const bool b1 = BuyItem(TEXT("kulkas")), b2 = BuyItem(TEXT("konter")), b3 = BuyItem(TEXT("printer")), b4 = BuyItem(TEXT("saldo"));
			const bool b5 = BuyItem(TEXT("krat"));
			// Upgrade beberapa unit: TV 1 sampai Lv 5, PC 1 sampai Lv 4, VIP 1 sampai Lv 2.
			int32 Ups = 0;
			for (AJRStation* S : Stations)
			{
				const int32 Target = S->Index != 0 ? 0 : S->Type == EJRUnitType::PS ? 5 : S->Type == EJRUnitType::PC ? 4 : 2;
				while (S->UpLevel < Target && UpgradeUnit(S)) ++Ups;
			}
			UE_LOG(LogTemp, Display, TEXT("JR_PAKET1_BUY lv=%d xp=%d kulkas=%d konter=%d printer=%d saldo=%d krat=%d ups=%d services=%d drinks=%d paper=%d saldo=%.0f maint=%d"),
				JuraganLevel(), Xp, b1, b2, b3, b4, b5, Ups, Services.Num(), Drinks, Paper, Saldo, Maintenance());
		}
		if (Era >= 2 && FParse::Param(FCommandLine::Get(), TEXT("jrnet"))) ChooseNetPlan(2);
		if (FParse::Param(FCommandLine::Get(), TEXT("jrloak")))
		{
			// Uji loak: beli TV bekas lalu langsung antar kardusnya
			LoakOffer = TEXT("pcbekas");
			Money += 200000.f; // modal uji
			const bool bBought = BuyItem(TEXT("pcbekas"));
			PendingOrders.Remove(TEXT("pcbekas"));
			if (bBought) SpawnDelivery(TEXT("pcbekas"));
			UE_LOG(LogTemp, Display, TEXT("JR_LOAK bought=%d offer=%s boxes=%d"), bBought ? 1 : 0, *LoakOffer.ToString(), Boxes.Num());
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("jrdecor")))
		{
			Decor = { TEXT("poster"), TEXT("tanaman"), TEXT("kipas"), TEXT("neon") };
			if (Era >= 2) Decor.Add(TEXT("ac"));
			SpawnDecor();
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("jrtiers")))
		{
			SeatTier = Era >= 2 ? 2 : 1;
			bFastPC = Era >= 2;
			for (AJRStation* S : Stations) S->ApplySeatTier(SeatTier);
		}
	}
}

void AJRGameMode::BuildWorld()
{
	UWorld* World = GetWorld();
	const FJRLayout& L = Layout();
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Room = World->SpawnActor<AJRRoom>(FVector::ZeroVector, FRotator::ZeroRotator, SP);
	Room->Build(L, Era);
	Kasir = World->SpawnActor<AJRKasir>(L.Kasir(), FRotator::ZeroRotator, SP);
	Radio = World->SpawnActor<AJRRadio>(L.Kasir() + FVector(0, -6, 100), FRotator::ZeroRotator, SP); // di atas meja, antara laci dan buku utang
	Mcb = World->SpawnActor<AJRMcb>(L.Mcb(), FRotator(0.f, 90.f, 0.f), SP);
	Stove = World->SpawnActor<AJRStove>(L.Stove(), FRotator(0.f, 90.f, 0.f), SP);
	Router = L.bRouter ? World->SpawnActor<AJRRouter>(L.Router(), FRotator(0.f, 180.f, 0.f), SP) : nullptr;
	if (!SlotMarker) SlotMarker = World->SpawnActor<AJRSlotMarker>(FVector::ZeroVector, FRotator::ZeroRotator, SP);

	QueueUsed.Init(false, L.Queue.Num());
	for (int32 i = 0; i < OwnedPS; ++i) SpawnStation(EJRUnitType::PS);
	for (int32 i = 0; i < OwnedPC; ++i) SpawnStation(EJRUnitType::PC);
	if (L.HasUpstairs()) for (int32 i = 0; i < OwnedVIP; ++i) SpawnStation(EJRUnitType::VIP);
	SpawnStaffActors();
	SpawnDecor();
	SpawnBillingPC();
	SpawnServices();
}

void AJRGameMode::SpawnBillingPC()
{
	if (BillingPC) { BillingPC->Destroy(); BillingPC = nullptr; }
	if (!bBilling) return;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Di samping kanan meja kasir (sisi +Y), menghadap pemain
	BillingPC = GetWorld()->SpawnActor<AJRBillingPC>(Layout().Kasir() + FVector(5.f, 135.f, 0.f), FRotator::ZeroRotator, SP);
}

void AJRGameMode::SpawnDecor()
{
	for (AJRDecor* D : DecorActors) if (D) D->Destroy();
	DecorActors.Reset();
	const FJRLayout& L = Layout();
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TArray<FName> All = Decor;
	All.Insert(TEXT("luar"), 0);	// pemandangan jalan di luar pintu selalu ada
	for (const FName& Id : All)
	{
		if (AJRDecor* D = GetWorld()->SpawnActor<AJRDecor>(FVector::ZeroVector, FRotator::ZeroRotator, SP))
		{
			D->Setup(Id, L.HalfX, L.HalfY, L.DoorY);
			DecorActors.Add(D);
		}
	}
}

int32 AJRGameMode::DecorBonus() const
{
	int32 B = 0;
	for (const FName& Id : Decor)
	{
		B += Id == TEXT("poster") ? 2 : Id == TEXT("kipas") ? 3 : Id == TEXT("tanaman") ? 2 : Id == TEXT("neon") ? 3 : Id == TEXT("ac") ? 8 : 0;
	}
	return B;
}

int32 AJRGameMode::DecorVA() const
{
	return (Decor.Contains(TEXT("kipas")) ? 100 : 0) + (Decor.Contains(TEXT("neon")) ? 30 : 0) + (Decor.Contains(TEXT("ac")) ? 600 : 0);
}

void AJRGameMode::MoveToEra2()
{
	MoveToEra(2);
}

void AJRGameMode::MoveToEra(int32 NewEra)
{
	// Simpan jumlah unit dan isi kardus, lalu bongkar tempat lama.
	OwnedPS = Count(EJRUnitType::PS);
	OwnedPC = Count(EJRUnitType::PC);
	OwnedVIP = Count(EJRUnitType::VIP);
	TArray<FName> BoxIds;
	for (AJRDeliveryBox* B : Boxes) if (B) BoxIds.Add(B->ItemId);
	ClearWorld();

	Era = NewEra;
	CapVA = FMath::Max(CapVA, Era >= 3 ? 4400 : 2900); // ruko / game center: listrik bisnis
	BuildWorld();
	for (const FName& Id : BoxIds) SpawnDelivery(Id);

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn()) Pawn->SetActorLocation(Layout().PlayerStart(), false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(FRotator::ZeroRotator);
	}
	AddToast(Era >= 3 ? JRT(TEXT("Selamat datang di Game Center 2 lantai! Beli booth VIP untuk lantai 2 lewat HP > Toko."))
		: JRT(TEXT("Selamat datang di ruko baru! Sewa dan internet ditagih tiap tutup toko.")), Good);
}

/* ---------- Menu developer ---------- */

void AJRGameMode::DevJumpEra(int32 NewEra)
{
	if (!bDevMenu || NewEra <= Era || NewEra > 3) return;
	// Bekal kira-kira seperti pemain yang baru tiba di era itu (sama dengan -jrera2 / -jrera3).
	const bool b3 = NewEra >= 3;
	Money = FMath::Max(Money, b3 ? 1500000.f : 1000000.f);
	Rep = FMath::Max(Rep, b3 ? 62.f : 45.f);
	Xp = FMath::Max(Xp, XpForLevel(b3 ? 7 : 4));
	if (b3) bPS2 = true;
	MoveToEra(NewEra);
	const int32 WantPS = b3 ? 6 : 4, WantPC = b3 ? 5 : 3, WantVIP = b3 ? 3 : 0;
	while (Count(EJRUnitType::PS) < WantPS && Count(EJRUnitType::PS) < MaxFor(EJRUnitType::PS)) SpawnStation(EJRUnitType::PS);
	while (Count(EJRUnitType::PC) < WantPC && Count(EJRUnitType::PC) < MaxFor(EJRUnitType::PC)) SpawnStation(EJRUnitType::PC);
	while (Count(EJRUnitType::VIP) < WantVIP && Count(EJRUnitType::VIP) < MaxFor(EJRUnitType::VIP)) SpawnStation(EJRUnitType::VIP);
	OwnedPS = Count(EJRUnitType::PS); OwnedPC = Count(EJRUnitType::PC); OwnedVIP = Count(EJRUnitType::VIP);
	UE_LOG(LogTemp, Display, TEXT("JR_DEV era=%d units=%d money=%.0f lv=%d"), Era, TotalUnits(), Money, JuraganLevel());
}

void AJRGameMode::DevAddMoney(float Amount)
{
	if (!bDevMenu) return;
	Money += Amount;
	AddToast(JRPrintf(JRT(TEXT("[DEV] Kas +%s")), *JRFormatRp(Amount)), Good);
}

void AJRGameMode::DevLevelUp()
{
	if (!bDevMenu) return;
	AddXp(FMath::Max(1, XpForLevel(JuraganLevel() + 1) - Xp));
}

void AJRGameMode::DevAddRep(float Amount)
{
	if (!bDevMenu) return;
	Rep = FMath::Min(100.f, Rep + Amount);
	AddToast(JRPrintf(JRT(TEXT("[DEV] Reputasi %.0f")), Rep), Good);
}

bool AJRGameMode::Gate2Open() const
{
	return Era == 2 && Money >= 3000000.f && Rep >= 60.f && TotalUnits() >= 10;
}

// MakePath dan PathLen ada di JRNav.cpp

void AJRGameMode::ClearWorld()
{
	for (AJRCustomer* C : Customers) if (C) C->Destroy();
	Customers.Reset();
	for (AJRDeliveryBox* B : Boxes) if (B) B->Destroy();
	Boxes.Reset();
	for (AJRStation* S : Stations) if (S) S->Destroy();
	Stations.Reset();
	for (AActor* A : TArray<AActor*>{ Room, Kasir, Radio, Mcb, Stove, Router, BillingPC }) if (A) A->Destroy();
	BillingPC = nullptr;
	DestroyStaff();
	for (AJRTrash* T : Trash) if (T) T->Destroy();
	Trash.Reset();
	for (AJRDecor* D : DecorActors) if (D) D->Destroy();
	DecorActors.Reset();
	for (AJRService* S : Services) if (S) S->Destroy();
	Services.Reset();
	ClearWalkIns();
}

/* ---------- Provider internet ---------- */

const TCHAR* AJRGameMode::NetName(int32 Plan)
{
	switch (Plan) { case 1: return JRT(TEXT("KilatNet 20 Mbps")); case 2: return JRT(TEXT("FiberNusa 50 Mbps")); default: return JRT(TEXT("NetRakyat 5 Mbps")); }
}

int32 AJRGameMode::NetCost(int32 Plan)
{
	switch (Plan) { case 1: return 45000; case 2: return 80000; default: return 25000; }
}

const TCHAR* AJRGameMode::NetDesc(int32 Plan)
{
	switch (Plan)
	{
	case 1: return JRT(TEXT("Jarang lemot. Pemain PC sedikit lebih puas."));
	case 2: return JRT(TEXT("Hampir tidak pernah lemot. Pemain PC jauh lebih puas, gamer lebih sering datang."));
	default: return JRT(TEXT("Murah, tapi sering lemot saat ramai."));
	}
}

void AJRGameMode::ChooseNetPlan(int32 NewPlan)
{
	if (Era < 2) return;
	PendingNetPlan = NewPlan == NetPlan ? -1 : NewPlan;
	AddToast(PendingNetPlan < 0 ? JRPrintf(JRT(TEXT("Tetap pakai %s")), NetName(NetPlan))
		: JRPrintf(JRT(TEXT("Pindah ke %s mulai besok (%s/hari)")), NetName(NewPlan), *JRFormatRp(NetCost(NewPlan))));
}

/* ---------- Turnamen akhir pekan ---------- */

void AJRGameMode::OfferTournament()
{
	PushChoice(JRT(TEXT("Turnamen akhir pekan")),
		JRPrintf(JRT(TEXT("Anak-anak tongkrongan bertanya: \"Bang, Sabtu ini ada turnamen bola PS nggak?\" Turnamen berjalan pukul 16.00-20.00. Peserta bayar %s, juara dapat %s.")), *JRFormatRp(TourFee), *JRFormatRp(TourPrize)),
		{ JRT(TEXT("Adakan turnamen")), JRT(TEXT("Tidak minggu ini")) },
		{
			[this]() { bTourAccepted = true; AddToast(JRT(TEXT("Turnamen diumumkan! Mulai pukul 16.00, pendaftaran di setiap TV.")), Good); },
			[this]() { AddToast(JRT(TEXT("Tidak ada turnamen minggu ini."))); }
		});
}

void AJRGameMode::TickTournament()
{
	if (!bTourAccepted || bTourDone) return;
	if (!bTourRunning && Hour >= 16.f && Hour < 20.f)
	{
		bTourRunning = true;
		JRSound::Play2D(this, TEXT("SFX_TV_Gol"), 0.8f);
		AddToast(JRT(TEXT("TURNAMEN DIMULAI! Peserta yang menyalakan TV otomatis terdaftar.")), Good);
	}
	if (bTourRunning && Hour >= 20.f && Choices.Num() == 0)
	{
		bTourRunning = false;
		bTourDone = true;
		TournamentFinal();
	}
}

void AJRGameMode::TournamentFinal()
{
	AddMoment(EJRMoment::Tour);
	CountLife(EJRLife::Tour);
	JRSound::Play2D(this, TEXT("SFX_TV_Gol"), 1.f);
	TArray<FString> Labels = { JRPrintf(JRT(TEXT("Serahkan hadiah %s")), *JRFormatRp(TourPrize)) };
	TArray<TFunction<void()>> Actions = { [this]()
	{
		Money -= TourPrize;
		Stats.RepDelta += 4.f;
		Regulars = FMath::Min(10, Regulars + 1);
		AddToast(JRT(TEXT("Juara mengangkat piala. Penonton bersorak! Reputasi +4")), Good);
		AddXp(40);
	} };
	if (Era >= 3 || Rep >= 60.f)
	{
		Labels.Add(JRT(TEXT("Hadiah dari sponsor (gratis)")));
		Actions.Add([this]() { Stats.RepDelta += 2.f; AddToast(JRT(TEXT("Toko HP sebelah jadi sponsor hadiah. Reputasi +2")), Good); });
	}
	Labels.Add(JRT(TEXT("Batalkan hadiah")));
	Actions.Add([this]() { Stats.RepDelta -= 6.f; AddToast(JRT(TEXT("Peserta kecewa hadiahnya batal. Reputasi -6")), FLinearColor(0.94f, 0.42f, 0.36f)); });
	PushChoice(JRT(TEXT("Final turnamen!")),
		JRPrintf(JRT(TEXT("%d peserta sudah bertanding. Final berakhir dramatis lewat adu penalti, penonton berdesakan di depan TV.")), TourPlayers),
		MoveTemp(Labels), MoveTemp(Actions));
}

/* ---------- Berantem dan tamu spesial ---------- */

void AJRGameMode::FightEvent(AJRCustomer* A, AJRCustomer* B)
{
	bFightToday = true;
	AddMoment(EJRMoment::Fight);
	A->bRaging = true;
	B->bRaging = true;
	A->Mood = TEXT("!!");
	B->Mood = TEXT("!!");
	JRSound::PlayAt(this, TEXT("SFX_Stik_Banting"), A->GetActorLocation());
	JRSound::Voice(this, VoiceCode[A->TypeIndex], TEXT("Rage"), A->GetActorLocation(), true);
	TWeakObjectPtr<AJRCustomer> WA(A), WB(B);
	const FString Where = A->Station && B->Station ? JRPrintf(JRT(TEXT("%s dan %s")), *A->Station->DisplayName, *B->Station->DisplayName) : FString(TEXT("TV"));
	auto Calm = [WA, WB]() { for (AJRCustomer* X : { WA.Get(), WB.Get() }) if (X) { X->bRaging = false; X->Mood.Reset(); } };
	PushChoice(JRT(TEXT("Berantem!")),
		JRPrintf(JRT(TEXT("%s dan %s di %s saling dorong. \"Curang lu! Pakai jurus itu terus!\" Pelanggan lain mulai menonton.")), *A->DisplayName, *B->DisplayName, *Where),
		{ JRT(TEXT("Lerai dan tegur")), JRT(TEXT("Usir keduanya (bayar setengah)")), JRT(TEXT("Biarkan saja")) },
		{
			[this, WA, WB, Calm]()
			{
				Calm();
				for (AJRCustomer* X : { WA.Get(), WB.Get() }) if (X) X->SatAdj -= 5.f;
				AddToast(JRT(TEXT("Mereka salaman lalu lanjut main. Suasana kembali tenang.")), Good);
			},
			[this, WA, WB, Calm]()
			{
				Calm();
				for (AJRCustomer* X : { WA.Get(), WB.Get() })
				{
					if (!X || X->State != EJRCustomerState::Playing) continue;
					const float Paid = FMath::RoundToFloat(X->Bill * 0.5f);
					Money += Paid;
					Stats.Income += Paid;
					Stats.Satisfaction.Add(30);
					Leave(X);
				}
				Stats.RepDelta += 1.f;
				AddToast(JRT(TEXT("Keduanya diusir. Warga senang rental ini tertib. Reputasi +1")), Good);
			},
			[this, WA, Calm]()
			{
				Calm();
				if (AJRCustomer* X = WA.Get()) if (X->Station) X->Station->bBroken = true;
				for (AJRCustomer* X : Customers) if (X && X->State == EJRCustomerState::Playing) X->SatAdj -= 5.f;
				Stats.RepDelta -= 2.f;
				AddToast(JRT(TEXT("Stik terlempar dan rusak. Pelanggan lain terganggu. Reputasi -2")), Bad);
			}
		});
}

void AJRGameMode::TickGuests()
{
	if (GuestAt < 0.f || Hour < GuestAt || Choices.Num() > 0) return;
	const int32 Which = GuestWhich;
	GuestAt = -1.f;
	GuestWhich = -1;
	GuestEvent(Which);
}

void AJRGameMode::GuestEvent(int32 Which)
{
	const FJRLayout& L = Layout();
	const FVector Door = L.DoorOutside();
	const FVector Inside(-(L.HalfX - 100.f), L.DoorY, 0.f);
	GuestsDone |= 1 << Which;
	AddMoment(Which == 0 ? EJRMoment::Streamer : Which == 1 ? EJRMoment::Lurah : EJRMoment::Rival);
	JRSound::Play2D(this, TEXT("SFX_Bel_Pintu"), 0.8f);
	static const TCHAR* Mesh[] = { TEXT("SK_Gamer"), TEXT("SK_Bapak"), TEXT("SK_Kantoran") };
	TWeakObjectPtr<AJRVisitor> WV(SpawnMysteryVisitor(Mesh[Which], Inside));
	auto Bye = [WV, Door]() { if (WV.IsValid()) WV->LeaveTo(Door); };

	if (Which == 0)
	{
		PushChoice(JRT(TEXT("Tamu spesial: Bang Jago")),
			JRT(TEXT("Streamer game \"Bang Jago\" masuk sambil membawa ponsel di tongkat. \"Halo guys, kita live dari rental yang lagi rame dibicarain nih! Bang, boleh numpang main sejam?\"")),
			{ JRT(TEXT("Silakan, gratis untuk Bang Jago")), JRT(TEXT("Boleh, tarif normal ya")) },
			{
				[this, Bye]() { Bye(); ViralDays = 2; Stats.RepDelta += 3.f; AddToast(TEXT("Live-nya ditonton ribuan orang. Rentalmu VIRAL: pengunjung melonjak 2 hari! Reputasi +3"), Good); },
				[this, Bye]() { Bye(); Money += 10000.f; Stats.Income += 10000.f; Stats.RepDelta += 1.f; AddToast(TEXT("Bang Jago bayar Rp 10.000 dan menyebut rentalmu di live. Reputasi +1"), Good); }
			});
	}
	else if (Which == 1)
	{
		const bool bNice = DecorBonus() >= 4;
		PushChoice(JRT(TEXT("Pak Lurah sidak")),
			bNice ? JRT(TEXT("Pak Lurah keliling melihat-lihat. \"Wah, rapi dan nyaman. Ada tanaman, ada hiasan. Begini ini contoh usaha anak muda kampung kita!\""))
				: JRT(TEXT("Pak Lurah keliling melihat-lihat. \"Usahanya bagus, Mas, tapi ruangannya masih polos. Coba dikasih hiasan atau tanaman biar warga betah.\"")),
			{ JRT(TEXT("Terima kasih, Pak Lurah")) },
			{
				[this, Bye, bNice]()
				{
					Bye();
					Stats.RepDelta += bNice ? 4.f : 0.f;
					AddToast(bNice ? JRT(TEXT("Pak Lurah memuji rentalmu di grup warga. Reputasi +4")) : JRT(TEXT("Saran Pak Lurah: beli dekorasi lewat HP > Toko.")),
						bNice ? Good : FLinearColor(0.37f, 0.70f, 1.f));
				}
			});
	}
	else
	{
		const int32 Offer = 140000;
		PushChoice(JRT(TEXT("Juragan rental sebelah")),
			JRPrintf(JRT(TEXT("Juragan rental dari kampung sebelah mampir. \"Rame juga ya tempatmu. Begini, saya lagi butuh unit cepat. Satu TV-mu saya beli %s, tunai sekarang.\"")), *JRFormatRp(Offer)),
			{ JRT(TEXT("Jual satu TV")), JRT(TEXT("Tolak, unitnya masih dipakai")) },
			{
				[this, Bye, Offer]()
				{
					Bye();
					// Jual TV paling ujung (indeks terbesar) agar slot berikutnya tetap urut
					int32 Last = INDEX_NONE;
					for (int32 i = 0; i < Stations.Num(); ++i)
						if (Stations[i] && Stations[i]->Type == EJRUnitType::PS && (Last == INDEX_NONE || Stations[i]->Index > Stations[Last]->Index)) Last = i;
					for (int32 i = Last; i != INDEX_NONE; i = INDEX_NONE)
					{
						AJRStation* S = Stations[i];
						if (S->IsFree() && Count(EJRUnitType::PS) > 1)
						{
							Stations.RemoveAt(i);
							S->Destroy();
							Money += Offer;
							AddToast(JRPrintf(JRT(TEXT("Satu TV terjual %s. Tempatnya bisa diisi unit baru.")), *JRFormatRp(Offer)), Good);
							return;
						}
					}
					AddToast(JRT(TEXT("Semua TV sedang dipakai. Juragan sebelah pulang dengan kecewa.")));
				},
				[this, Bye]() { Bye(); AddToast(TEXT("\"Ya sudah, sukses terus ya!\" katanya sambil pamit.")); }
			});
	}
}

/* ---------- Karyawan ---------- */

int32 AJRGameMode::StaffWage(EJRStaff S)
{
	switch (S)
	{
	case EJRStaff::Kasir: return 40000;
	case EJRStaff::Teknisi: return 50000;
	case EJRStaff::Cleaner: return 30000;
	default: return 35000;
	}
}

const TCHAR* AJRGameMode::StaffName(EJRStaff S)
{
	switch (S)
	{
	case EJRStaff::Kasir: return JRT(TEXT("Mas Kasir"));
	case EJRStaff::Teknisi: return JRT(TEXT("Teknisi"));
	case EJRStaff::Cleaner: return JRT(TEXT("OB (kebersihan)"));
	default: return JRT(TEXT("Koki Kantin"));
	}
}

const TCHAR* AJRGameMode::StaffDesc(EJRStaff S)
{
	switch (S)
	{
	case EJRStaff::Kasir: return JRT(TEXT("Menagih pelanggan yang antre bayar, jadi tidak ada yang kabur."));
	case EJRStaff::Teknisi: return JRT(TEXT("Memperbaiki stik rusak, PC hang, dan virus."));
	case EJRStaff::Cleaner: return JRT(TEXT("Memungut mangkuk, gelas, dan bungkus yang ditinggal pelanggan."));
	default: return JRT(TEXT("Memasak dan mengantar mi pesanan (cek daya dulu)."));
	}
}

int32 AJRGameMode::TotalWages() const
{
	int32 W = 0;
	for (int32 i = 0; i < (int32)EJRStaff::Count; ++i) if (Staff[i]) W += StaffWage((EJRStaff)i);
	return W;
}

void AJRGameMode::ToggleStaff(EJRStaff S)
{
	if (Era < 2) return;
	bool& B = Staff[(int32)S];
	B = !B;
	AddToast(B ? JRPrintf(JRT(TEXT("%s mulai bekerja. Gaji %s/hari")), StaffName(S), *JRFormatRp(StaffWage(S)))
		: JRPrintf(JRT(TEXT("%s diberhentikan")), StaffName(S)), B ? Good : FLinearColor(0.37f, 0.70f, 1.f));
	SpawnStaffActors();
}

void AJRGameMode::DestroyStaff()
{
	for (AJRVisitor* V : { KasirStaff.Get(), TechStaff.Get(), CookStaff.Get(), CleanStaff.Get() }) if (V) V->Destroy();
	KasirStaff = TechStaff = CookStaff = CleanStaff = nullptr;
	TechState = CookState = CleanState = 0;
	CleanJob.Reset();
	TechJob.Reset();
	CookJob.Reset();
}

void AJRGameMode::SpawnStaffActors()
{
	DestroyStaff();
	const FJRLayout& L = Layout();
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto Spawn = [&](EJRStaff Who, const TCHAR* Mesh, const FVector& Loc, float Yaw) -> AJRVisitor*
	{
		if (!HasStaff(Who)) return nullptr;
		AJRVisitor* V = GetWorld()->SpawnActor<AJRVisitor>(Loc, FRotator(0.f, Yaw, 0.f), SP);
		if (V) { V->Setup(Mesh, TEXT("A_Idle")); V->WalkSpeed = 360.f; }	// karyawan berjalan gesit
		return V;
	};
	KasirStaff = Spawn(EJRStaff::Kasir, TEXT("SK_Mahasiswa"), L.Kasir() + FVector(75.f, -30.f, 0.f), 180.f);	// di belakang meja kasir
	TechStaff = Spawn(EJRStaff::Teknisi, TEXT("SK_Gamer"), TechPost(), 180.f);								// pojok belakang, tidak menghalangi unit
	CookStaff = Spawn(EJRStaff::Koki, TEXT("SK_Bapak"), CookPost(), -90.f);									// di depan kompor
	CleanStaff = Spawn(EJRStaff::Cleaner, TEXT("SK_Anak_SMA"), CleanPost(), 0.f);								// dekat pintu, siap memungut sampah
}

FVector AJRGameMode::StaffSpot(const AJRStation* S) const
{
	// Berdiri di samping depan unit (bukan di tempat duduk pelanggan)
	const FVector Fwd = S->GetActorForwardVector(), Right = S->GetActorRightVector();
	FVector P = S->GetActorLocation() + Fwd * (S->Type == EJRUnitType::PC ? 110.f : 85.f) + Right * 80.f;
	P.Z = S->GetActorLocation().Z;
	return P;
}

void AJRGameMode::SendStaff(AJRVisitor* V, const FVector& To, float Yaw)
{
	if (!V) return;
	TArray<FVector> Pts;
	MakePath(V->GetActorLocation(), To, Pts);
	V->WalkPath(Pts, Yaw);
}

void AJRGameMode::TickStaff(float DeltaSeconds)
{
	if (Era < 2 || bBalance) return;
	// Kasir: satu pelanggan tiap ~5 detik
	if (HasStaff(EJRStaff::Kasir) && NextPayer())
	{
		float& T = StaffTimer[(int32)EJRStaff::Kasir];
		T += DeltaSeconds;
		if (T >= 5.f) { T = 0.f; CollectNextPayer(); StaffDone[0]++; }
	}
	// Teknisi: berjalan ke unit rusak (naik tangga bila perlu), memperbaiki, lalu kembali ke pos
	if (TechStaff)
	{
		auto FindBroken = [this]() -> AJRStation*
		{
			AJRStation* Best = nullptr;
			float BestD = TNumericLimits<float>::Max();
			for (AJRStation* S : Stations)
			{
				if (!S || !S->bBroken) continue;
				const float D = PathLen(TechStaff->GetActorLocation(), S->GetActorLocation());
				if (D < BestD) { BestD = D; Best = S; }
			}
			return Best;
		};
		AJRStation* Job = TechJob.Get();
		switch (TechState)
		{
		case 0:
		case 3:
			if (AJRStation* B = FindBroken())
			{
				TechJob = B;
				SendStaff(TechStaff, StaffSpot(B), (B->GetActorLocation() - StaffSpot(B)).Rotation().Yaw);
				TechState = 1;
			}
			else if (TechState == 3 && !TechStaff->IsWalking()) TechState = 0;
			break;
		case 1:
			if (!Job || !Job->bBroken) { TechState = 3; SendStaff(TechStaff, TechPost(), 180.f); }	// sudah dibereskan pemain
			else if (!TechStaff->IsWalking()) { TechState = 2; TechTimer = 0.f; }
			break;
		case 2:
			TechTimer += DeltaSeconds;
			if (!Job || !Job->bBroken) { TechState = 3; SendStaff(TechStaff, TechPost(), 180.f); }
			else if (TechTimer >= (Job->bVirus ? 12.f : 6.f))
			{
				Job->bBroken = false;
				Job->bVirus = false;
				StaffDone[1]++;
				Stats.Repairs++;
				JRSound::PlayAt(this, TEXT("SFX_Pasang"), Job->GetActorLocation() + FVector(0, 0, 80), 0.6f);
				AddToast(JRPrintf(JRT(TEXT("Teknisi membereskan %s")), *Job->DisplayName), Good);
				TechState = 3;
				SendStaff(TechStaff, TechPost(), 180.f);
			}
			break;
		}
	}
	// OB: berjalan ke sampah terdekat, memungut, lanjut ke sampah berikutnya, lalu kembali ke pos
	if (CleanStaff)
	{
		auto Nearest = [this]() -> AJRTrash*
		{
			AJRTrash* Best = nullptr;
			float BestD = TNumericLimits<float>::Max();
			for (AJRTrash* T : Trash)
			{
				if (!T) continue;
				// Jarak jalan sebenarnya: sampah di lantai lain (lewat tangga) baru diambil setelah lantai ini bersih.
				const float D = PathLen(CleanStaff->GetActorLocation(), T->GetActorLocation());
				if (D < BestD) { BestD = D; Best = T; }
			}
			return Best;
		};
		switch (CleanState)
		{
		case 0:
		case 3:
			if (AJRTrash* T = Nearest())
			{
				CleanJob = T;
				FVector Spot = T->GetActorLocation() + FVector(-40.f, 0.f, 0.f);
				const FJRLayout& L = Layout();
				Spot.Z = (L.HasUpstairs() && Spot.Z > L.FloorH * 0.5f) ? L.FloorH : 0.f;	// berdiri di lantai, bukan melayang setinggi kursi
				SendStaff(CleanStaff, Spot, 0.f);
				CleanState = 1;
			}
			else if (CleanState == 3 && !CleanStaff->IsWalking()) CleanState = 0;
			break;
		case 1:
			if (!CleanJob.IsValid()) { CleanState = 3; SendStaff(CleanStaff, CleanPost(), 0.f); }	// sudah dipungut pemain
			else if (!CleanStaff->IsWalking()) { CleanState = 2; CleanTimer = 0.f; }
			break;
		case 2:
			CleanTimer += DeltaSeconds;
			if (!CleanJob.IsValid()) CleanState = 3;
			else if (CleanTimer >= 0.8f)
			{
				// Pungut sampah ini sekaligus sampah lain yang berserakan di dekatnya (satu sapuan).
				const FVector At = CleanJob->GetActorLocation();
				PickTrash(CleanJob.Get());
				StaffDone[3]++;
				for (int32 i = Trash.Num() - 1; i >= 0; --i)
				{
					if (Trash[i] && FVector::Dist(Trash[i]->GetActorLocation(), At) < 130.f) { PickTrash(Trash[i]); StaffDone[3]++; }
				}
				CleanState = 3;
				if (!Nearest()) SendStaff(CleanStaff, CleanPost(), 0.f);
			}
			break;
		}
	}
	// Koki: masak di kompor, lalu berjalan mengantar mi ke pemesan dan kembali ke kompor
	if (CookStaff)
	{
		auto FindOrder = [this]() -> AJRStation*
		{
			for (AJRStation* S : Stations) if (S && S->Customer && S->Customer->bOrder && S->Customer->State == EJRCustomerState::Playing) return S;
			return nullptr;
		};
		switch (CookState)
		{
		case 0:
			if (bCarryMie) break;	// pemain sedang mengurus mi sendiri
			if (StoveState == EJRStoveState::Idle && FindOrder() && CarryBox.IsNone() && PowerVA + 600 <= CapVA)
			{
				CookTimer += DeltaSeconds;
				if (CookTimer >= 1.5f) { CookTimer = 0.f; StartCook(); }
			}
			else if (StoveState == EJRStoveState::Ready)
			{
				if (AJRStation* O = FindOrder())
				{
					StoveState = EJRStoveState::Idle;	// mi diangkat koki
					CookJob = O;
					SendStaff(CookStaff, StaffSpot(O), (O->GetActorLocation() - StaffSpot(O)).Rotation().Yaw);
					CookState = 1;
				}
			}
			break;
		case 1:
			if (!CookStaff->IsWalking())
			{
				AJRStation* O = CookJob.Get();
				AJRCustomer* C = O ? O->Customer.Get() : nullptr;
				if (C && C->bOrder && C->State == EJRCustomerState::Playing)
				{
					C->bOrder = false;
					C->bAte = true;
					C->Bill += MiePriceNow();
					StaffDone[2]++;
					Stats.MieSold++;
					CountLife(EJRLife::Mie);
					AddXp(1);
					JRSound::PlayAt(this, TEXT("SFX_Notif"), O->GetActorLocation(), 0.7f);
					AddToast(JRPrintf(JRT(TEXT("Koki mengantar mi ke %s. +%s masuk tagihan")), *O->DisplayName, *JRFormatRp(MiePriceNow())), Good);
				}
				CookState = 3;
				SendStaff(CookStaff, CookPost(), -90.f);
			}
			break;
		case 3:
			if (!CookStaff->IsWalking()) CookState = 0;
			break;
		}
	}
}

/* ---------- Cerita misteri "TV 1" ---------- */

AJRVisitor* AJRGameMode::SpawnMysteryVisitor(const TCHAR* Mesh, const FVector& Where)
{
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AJRVisitor* V = GetWorld()->SpawnActor<AJRVisitor>(Where, FRotator(0.f, 0.f, 0.f), SP);
	if (V) V->Setup(Mesh, TEXT("A_Idle"));
	return V;
}

void AJRGameMode::TickMystery(float DeltaSeconds)
{
	if (bMysteryTest && MysteryStep >= 8 && Choices.Num() == 0)
	{
		MysteryTestTimer += DeltaSeconds;
		if (MysteryTestTimer > 1.5f)
		{
			UE_LOG(LogTemp, Display, TEXT("JR_MYSTERY_DONE step=%d gold=%d naga=%d money=%.0f"), MysteryStep, bGoldCassette ? 1 : 0, bNagaServer ? 1 : 0, Money);
			FGenericPlatformMisc::RequestExit(false);
		}
		return;
	}
	if (MysteryStep >= 8 || Choices.Num() > 0 || bAutoTest || bBalance) return;
	if (bMysteryTest)
	{
		MysteryTestTimer += DeltaSeconds;
		if (MysteryTestTimer < 3.f) return;
		MysteryTestTimer = 0.f;
		RunMysteryStep();
		return;
	}
	// Tiap bab punya jamnya sendiri, menjelang tutup.
	static const float StepHour[] = { 23.2f, 23.4f, 23.0f, 22.4f, 23.5f, 23.1f, 22.5f, 22.0f };
	if (MysteryStep >= 5 && Era < 2) return;	// bab 2 baru dimulai di ruko
	if (Day < MysteryDay || Hour < StepHour[MysteryStep]) return;
	RunMysteryStep();
}

void AJRGameMode::RunMysteryStep()
{
	const FJRLayout& L = Layout();
	const FVector Door = L.DoorOutside();
	const FVector Inside(-(L.HalfX - 100.f), L.DoorY, 0.f);
	AJRStation* Tv1 = nullptr;
	for (AJRStation* S : Stations) if (S && S->Type == EJRUnitType::PS) { Tv1 = S; break; }
	const int32 Step = MysteryStep;
	MysteryStep++;
	MysteryDay = Day + (FMath::RandBool() ? 1 : 2);
	JRSound::Play2D(this, TEXT("SFX_Misteri"), 0.8f);
	const FString TvName = Tv1 ? Tv1->DisplayName : FString(JRT(TEXT("TV 1")));
	TWeakObjectPtr<AJRStation> WT(Tv1);

	switch (Step)
	{
	case 0:
	{
		FlickerTime = 2.5f;
		if (Tv1 && !Tv1->Customer) Tv1->bHaunted = true;
		if (Tv1) JRSound::PlayAt(this, TEXT("SFX_TV_Nyala"), Tv1->GetActorLocation() + FVector(0, 0, 100), 0.5f);
		PushChoice(JRPrintf(JRT(TEXT("%s menyala sendiri")), *TvName),
			JRPrintf(JRT(TEXT("Lampu berkedip. Tidak ada yang main, tapi layar %s menyala biru dan menampilkan tulisan \"LANJUTKAN?\" dengan logo game naga yang tidak kamu kenal.")), *TvName),
			{ JRT(TEXT("Matikan TV-nya")), JRT(TEXT("Biarkan saja")) },
			{
				[this, WT]() { if (WT.IsValid()) WT->bHaunted = false; AddToast(JRT(TEXT("TV dimatikan. Colokannya... memang sudah dicabut dari tadi?"))); },
				[this, WT]() { AddToast(JRT(TEXT("Layar itu terus berkedip sampai toko tutup."))); }
			});
		break;
	}
	case 1:
	{
		TWeakObjectPtr<AJRVisitor> WV(SpawnMysteryVisitor(TEXT("SK_Mahasiswa"), Inside));
		PushChoice(JRT(TEXT("Pemuda berjaket")),
			JRPrintf(JRT(TEXT("Seorang pemuda berjaket lusuh berdiri di pintu. \"Bang, masih buka? Saya cuma mau cek save-an saya... di %s.\" Ia menyodorkan uang kertas seribu bergambar lama, tahun 1999.")), *TvName),
			{ JRT(TEXT("Silakan, gratis saja")), JRT(TEXT("Maaf, sudah mau tutup")) },
			{
				[this, WV, Door]()
				{
					if (WV.IsValid()) WV->LeaveTo(Door);
					Stats.RepDelta += 1.f;
					AddToast(JRT(TEXT("Ia hanya menatap layar sebentar, tersenyum, lalu pergi tanpa bicara.")));
				},
				[this, WV, Door]()
				{
					if (WV.IsValid()) WV->LeaveTo(Door);
					AddToast(JRT(TEXT("Ia mengangguk pelan. Uang seribu lamanya tertinggal di meja kasir.")));
				}
			});
		break;
	}
	case 2:
	{
		PushChoice(JRT(TEXT("Tulisan di buku utang")),
			JRPrintf(JRT(TEXT("Di halaman paling belakang buku utang ada tulisan pensil yang bukan tulisanmu:\n\"Bayu - %s - belum tamat - 1999\"")), *TvName),
			{ JRT(TEXT("Tanya Pak RT besok")), JRT(TEXT("Tutup bukunya")) },
			{
				[this]() { AddToast(JRT(TEXT("Pak RT pasti tahu sejarah tempat ini."))); },
				[this]() { AddToast(JRT(TEXT("Kamu menutup buku utang. Tulisan itu tidak bisa dihapus."))); }
			});
		break;
	}
	case 3:
	{
		TWeakObjectPtr<AJRVisitor> WR(SpawnMysteryVisitor(TEXT("SK_Pak_RT"), Inside));
		PushChoice(JRT(TEXT("Cerita Pak RT")),
			JRT(TEXT("\"Bayu? Wah, lama sekali itu. Dulu, sebelum jadi tempatmu, di sini ada rental PS kecil punya Pak Darto. Bayu itu bocah langganan, tiap malam main sampai jam dua belas. Waktu rentalnya tutup tahun '99, dia nangis karena game naganya belum tamat. Katanya sekarang sudah kerja di kota.\"")),
			{ JRT(TEXT("Jadi dia masih hidup?")), JRT(TEXT("Terima kasih, Pak")) },
			{
				[this, WR, Door]() { if (WR.IsValid()) WR->LeaveTo(Door); AddToast(JRT(TEXT("\"Ya masih, Mas! Memangnya kenapa?\" Pak RT tertawa sambil pulang."))); },
				[this, WR, Door]() { if (WR.IsValid()) WR->LeaveTo(Door); AddToast(JRT(TEXT("Pak RT pulang. Kamu melirik TV itu lagi."))); }
			});
		break;
	}
	case 4:
	{
		FlickerTime = 1.5f;
		if (Tv1 && !Tv1->Customer) Tv1->bHaunted = true;
		TWeakObjectPtr<AJRVisitor> WV(SpawnMysteryVisitor(TEXT("SK_Kantoran"), Inside));
		auto Reward = [this, WT](bool bStay)
		{
			if (WT.IsValid()) WT->bHaunted = false;
			bGoldCassette = true;
			if (Kasir) Kasir->ShowGoldCassette(true);
			Money += 50000.f;
			Stats.Income += 50000.f;
			Stats.RepDelta += 3.f;
			JRSound::Play2D(this, TEXT("SFX_Kaset_Emas"), 0.9f);
			AddToast(bStay ? JRT(TEXT("Kaset Emas dipajang di meja kasir. +Rp 50.000, reputasi +3"))
				: JRT(TEXT("Paginya ada Kaset Emas dan amplop di depan pintu. +Rp 50.000, reputasi +3")), Good);
		};
		PushChoice(JRT(TEXT("Bayu kembali")),
			JRPrintf(JRT(TEXT("Pria berkemeja rapi masuk. \"Bang... saya Bayu. Dulu tiap malam saya main di sini, di %s. Boleh saya tamatin game itu sekali ini aja? Sudah dua puluh tahun lebih kepikiran.\"")), *TvName),
			{ JRT(TEXT("Silakan, Mas")), JRT(TEXT("Maaf, sudah tutup")) },
			{
				[this, WV, Door, Reward]()
				{
					if (WV.IsValid()) WV->LeaveTo(Door);
					Reward(true);
					PushChoice(JRT(TEXT("TAMAT")),
						JRT(TEXT("Layar menampilkan tulisan TAMAT. Bayu tertawa, matanya basah. Ia menaruh sebuah kaset berlapis emas di meja kasir: \"Buat rental ini. Terima kasih sudah menghidupkan tempat ini lagi.\"")),
						{ JRT(TEXT("Sama-sama, Mas Bayu")) }, { []() {} });
				},
				[WV, Door, Reward]()
				{
					if (WV.IsValid()) WV->LeaveTo(Door);
					Reward(false);
				}
			});
		break;
	}
	// ---------- Bab 2: "Naga Online" (Era 2+) ----------
	case 5:
	{
		AJRStation* Pc = nullptr;
		for (AJRStation* S : Stations) if (S && S->Type == EJRUnitType::PC && (!Pc || S->Index > Pc->Index)) Pc = S;
		TWeakObjectPtr<AJRStation> WP(Pc);
		if (Pc && !Pc->Customer) { Pc->bHaunted = true; Pc->HauntText = JRT(TEXT("ADA YANG MAIN NAGA?  -B")); }
		FlickerTime = 1.f;
		PushChoice(JRT(TEXT("Pesan di layar PC")),
			JRPrintf(JRT(TEXT("Di layar %s muncul jendela chat hijau: \"ADA YANG MASIH MAIN NAGA? -B\". Tidak ada yang sedang memakai PC itu, dan kabel LAN-nya bahkan tidak tersambung.")), Pc ? *Pc->DisplayName : TEXT("PC")),
			{ JRT(TEXT("Balas: siapa ini?")), JRT(TEXT("Matikan monitornya")) },
			{
				[this, WP]() { if (WP.IsValid()) WP->HauntText = JRT(TEXT("BESOK MALAM. -B")); AddToast(JRT(TEXT("Balasan muncul: \"BESOK MALAM. -B\" lalu layar mati sendiri."))); },
				[this, WP]() { if (WP.IsValid()) WP->bHaunted = false; AddToast(JRT(TEXT("Monitor dimatikan. Inisial \"B\"... rasanya kamu kenal."))); }
			});
		break;
	}
	case 6:
	{
		for (AJRStation* S : Stations) if (S) { S->bHaunted = false; S->HauntText.Reset(); }
		TWeakObjectPtr<AJRVisitor> WV(SpawnMysteryVisitor(TEXT("SK_Kantoran"), Inside));
		PushChoice(JRT(TEXT("Bayu dan server tua")),
			JRT(TEXT("Bayu masuk sambil tertawa. \"Bang, itu saya yang ngetik dari laptop! Ternyata game naga itu dulu punya mode online, servernya mati tahun 2004. Saya hidupkan lagi servernya di rumah. Boleh dites di sini malam Minggu? Teman-teman lama mau ikut.\"")),
			{ JRT(TEXT("Boleh, pakai semua PC gratis")), JRT(TEXT("Boleh, tapi bayar normal")) },
			{
				[this, WV, Door]() { if (WV.IsValid()) WV->LeaveTo(Door); NagaPaid = false; Stats.RepDelta += 1.f; AddToast(JRT(TEXT("Bayu sumringah: \"Abang memang juragan sejati!\""))); },
				[this, WV, Door]() { if (WV.IsValid()) WV->LeaveTo(Door); NagaPaid = true; AddToast(JRT(TEXT("\"Siap, Bang, adil itu!\" kata Bayu."))); }
			});
		break;
	}
	case 7:
	{
		FlickerTime = 0.f;
		TWeakObjectPtr<AJRVisitor> WV(SpawnMysteryVisitor(TEXT("SK_Kantoran"), Inside));
		auto Finish = [this, WV, Door]()
		{
			if (WV.IsValid()) WV->LeaveTo(Door);
			bNagaServer = true;
			Regulars = FMath::Min(10, Regulars + 3);
			const float Pay = NagaPaid ? 120000.f : 50000.f;
			Money += Pay;
			Stats.Income += Pay;
			Stats.RepDelta += NagaPaid ? 2.f : 5.f;
			JRSound::Play2D(this, TEXT("SFX_Kaset_Emas"), 0.9f);
			AddToast(JRPrintf(JRT(TEXT("Server Naga Online terpasang di warnetmu! Gamer makin sering datang. +%s")), *JRFormatRp(Pay)), Good);
		};
		PushChoice(JRT(TEXT("Malam Naga Online")),
			JRT(TEXT("Bayu datang bersama teman-teman lamanya, sekarang sudah bapak-bapak semua. Mereka main \"Legenda Naga Online\" sampai larut, teriak dan tertawa seperti anak SMP lagi. Sebelum pulang Bayu menyerahkan flashdisk: \"Servernya buat Abang. Pasang terus ya, biar anak-anak sekarang kenal game ini.\"")),
			{ JRT(TEXT("Terima kasih, Mas Bayu")) },
			{ Finish });
		break;
	}
	default:
		break;
	}

	if (bMysteryTest && MysteryStep >= 8)
	{
		MysteryTestTimer = -4.f; // beri waktu dialog terakhir dipotret
	}
}

/* ---------- Simpan dan muat ---------- */

const TCHAR* AJRGameMode::SlotName()
{
	// Uji otomatis memakai slot terpisah agar simpanan pemain tidak tertimpa.
	return FParse::Param(FCommandLine::Get(), TEXT("jrautotest")) || FParse::Param(FCommandLine::Get(), TEXT("jrbalance")) ? TEXT("JRSave_Test") : TEXT("JRSave");
}

namespace JRLocal_Save
{
	FString Note;	// alasan simpan terakhir (bertahan saat level dimuat ulang), ditampilkan di menu utama bila menu developer aktif
}

FString AJRGameMode::BackupPath()
{
	// Salinan kedua di folder lain: jaga-jaga bila slot SaveGame bawaan gagal ditulis/dibaca di HP tertentu.
	return FPaths::Combine(FPaths::ProjectPersistentDownloadDir(), FString(SlotName()) + TEXT("_cadangan.sav"));
}

const FString& AJRGameMode::SaveNote() { return JRLocal_Save::Note; }

bool AJRGameMode::HasSave()
{
	return UGameplayStatics::DoesSaveGameExist(SlotName(), 0) || FPaths::FileExists(BackupPath());
}

UJRSaveGame* AJRGameMode::LoadSaveObject()
{
	UJRSaveGame* Slot = UGameplayStatics::DoesSaveGameExist(SlotName(), 0) ? Cast<UJRSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0)) : nullptr;
	UJRSaveGame* Backup = nullptr;
	TArray<uint8> Bytes;
	if (FPaths::FileExists(BackupPath()) && FFileHelper::LoadFileToArray(Bytes, *BackupPath()) && Bytes.Num() > 0)
		Backup = Cast<UJRSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	// Pakai yang paling baru (slot bisa tertinggal bila penulisan terakhirnya gagal).
	if (Slot && Backup) return Backup->SavedAt > Slot->SavedAt ? Backup : Slot;
	return Slot ? Slot : Backup;
}

FString AJRGameMode::SaveSummary()
{
	const UJRSaveGame* S = LoadSaveObject();
	if (!S) return FString();
	if (S->bMidDay)
	{
		const int32 H = FMath::FloorToInt(S->Hour), M = FMath::FloorToInt((S->Hour - H) * 60.f);
		return JRPrintf(JRT(TEXT("Hari %d, %02d:%02d  -  %s  -  %s")), S->Day, H, M,
			S->Era >= 3 ? JRT(TEXT("Game Center")) : S->Era == 2 ? JRT(TEXT("Ruko Kecil")) : JRT(TEXT("Garasi")), *JRFormatRp(S->Money));
	}
	return JRPrintf(JRT(TEXT("Hari %d  -  %s  -  %s")), S->Day, S->Era >= 3 ? JRT(TEXT("Game Center")) : S->Era == 2 ? JRT(TEXT("Ruko Kecil")) : JRT(TEXT("Garasi")), *JRFormatRp(S->Money));
}

void AJRGameMode::TickSky()
{
	if (!bSkyInit)
	{
		bSkyInit = true;
		{ TActorIterator<ADirectionalLight> It(GetWorld()); if (It) SkySun = *It; }
		{ TActorIterator<ASkyLight> It(GetWorld()); if (It) SkyFill = *It; }
		if (ADirectionalLight* S = SkySun.Get())
		{
			S->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			SunBase = S->GetLightComponent()->Intensity;
			SunYaw = S->GetActorRotation().Yaw;
		}
		if (ASkyLight* F = SkyFill.Get())
		{
			F->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			FillBase = F->GetLightComponent()->Intensity;
		}
		// HP: awan volumetrik sangat berat di GPU HP (ray-march per piksel); langit tetap biru dari SkyAtmosphere.
		const bool bMobileGfx = PLATFORM_ANDROID || PLATFORM_IOS || FParse::Param(FCommandLine::Get(), TEXT("jrgfx"));
		if (bMobileGfx)
		{
			for (TActorIterator<AVolumetricCloud> It(GetWorld()); It; ++It) { It->SetActorHiddenInGame(true); It->GetRootComponent()->SetVisibility(false, true); }
		}
		UE_LOG(LogTemp, Display, TEXT("JR_SKY sun=%d (%.1f) skylight=%d (%.2f realtime=%d)"), SkySun.IsValid() ? 1 : 0, SunBase,
			SkyFill.IsValid() ? 1 : 0, FillBase, SkyFill.IsValid() && SkyFill->GetLightComponent()->bRealTimeCapture ? 1 : 0);
	}
	// Menu utama: sore yang hangat. Layar laporan (tutup jam 24.00): malam.
	const float H = Phase == EJRGamePhase::Menu ? 16.5f : Hour;
	if (FMath::Abs(H - LastSkyHour) < 0.04f) return;
	LastSkyHour = H;
	const float Elev = (H - 6.f) / 12.f * 180.f;	// 0 = terbit pukul 06.00, 180 = terbenam pukul 18.00
	const float Lit = FMath::Clamp(FMath::Sin(FMath::DegreesToRadians(Elev)) * 3.f, 0.f, 1.f);
	if (ADirectionalLight* S = SkySun.Get())
	{
		S->SetActorRotation(FRotator(-FMath::Clamp(Elev, 4.f, 176.f), SunYaw, 0.f));	// di bawah cakrawala cukup dimatikan
		S->GetLightComponent()->SetIntensity(SunBase * Lit);
	}
	if (ASkyLight* F = SkyFill.Get())
	{
		USkyLightComponent* C = F->GetLightComponent();
		C->SetIntensity(FillBase * FMath::Lerp(0.12f, 1.f, Lit));
		// Tangkap ulang langit hanya saat terangnya berubah cukup jauh (mahal di HP)
		if (!C->bRealTimeCapture && FMath::Abs(Lit - LastCaptureDay) > 0.2f) { C->RecaptureSky(); LastCaptureDay = Lit; }
	}
}

bool AJRGameMode::SaveNow()
{
	// Hanya saat toko buka: di layar laporan simpanan akhir hari sudah ada, di menu/opening belum ada yang perlu disimpan.
	if (Phase != EJRGamePhase::Playing || IsOpening() || PromoId > 0 || bBalance || bDemoOver)
	{
		JRLocal_Save::Note = FString::Printf(TEXT("Tidak disimpan: fase=%d pembuka=%d demo=%d"), (int32)Phase, IsOpening() ? 1 : 0, bDemoOver ? 1 : 0);	// JRNOWRAP
		return false;
	}
	SaveProgress(true);
	return true;
}

void AJRGameMode::OnAppBackground()
{
	// Android: HP dikunci, pindah aplikasi, atau aplikasi ditutup dari daftar aplikasi terbaru.
	if (SaveNow()) UE_LOG(LogTemp, Display, TEXT("JR_AUTOSAVE latar belakang hari=%d jam=%.2f"), Day, Hour);
}

void AJRGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(AppBgHandle);
	FCoreDelegates::GetApplicationWillTerminateDelegate().Remove(AppQuitHandle);
	Super::EndPlay(Reason);
}

void AJRGameMode::SaveProgress(bool bMidDay)
{
	if (PromoId > 0) return;	// rekaman promosi tidak boleh menimpa simpanan pemain
	UJRSaveGame* S = Cast<UJRSaveGame>(UGameplayStatics::CreateSaveGameObject(UJRSaveGame::StaticClass()));
	S->SavedAt = FDateTime::Now();
	S->Era = Era; S->Day = Day; S->Money = Money; S->Rep = Rep; S->CapVA = CapVA;
	S->PricePS = PricePS; S->PricePC = PricePC;
	S->OwnedPS = Count(EJRUnitType::PS); S->OwnedPC = Count(EJRUnitType::PC); S->OwnedVIP = Count(EJRUnitType::VIP); S->PriceVIP = PriceVIP;
	S->bBilling = bBilling;
	for (const AJRTrash* T : Trash) if (T) { S->TrashPos.Add(T->GetActorLocation()); S->TrashKind.Add(T->Kind); }
	for (const FJRMission& M : Missions)
	{
		S->MissionKind.Add((int32)M.Kind); S->MissionTarget.Add(M.Target); S->MissionProgress.Add(M.Progress);
		S->MissionReward.Add(M.Reward); S->MissionDone.Add(M.bDone); S->MissionWeekly.Add(M.bWeekly);
	}
	S->bStikOri = bStikOri; S->bPS2 = bPS2; S->bAntivirus = bAntivirus; S->bFastNet = bFastNet; S->bPaketMalam = bPaketMalam;
	S->HypeName = HypeName; S->HypeDays = HypeDays; S->bHypeInstalled = bHypeInstalled; S->Regulars = Regulars;
	S->Debts = Debts;
	S->PendingOrders = PendingOrders;
	for (const AJRDeliveryBox* B : Boxes) if (B) S->Boxes.Add(B->ItemId);
	S->ReportLines = ReportLines;
	S->MysteryStep = MysteryStep; S->MysteryDay = MysteryDay; S->bGoldCassette = bGoldCassette; S->bNagaServer = bNagaServer;
	for (bool B : Staff) S->Staff.Add(B);
	S->SeatTier = SeatTier; S->bFastPC = bFastPC; S->Decor = Decor;
	S->GuestsDone = GuestsDone; S->ViralDays = ViralDays;
	S->NetPlan = NetPlan; S->PendingNetPlan = PendingNetPlan;
	for (const AJRStation* X : Stations) if (X && X->bUsed) (X->Type == EJRUnitType::PS ? S->UsedPS : S->UsedPC)++;
	S->Xp = Xp; S->LvPS = LvPS; S->LvPC = LvPC; S->LvVIP = LvVIP;
	S->bKulkas = bKulkas; S->bKonter = bKonter; S->bPrinter = bPrinter;
	S->Drinks = Drinks; S->Paper = Paper; S->Saldo = Saldo;
	S->RentalName = RentalName;
	S->NegDays = NegDays; S->SeenEndings = SeenEndings;
	if (RecordsProgress()) JRSettings::Save();	// hitungan lencana ikut tersimpan
	for (const FJRPost& P : Posts) { S->PostAuthor.Add(P.Author); S->PostText.Add(P.Text); S->PostLikes.Add(P.Likes); S->PostDay.Add(P.Day); }
	if (bMidDay)
	{
		// Tengah hari: simpan juga catatan hari ini agar laporan malam tetap lengkap setelah dimuat.
		S->bMidDay = true;
		S->Hour = Hour;
		S->TutStep = TutStep;
		if (!CarryBox.IsNone()) S->Boxes.Add(CarryBox);	// kardus yang sedang dibawa kembali ke depan pintu
		const FJRDayStats& D = Stats;
		S->DayFloats = { D.Income, D.Elec, D.Kabur, D.Stock, D.Utang, D.RepDelta, D.Bonus, D.Side };
		S->DayInts = { D.Customers, D.AngryQueue, D.AngryStart, D.Ran, D.Breaks, D.Trips, D.Paid, D.MieSold, D.Started, D.Repairs,
			D.DrinksSold, D.TopUps, D.Prints, D.WalkInLost, D.Xp, D.Upsells, TourPlayers, (int32)News, (int32)Plan, D.ExactChange };
		S->DaySat = D.Satisfaction;
		for (EJRMoment M : D.Moments) S->DayMoments.Add((int32)M);
		S->LoakOffer = LoakOffer; S->GuestAt = GuestAt; S->GuestWhich = GuestWhich; S->RtAt = RtAt;
		S->bQuiet = bQuiet; S->bFightToday = bFightToday; S->bTourAccepted = bTourAccepted; S->bTourDone = bTourDone;
	}
	const bool bSlot = UGameplayStatics::SaveGameToSlot(S, SlotName(), 0);
	TArray<uint8> Bytes;
	const bool bBackup = UGameplayStatics::SaveGameToMemory(S, Bytes) && FFileHelper::SaveArrayToFile(Bytes, *BackupPath());
	JRLocal_Save::Note = FString::Printf(TEXT("Simpan hari %d jam %.2f: slot=%d cadangan=%d ada=%d"), Day, Hour, bSlot ? 1 : 0, bBackup ? 1 : 0, HasSave() ? 1 : 0);	// JRNOWRAP
	UE_LOG(LogTemp, Display, TEXT("JR_SAVE %s (%s)"), *JRLocal_Save::Note, *BackupPath());
	if (bSlot || bBackup)
	{
		if (!bMidDay) ReportLines.Add(JRT(TEXT("Permainan tersimpan otomatis.")));
		SaveFlash = 2.5f;
		UE_LOG(LogTemp, Display, TEXT("JR_AUTOSAVE ok tengah=%d hari=%d jam=%.2f kas=%.0f"), bMidDay ? 1 : 0, Day, Hour, Money);
	}
	else
	{
		AddToast(JRT(TEXT("Gagal menyimpan permainan!")), Bad);
	}
}

bool AJRGameMode::LoadProgress()
{
	const UJRSaveGame* S = LoadSaveObject();
	if (!S) return false;
	ClearWorld();
	Era = S->Era; Day = S->Day; Money = S->Money; Rep = S->Rep; CapVA = S->CapVA;
	PricePS = S->PricePS; PricePC = S->PricePC;
	OwnedPS = S->OwnedPS; OwnedPC = S->OwnedPC; OwnedVIP = S->OwnedVIP; PriceVIP = S->PriceVIP;
	bBilling = S->bBilling;
	const TArray<FVector> SavedTrash = S->TrashPos;
	const TArray<int32> SavedTrashKind = S->TrashKind;
	Missions.Reset();
	for (int32 i = 0; i < S->MissionKind.Num(); ++i)
	{
		FJRMission M;
		M.Kind = (EJRMission)S->MissionKind[i];
		M.Target = S->MissionTarget.IsValidIndex(i) ? S->MissionTarget[i] : 0;
		M.Progress = S->MissionProgress.IsValidIndex(i) ? S->MissionProgress[i] : 0;
		M.Reward = S->MissionReward.IsValidIndex(i) ? S->MissionReward[i] : 0;
		M.bDone = S->MissionDone.IsValidIndex(i) && S->MissionDone[i];
		M.bWeekly = S->MissionWeekly.IsValidIndex(i) && S->MissionWeekly[i];
		Missions.Add(M);
	}
	bStikOri = S->bStikOri; bPS2 = S->bPS2; bAntivirus = S->bAntivirus; bFastNet = S->bFastNet; bPaketMalam = S->bPaketMalam;
	HypeName = S->HypeName; HypeDays = S->HypeDays; bHypeInstalled = S->bHypeInstalled; Regulars = S->Regulars;
	Debts = S->Debts;
	PendingOrders = S->PendingOrders;
	MysteryStep = S->MysteryStep; MysteryDay = S->MysteryDay; bGoldCassette = S->bGoldCassette; bNagaServer = S->bNagaServer;
	for (int32 i = 0; i < (int32)EJRStaff::Count; ++i) Staff[i] = S->Staff.IsValidIndex(i) && S->Staff[i];
	SeatTier = S->SeatTier; bFastPC = S->bFastPC; Decor = S->Decor;
	GuestsDone = S->GuestsDone; ViralDays = S->ViralDays;
	NetPlan = FMath::Max(S->NetPlan, bFastNet ? 1 : 0); PendingNetPlan = S->PendingNetPlan;
	// Simpanan lama (sebelum Level Juragan): beri XP sesuai era agar barang yang sudah wajar tidak terkunci.
	Xp = S->Xp >= 0 ? S->Xp : XpForLevel(Era >= 3 ? 7 : Era == 2 ? 4 : 1);
	LvPS = S->LvPS; LvPC = S->LvPC; LvVIP = S->LvVIP;
	bKulkas = S->bKulkas; bKonter = S->bKonter; bPrinter = S->bPrinter;
	Drinks = S->Drinks; Paper = S->Paper; Saldo = S->Saldo;
	if (!S->RentalName.IsEmpty()) RentalName = S->RentalName;
	NegDays = S->NegDays; SeenEndings = S->SeenEndings;
	Posts.Reset();
	for (int32 i = 0; i < S->PostText.Num(); ++i)
	{
		FJRPost P;
		P.Author = S->PostAuthor.IsValidIndex(i) ? S->PostAuthor[i] : FString();
		P.Text = S->PostText[i];
		P.Likes = S->PostLikes.IsValidIndex(i) ? S->PostLikes[i] : 0;
		P.Day = S->PostDay.IsValidIndex(i) ? S->PostDay[i] : 0;
		Posts.Add(P);
	}
	bShowMoment = false;	// laporan hasil muat: tanpa Kartu Momen (data hari itu tidak tersimpan)
	bNeedName = false;
	CarryBox = NAME_None; bCarryMie = false; StoveState = EJRStoveState::Idle; bMcbTripped = false; bNetSlow = false;
	Choices.Reset();
	Hour = S->bMidDay ? FMath::Clamp(S->Hour, 8.f, 23.9f) : 24.f;
	BuildWorld();
	for (const FName& Id : S->Boxes) SpawnDelivery(Id);
	for (int32 i = 0; i < SavedTrash.Num(); ++i) SpawnTrash(SavedTrash[i], SavedTrashKind.IsValidIndex(i) ? SavedTrashKind[i] : 2);
	// Tandai unit bekas (yang paling akhir dipasang)
	int32 UsedLeft[2] = { S->UsedPS, S->UsedPC };
	for (int32 i = Stations.Num() - 1; i >= 0; --i)
	{
		AJRStation* X = Stations[i];
		int32& N = UsedLeft[X->Type == EJRUnitType::PS ? 0 : 1];
		if (N > 0) { X->MarkUsed(); --N; }
	}

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn()) Pawn->SetActorLocation(Layout().PlayerStart(), false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(FRotator::ZeroRotator);
	}
	TutStep = TutOff;
	ReportLines = S->ReportLines;
	ReportLines.Remove(JRT(TEXT("Permainan tersimpan otomatis.")));
	if (S->bMidDay)
	{
		// Lanjut di tengah hari, pada jam yang sama. Laporan kemarin tetap tersimpan untuk HP > Info.
		Stats = FJRDayStats();
		auto F = [S](int32 i) { return S->DayFloats.IsValidIndex(i) ? S->DayFloats[i] : 0.f; };
		auto I = [S](int32 i) { return S->DayInts.IsValidIndex(i) ? S->DayInts[i] : 0; };
		Stats.Income = F(0); Stats.Elec = F(1); Stats.Kabur = F(2); Stats.Stock = F(3); Stats.Utang = F(4); Stats.RepDelta = F(5); Stats.Bonus = F(6); Stats.Side = F(7);
		Stats.Customers = I(0); Stats.AngryQueue = I(1); Stats.AngryStart = I(2); Stats.Ran = I(3); Stats.Breaks = I(4); Stats.Trips = I(5);
		Stats.Paid = I(6); Stats.MieSold = I(7); Stats.Started = I(8); Stats.Repairs = I(9); Stats.DrinksSold = I(10); Stats.TopUps = I(11);
		Stats.Prints = I(12); Stats.WalkInLost = I(13); Stats.Xp = I(14); Stats.Upsells = I(15); TourPlayers = I(16);
		News = (EJRNews)FMath::Clamp(I(17), 0, (int32)EJRNews::BolaMalam); Plan = (EJRPlan)FMath::Clamp(I(18), 0, (int32)EJRPlan::Tutor); Stats.ExactChange = I(19);
		Stats.Satisfaction = S->DaySat;
		for (int32 M : S->DayMoments) Stats.Moments.Add((EJRMoment)M);
		LoakOffer = S->LoakOffer; GuestAt = S->GuestAt; GuestWhich = S->GuestWhich; RtAt = S->RtAt;
		bQuiet = S->bQuiet; bFightToday = S->bFightToday; bTourAccepted = S->bTourAccepted; bTourDone = S->bTourDone; bTourRunning = false;
		TutStep = S->TutStep;
		NextAutoSaveHour = (FMath::FloorToFloat(Hour / AutoSaveEveryHours) + 1.f) * AutoSaveEveryHours;
		const int32 H = FMath::FloorToInt(Hour), Mn = FMath::FloorToInt((Hour - H) * 60.f);
		AddToast(JRPrintf(JRT(TEXT("Permainan dimuat. Lanjut hari %d pukul %02d:%02d.")), Day, H, Mn), Good);
		Phase = EJRGamePhase::Playing;
	}
	else
	{
		ReportLines.Insert(JRPrintf(JRT(TEXT("Permainan dimuat (disimpan %s).")), *S->SavedAt.ToString(TEXT("%d/%m/%Y %H:%M"))), 0);
		Phase = EJRGamePhase::Report;
	}
	CheckDemoOver();	// demo yang sudah selesai tidak bisa dilanjutkan
	return true;
}

void AJRGameMode::ExtendSession(AJRStation* S)
{
	AJRCustomer* C = S ? S->Customer.Get() : nullptr;
	if (!C || !C->bAskExtend) return;
	C->bAskExtend = false;
	C->HoursLeft = 1.f;
	const int32 Add = UnitPrice(S);
	C->Bill += Add;
	C->SatAdj += 4.f;
	Stats.Upsells++;
	CountLife(EJRLife::Upsell);
	if (bAutoTest) UE_LOG(LogTemp, Display, TEXT("JR_UPSELL ditambah %s total=%d"), *S->DisplayName, Stats.Upsells);
	if (Stats.Upsells == 1) AddMoment(EJRMoment::Upsell);
	AddXp(2);
	JRSound::PlayAt(this, TEXT("SFX_Kaching"), S->GetActorLocation() + FVector(0, 0, 100), 0.6f);
	AddToast(JRPrintf(JRT(TEXT("Siap, Bang! %s main sejam lagi (+%s)")), *C->DisplayName, *JRFormatRp(Add)), Good);
}

void AJRGameMode::AddMoment(EJRMoment M)
{
	if (!Stats.Moments.Contains(M)) Stats.Moments.Add(M);
}

FString AJRGameMode::MomentText(EJRMoment M)
{
	switch (M)
	{
	case EJRMoment::Emak: return JRT(TEXT("Emak datang mencari bocil yang bolos"));
	case EJRMoment::RageQuit: return JRT(TEXT("Ada yang rage quit sampai membanting stik"));
	case EJRMoment::Fight: return JRT(TEXT("Dua pemain PS hampir berantem"));
	case EJRMoment::RunAway: return JRT(TEXT("Ada yang kabur tanpa bayar"));
	case EJRMoment::Trip: return JRT(TEXT("MCB jeglek, rental gelap gulita"));
	case EJRMoment::PakRt: return JRT(TEXT("Pak RT datang menegur karena berisik"));
	case EJRMoment::Upsell: return JRT(TEXT("Pelanggan minta nambah sejam lagi"));
	case EJRMoment::Streamer: return JRT(TEXT("Streamer Bang Jago live dari rental"));
	case EJRMoment::Lurah: return JRT(TEXT("Pak Lurah sidak ke rental"));
	case EJRMoment::Rival: return JRT(TEXT("Juragan rental sebelah datang mengintip"));
	case EJRMoment::Tour: return JRT(TEXT("Turnamen PS akhir pekan pecah!"));
	case EJRMoment::LevelUp: return JRT(TEXT("Naik Level Juragan"));
	default: return FString();
	}
}

void AJRGameMode::BeginNewGame()
{
	Phase = EJRGamePhase::Intro;
	TutStep = TutStart;
	Missions.Reset();
	NewDailyMissions();
	NewWeeklyMission();
	bNeedName = !bMenuTest && !FParse::Param(FCommandLine::Get(), TEXT("jrtouchflow"));
	// Adegan pembuka garasi (dilewati di uji otomatis yang memakai alur menu)
	const TCHAR* Cmd = FCommandLine::Get();
	const bool bSkip = bMenuTest || FParse::Param(Cmd, TEXT("jrtouchflow")) || FParse::Param(Cmd, TEXT("jrnoopening"));
	if (Era == 1 && !bSkip)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AJROpening* O = GetWorld()->SpawnActor<AJROpening>(FVector::ZeroVector, FRotator::ZeroRotator, SP))
		{
			O->Start(Layout().HalfX, Layout().DoorY);
			Opening = O;
		}
	}
}

void AJRGameMode::SkipOpening()
{
	if (AJROpening* O = Opening.Get()) O->Finish();
	Opening.Reset();
}

/* ---------- Misi harian dan mingguan ---------- */

FString AJRGameMode::MissionText(const FJRMission& M) const
{
	switch (M.Kind)
	{
	case EJRMission::Serve: return JRPrintf(JRT(TEXT("Layani %d pelanggan sampai bayar")), M.Target);
	case EJRMission::Mie: return JRPrintf(JRT(TEXT("Jual %d mangkuk mi")), M.Target);
	case EJRMission::Income: return JRPrintf(JRT(TEXT("Pemasukan %s")), *JRFormatRp(M.Target));
	case EJRMission::Start: return JRPrintf(JRT(TEXT("Nyalakan %d sesi main")), M.Target);
	case EJRMission::Repair: return JRPrintf(JRT(TEXT("Perbaiki %d unit rusak")), M.Target);
	case EJRMission::NoRun: return JRT(TEXT("Tidak ada yang kabur tanpa bayar"));
	case EJRMission::Upsell: return JRPrintf(JRT(TEXT("\"Bang, nambah sejam!\" %d kali")), M.Target);
	case EJRMission::ExactChange: return JRPrintf(JRT(TEXT("Kembalian pas %d kali")), M.Target);
	default: return JRPrintf(JRT(TEXT("Kepuasan rata-rata minimal %d")), M.Target);
	}
}

void AJRGameMode::NewDailyMissions()
{
	Missions.RemoveAll([](const FJRMission& M) { return !M.bWeekly; });
	const int32 E = FMath::Clamp(Era, 1, 3) - 1;
	static const int32 Serve[] = { 15, 25, 35 }, MieN[] = { 3, 5, 7 }, Inc[] = { 90000, 250000, 450000 }, Start[] = { 12, 20, 30 }, Fix[] = { 2, 3, 4 };
	static const int32 Reward[] = { 15000, 30000, 50000 };
	TArray<EJRMission> Pool = { EJRMission::Serve, EJRMission::Mie, EJRMission::Income, EJRMission::Start, EJRMission::Repair, EJRMission::NoRun, EJRMission::Happy, EJRMission::ExactChange };
	// Target khas yang selalu ada: "Bang, nambah sejam!"
	{
		static const int32 Up[] = { 3, 5, 8 };
		FJRMission M;
		M.Kind = EJRMission::Upsell;
		M.Target = Up[E];
		M.Reward = Reward[E];
		Missions.Insert(M, 0);
	}
	for (int32 i = 1; i < 3 && Pool.Num() > 0; ++i)
	{
		const EJRMission K = Pool[FMath::RandRange(0, Pool.Num() - 1)];
		Pool.Remove(K);
		FJRMission M;
		M.Kind = K;
		M.Reward = Reward[E];
		switch (K)
		{
		case EJRMission::Serve: M.Target = Serve[E]; break;
		case EJRMission::Mie: M.Target = MieN[E]; break;
		case EJRMission::Income: M.Target = Inc[E]; break;
		case EJRMission::Start: M.Target = Start[E]; break;
		case EJRMission::Repair: M.Target = Fix[E]; break;
		case EJRMission::Happy: M.Target = 60; break;
		case EJRMission::ExactChange: M.Target = 3 + E; break;
		default: M.Target = 0; break;
		}
		Missions.Insert(M, i);
	}
}

void AJRGameMode::NewWeeklyMission()
{
	Missions.RemoveAll([](const FJRMission& M) { return M.bWeekly; });
	const int32 E = FMath::Clamp(Era, 1, 3) - 1;
	static const int32 Serve[] = { 120, 200, 300 }, Inc[] = { 600000, 1500000, 3000000 }, Reward[] = { 100000, 250000, 500000 };
	FJRMission M;
	M.bWeekly = true;
	M.Reward = Reward[E];
	if (FMath::RandBool()) { M.Kind = EJRMission::Serve; M.Target = Serve[E]; }
	else { M.Kind = EJRMission::Income; M.Target = Inc[E]; }
	Missions.Add(M);
}

void AJRGameMode::TickMissions(bool bEndOfDay)
{
	float Avg = 0.f;
	for (int32 V : Stats.Satisfaction) Avg += V;
	Avg = Stats.Satisfaction.Num() ? Avg / Stats.Satisfaction.Num() : 0.f;
	for (FJRMission& M : Missions)
	{
		if (M.bDone) continue;
		int32 Today = 0;
		switch (M.Kind)
		{
		case EJRMission::Serve: Today = Stats.Paid; break;
		case EJRMission::Mie: Today = Stats.MieSold; break;
		case EJRMission::Income: Today = FMath::RoundToInt(Stats.Income); break;
		case EJRMission::Start: Today = Stats.Started; break;
		case EJRMission::Repair: Today = Stats.Repairs; break;
		case EJRMission::Upsell: Today = Stats.Upsells; break;
		case EJRMission::ExactChange: Today = Stats.ExactChange; break;
		default: break;
		}
		bool bComplete = false;
		if (M.Kind == EJRMission::NoRun) bComplete = bEndOfDay && Stats.Ran == 0 && Stats.Paid > 0;
		else if (M.Kind == EJRMission::Happy) bComplete = bEndOfDay && Stats.Satisfaction.Num() > 0 && Avg >= M.Target;
		else
		{
			const int32 Shown = M.bWeekly ? M.Progress + Today : Today;
			bComplete = Shown >= M.Target;
			if (bEndOfDay && M.bWeekly) M.Progress += Today;	// mingguan menumpuk tiap hari
		}
		if (bComplete)
		{
			M.bDone = true;
			Money += M.Reward;
			Stats.Bonus += M.Reward;
			Stats.RepDelta += M.bWeekly ? 2.f : 0.5f;
			JRSound::Play2D(this, TEXT("SFX_Kaching"), 0.7f);
			AddToast(JRPrintf(JRT(TEXT("Misi %s selesai: %s  +%s")), M.bWeekly ? JRT(TEXT("mingguan")) : JRT(TEXT("harian")), *MissionText(M), *JRFormatRp(M.Reward)), Good);
			AddXp(M.bWeekly ? 120 : FMath::Max(10, M.Reward / 2000));
		}
	}
}

/* ---------- Tutorial hari pertama ---------- */

void AJRGameMode::TutorialText(bool bTouch, FString& OutTitle, FString& OutBody) const
{
	const TCHAR* Press = bTouch ? JRT(TEXT("ketuk tombol aksi")) : JRT(TEXT("tekan E"));
	OutTitle = JRPrintf(JRT(TEXT("TUTORIAL %d/%d")), FMath::Min(TutStep + 1, TutCount), TutCount);
	switch (TutStep)
	{
	case TutStart: OutBody = JRPrintf(JRT(TEXT("Pelanggan pertama datang! Tunggu ia duduk di depan TV, lalu lihat TV-nya dan %s untuk menyalakan.")), Press); break;
	case TutPay: OutBody = JRPrintf(JRT(TEXT("Setelah selesai main, pelanggan antre di kasir dekat pintu. Lihat meja kasir dan %s untuk menerima bayaran.")), Press); break;
	case TutMie: OutBody = JRPrintf(JRT(TEXT("Ada yang pesan mi! %s di kompor (dinding kiri) untuk memasak, angkat mi saat matang, lalu antar ke pemesannya.")), bTouch ? JRT(TEXT("Ketuk tombol aksi")) : JRT(TEXT("Tekan E"))); break;
	case TutPhone: OutBody = bTouch ? JRT(TEXT("Ketuk tombol HP. Di tab Toko kamu bisa belanja unit dan upgrade, di tab Kelola atur tarif."))
		: JRT(TEXT("Tekan Tab untuk membuka HP. Di tab Toko kamu bisa belanja unit dan upgrade, di tab Kelola atur tarif.")); break;
	case TutRepair: OutBody = JRPrintf(JRT(TEXT("Stik rusak! Lihat unit yang bertanda RUSAK lalu %s sampai selesai untuk memperbaikinya.")), bTouch ? JRT(TEXT("tahan tombol aksi")) : JRT(TEXT("tahan E"))); break;
	default: OutBody.Reset(); break;
	}
}

void AJRGameMode::TutorialAdvance(int32 FromStep)
{
	if (TutStep != FromStep) return;
	TutStep++;
	JRSound::Play2D(this, TEXT("SFX_Ding"), 0.7f);
	if (TutStep == TutDone)
	{
		Money += 20000.f;
		AddToast(JRT(TEXT("Tutorial selesai! Bonus Rp 20.000. Target Gerbang 1: kas Rp 600.000 dan reputasi 40.")), Good);
		if (TutArrow) TutArrow->Point(false, FVector::ZeroVector);
	}
	else
	{
		AddToast(JRT(TEXT("Misi selesai!")), Good);
	}
}

void AJRGameMode::SkipTutorial()
{
	if (!IsTutorial()) return;
	TutStep = TutDone;
	if (TutArrow) TutArrow->Point(false, FVector::ZeroVector);
	AddToast(JRT(TEXT("Tutorial dilewati. Target Gerbang 1: kas Rp 600.000 dan reputasi 40.")));
}

bool AJRGameMode::TutorialTarget(FVector& Out) const
{
	switch (TutStep)
	{
	case TutStart:
		for (AJRStation* S : Stations) if (S->Customer && S->Customer->State == EJRCustomerState::WaitStart) { Out = S->GetActorLocation(); return true; }
		for (AJRStation* S : Stations) if (S->Type == EJRUnitType::PS) { Out = S->GetActorLocation(); return true; }
		return false;
	case TutPay:
		if (!NextPayer()) return false;
		Out = Layout().Kasir();
		return true;
	case TutMie:
		if (bCarryMie)
		{
			for (AJRStation* S : Stations) if (S->Customer && S->Customer->bOrder) { Out = S->GetActorLocation(); return true; }
			return false;
		}
		Out = Layout().Stove();
		return true;
	case TutRepair:
		for (AJRStation* S : Stations) if (S->bBroken) { Out = S->GetActorLocation(); return true; }
		return false;
	default:
		return false;
	}
}

void AJRGameMode::TickTutorial(float DeltaSeconds)
{
	if (!IsTutorial()) return;
	if (!TutArrow)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		TutArrow = GetWorld()->SpawnActor<AJRTutorialArrow>(FVector::ZeroVector, FRotator::ZeroRotator, SP);
	}
	// Pemicu agar pemain tidak menunggu lama: pesanan mi dan stik rusak.
	if (TutStep == TutMie && !bCarryMie && StoveState == EJRStoveState::Idle)
	{
		bool bAny = false;
		for (AJRStation* S : Stations) if (S->Customer && S->Customer->bOrder) bAny = true;
		if (!bAny)
		{
			for (AJRStation* S : Stations)
			{
				AJRCustomer* C = S->Customer;
				if (C && C->State == EJRCustomerState::Playing && !C->bAte) { C->bOrder = true; C->OrderHours = 2.f; AddToast(JRPrintf(JRT(TEXT("%s di %s pesan mi instan")), *C->DisplayName, *S->DisplayName)); break; }
			}
		}
	}
	if (TutStep == TutRepair)
	{
		bool bAny = false;
		for (AJRStation* S : Stations) if (S->bBroken) bAny = true;
		if (!bAny)
		{
			for (AJRStation* S : Stations)
			{
				if (S->Type == EJRUnitType::PS && S->Customer && S->Customer->State == EJRCustomerState::Playing)
				{
					S->bBroken = true;
					JRSound::PlayAt(this, TEXT("SFX_Stik_Rusak"), S->GetActorLocation() + FVector(0, 0, 100));
					break;
				}
			}
		}
	}
	FVector T;
	const bool bShow = TutorialTarget(T);
	if (TutArrow) TutArrow->Point(bShow, T);
}

/* ---------- Mode uji otomatis ---------- */

void AJRGameMode::TickAutoTest(float DeltaSeconds)
{
	AutoTestTime += DeltaSeconds;

	// Pemain otomatis: nyalakan unit, perbaiki, terima bayaran, naikkan MCB, restart router.
	for (AJRStation* S : Stations)
	{
		if (S->bBroken && !bStaffTest) S->Interact(); // sama seperti pemain menahan E
		if (S->Customer && S->Customer->State == EJRCustomerState::WaitStart) StartSession(S);
		if (S->Customer && S->Customer->bAskExtend && FMath::FRand() < 0.05f) ExtendSession(S);	// bot menambah jam
	}
	// Uji "nambah sejam": habiskan waktu main dua pelanggan agar mereka minta tambah jam.
	if (AutoTestTime > 5.f && UpsellForced < 2)
	{
		for (AJRCustomer* C : Customers)
		{
			if (C->State == EJRCustomerState::Playing && !C->bAskedExtend && !C->bRaging && C->TypeIndex != 0) { C->HoursLeft = 0.01f; ++UpsellForced; break; }
		}
	}
	// -jrmoment: tutup hari lebih awal agar Kartu Momen tampil (untuk dipotret)
	if (FParse::Param(FCommandLine::Get(), TEXT("jrmoment")) && AutoTestTime > 18.f) { EndDay(); return; }
	if (bMcbTripped) ResetMcb();
	if (bNetSlow && AutoTestTime > 12.f) RestartRouter();
	// Kantin otomatis: masak, angkat, antar ke pemesan pertama.
	for (AJRStation* S : Stations)
	{
		if (bStaffTest) break; // koki yang mengurus mi
		if (!S->Customer || !S->Customer->bOrder) continue;
		if (bCarryMie) { DeliverMie(S); AutoTestMieDelivered++; }
		else if (StoveState == EJRStoveState::Idle && CarryBox.IsNone() && PowerVA + 600 <= CapVA) StartCook();
		else if (StoveState == EJRStoveState::Ready) TakeMie();
		break;
	}
	if (NextPayer() && FMath::FRand() < 0.02f && !bStaffTest) CollectNextPayer(FParse::Param(FCommandLine::Get(), TEXT("jrchange")));	// -jrchange: bot menagih seperti pemain (uji kembalian pas)
	if (Trash.Num() > 0 && !bStaffTest && FMath::FRand() < 0.01f) PickTrash(Trash[0]);
	// Uji kejadian baru (-jrguests): berantem lalu tiga tamu spesial berurutan (masing-masing setelah dialog sebelumnya selesai).
	if (FParse::Param(FCommandLine::Get(), TEXT("jrguests")) && Choices.Num() == 0)
	{
		static int32 GuestStep = 0;
		static const float At[] = { 4.f, 8.f, 12.f, 16.f };
		if (GuestStep < 4 && AutoTestTime > At[GuestStep])
		{
			if (GuestStep == 0)
			{
				TArray<AJRCustomer*> Ps;
				for (AJRStation* S : Stations) if (S->Type == EJRUnitType::PS && S->Customer && S->Customer->State == EJRCustomerState::Playing && !S->Customer->bRaging) Ps.Add(S->Customer);
				if (Ps.Num() >= 2) FightEvent(Ps[0], Ps[1]);
				else if (AutoTestTime < 16.f) return; // tunggu sampai ada dua pemain PS
				UE_LOG(LogTemp, Display, TEXT("JR_GUESTTEST fight=%d"), bFightToday ? 1 : 0);
			}
			else
			{
				GuestEvent(GuestStep - 1);
				UE_LOG(LogTemp, Display, TEXT("JR_GUESTTEST guest=%d done=%d"), GuestStep - 1, GuestsDone);
			}
			GuestStep++;
		}
	}
	// -jrbilling: buka layar billing lalu potret
	if (bBilling && FParse::Param(FCommandLine::Get(), TEXT("jrbilling")))
	{
		APlayerController* BPC = GetWorld()->GetFirstPlayerController();
		AJRHUD* BH = BPC ? Cast<AJRHUD>(BPC->GetHUD()) : nullptr;
		if (BH && AutoTestTime > 14.f && AutoTestTime - DeltaSeconds <= 14.f) BH->SetBillingOpen(true);
		if (AutoTestTime > 15.f && AutoTestTime - DeltaSeconds <= 15.f)
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoTest/JR_Billing.png")), true, false);
		if (BH && AutoTestTime > 16.f && AutoTestTime - DeltaSeconds <= 16.f) BH->SetBillingOpen(false);
	}
	// -jrtour: potret spanduk turnamen lalu paksa final
	if (bTourAccepted && FParse::Param(FCommandLine::Get(), TEXT("jrtour")))
	{
		if (AutoTestTime > 12.f && AutoTestTime - DeltaSeconds <= 12.f)
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoTest/JR_Tour.png")), true, false);
		if (AutoTestTime > 16.f && AutoTestTime - DeltaSeconds <= 16.f) { UE_LOG(LogTemp, Display, TEXT("JR_TOUR players=%d running=%d"), TourPlayers, bTourRunning ? 1 : 0); Hour = 20.01f; }
	}
	// -jrfasad: potret tampak depan gedung dari seberang jalan
	if (FParse::Param(FCommandLine::Get(), TEXT("jrfasad")) && AutoTestTime < 4.f)
	{
		APlayerController* FPC = GetWorld()->GetFirstPlayerController();
		if (ACharacter* Pw = FPC ? Cast<ACharacter>(FPC->GetPawn()) : nullptr)
		{
			const FJRLayout& FL = Layout();
			Pw->GetCharacterMovement()->DisableMovement();
			Pw->SetActorEnableCollision(false);
			Pw->SetActorLocation(FVector(-(FL.HalfX + 1050.f), FL.DoorY - 150.f, (FL.HasUpstairs() ? 320.f : 220.f) - 64.f));
			FPC->SetControlRotation(FRotator(FL.HasUpstairs() ? 4.f : -2.f, 5.f, 0.f));
		}
		if (AutoTestTime > 2.f && AutoTestTime - DeltaSeconds <= 2.f)
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / FString::Printf(TEXT("AutoTest/JR_Fasad_Era%d.png"), Era)), true, false);
	}
	// -jrluar: potret pemandangan jalan dari dalam pintu
	if (FParse::Param(FCommandLine::Get(), TEXT("jrluar")) && AutoTestTime < 4.f)
	{
		APlayerController* LPC = GetWorld()->GetFirstPlayerController();
		if (ACharacter* Pw = LPC ? Cast<ACharacter>(LPC->GetPawn()) : nullptr)
		{
			const FJRLayout& LL = Layout();
			Pw->GetCharacterMovement()->DisableMovement();
			Pw->SetActorEnableCollision(false);
			Pw->SetActorLocation(FVector(-(LL.HalfX - 220.f), LL.DoorY - 60.f, 175.f - 64.f));
			LPC->SetControlRotation(FRotator(-6.f, 172.f, 0.f));
		}
		if (AutoTestTime > 2.f && AutoTestTime - DeltaSeconds <= 2.f)
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoTest/JR_Shot_Luar.png")), true, false);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("jrtuttest")))
	{
		static int32 LastTut = -2;
		if (TutStep != LastTut) { UE_LOG(LogTemp, Display, TEXT("JR_TUT step=%d t=%.1f"), TutStep, AutoTestTime); LastTut = TutStep; }
		if (AutoTestTime > 3.f && AutoTestTime - DeltaSeconds <= 3.f)
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoTest/JR_Tutorial.png")), true, false);
		if (TutStep == TutPay && NextPayer()) CollectNextPayer();
		else if (TutStep == TutPay) { for (AJRCustomer* C : Customers) if (C->State == EJRCustomerState::Playing && !C->bRaging) { EndSession(C); break; } }
		if (TutStep == TutPhone)
		{
			if (AJRHUD* H = GetWorld()->GetFirstPlayerController() ? Cast<AJRHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()) : nullptr) { H->SetPhoneOpen(true); H->SetPhoneOpen(false); }
		}
	}
	// Uji karyawan: rusakkan satu unit dan selesaikan dua sesi agar teknisi dan kasir pasti bekerja.
	if (bStaffTest && AutoTestTime > 5.f && AutoTestTime - DeltaSeconds <= 5.f && Stations.Num() > 0) Stations[0]->bBroken = true;
	if (bStaffTest && AutoTestTime > 13.f && AutoTestTime - DeltaSeconds <= 13.f)
	{
		for (AJRStation* S : Stations) if (S->Type == EJRUnitType::VIP) { S->bBroken = true; UE_LOG(LogTemp, Display, TEXT("JR_STAFF_VIPBREAK %s"), *S->DisplayName); break; }
	}
	// Uji OB: sebar sampah di beberapa kursi (termasuk lantai 2), lalu pantau tiap detik.
	if (bStaffTest && AutoTestTime > 6.f && AutoTestTime - DeltaSeconds <= 6.f)
	{
		int32 N = 0;
		for (AJRStation* S : Stations) if (S && (N++ % 2 == 0)) SpawnTrash(S->GetSeatLocation() + FVector(30.f, 20.f, 1.f), 1);
		UE_LOG(LogTemp, Display, TEXT("JR_OBTEST spawned trash=%d"), Trash.Num());
	}
	if (bStaffTest && CleanStaff && FMath::FloorToInt(AutoTestTime) != FMath::FloorToInt(AutoTestTime - DeltaSeconds))
	{
		UE_LOG(LogTemp, Display, TEXT("JR_OBTEST t=%.0f state=%d trash=%d picked=%d walking=%d pos=%s"), AutoTestTime, CleanState, Trash.Num(), StaffDone[3], CleanStaff->IsWalking() ? 1 : 0, *CleanStaff->GetActorLocation().ToCompactString());
	}
	if (bStaffTest && ((AutoTestTime > 9.f && AutoTestTime - DeltaSeconds <= 9.f) || (AutoTestTime > 12.f && AutoTestTime - DeltaSeconds <= 12.f)))
	{
		for (AJRCustomer* C : Customers) if (C->State == EJRCustomerState::Playing && !C->bRaging) { EndSession(C); break; }
	}
	// Uji Paket 1: pembeli konter/printer dilayani pemain (bot), krat dibawa ke kulkas.
	if (bPaket1Test)
	{
		if (AutoTestTime > 2.f && AutoTestTime - DeltaSeconds <= 2.f)
		{
			SpawnWalkIn(EJRService::Konter);
			SpawnWalkIn(EJRService::Printer);
			SpawnDelivery(TEXT("krat"));
			Drinks = 10;
		}
		if (AutoTestTime > 15.f && AutoTestTime - DeltaSeconds <= 15.f) for (int32 k = 0; k < 6; ++k) SellDrink();	// 6 pelanggan bayar di kasir
		if (AutoTestTime > 12.f)
		{
			if (WaitingAt(EJRService::Konter)) ServiceInteract(EJRService::Konter);
			if (WaitingAt(EJRService::Printer)) ServiceInteract(EJRService::Printer);
		}
		if (FMath::FloorToInt(AutoTestTime / 5.f) != FMath::FloorToInt((AutoTestTime - DeltaSeconds) / 5.f))
		{
			UE_LOG(LogTemp, Display, TEXT("JR_PAKET1 t=%.0f walkins=%d drinks=%d sold=%d topups=%d prints=%d side=%.0f saldo=%.0f paper=%d xp=%d lv=%d"),
				AutoTestTime, WalkIns.Num(), Drinks, Stats.DrinksSold, Stats.TopUps, Stats.Prints, Stats.Side, Saldo, Paper, Xp, JuraganLevel());
		}
	}
	// Kurir otomatis: angkat kardus lalu pasang.
	if (!CarryBox.IsNone()) { InstallBox(); AutoTestInstalled++; }
	else if (Boxes.Num() > 0 && !bCarryMie && AutoTestTime > 3.f) PickBox(Boxes[0]);
	if (Customers.Num() < 7 && FMath::FRand() < 0.05f) SpawnCustomer();
	// Era 2: picu internet lemot dan Pak RT agar ikut teruji.
	if (Era >= 2 && AutoTestTime > 8.f && AutoTestTime - DeltaSeconds <= 8.f) { bNetSlow = true; AddToast(JRT(TEXT("Internet lemot! (uji)")), Bad); }
	if (Era >= 2 && AutoTestTime > 10.f && AutoTestTime - DeltaSeconds <= 10.f) PakRtEvent();

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
	static const FShot ShotsCenter[] = {
		{ 20.f, FVector(-820.f, -1150.f, 280.f), FRotator(-14.f, 38.f, 0.f), TEXT("JR_Shot_GC_Lantai1") },
		{ 23.f, FVector(-250.f, -900.f, 190.f), FRotator(8.f, -2.f, 0.f), TEXT("JR_Shot_GC_Tangga") },
		{ 26.f, FVector(-150.f, 300.f, 350.f + 190.f), FRotator(-12.f, -15.f, 0.f), TEXT("JR_Shot_GC_VIP") },
	};
	static const FShot ShotsPaket1[] = {
		{ 20.f, FVector(250.f, -560.f, 210.f), FRotator(-18.f, -90.f, 0.f), TEXT("JR_Shot_Bisnis") },
		{ 23.f, FVector(470.f, -700.f, 190.f), FRotator(-12.f, -40.f, 0.f), TEXT("JR_Shot_UpgradePS") },
		{ 26.f, FVector(560.f, 820.f, 200.f), FRotator(-18.f, 70.f, 0.f), TEXT("JR_Shot_UpgradePC") },
	};
	const FShot* Shots = bPaket1Test ? ShotsPaket1 : Era >= 3 ? ShotsCenter : Era == 2 ? ShotsRuko : ShotsGarasi;
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
			const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / JRT(TEXT("AutoTest")) / FString(Shots[i].Name) + TEXT(".png"));
			FScreenshotRequest::RequestScreenshot(File, true, false);
			AutoTestShot = i + 1;
		}
	}
	// Buka HP dan potret layarnya juga.
	if (AutoTestShot == NumShots && AutoTestTime > 27.f)
	{
		if (AJRHUD* Hud = PC ? Cast<AJRHUD>(PC->GetHUD()) : nullptr)
		{
			if (!Hud->IsPhoneOpen())
			{
				Hud->SetPhoneOpen(true);
				int32 Tab = 0;
				if (FParse::Value(FCommandLine::Get(), TEXT("jrphonetab="), Tab) && Hud->GetPhone()) Hud->GetPhone()->SetTab(Tab);
			}
			if (AutoTestTime > 28.5f)
			{
				const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / JRT(TEXT("AutoTest/JR_Shot_HP.png")));
				FScreenshotRequest::RequestScreenshot(File, true, false);
				AutoTestShot++;
			}
		}
	}
	// Uji simpan/muat: tutup hari (otomatis tersimpan), muat lagi, bandingkan.
	if (AutoTestShot > NumShots && AutoTestTime > 29.f)
	{
		if (AJRHUD* Hud = PC ? Cast<AJRHUD>(PC->GetHUD()) : nullptr) Hud->SetPhoneOpen(false);
		EndDay();
		const float M0 = Money; const int32 D0 = Day, U0 = Stations.Num(), B0 = Boxes.Num(), O0 = PendingOrders.Num();
		const bool bLoaded = LoadProgress();
		const bool bSame = bLoaded && FMath::IsNearlyEqual(M0, Money, 1.f) && D0 == Day && U0 == Stations.Num() && B0 == Boxes.Num() && O0 == PendingOrders.Num();
		UE_LOG(LogTemp, Display, TEXT("JR_NET plan=%d pending=%d"), NetPlan, PendingNetPlan);
		{
			int32 LvSum = 0;
			for (const AJRStation* X : Stations) LvSum += X->UpLevel;
			UE_LOG(LogTemp, Display, TEXT("JR_SOSIAL name=%s upsells=%d moments=%d posts=%d first=[%s]"), *RentalName, Stats.Upsells, Stats.Moments.Num(), Posts.Num(), Posts.Num() > 0 ? *Posts[0].Text : TEXT(""));
			UE_LOG(LogTemp, Display, TEXT("JR_PAKET1_LOAD lv=%d xp=%d unitlv=%d units=%d kulkas=%d konter=%d printer=%d services=%d drinks=%d paper=%d saldo=%.0f"),
				JuraganLevel(), Xp, LvSum, Stations.Num(), bKulkas, bKonter, bPrinter, Services.Num(), Drinks, Paper, Saldo);
		}
		{
			int32 Used = 0; FString Names;
			for (const AJRStation* X : Stations) if (X->bUsed) { Used++; Names += X->DisplayName + TEXT(";"); }
			UE_LOG(LogTemp, Display, TEXT("JR_LOAK used=%d %s"), Used, *Names);
		}
		UE_LOG(LogTemp, Display, TEXT("JR_STAFF kasir=%d teknisi=%d koki=%d ob=%d trash=%d wages=%d"), StaffDone[0], StaffDone[1], StaffDone[2], StaffDone[3], Trash.Num(), TotalWages());
		UE_LOG(LogTemp, Display, TEXT("JR_SAVETEST %s money=%.0f day=%d units=%d boxes=%d orders=%d phase=%d"),
			bSame ? TEXT("OK") : TEXT("BEDA"), Money, Day, Stations.Num(), Boxes.Num(), PendingOrders.Num(), (int32)Phase);
		UE_LOG(LogTemp, Display, TEXT("JR_AUTOTEST_DONE era=%d customers=%d income=%.0f mie=%d stock=%.0f events=%d installed=%d stations=%d netslow=%d"),
			Era, Stats.Customers, Stats.Income, AutoTestMieDelivered, Stats.Stock, AutoTestEvents, AutoTestInstalled, Stations.Num(), bNetSlow ? 1 : 0);
		FGenericPlatformMisc::RequestExit(false);
		return;
	}
	if (AutoTestTime > 30.f)
	{
		UE_LOG(LogTemp, Display, TEXT("JR_AUTOTEST_DONE era=%d customers=%d income=%.0f mie=%d stock=%.0f events=%d installed=%d stations=%d netslow=%d"),
			Era, Stats.Customers, Stats.Income, AutoTestMieDelivered, Stats.Stock, AutoTestEvents, AutoTestInstalled, Stations.Num(), bNetSlow ? 1 : 0);
		FGenericPlatformMisc::RequestExit(false);
	}
}

/* ---------- Bot keseimbangan ekonomi (-jrbalance) ---------- */

void AJRGameMode::TickBalance(float DeltaSeconds)
{
	// Reaksi pemain: tidak instan. Peluang per detik permainan (1 hari = 360 detik).
	auto Chance = [DeltaSeconds](float AvgSeconds) { return FMath::FRand() < DeltaSeconds / AvgSeconds; };
	for (AJRStation* S : Stations)
	{
		if (S->bBroken && Chance(S->bVirus ? 14.f : 9.f)) { S->bBroken = false; S->bVirus = false; }
		if (S->Customer && S->Customer->State == EJRCustomerState::WaitStart && Chance(4.f)) StartSession(S);
	}
	if (bMcbTripped && Chance(6.f)) ResetMcb();
	if (bNetSlow && Chance(10.f)) RestartRouter();
	for (AJRStation* S : Stations)
	{
		if (!S->Customer || !S->Customer->bOrder) continue;
		if (bCarryMie) { if (Chance(4.f)) DeliverMie(S); }
		else if (StoveState == EJRStoveState::Idle && CarryBox.IsNone()) { if (PowerVA + 600 <= CapVA && Chance(5.f)) StartCook(); } // pemain cermat cek daya dulu
		else if (StoveState == EJRStoveState::Ready && Chance(3.f)) TakeMie();
		break;
	}
	if (NextPayer() && Chance(5.f)) CollectNextPayer();
	if (Trash.Num() > 0 && Chance(8.f)) PickTrash(Trash[0]);
	if (!CarryBox.IsNone()) { if (Chance(6.f)) InstallBox(); }
	else if (Boxes.Num() > 0 && !bCarryMie && Chance(6.f)) PickBox(Boxes[0]);
}

void AJRGameMode::BalanceBetweenDays(float DeltaSeconds)
{
	if (Phase == EJRGamePhase::Intro) { Phase = EJRGamePhase::Playing; return; }
	if (Phase != EJRGamePhase::Report) return;
	const int32 Units = TotalUnits();
	if (BalanceLoggedDay != Day)
	{
		BalanceLoggedDay = Day;
		BalanceReportTime = 0.f;
		const FJRLayout& L = Layout();
		float Avg = 0.f;
		for (int32 V : Stats.Satisfaction) Avg += V;
		Avg = Stats.Satisfaction.Num() ? Avg / Stats.Satisfaction.Num() : 0.f;
		UE_LOG(LogTemp, Display, TEXT("JR_BAL day=%d era=%d money=%.0f rep=%.1f income=%.0f elec=%.0f stock=%.0f rent=%.0f cust=%d units=%d va=%d | sat=%.0f paid=%d angryQ=%d angryS=%d kabur=%d breaks=%d trips=%d repDelta=%.2f events=%d"),
			Day, Era, Money, Rep, Stats.Income, Stats.Elec, Stats.Stock, L.Rent + InternetToday(), Stats.Customers, Units, CapVA,
			Avg, Stats.Paid, Stats.AngryQueue, Stats.AngryStart, Stats.Ran, Stats.Breaks, Stats.Trips, Stats.RepDelta, AutoTestEvents);
		UE_LOG(LogTemp, Display, TEXT("JR_BAL_LV day=%d lv=%d xp=%d xpToday=%d"), Day, JuraganLevel(), Xp, Stats.Xp);
		const bool bGate2 = false; // bot lanjut ke Era 3; berhenti di batas hari
		int32 MaxDays = 30;
		FParse::Value(FCommandLine::Get(), TEXT("jrbaldays="), MaxDays);
		if (bGate2 || Day >= MaxDays)
		{
			UE_LOG(LogTemp, Display, TEXT("JR_BAL_DONE day=%d era=%d gate2=%d"), Day, Era, bGate2 ? 1 : 0);
			FGenericPlatformMisc::RequestExit(false);
			return;
		}
	}
	BalanceReportTime += DeltaSeconds;
	if (BalanceReportTime < 0.5f) return;

	// Belanja ala pemain wajar: sisakan cadangan kas.
	auto Try = [this](FName Id, float Reserve)
	{
		for (const FJRShopItem& It : GetShopItems())
		{
			if (It.Id == Id && It.Lock.IsEmpty() && Money >= It.Cost + Reserve)
			{
				const int32 D = Day;
				if (BuyItem(Id)) UE_LOG(LogTemp, Display, TEXT("JR_BAL_BUY day=%d %s %d"), D, *Id.ToString(), It.Cost);
				return true;
			}
		}
		return false;
	};
	if (Era == 1)
	{
		if (Gate1Open() && Try(TEXT("move"), 0.f)) { UE_LOG(LogTemp, Display, TEXT("JR_BAL_GATE1 day=%d"), Day); return; }
		Try(TEXT("stik"), 100000.f);
		if (!Try(TEXT("ps"), 120000.f) && !Try(TEXT("pc"), 150000.f)) Try(TEXT("daya"), 100000.f); // unit terkunci daya -> beli daya
	}
	else
	{
		if (!bPaketMalam) TogglePaketMalam();
		if (Gate2Open() && Try(TEXT("move3"), 0.f)) { UE_LOG(LogTemp, Display, TEXT("JR_BAL_GATE2 day=%d"), Day); return; }
		if (Era >= 3) Try(TEXT("vip"), 300000.f);
		Try(TEXT("daya2"), 150000.f);
		Try(TEXT("hype"), 150000.f);
		Try(TEXT("net"), 300000.f);
		Try(TEXT("av"), 300000.f);
		if (Count(EJRUnitType::PS) >= 4) Try(TEXT("ps2"), 400000.f);
		if (!Try(TEXT("pc"), 250000.f)) Try(TEXT("ps"), 250000.f);
	}
	Confirm();
}

/* ---------- Unit ---------- */

FVector AJRGameMode::SlotLocation(EJRUnitType Type, int32 Index, FRotator& OutRot) const
{
	const FJRLayout& L = Layout();
	OutRot = FRotator(0.f, Type == EJRUnitType::PC ? -90.f : 180.f, 0.f); // menghadap ke dalam ruangan
	if (Type == EJRUnitType::VIP) return FVector(L.HalfX - 110.f, L.VipY.IsValidIndex(Index) ? L.VipY[Index] : 0.f, L.FloorH);	// lantai 2
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
	if (I >= MaxFor(Type)) return;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FRotator Rot;
	const FVector Loc = SlotLocation(Type, I, Rot);
	AJRStation* S = GetWorld()->SpawnActor<AJRStation>(Loc, Rot, SP);
	S->Setup(Type, I);
	S->ApplySeatTier(SeatTier);
	S->ApplyUpLevel(UnitLevelOf(Type, I));
	Stations.Add(S);
}

/* ---------- Loop utama ---------- */

void AJRGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SaveFlash = FMath::Max(0.f, SaveFlash - DeltaSeconds);
	for (FJRToast& T : Toasts) T.TimeLeft -= DeltaSeconds;
	Toasts.RemoveAll([](const FJRToast& T) { return T.TimeLeft <= 0.f; });
	if (PromoId > 0) TickPromo(DeltaSeconds);

	// Uji -jrexittest (simpanan asli!): main baru -> buka toko -> keluar ke menu lewat jalur tombol jeda -> cek Lanjutkan.
	static const bool bExitTest = FParse::Param(FCommandLine::Get(), TEXT("jrexittest"));
	if (bExitTest)
	{
		static int32 ExitStep = 0;	// statis: bertahan setelah level dimuat ulang
		static double ExitT = FPlatformTime::Seconds();
		const double Now = FPlatformTime::Seconds();
		if (ExitStep == 0 && Phase == EJRGamePhase::Menu && Now - ExitT > 3.0) { BeginNewGame(); SkipOpening(); bNeedName = false; ExitStep = 1; ExitT = Now; }
		else if (ExitStep == 1 && Now - ExitT > 2.0) { Confirm(); ExitStep = 2; ExitT = Now; UE_LOG(LogTemp, Display, TEXT("JR_EXITTEST buka phase=%d tut=%d"), (int32)Phase, TutStep); }
		else if (ExitStep == 2 && Now - ExitT > 3.0)
		{
			const bool bOk = SaveNow();
			UE_LOG(LogTemp, Display, TEXT("JR_EXITTEST simpan=%d phase=%d opening=%d jam=%.2f ada=%d"), bOk ? 1 : 0, (int32)Phase, IsOpening() ? 1 : 0, Hour, HasSave() ? 1 : 0);
			ExitStep = 3; ExitT = Now;
			UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
		}
		else if (ExitStep == 3 && Now - ExitT > 3.0)
		{
			UE_LOG(LogTemp, Display, TEXT("JR_EXITTEST menu ada=%d ringkasan=[%s] catatan=[%s]"), HasSave() ? 1 : 0, *SaveSummary(), *SaveNote());
			// Slot bawaan dihapus: Lanjutkan harus tetap bisa dari salinan cadangan.
			UGameplayStatics::DeleteGameInSlot(SlotName(), 0);
			UE_LOG(LogTemp, Display, TEXT("JR_EXITTEST tanpaslot ada=%d ringkasan=[%s] muat=%d"), HasSave() ? 1 : 0, *SaveSummary(), LoadProgress() ? 1 : 0);
			ExitStep = 4;
			FGenericPlatformMisc::RequestExit(false);
		}
	}

	// Mode uji: potret dialog pertama lalu pilih opsi pertama.
	if ((bAutoTest || bBalance || bMysteryTest) && Choices.Num() > 0)
	{
		AutoChoiceTimer += DeltaSeconds;
		if (bMysteryTest && AutoChoiceTimer > 0.8f && AutoChoiceTimer - DeltaSeconds <= 0.8f)
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / FString::Printf(TEXT("AutoTest/JR_Misteri_%d.png"), MysteryStep)), true, false);
		}
		if (bAutoTest && (FParse::Param(FCommandLine::Get(), TEXT("jrguests")) || FParse::Param(FCommandLine::Get(), TEXT("jrchange"))) && AutoChoiceTimer > 0.6f && AutoChoiceTimer - DeltaSeconds <= 0.6f)
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / FString::Printf(TEXT("AutoTest/JR_Event_%d.png"), AutoTestEvents)), true, false);
		}
		if (bAutoTest && AutoTestEvents == 0 && AutoChoiceTimer > 0.6f && AutoChoiceTimer - DeltaSeconds <= 0.6f)
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoTest/JR_Shot_Event.png")), true, false);
		}
		if (AutoChoiceTimer > 1.5f)
		{
			AutoTestEvents++; AutoChoiceTimer = 0.f;
			// Bot keseimbangan memilih acak seperti pemain sungguhan; uji otomatis selalu opsi pertama.
			ResolveChoice(bBalance && Choices.Num() > 0 ? FMath::RandRange(0, Choices[0].Labels.Num() - 1) : 0);
		}
		AutoTestTime += DeltaSeconds;
	}

	// -jrautotest -jrlvtip: paksa naik ke Level 2 agar penjelasan Level Juragan muncul (dipotret sebagai dialog pertama).
	if (bAutoTest && FParse::Param(FCommandLine::Get(), TEXT("jrlvtip")))
	{
		static bool bLvTipDone = false;
		if (!bLvTipDone && AutoTestTime > 4.f && Choices.Num() == 0)
		{
			bLvTipDone = true;
			AddXp(FMath::Max(1, XpForLevel(2) - Xp));
			UE_LOG(LogTemp, Display, TEXT("JR_LVTIP lv=%d choices=%d title=[%s]"), JuraganLevel(), Choices.Num(), Choices.Num() > 0 ? *Choices[0].Title : TEXT(""));
		}
	}

	// -jrautotest -jrdevtest: menu developer lompat ke Era 2 lalu Era 3, tambah kas dan level.
	if (bAutoTest && FParse::Param(FCommandLine::Get(), TEXT("jrdevtest")))
	{
		static float DevT = 0.f;
		static int32 DevStep = 0;
		DevT += DeltaSeconds;
		if (DevStep == 0 && DevT > 3.f) { DevJumpEra(2); DevStep = 1; }
		else if (DevStep == 1 && DevT > 5.f) { DevJumpEra(3); const int32 Lv = JuraganLevel(); DevLevelUp(); DevAddMoney(5000000.f); DevAddRep(20.f);
			UE_LOG(LogTemp, Display, TEXT("JR_DEVTEST era=%d vip=%d lv=%d->%d money=%.0f rep=%.0f layout=%s"), Era, Count(EJRUnitType::VIP), Lv, JuraganLevel(), Money, Rep, *Layout().Name); DevStep = 2; }
	}

	// -jrautotest -jrchange: tagih setiap pelanggan yang menunggu bayar seperti pemain (mini-game kembalian pas)
	if (bAutoTest && Phase == EJRGamePhase::Playing && Choices.Num() == 0 && FParse::Param(FCommandLine::Get(), TEXT("jrchange")) && NextPayer()) CollectNextPayer(true);

	// -jrperf: catat draw call dan jumlah segitiga per frame (rata-rata 2 detik) untuk membandingkan optimasi.
	if (FParse::Param(FCommandLine::Get(), TEXT("jrperf")))
	{
		static float PerfT = 0.f, PerfDraws = 0.f, PerfPrims = 0.f, PerfDt = 0.f;
		static int32 PerfN = 0;
		PerfT += DeltaSeconds;
		if (PerfT > 8.f)
		{
			PerfDraws += GNumDrawCallsRHI[0]; PerfPrims += GNumPrimitivesDrawnRHI[0]; PerfDt += DeltaSeconds; ++PerfN;
			if (PerfT > 10.f)
			{
				int32 Comps = 0;
				for (TActorIterator<AActor> It(GetWorld()); It; ++It) { TInlineComponentArray<UPrimitiveComponent*> P(*It); Comps += P.Num(); }
				UE_LOG(LogTemp, Display, TEXT("JR_PERF era=%d draws=%.0f tris=%.0f ms=%.1f primitif=%d"), Era, PerfDraws / PerfN, PerfPrims / PerfN, PerfDt / PerfN * 1000.f, Comps);
				PerfT = 0.f; PerfDraws = PerfPrims = PerfDt = 0.f; PerfN = 0;
			}
		}
	}

	// -jrautotest -jrmidsave: simpan di tengah hari, ubah kas, muat lagi; kas, jam, dan catatan hari harus kembali.
	if (bAutoTest && FParse::Param(FCommandLine::Get(), TEXT("jrmidsave")))
	{
		static float SaveT = 0.f;
		static int32 SaveStep = 0;
		static float WantMoney = 0.f, WantHour = 0.f, WantIncome = 0.f;
		static int32 WantPaid = 0;
		SaveT += DeltaSeconds;
		if (SaveStep == 0 && SaveT > 12.f && Phase == EJRGamePhase::Playing)
		{
			const bool bOk = SaveNow();
			WantMoney = Money; WantHour = Hour; WantIncome = Stats.Income; WantPaid = Stats.Paid;
			Money += 777777.f; Hour = 20.f; Stats = FJRDayStats(); NextAutoSaveHour = 99.f;	// jangan sampai simpanan uji tertimpa
			UE_LOG(LogTemp, Display, TEXT("JR_MIDSAVE simpan=%d summary=[%s]"), bOk ? 1 : 0, *SaveSummary());
			SaveStep = 1;
		}
		else if (SaveStep == 1 && SaveT > 14.f)
		{
			const bool bLoad = LoadProgress();
			const bool bMatch = FMath::IsNearlyEqual(Money, WantMoney, 1.f) && FMath::IsNearlyEqual(Hour, WantHour, 0.01f)
				&& FMath::IsNearlyEqual(Stats.Income, WantIncome, 1.f) && Stats.Paid == WantPaid && Phase == EJRGamePhase::Playing;
			UE_LOG(LogTemp, Display, TEXT("JR_MIDSAVE muat=%d cocok=%d kas=%.0f/%.0f jam=%.2f/%.2f paid=%d/%d phase=%d"),
				bLoad ? 1 : 0, bMatch ? 1 : 0, Money, WantMoney, Hour, WantHour, Stats.Paid, WantPaid, (int32)Phase);
			SaveStep = 2;
		}
		else if (SaveStep == 2 && SaveT > 20.f)
		{
			UE_LOG(LogTemp, Display, TEXT("JR_MIDSAVE lanjut phase=%d jam=%.2f pelanggan=%d"), (int32)Phase, Hour, Customers.Num());
			FGenericPlatformMisc::RequestExit(false);
		}
	}

	// -jrautotest -jrera2 -jrmovetest: penuhi Gerbang 2, tutup toko, pindah ke Era 3, simpan dan muat.
	if (bAutoTest && FParse::Param(FCommandLine::Get(), TEXT("jrmovetest")))
	{
		static float MoveT = 0.f;
		static int32 MoveStep = 0;
		MoveT += DeltaSeconds;
		if (MoveStep == 0 && MoveT > 4.f && Choices.Num() == 0)
		{
			while (TotalUnits() < 10) SpawnStation(Count(EJRUnitType::PS) <= Count(EJRUnitType::PC) ? EJRUnitType::PS : EJRUnitType::PC);
			EndDay();
			Money = 3200000.f; Rep = 61.f;
			const bool bOpen = Gate2Open();
			const bool bBought = BuyItem(TEXT("move3"));
			UE_LOG(LogTemp, Display, TEXT("JR_MOVETEST gate2=%d bought=%d era=%d units=%d cap=%d money=%.0f"), bOpen ? 1 : 0, bBought ? 1 : 0, Era, TotalUnits(), CapVA, Money);
			MoveStep = 1;
		}
		else if (MoveStep == 1 && MoveT > 6.f)
		{
			Money = 2000000.f;
			const bool bVip = BuyItem(TEXT("vip"));
			PendingOrders.Remove(TEXT("vip"));
			SpawnStation(EJRUnitType::VIP);
			SaveProgress();
			const bool bLoad = LoadProgress();
			UE_LOG(LogTemp, Display, TEXT("JR_MOVETEST vipbuy=%d load=%d era=%d vip=%d units=%d name=%s"), bVip ? 1 : 0, bLoad ? 1 : 0, Era, Count(EJRUnitType::VIP), TotalUnits(), *Layout().Name);
			MoveStep = 2;
		}
		else if (MoveStep == 2 && MoveT > 8.f)
		{
			FGenericPlatformMisc::RequestExit(false);
		}
	}

	// -jrautotest -jrdemo: lompat ke akhir hari ke-7, potret layar akhir demo, lalu keluar.
	if (bAutoTest && bDemo)
	{
		static float DemoT = 0.f;
		DemoT += DeltaSeconds;
		if (DemoT > 10.f && Phase == EJRGamePhase::Playing && Choices.Num() == 0) { Day = DemoLastDay; EndDay(); }
		if (bDemoOver && DemoT > 13.f && DemoT - DeltaSeconds <= 13.f)
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoTest/JR_DemoEnd.png")), true, false);
		if (DemoT > 14.f)
		{
			UE_LOG(LogTemp, Display, TEXT("JR_DEMOTEST over=%d day=%d"), bDemoOver ? 1 : 0, Day);
			FGenericPlatformMisc::RequestExit(false);
		}
	}

	// -jrmenutest: potret menu utama, halaman pengaturan, dan menu jeda, lalu keluar.
	if (bMenuTest)
	{
		MenuTestTime += DeltaSeconds;
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		AJRHUD* Hud = PC ? Cast<AJRHUD>(PC->GetHUD()) : nullptr;
		UJRMenuWidget* M = Hud ? Hud->GetMenu() : nullptr;
		auto Shot = [](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoTest") / FString(Name) + TEXT(".png")), true, false); };
		const float Steps[] = { 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f };
		if (M && MenuTestStep < UE_ARRAY_COUNT(Steps) && MenuTestTime >= Steps[MenuTestStep])
		{
			switch (MenuTestStep)
			{
			case 0: Shot(TEXT("JR_Menu_Utama")); break;
			case 1: M->ShowSettings(); break;
			case 2: Shot(TEXT("JR_Menu_Pengaturan")); break;
			case 3: M->Close(); BeginNewGame(); break;
			case 4: Hud->TogglePause(); break;
			case 5: Shot(TEXT("JR_Menu_Jeda")); break;
			default:
				UE_LOG(LogTemp, Display, TEXT("JR_MENUTEST_DONE paused=%d page=%d"), UGameplayStatics::IsGamePaused(this) ? 1 : 0, (int32)M->GetPage());
				FGenericPlatformMisc::RequestExit(false);
			}
			MenuTestStep++;
		}
	}

	if (bBalance && Phase != EJRGamePhase::Playing) BalanceBetweenDays(DeltaSeconds);
	// Lampu berkedip (cerita misteri) tetap berjalan meski dialog sedang terbuka.
	if (FlickerTime > 0.f && Room)
	{
		FlickerTime -= DeltaSeconds;
		Room->SetLightsOn(FlickerTime <= 0.f || FMath::FRand() < 0.55f);
	}

	TickSky();
	if (!IsPlaying()) return;
	if (bAutoTest) TickAutoTest(DeltaSeconds);
	TickMystery(DeltaSeconds);
	TickStaff(DeltaSeconds);
	TickTutorial(DeltaSeconds);
	TickMissions(false);
	if (bBalance) TickBalance(DeltaSeconds);

	const float DtH = DeltaSeconds * HoursPerSecond();
	Hour += DtH;
	TickWalkIns(DeltaSeconds, DtH);
	// Simpan otomatis tiap 2 jam dalam game (10.00, 12.00, ... 22.00); pukul 24.00 disimpan oleh EndDay.
	if (Hour >= NextAutoSaveHour)
	{
		NextAutoSaveHour = (FMath::FloorToFloat(Hour / AutoSaveEveryHours) + 1.f) * AutoSaveEveryHours;
		if (Hour < 23.5f) SaveNow();
	}

	if (PromoId == 0 && Hour < 23.f && FMath::FRand() < SpawnRate(Hour) * DtH)
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
			AddToast(JRT(TEXT("Mi matang! Angkat di kompor.")), Good);
		}
	}

	// Era 2: internet lemot dan Pak RT
	static const float SlowPerHour[] = { 0.12f, 0.04f, 0.012f };
	if (Era >= 2 && !bNetSlow && !bAutoTest && FMath::FRand() < SlowPerHour[FMath::Clamp(NetPlan, 0, 2)] * DtH)
	{
		bNetSlow = true;
		JRSound::Play2D(this, TEXT("SFX_Modem"), 0.6f);
		for (AJRCustomer* X : Customers)
		{
			if (X->State == EJRCustomerState::Playing && X->Used == EJRUnitType::PC && JRSound::Voice(this, VoiceCode[X->TypeIndex], TEXT("Lemot"), X->GetActorLocation())) break;
		}
		AddToast(JRT(TEXT("Internet lemot! Gamer mulai ngomel. Tahan E di router (dinding belakang) untuk restart.")), Bad);
	}
	TickGuests();
	TickTournament();
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
		JRSound::PlayAt(this, TEXT("SFX_Motor_Lewat"), Layout().DoorOutside() + FVector(0, FMath::FRandRange(-600.f, 600.f), 80.f), 0.9f);
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
	if (Era >= 2 && bPaketMalam && H >= 21.f) Base *= 1.8f;
	if (bQuiet && H >= 22.f) Base *= 0.5f;
	const float RepMul = 0.5f + Rep / 100.f;
	const float PriceMul = FMath::Pow((float(FairFor(EJRUnitType::PS)) / PricePS + float(FairFor(EJRUnitType::PC)) / PricePC) / 2.f, 1.5f);
	float R = (bTourRunning ? 1.6f : 1.f) * (ViralDays > 0 ? 1.5f : 1.f) * (1.f + (DecorBonus() - TrashPenalty()) / 100.f) * Base * RepMul * PriceMul * FMath::Min(Era >= 2 ? 2.6f : 1.8f, 0.55f + Stations.Num() * 0.18f);
	if (Era >= 2)
	{
		R *= 1.3f * (1.f + 0.04f * Regulars); // ruko di jalan besar
		if (bHypeInstalled && !HypeName.IsEmpty()) R *= 1.3f;
		if (NetPlan == 2) R *= 1.1f; // gamer cari warnet fiber
		if (bNagaServer) R *= 1.1f;  // komunitas Naga Online
	}
	R *= DailySpawnMul(H);	// kabar pagi dan rencana hari ini
	// Unit yang diupgrade jadi omongan warga: tiap rata-rata +1 level menambah 5% pengunjung.
	if (Stations.Num() > 0)
	{
		int32 Extra = 0;
		for (const AJRStation* S : Stations) Extra += S->UpLevel - 1;
		R *= 1.f + 0.05f * Extra / Stations.Num();
	}
	return R;
}

int32 AJRGameMode::PickType(float H) const
{
	const float HypeMul = (Era >= 2 && bHypeInstalled && !HypeName.IsEmpty()) ? 2.f : 1.f;
	const float GamerDay = Era >= 2 ? HypeMul : 0.f;
	float W[6] = { 0, 1, 0, 3, (Era >= 2 ? 4.f : 3.f) * HypeMul, 2 };
	if (H < 13.f)      { const float X[6] = { 4, 0, 3, 1, 0, 1 }; FMemory::Memcpy(W, X, sizeof(W)); }
	else if (H < 17.f) { const float X[6] = { 1, 5, 2, 0, GamerDay, 1 }; FMemory::Memcpy(W, X, sizeof(W)); }
	else if (H < 19.f) { const float X[6] = { 0, 2, 1, 3, GamerDay, 2 }; FMemory::Memcpy(W, X, sizeof(W)); }
	for (int32 i = 0; i < 6; ++i) W[i] *= DailyTypeMul(i, H);
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
	// Pelanggan memilih unit kosong dengan level upgrade tertinggi.
	AJRStation* Best = nullptr;
	for (AJRStation* S : Stations) if (S->Type == Type && S->IsFree() && (!Best || S->UpLevel > Best->UpLevel)) Best = S;
	return Best;
}

void AJRGameMode::SpawnCustomer()
{
	const int32 TI = PickType(Hour);
	const FTypeDef& T = Types[TI];
	EJRUnitType Want = T.Want == 2 ? (FMath::RandBool() ? EJRUnitType::PS : EJRUnitType::PC) : (T.Want == 0 ? EJRUnitType::PS : EJRUnitType::PC);
	if (Want == EJRUnitType::PC && Count(EJRUnitType::PC) == 0) Want = EJRUnitType::PS;
	if (Want == EJRUnitType::PS && Count(EJRUnitType::PS) == 0) Want = EJRUnitType::PC;
	// Game Center: pekerja, gamer, dan bapak-bapak kadang memilih ruang VIP lantai 2.
	bool bVip = false;
	if (Count(EJRUnitType::VIP) > 0 && (TI == Pekerja || TI == Gamer || TI == 5) && FMath::FRand() < 0.35f * (8000.f / FMath::Max(PriceVIP, 1)))
	{
		Want = EJRUnitType::VIP;
		bVip = true;
	}
	if (!FindFree(Want) && !QueueUsed.Contains(false)) return; // toko penuh, tidak jadi masuk
	// Antrean sudah panjang: sebagian calon pelanggan lewat saja (tanpa mengurangi reputasi).
	int32 Waiting = 0;
	for (bool bUsed : QueueUsed) Waiting += bUsed ? 1 : 0;
	if (!FindFree(Want) && Waiting >= 2 && FMath::FRand() < 0.15f * Waiting) return;

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AJRCustomer* C = GetWorld()->SpawnActor<AJRCustomer>(Layout().DoorOutside(), FRotator::ZeroRotator, SP);
	C->Setup(TI, JRT(T.Name), T.Mesh, T.Scale, Want, T.Want == 2 && !bVip);	// diterjemahkan saat dipakai, bukan saat inisialisasi statis
	Customers.Add(C);
	Stats.Customers++;
	JRSound::PlayAt(this, TEXT("SFX_Bel_Pintu"), FVector(-(Layout().HalfX - 40.f), Layout().DoorY, 220.f), 0.6f);
	if (!Assign(C)) ToQueue(C);
}

AJRCustomer* AJRGameMode::PromoSeat(AJRStation* S, int32 TypeIndex)
{
	// Rekaman promosi: pelanggan langsung duduk di unit (tanpa berjalan dari pintu).
	if (!S || S->Customer) return nullptr;
	const FTypeDef& T = Types[FMath::Clamp(TypeIndex, 0, (int32)UE_ARRAY_COUNT(Types) - 1)];
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AJRCustomer* C = GetWorld()->SpawnActor<AJRCustomer>(S->GetSeatLocation(), S->GetSeatRotation(), SP);
	if (!C) return nullptr;
	C->Setup(TypeIndex, JRT(T.Name), T.Mesh, T.Scale, S->Type, false);
	Customers.Add(C);
	S->Customer = C;
	C->Station = S;
	C->Patience = 99.f;
	C->WalkTo(S->GetSeatLocation(), EJRCustomerState::WaitStart);
	return C;
}

bool AJRGameMode::PromoStart(AJRStation* S)
{
	AJRCustomer* C = S ? S->Customer.Get() : nullptr;
	if (!C || C->State != EJRCustomerState::WaitStart) return false;
	StartSession(S);
	C->HoursLeft = 50.f;	// tidak selesai selama rekaman
	C->EmakAt = -1.f;
	return true;
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
	C->Patience = 1.5f;	// sabar menunggu unit dinyalakan (jam permainan)
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
	C->Patience = 1.8f;	// sabar di antrean
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
	// Bosan antre = kecewa (belum sempat main); unit tidak dinyalakan = marah.
	Stats.Satisfaction.Add(C->State == EJRCustomerState::Queue ? 32 : 12);
	C->Mood = TEXT(">:(");
	JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Marah_Pulang"), C->GetActorLocation());
	AddToast(Message, Bad);
	Leave(C);
}

void AJRGameMode::RunAway(AJRCustomer* C)
{
	Stats.Kabur += C->Bill;
	Stats.Ran++;
	AddMoment(EJRMoment::RunAway);
	Stats.RepDelta -= 0.5f;
	Stats.Satisfaction.Add(30);
	C->Mood = TEXT("KABUR!");
	C->Speed = 450.f;
	AddToast(JRPrintf(JRT(TEXT("%s kabur tanpa bayar %s! Kasir terlalu lama kosong.")), *C->DisplayName, *JRFormatRp(C->Bill)), Bad);
	Leave(C);
}

void AJRGameMode::StartSession(AJRStation* S)
{
	AJRCustomer* C = S->Customer;
	if (!C) return;
	C->Used = S->Type;
	C->UnitLevel = S->UpLevel;
	C->State = EJRCustomerState::Playing;

	// Paket malam warnet (Era 2): flat sampai tutup.
	if (S->Type == EJRUnitType::PC && Era >= 2 && bPaketMalam && Hour >= 21.f && (C->TypeIndex == Gamer || C->TypeIndex == Pekerja))
	{
		C->bPaket = true;
		C->HoursLeft = 24.f - Hour;
		C->Bill = 10000.f;
		JRSound::PlayAt(this, TEXT("SFX_TV_Nyala"), S->GetActorLocation(), 0.6f);
		JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Paket"), C->GetActorLocation());
		AddToast(JRPrintf(JRT(TEXT("%s: %s ambil paket malam sampai tutup (Rp 10.000)")), *S->DisplayName, *C->DisplayName));
		return;
	}

	static const float OptionsGarasi[] = { 1.f, 1.5f, 2.f };
	static const float OptionsRuko[] = { 1.f, 1.5f, 2.f, 2.5f, 3.f };
	const float Hours = C->TypeIndex == Bocil ? 1.f
		: (Era >= 2 ? OptionsRuko[FMath::RandRange(0, 4)] : OptionsGarasi[FMath::RandRange(0, 2)]);
	const float Main = Hours + DailyExtraHours(C->TypeIndex);	// hujan / gajian: main lebih lama
	C->HoursLeft = Main;
	C->Bill = Main * UnitPrice(S);
	if (bAutoTest && FParse::Param(FCommandLine::Get(), TEXT("jrchange"))) { C->HoursLeft = 0.25f; C->bAskedExtend = true; }	// uji kembalian pas: cepat selesai tanpa nambah sejam
	if (C->TypeIndex == Bocil && FMath::FRand() < (bAutoTest ? 1.f : 0.2f))
	{
		C->EmakAt = bAutoTest ? 0.05f : FMath::FRandRange(0.25f, 0.7f);
	}
	JRSound::PlayAt(this, S->Type != EJRUnitType::PC ? TEXT("SFX_Konsol_Boot") : TEXT("SFX_TV_Nyala"), S->GetActorLocation() + FVector(0, 0, 100), S->Type == EJRUnitType::PS ? 0.8f : 0.6f);
	TutorialAdvance(TutStart);
	Stats.Started++;
	if (bTourRunning && S->Type != EJRUnitType::PC && TourPlayers < TourMax)
	{
		++TourPlayers;
		C->Bill += TourFee;
		AddToast(JRPrintf(JRT(TEXT("Peserta turnamen %d/%d: %s (biaya daftar %s)")), TourPlayers, TourMax, *C->DisplayName, *JRFormatRp(TourFee)));
	}
	AddToast(JRPrintf(JRT(TEXT("%s menyala: %s main %s jam (%s)")), *S->DisplayName, *C->DisplayName,
		*FString::SanitizeFloat(Main), *JRFormatRp(C->Bill)));

	if (Era >= 2 && PromoId == 0 && S->Type == EJRUnitType::PC && Hour >= 19.f && FMath::FRand() < 0.15f) JokiEvent(C);	// rekaman promosi: tanpa kejadian acak
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

	const float BreakPerHour = (S->bUsed ? 1.6f : 1.f) * (1.f - 0.12f * (S->UpLevel - 1)) * (S->Type == EJRUnitType::PS ? (bStikOri ? 0.07f : 0.22f) : S->Type == EJRUnitType::VIP ? 0.06f : 0.18f) * DailyBreakMul();
	if (PromoId == 0 && FMath::FRand() < BreakPerHour * DtH)	// rekaman promosi: tanpa kerusakan acak
	{
		S->bBroken = true;
		C->Breakdowns++;
		Stats.Breaks++;
		if (S->Type == EJRUnitType::PC && Era >= 2 && FMath::FRand() < (bAntivirus ? 0.1f : 0.45f))
		{
			S->bVirus = true;
			JRSound::PlayAt(this, TEXT("SFX_Error_PC"), S->GetActorLocation() + FVector(0, 0, 100));
			AddToast(JRPrintf(JRT(TEXT("%s kena virus! Tahan E lebih lama di sana untuk install ulang.")), *S->DisplayName), Bad);
		}
		else
		{
			JRSound::PlayAt(this, S->Type != EJRUnitType::PC ? TEXT("SFX_Stik_Rusak") : TEXT("SFX_Error_PC"), S->GetActorLocation() + FVector(0, 0, 100));
			AddToast(S->Type != EJRUnitType::PC
				? JRPrintf(JRT(TEXT("Stik di %s rusak! Tahan E di sana untuk memperbaiki.")), *S->DisplayName)
				: JRPrintf(JRT(TEXT("%s hang! Tahan E di sana untuk restart.")), *S->DisplayName), Bad);
		}
		JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Rusak"), C->GetActorLocation());
	}
	// Pesan mi instan
	if (!C->bOrder && !C->bAte && FMath::FRand() < (bAutoTest ? 6.f : 0.4f * DailyMieMul()) * DtH)
	{
		C->bOrder = true;
		C->OrderHours = 1.f;
		JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Pesan_Mi"), C->GetActorLocation());
		AddToast(JRPrintf(JRT(TEXT("%s di %s pesan mi instan")), *C->DisplayName, *S->DisplayName));
	}
	if (C->bOrder)
	{
		C->OrderHours -= DtH;
		if (C->OrderHours <= 0.f)
		{
			C->bOrder = false;
			C->MissedOrders++;
			AddToast(JRPrintf(JRT(TEXT("Pesanan mi di %s kelamaan, batal")), *S->DisplayName), Bad);
		}
	}
	// Berantem: dua pemain PS bersebelahan (paling banyak sekali sehari)
	if (!bFightToday && PromoId == 0 && S->Type == EJRUnitType::PS && C->TypeIndex != Bocil && !bBalance && !bAutoTest && Choices.Num() == 0
		&& FMath::FRand() < 0.05f * DtH)
	{
		for (AJRStation* O : Stations)
		{
			AJRCustomer* X = O ? O->Customer.Get() : nullptr;
			if (O != S && O->Type == EJRUnitType::PS && FMath::Abs(O->Index - S->Index) == 1 && X && X->State == EJRCustomerState::Playing && !X->bRaging)
			{
				FightEvent(C, X);
				return;
			}
		}
	}
	if (C->HoursLeft <= 0.f)
	{
		// "Bang, nambah sejam!": pelanggan menunggu sebentar; pemain menambah jam lewat AKSI di unitnya.
		if (C->bAskExtend)
		{
			C->ExtendLeft -= DtH;
			if (C->ExtendLeft > 0.f) return;
			C->bAskExtend = false;
			C->SatAdj -= 3.f;
			AddToast(JRPrintf(JRT(TEXT("%s batal nambah jam dan pulang")), *C->DisplayName));
			EndSession(C);
			return;
		}
		if (!C->bAskedExtend && !C->bPaket && Hour < 22.5f && !bBalance && FMath::FRand() < (bAutoTest ? 1.f : 0.35f))
		{
			C->bAskedExtend = true;
			C->bAskExtend = true;
			C->ExtendLeft = 0.6f;
			if (bAutoTest) UE_LOG(LogTemp, Display, TEXT("JR_UPSELL minta %s"), *S->DisplayName);
			JRSound::PlayAt(this, TEXT("SFX_Notif"), S->GetActorLocation() + FVector(0, 0, 100), 0.6f);
			AddToast(JRPrintf(JRT(TEXT("%s di %s: \"Bang, nambah sejam!\"")), *C->DisplayName, *S->DisplayName), FLinearColor(1.f, 0.76f, 0.29f));
			return;
		}
		if (S->Type == EJRUnitType::PS && C->TypeIndex != Bocil && FMath::FRand() < (bAutoTest ? 0.5f : 0.06f))
		{
			RageQuitEvent(C);
			return;
		}
		EndSession(C);
	}
}

/* ---------- Kejadian ---------- */

void AJRGameMode::PushChoice(const FString& Title, const FString& Body, TArray<FString> Labels, TArray<TFunction<void()>> Actions, bool bNeutral)
{
	FJRChoice C;
	C.bNeutral = bNeutral;
	C.Title = Title;
	C.Body = Body;
	C.Labels = MoveTemp(Labels);
	C.Actions = MoveTemp(Actions);
	static int32 NextSerial = 1;
	C.Serial = NextSerial++;
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
	AddMoment(EJRMoment::Emak);
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FJRLayout& L = Layout();
	AJRVisitor* Emak = GetWorld()->SpawnActor<AJRVisitor>(FVector(-(L.HalfX - 100.f), L.DoorY, 0.f), FRotator::ZeroRotator, SP);
	if (Emak)
	{
		Emak->Setup(TEXT("SK_Emak"), TEXT("A_Marah"));
		JRSound::Voice(this, TEXT("Emak"), TEXT("Datang"), Emak->GetActorLocation(), true);
	}
	const FString Where = C->Station ? C->Station->DisplayName : FString(TEXT("TV"));
	TWeakObjectPtr<AJRCustomer> WC(C);
	TWeakObjectPtr<AJRVisitor> WE(Emak);
	const FVector Door = L.DoorOutside();
	PushChoice(JRT(TEXT("Emak datang!")),
		JRPrintf(JRT(TEXT("Seorang ibu berdiri di pintu membawa sapu: \"Bang, lihat anak saya? Seragam putih biru, harusnya masih di sekolah!\" Bocil di %s pura-pura tidak dengar.")), *Where),
		{ JRT(TEXT("Serahkan bocilnya")), JRT(TEXT("Bilang tidak ada")) },
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
				Stats.RepDelta += 1.f;
				B->bOrder = false;
				B->Mood = TEXT("T_T");
				if (WE.IsValid()) JRSound::Voice(this, TEXT("Emak"), TEXT("Jewer"), WE->GetActorLocation(), true);
				JRSound::Voice(this, TEXT("Bocil"), TEXT("Ketahuan"), B->GetActorLocation(), true);
				AddToast(JRPrintf(JRT(TEXT("Bocil dijewer pulang. Emak bayar setengah: %s. Reputasi warga +1")), *JRFormatRp(Paid)), Good);
				Leave(B);
			},
			[this, WE, Door]()
			{
				if (WE.IsValid())
				{
					JRSound::Voice(this, TEXT("Emak"), TEXT("Pergi"), WE->GetActorLocation(), true);
					WE->LeaveTo(Door);
				}
				Stats.RepDelta -= 1.f;
				AddToast(JRT(TEXT("Emak pergi sambil ngomel. Reputasi warga -1")), Bad);
			}
		});
}

void AJRGameMode::RageQuitEvent(AJRCustomer* C)
{
	AJRStation* S = C->Station;
	C->bRaging = true;
	if (S) S->bBroken = true;
	AddMoment(EJRMoment::RageQuit);
	JRSound::PlayAt(this, TEXT("SFX_Stik_Banting"), C->GetActorLocation());
	JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Rage"), C->GetActorLocation(), true);
	TWeakObjectPtr<AJRCustomer> WC(C);
	PushChoice(JRT(TEXT("Rage quit!")),
		JRPrintf(JRT(TEXT("%s kalah adu penalti lalu membanting stik di %s. Stiknya rusak.")), *C->DisplayName, S ? *S->DisplayName : TEXT("TV")),
		{ JRT(TEXT("Tagih ganti rugi Rp 15.000")), JRT(TEXT("Sudahlah, maafkan")) },
		{
			[this, WC]()
			{
				AJRCustomer* X = WC.Get();
				if (!X) return;
				X->Bill += 15000.f;
				X->bFined = true;
				X->SatAdj -= 15.f;
				Stats.RepDelta -= 1.f;
				AddToast(JRT(TEXT("Ganti rugi masuk tagihan. Reputasi -1")), Bad);
				X->bRaging = false;
				EndSession(X);
			},
			[this, WC]()
			{
				AJRCustomer* X = WC.Get();
				if (!X) return;
				Stats.RepDelta += 1.f;
				AddToast(JRT(TEXT("Warga menilai kamu sabar. Reputasi +1")), Good);
				X->bRaging = false;
				EndSession(X);
			}
		});
}

void AJRGameMode::PakRtEvent()
{
	AddMoment(EJRMoment::PakRt);
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FJRLayout& L = Layout();
	AJRVisitor* Rt = GetWorld()->SpawnActor<AJRVisitor>(FVector(-(L.HalfX - 100.f), L.DoorY, 0.f), FRotator::ZeroRotator, SP);
	if (Rt)
	{
		Rt->Setup(TEXT("SK_Pak_RT"), TEXT("A_Marah"));
		JRSound::Voice(this, TEXT("PakRT"), TEXT("Datang"), Rt->GetActorLocation(), true);
	}
	TWeakObjectPtr<AJRVisitor> WR(Rt);
	const FVector Door = L.DoorOutside();
	auto Bye = [this, WR, Door](const TCHAR* Line)
	{
		if (!WR.IsValid()) return;
		JRSound::Voice(this, TEXT("PakRT"), Line, WR->GetActorLocation(), true);
		WR->LeaveTo(Door);
	};
	PushChoice(JRT(TEXT("Pak RT datang")),
		JRT(TEXT("\"Mas, sudah jam sepuluh lewat. Warga komplain suara teriakan main game sampai ke gang sebelah.\"")),
		{ JRT(TEXT("Kecilkan suara")), JRT(TEXT("Kasih kopi gratis (Rp 5.000)")), JRT(TEXT("Abaikan")) },
		{
			[this, Bye]() { Bye(TEXT("Kecil")); bQuiet = true; AddToast(JRT(TEXT("Suara dikecilkan. Malam ini pelanggan lebih sepi"))); },
			[this, Bye]() { Bye(TEXT("Kopi")); Money -= 5000.f; Stats.Stock += 5000.f; Stats.RepDelta += 1.f; AddToast(JRT(TEXT("Pak RT ngopi dulu, urusan beres. Reputasi +1")), Good); },
			[this, Bye]() { Bye(TEXT("Abai")); Stats.RepDelta -= 3.f; AddToast(JRT(TEXT("Pak RT tersinggung. Reputasi warga -3")), Bad); }
		});
}

void AJRGameMode::JokiEvent(AJRCustomer* C)
{
	TWeakObjectPtr<AJRCustomer> WC(C);
	JRSound::Voice(this, TEXT("Gamer"), TEXT("Joki"), C->GetActorLocation(), true);
	PushChoice(JRT(TEXT("Joki rank datang")),
		JRPrintf(JRT(TEXT("\"Bang, saya joki rank. Pakai %s, kasih diskon dong. Nanti saya bawa satu tim ke sini.\"")), C->Station ? *C->Station->DisplayName : TEXT("PC")),
		{ JRT(TEXT("Kasih diskon 20%")), JRT(TEXT("Harga normal")) },
		{
			[this, WC]()
			{
				AJRCustomer* X = WC.Get();
				if (!X) return;
				X->Bill = FMath::RoundToFloat(X->Bill * 0.8f);
				Regulars = FMath::Min(10, Regulars + 1);
				Stats.RepDelta += 1.f;
				AddToast(JRT(TEXT("Joki jadi langganan. Gamer online makin sering datang")), Good);
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
		AddToast(JRT(TEXT("Kas tidak cukup untuk beli mi")), Bad);
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

void AJRGameMode::PutBackMie()
{
	if (!bCarryMie || StoveState != EJRStoveState::Idle) return;
	bCarryMie = false;
	StoveState = EJRStoveState::Ready;
	AddToast(JRT(TEXT("Mi ditaruh lagi di kompor. Siap untuk pesanan berikutnya")));
}

bool AJRGameMode::CanOfferMie(const AJRStation* S) const
{
	const AJRCustomer* C = S ? S->Customer.Get() : nullptr;
	return bCarryMie && C && C->State == EJRCustomerState::Playing && (C->bOrder || !C->bAte);
}

void AJRGameMode::DeliverMie(AJRStation* S)
{
	AJRCustomer* C = S ? S->Customer.Get() : nullptr;
	if (!C || !CanOfferMie(S)) return;
	bCarryMie = false;
	C->bOrder = false;
	C->bAte = true;
	C->Bill += MiePriceNow();
	TutorialAdvance(TutMie);
	Stats.MieSold++;
	CountLife(EJRLife::Mie);
	AddXp(1);
	JRSound::PlayAt(this, TEXT("SFX_Notif"), S->GetActorLocation(), 0.7f);
	AddToast(JRPrintf(JRT(TEXT("Mi diantar ke %s. +%s masuk tagihan")), *S->DisplayName, *JRFormatRp(MiePriceNow())), Good);
}

/* ---------- Kasir ---------- */

void AJRGameMode::SpawnTrash(const FVector& At, int32 Kind)
{
	if (Trash.Num() >= 30) return;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AJRTrash* T = GetWorld()->SpawnActor<AJRTrash>(At, FRotator::ZeroRotator, SP))
	{
		T->Setup(Kind);
		Trash.Add(T);
	}
}

void AJRGameMode::PickTrash(AJRTrash* T)
{
	if (!T) return;
	Trash.Remove(T);
	JRSound::PlayAt(this, TEXT("SFX_Kardus"), T->GetActorLocation(), 0.4f);
	T->Destroy();
}

void AJRGameMode::EndSession(AJRCustomer* C)
{
	// Pelanggan meninggalkan sampah di dekat tempat duduk
	if (C->Station && FMath::FRand() < (C->bAte ? 0.7f : 0.3f))
	{
		const FVector Seat = C->Station->GetSeatLocation();
		SpawnTrash(Seat + FVector(FMath::FRandRange(-45.f, 45.f), FMath::FRandRange(-45.f, 45.f), 1.f), C->bAte ? 0 : FMath::RandRange(1, 2));
	}
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

void AJRGameMode::CollectNextPayer(bool bByPlayer)
{
	CollectPayer(NextPayer(), bByPlayer);
}

void AJRGameMode::CollectPayer(AJRCustomer* C, bool bByPlayer)
{
	if (!C || C->State != EJRCustomerState::WaitPay) return;
	if (C->TypeIndex != Bocil && !C->bFined && FMath::FRand() < 0.07f)
	{
		const TCHAR* Excuses[] = { JRT(TEXT("Besok bayar, bang. Sumpah.")), JRT(TEXT("Gajian minggu depan, bang.")), JRT(TEXT("Dompet ketinggalan di motor, bang.")) };
		TWeakObjectPtr<AJRCustomer> WC(C);
		JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Ngutang"), C->GetActorLocation());
		PushChoice(JRT(TEXT("Bang, ngutang dulu ya")),
			JRPrintf(JRT(TEXT("\"%s\" Tagihannya %s.")), Excuses[FMath::RandRange(0, 2)], *JRFormatRp(C->Bill)),
			{ JRT(TEXT("Catat di buku utang")), JRT(TEXT("Tolak, harus bayar")) },
			{
				[this, WC]()
				{
					AJRCustomer* X = WC.Get();
					if (!X || X->State != EJRCustomerState::WaitPay) return;
					Debts.Add(X->Bill);
					Stats.Utang += X->Bill;
					AddToast(JRPrintf(JRT(TEXT("%s dicatat di buku utang")), *JRFormatRp(X->Bill)));
					FinishPayment(X, 0.f);
				},
				[this, WC]()
				{
					AJRCustomer* X = WC.Get();
					if (!X || X->State != EJRCustomerState::WaitPay) return;
					X->SatAdj -= 20.f;
					Stats.RepDelta -= 0.25f;
					FinishPayment(X, X->Bill);
				}
			});
		return;
	}
	// Mini-game kembalian pas: hanya saat pemain sendiri yang menagih (bukan karyawan kasir), di luar tutorial
	const bool bForceChange = FParse::Param(FCommandLine::Get(), TEXT("jrchange"));
	if (bByPlayer && Choices.Num() == 0 && !IsTutorial() && C->Bill > 0.f && (bForceChange || FMath::FRand() < 0.35f))
	{
		ChangeGame(C);
		return;
	}
	FinishPayment(C, C->Bill);
}

void AJRGameMode::FinishPayment(AJRCustomer* C, float Paid)
{
	Money += Paid;
	Stats.Income += Paid;
	Stats.Paid++;
	TutorialAdvance(TutPay);
	const int32 S = CalcSatisfaction(C);
	Stats.Satisfaction.Add(S);
	C->Mood = Paid <= 0.f ? TEXT(";)") : S >= 60 ? TEXT(":)") : S >= 35 ? TEXT(":|") : TEXT(">:(");
	AddXp(Paid <= 0.f ? 1 : C->Used == EJRUnitType::VIP ? 4 : 2);
	if (Paid > 0.f)
	{
		SellDrink();	// kulkas minuman di Game Center
		CountLife(EJRLife::Paid);
		JRSound::PlayAt(this, TEXT("SFX_Kaching"), Layout().Kasir() + FVector(0, 0, 110));
		if (FMath::FRand() < 0.5f) JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Bayar"), C->GetActorLocation());
		AddToast(JRPrintf(JRT(TEXT("+%s dari %s")), *JRFormatRp(Paid), *C->DisplayName), Good);
	}
	Leave(C);
}

int32 AJRGameMode::CalcSatisfaction(const AJRCustomer* C) const
{
	const float LvBonus = LevelPriceBonus(C->Used, C->UnitLevel);
	const float PriceRatio = C->bPaket ? 1.f : (PriceFor(C->Used) + LvBonus) / (FairFor(C->Used) + LvBonus);
	const float S = 75.f - 0.3f * FMath::Max(0.f, C->WaitMinutes - 10.f) - 12.f * C->Breakdowns - 40.f * (PriceRatio - 1.f)
		+ (C->bAte ? 5.f : 0.f) - 10.f * C->MissedOrders + C->SatAdj
		+ 6.f * SeatTier + (C->Used == EJRUnitType::PC && bFastPC ? 5.f : 0.f)	// tempat duduk nyaman, PC kencang
		+ DecorBonus()	// suasana ruangan
		+ 3.f * (C->UnitLevel - 1)	// unit yang diupgrade
		- TrashPenalty()	// ruangan kotor
		+ (C->Used == EJRUnitType::PC && Era >= 2 ? (NetPlan == 2 ? 5.f : NetPlan == 1 ? 2.f : 0.f) : 0.f)	// internet kencang
		+ DailySatBonus(C);	// rencana hari ini
	return FMath::RoundToInt(FMath::Clamp(S, 0.f, 100.f));
}

/* ---------- Listrik dan internet ---------- */

void AJRGameMode::ComputePower()
{
	int32 U = (Era >= 2 ? 250 : 100) + DecorVA(); // lampu (+ kulkas di ruko) + kipas/neon/AC
	if (!bMcbTripped)
	{
		for (const AJRStation* S : Stations)
		{
			if (S->Customer && S->Customer->State == EJRCustomerState::Playing && !S->bBroken)
			{
				U += S->Type == EJRUnitType::PS ? (bPS2 ? 180 : 150) : S->Type == EJRUnitType::VIP ? 400 : 300;
			}
		}
	}
	if (!bMcbTripped && StoveState == EJRStoveState::Cooking) U += 600; // kompor listrik
	PowerVA = U;
	if (!bMcbTripped && U > CapVA && Plan != EJRPlan::Genset) Trip();	// genset sewaan menanggung kelebihan daya
}

void AJRGameMode::Trip()
{
	Stats.Trips++;
	bMcbTripped = true;
	AddMoment(EJRMoment::Trip);
	if (StoveState == EJRStoveState::Cooking)
	{
		StoveState = EJRStoveState::Idle;
		AddToast(JRT(TEXT("Mi di kompor gagal matang karena listrik mati")), Bad);
	}
	for (AJRCustomer* C : Customers)
	{
		if (C->State == EJRCustomerState::Playing) C->Breakdowns++;
	}
	JRSound::Play2D(this, TEXT("SFX_MCB_Turun"));
	AddToast(JRT(TEXT("JEGLEK! Daya lewat batas, MCB turun. Naikkan MCB di dinding kiri.")), Bad);
}

void AJRGameMode::ResetMcb()
{
	if (!bMcbTripped) return;
	bMcbTripped = false;
	JRSound::Play2D(this, TEXT("SFX_MCB_Naik"));
	AddToast(JRT(TEXT("Listrik menyala lagi")), Good);
}

void AJRGameMode::RestartRouter()
{
	if (!bNetSlow) return;
	bNetSlow = false;
	JRSound::Play2D(this, TEXT("SFX_Notif"), 0.7f);
	AddToast(JRT(TEXT("Internet lancar lagi")), Good);
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
	for (AJRStation* S : Stations) { S->Customer = nullptr; S->bHaunted = false; }
	if (Room) Room->SetLightsOn(true);
	FlickerTime = 0.f;
	bMcbTripped = false;
	bNetSlow = false;
	StoveState = EJRStoveState::Idle;
	bCarryMie = false;
	if (!CarryBox.IsNone()) { SpawnDelivery(CarryBox); CarryBox = NAME_None; }
	ClearWalkIns();
	Choices.Reset();
	TickMissions(true);
	const FJRLayout& L = Layout();
	const int32 Wages = TotalWages();
	const float Net = InternetToday();
	const int32 Maint = Maintenance();
	Money -= Stats.Elec + L.Rent + Net + Wages + Maint;

	float Avg = 50.f;
	if (Stats.Satisfaction.Num() > 0)
	{
		float Sum = 0.f;
		for (int32 V : Stats.Satisfaction) Sum += V;
		Avg = Sum / Stats.Satisfaction.Num();
	}
	const float OldRep = Rep;
	// Pelayanan baik (kepuasan ~60) menaikkan reputasi sekitar +2 per hari.
	// + dari mulut ke mulut: tiap pelanggan yang dilayani sampai bayar menambah sedikit reputasi.
	Rep = FMath::Clamp(Rep + (Avg - 50.f) / 5.f + 0.04f * Stats.Paid + Stats.RepDelta, 0.f, 100.f);

	ReportLines.Reset();
	ReportLines.Add(JRPrintf(JRT(TEXT("Pelanggan datang: %d")), Stats.Customers));
	if (ViralDays > 0 && --ViralDays == 0) ReportLines.Add(JRT(TEXT("Efek viral dari streamer mulai reda.")));
	ReportLines.Add(JRPrintf(JRT(TEXT("Pemasukan sewa + jajan: %s")), *JRFormatRp(Stats.Income)));
	ReportLines.Add(JRPrintf(JRT(TEXT("Belanja bahan kantin: %s")), *JRFormatRp(-Stats.Stock)));
	ReportLines.Add(JRPrintf(JRT(TEXT("Tagihan listrik: %s")), *JRFormatRp(-Stats.Elec)));
	if (Era >= 2)
	{
		ReportLines.Add(JRPrintf(JRT(TEXT("Sewa ruko: %s   Internet %s: %s")), *JRFormatRp(-L.Rent), NetName(NetPlan), *JRFormatRp(-Net)));
	}
	if (Wages > 0) ReportLines.Add(JRPrintf(JRT(TEXT("Gaji karyawan: %s")), *JRFormatRp(-Wages)));
	if (Maint > 0) ReportLines.Add(JRPrintf(JRT(TEXT("Perawatan unit upgrade: %s")), *JRFormatRp(-Maint)));
	if (Stats.Side > 0.f)
	{
		ReportLines.Add(JRPrintf(JRT(TEXT("Bisnis sampingan: +%s (minuman %d, top-up %d, print %d)")), *JRFormatRp(Stats.Side), Stats.DrinksSold, Stats.TopUps, Stats.Prints));
	}
	if (Stats.WalkInLost > 0) ReportLines.Add(JRPrintf(JRT(TEXT("Pembeli konter/printer yang pergi kecewa: %d")), Stats.WalkInLost));
	if (Stats.Bonus > 0.f) ReportLines.Add(JRPrintf(JRT(TEXT("Hadiah misi: +%s")), *JRFormatRp(Stats.Bonus)));
	if (TourPlayers > 0) ReportLines.Add(JRPrintf(JRT(TEXT("Turnamen: %d peserta, biaya daftar %s")), TourPlayers, *JRFormatRp(TourPlayers * TourFee)));
	if (Era >= 2 && (Day + 1) % 7 == 6) ReportLines.Add(JRT(TEXT("Besok akhir pekan: kamu bisa mengadakan turnamen PS.")));
	if (Trash.Num() > 0)
	{
		Stats.RepDelta -= 0.15f * Trash.Num();
		ReportLines.Add(JRPrintf(JRT(TEXT("Sampah berserakan: %d. Warga menilai rental kotor.")), Trash.Num()));
	}
	ReportLines.Add(JRPrintf(JRT(TEXT("Untung bersih: %s")), *JRFormatRp(Stats.Income - Stats.Elec - Stats.Stock - L.Rent - Net - Wages - Maint)));
	ReportLines.Add(JRPrintf(JRT(TEXT("Kabur tanpa bayar: %s")), *JRFormatRp(Stats.Kabur)));
	ReportLines.Add(JRPrintf(JRT(TEXT("Utang baru dicatat: %s (total piutang %s)")), *JRFormatRp(Stats.Utang), *JRFormatRp(TotalDebt())));
	if (PendingOrders.Num() > 0) ReportLines.Add(JRPrintf(JRT(TEXT("Pesanan diantar kurir besok pagi: %d kardus")), PendingOrders.Num()));
	ReportLines.Add(JRPrintf(JRT(TEXT("Kepuasan rata-rata: %d/100")), FMath::RoundToInt(Avg)));
	ReportLines.Add(JRPrintf(JRT(TEXT("Reputasi: %d -> %d")), FMath::RoundToInt(OldRep), FMath::RoundToInt(Rep)));
	LastNet = Stats.Income - Stats.Elec - Stats.Stock - L.Rent - Net - Wages - Maint;
	LastAvgSat = Avg;
	MakeNetizenPosts(Avg);
	bShowMoment = true;
	{
		const int32 Lv = JuraganLevel();
		ReportLines.Add(Lv >= MaxJuraganLevel ? JRPrintf(JRT(TEXT("Level Juragan %d (maksimal). XP hari ini +%d")), Lv, Stats.Xp)
			: JRPrintf(JRT(TEXT("Level Juragan %d: XP %d/%d (hari ini +%d)")), Lv, Xp, XpForLevel(Lv + 1), Stats.Xp));
	}

	if (News != EJRNews::None || Plan != EJRPlan::None)
	{
		ReportLines.Add(JRPrintf(JRT(TEXT("Kabar hari ini: %s. Rencana: %s.")), *NewsTitle(News), Plan != EJRPlan::None ? *PlanTitle(Plan) : JRT(TEXT("tidak ada"))));
	}

	// Berita besok (Era 2): game hype
	if (!HypeName.IsEmpty() && --HypeDays <= 0)
	{
		ReportLines.Add(JRPrintf(JRT(TEXT("Demam %s mulai reda.")), *HypeName));
		HypeName.Reset();
		bHypeInstalled = false;
	}
	if (Era >= 2 && HypeName.IsEmpty() && FMath::FRand() < 0.5f)
	{
		HypeName = HypeGames[FMath::RandRange(0, UE_ARRAY_COUNT(HypeGames) - 1)];
		HypeDays = 3;
		bHypeInstalled = false;
		ReportLines.Add(JRPrintf(JRT(TEXT("Besok game baru hype: %s. Install di semua PC lewat HP.")), *HypeName));
	}

	if (Era == 1)
	{
		ReportLines.Add(Gate1Open()
			? FString(JRT(TEXT("Gerbang 1 terbuka! Buka HP > Toko untuk pindah ke ruko kecil.")))
			: JRPrintf(JRT(TEXT("Target Gerbang 1: Rp 600.000 dan reputasi 40. Sekarang %s, reputasi %d.")), *JRFormatRp(Money), FMath::RoundToInt(Rep)));
	}
	else
	{
		const int32 Units = TotalUnits();
		if (Era >= 3)
		{
			const bool bWin = Money >= 10000000.f && Rep >= 85.f;
			ReportLines.Add(bWin ? FString(JRT(TEXT("SELAMAT! Kamu resmi jadi JURAGAN SEJATI. Game Center-mu jadi legenda kampung!")))
				: JRPrintf(JRT(TEXT("Target akhir Juragan Sejati: Rp 10 juta dan reputasi 85. Sekarang %s, reputasi %d.")), *JRFormatRp(Money), FMath::RoundToInt(Rep)));
		}
		else ReportLines.Add(Money >= 3000000.f && Rep >= 60.f && Units >= 10
			? FString(JRT(TEXT("Gerbang 2 terbuka! Buka HP > Toko untuk pindah ke Game Center 2 lantai.")))
			: JRPrintf(JRT(TEXT("Target Gerbang 2: Rp 3 juta, reputasi 60, 10 unit. Sekarang %s, reputasi %d, %d unit.")), *JRFormatRp(Money), FMath::RoundToInt(Rep), Units));
	}
	if (IsTutorial()) SkipTutorial();
	CheckBadges(true);
	CheckEndings();
	SaveProgress();

	CheckDemoOver();
}

/* ---------- Toko (HP), kurir ---------- */

TArray<FJRShopItem> AJRGameMode::GetShopItems() const
{
	TArray<FJRShopItem> Out;
	const int32 Max = Layout().MaxUnits();
	const int32 PipePS = InPipeline(TEXT("ps")) + InPipeline(TEXT("psbekas"));
	const int32 PipePC = InPipeline(TEXT("pc")) + InPipeline(TEXT("pcbekas"));
	const bool bFullPS = Count(EJRUnitType::PS) + PipePS >= Max;
	const bool bFullPC = Count(EJRUnitType::PC) + PipePC >= Max;
	// Daya saat semua unit (termasuk yang sudah dipesan) menyala bersamaan, tanpa kompor.
	const int32 VaPS = bPS2 ? 180 : 150, VaPC = 300;
	const int32 FullLoad = (Era >= 2 ? 250 : 100) + DecorVA()
		+ (Count(EJRUnitType::PS) + PipePS) * VaPS + (Count(EJRUnitType::PC) + PipePC) * VaPC
		+ (Count(EJRUnitType::VIP) + InPipeline(TEXT("vip"))) * 400;
	const int32 CapSoon = InPipeline(TEXT("daya2")) > 0 ? 4400 : InPipeline(TEXT("daya")) > 0 ? 2200 : CapVA;
	const FString PowerLockPS = FullLoad + VaPS > CapSoon ? JRPrintf(JRT(TEXT("Daya kurang (%d/%d VA). Tambah daya dulu")), FullLoad + VaPS, CapSoon) : FString();
	const FString PowerLockPC = FullLoad + VaPC > CapSoon ? JRPrintf(JRT(TEXT("Daya kurang (%d/%d VA). Tambah daya dulu")), FullLoad + VaPC, CapSoon) : FString();
	auto Once = [this](FName Id, bool bOwned) { return bOwned ? JRT(TEXT("Sudah dibeli")) : InPipeline(Id) > 0 ? JRT(TEXT("Sudah dipesan")) : TEXT(""); };
	auto Done = [](bool bOwned) { return bOwned ? JRT(TEXT("Sudah dibeli")) : TEXT(""); };

	if (Era == 1)
	{
		Out.Add({ TEXT("move"), JRT(TEXT("Pindah ke Ruko Kecil")),
			JRT(TEXT("Muat 6 TV dan 6 PC, daya 2.200 VA, router internet. Sewa Rp 75.000/hari + internet Rp 25.000/hari.")), 450000,
			bDemo ? JRT(TEXT("Tersedia di versi lengkap")) : !Gate1Open() ? JRT(TEXT("Syarat: kas Rp 600.000 dan reputasi 40")) : Phase == EJRGamePhase::Playing ? JRT(TEXT("Pindahan saat toko tutup")) : TEXT("") });
	}
	Out.Add({ TEXT("ps"), bPS2 ? JRT(TEXT("TV 21\" + PlayStasiun 2")) : JRT(TEXT("TV tabung + PlayStasiun 1")), JRT(TEXT("Tambah 1 unit rental. Diantar kurir besok pagi.")), bPS2 ? 140000 : 90000, bFullPS ? FString(JRT(TEXT("Tempat penuh"))) : PowerLockPS });
	Out.Add({ TEXT("pc"), JRT(TEXT("PC warnet")), JRT(TEXT("Tambah 1 bilik PC. Diantar kurir besok pagi.")), 250000, bFullPC ? FString(JRT(TEXT("Tempat penuh"))) : PowerLockPC });
	Out.Add({ TEXT("stik"), JRT(TEXT("Stik original")), JRT(TEXT("Stik jauh lebih jarang rusak.")), 50000, Once(TEXT("stik"), bStikOri) });
	if (!LoakOffer.IsNone())
	{
		const bool bPC = LoakOffer == TEXT("pcbekas");
		Out.Insert({ LoakOffer, bPC ? JRT(TEXT("LOAK: PC warnet bekas")) : JRT(TEXT("LOAK: TV + PlayStasiun 1 bekas")),
			JRT(TEXT("Murah, tapi lebih sering rusak. Tawaran hanya hari ini!")), bPC ? 140000 : 50000,
			bPC ? (bFullPC ? FString(JRT(TEXT("Tempat penuh"))) : PowerLockPC) : (bFullPS ? FString(JRT(TEXT("Tempat penuh"))) : PowerLockPS) }, 0);
	}
	if (Era == 2)
	{
		Out.Add({ TEXT("move3"), JRT(TEXT("Pindah ke Game Center 2 lantai")),
			JRT(TEXT("Lantai 1: 8 TV dan 8 PC. Lantai 2: ruang VIP sampai 6 booth. Daya 4.400 VA. Sewa Rp 150.000/hari.")), 1500000,
			bDemo ? FString(JRT(TEXT("Tersedia di versi lengkap"))) : !Gate2Open() ? FString(JRT(TEXT("Syarat: kas Rp 3 juta, reputasi 60, dan 10 unit")))
				: Phase == EJRGamePhase::Playing ? FString(JRT(TEXT("Pindahan saat toko tutup"))) : FString() });
	}
	if (Era >= 3)
	{
		const bool bFullVip = Count(EJRUnitType::VIP) + InPipeline(TEXT("vip")) >= Layout().MaxVip();
		Out.Add({ TEXT("vip"), JRT(TEXT("Booth VIP lantai 2")), JRT(TEXT("Sofa, TV 32 inci, dan PlayStasiun 3. Tarif wajar Rp 8.000/jam. Daya 400 VA.")), 400000,
			bFullVip ? FString(JRT(TEXT("Tempat penuh"))) : FullLoad + 400 > CapSoon ? JRPrintf(JRT(TEXT("Daya kurang (%d/%d VA). Tambah daya dulu")), FullLoad + 400, CapSoon) : FString() });

		// Bisnis sampingan: kulkas minuman, konter pulsa, printer. Pemasangan langsung, isi ulang lewat kurir/saldo.
		const FString Installed = JRT(TEXT("Sudah dipasang"));
		Out.Add({ TEXT("kulkas"), JRT(TEXT("Kulkas minuman")),
			JRT(TEXT("Pelanggan membeli es teh dan soda (Rp 5.000) saat bayar di kasir. Langsung terpasang, isi awal 24 botol.")), 350000, bKulkas ? Installed : FString() });
		if (bKulkas)
		{
			Out.Add({ TEXT("krat"), JRT(TEXT("Krat minuman (24 botol)")), JRT(TEXT("Modal Rp 2.500 per botol. Diantar kurir besok pagi, lalu bawa ke kulkas.")), 60000,
				Drinks + InPipeline(TEXT("krat")) * KratSize >= MaxDrinks ? FString(JRT(TEXT("Kulkas sudah penuh"))) : FString() });
		}
		Out.Add({ TEXT("konter"), JRT(TEXT("Konter pulsa & top-up")),
			JRT(TEXT("Warga datang beli pulsa dan top-up game. Untung biaya admin Rp 2.000-3.000 per transaksi. Butuh saldo.")), 500000, bKonter ? Installed : FString() });
		if (bKonter)
		{
			Out.Add({ TEXT("saldo"), JRT(TEXT("Isi saldo konter Rp 200.000")), JRT(TEXT("Saldo terpakai tiap top-up dan kembali jadi uang tunai saat pembeli membayar.")), 200000, FString() });
		}
		Out.Add({ TEXT("printer"), JRT(TEXT("Printer & fotokopi")),
			JRT(TEXT("Anak sekolah dan mahasiswa datang mencetak tugas: Rp 500 per lembar. Isi awal 1 rim kertas.")), 600000, bPrinter ? Installed : FString() });
		if (bPrinter)
		{
			Out.Add({ TEXT("kertas"), JRT(TEXT("Kertas HVS 1 rim (500 lembar)")), JRT(TEXT("Modal Rp 100 per lembar. Diantar kurir besok pagi, lalu bawa ke printer.")), 50000,
				Paper + InPipeline(TEXT("kertas")) * RimSize >= MaxPaper ? FString(JRT(TEXT("Kertas sudah penuh"))) : FString() });
		}
	}
	if (Era >= 2)
	{
		Out.Add({ TEXT("billing"), JRT(TEXT("Sistem billing warnet")),
			JRT(TEXT("Komputer di samping kasir: lihat semua unit dan nyalakan TV, PC, dan VIP dari satu tempat.")), 300000,
			bBilling ? FString(JRT(TEXT("Sudah dipasang"))) : FString() });
	}
	auto DecorItem = [this, &Out](FName Id, const TCHAR* Name, const TCHAR* Desc, int32 Cost)
	{
		Out.Add({ Id, Name, Desc, Cost, Decor.Contains(Id) ? FString(JRT(TEXT("Sudah dipasang"))) : FString() });
	};
	DecorItem(TEXT("poster"), JRT(TEXT("Dekor: poster game")), JRT(TEXT("Dua poster di dinding belakang. Kepuasan +2.")), 30000);
	DecorItem(TEXT("tanaman"), JRT(TEXT("Dekor: tanaman pot")), JRT(TEXT("Tiga pot tanaman dekat pintu. Kepuasan +2.")), 40000);
	DecorItem(TEXT("kipas"), JRT(TEXT("Dekor: kipas angin berdiri")), JRT(TEXT("Dua kipas, ruangan tidak gerah. Kepuasan +3, daya 100 VA.")), 60000);
	DecorItem(TEXT("neon"), JRT(TEXT("Dekor: lampu neon RGB")), JRT(TEXT("Lis lampu ungu-biru ala game center. Kepuasan +3, daya 30 VA.")), 80000);
	if (Era >= 2)
	{
		DecorItem(TEXT("ac"), JRT(TEXT("Dekor: AC split")), JRT(TEXT("Sejuk seperti warnet kota. Kepuasan +8, daya 600 VA.")), 400000);		if (!Decor.Contains(TEXT("ac")) && FullLoad + 600 > CapSoon) Out.Last().Lock = JRPrintf(JRT(TEXT("Daya kurang (%d/%d VA). Tambah daya dulu")), FullLoad + 600, CapSoon);
	}
	Out.Add({ TEXT("duduk1"), JRT(TEXT("Bantal & kursi busa")), JRT(TEXT("Semua unit lebih nyaman: kepuasan pelanggan +6.")), 100000, Done(SeatTier >= 1) });
	if (Era == 1)
	{
		Out.Add({ TEXT("daya"), JRT(TEXT("Tambah daya ke 2.200 VA")), JRT(TEXT("MCB tidak gampang turun.")), 150000, Once(TEXT("daya"), CapVA >= 2200) });
	}
	else
	{
		Out.Add({ TEXT("daya2"), JRT(TEXT("Tambah daya ke 4.400 VA")), JRT(TEXT("Cukup untuk ruko penuh + kompor.")), 350000, Once(TEXT("daya2"), CapVA >= 4400) });
		Out.Add({ TEXT("ps2"), JRT(TEXT("PlayStasiun 2 untuk semua TV")), JRT(TEXT("Harga wajar rental PS naik ke Rp 3.000/jam. 180 VA per unit.")), 90000 * FMath::Max(1, Count(EJRUnitType::PS)), Done(bPS2) });
		Out.Add({ TEXT("duduk2"), JRT(TEXT("Bean bag & kursi gaming")), JRT(TEXT("Kepuasan pelanggan +12 (menggantikan busa).")), 250000,
			SeatTier >= 2 ? FString(JRT(TEXT("Sudah dibeli"))) : SeatTier < 1 ? FString(JRT(TEXT("Beli bantal & kursi busa dulu"))) : FString() });
		Out.Add({ TEXT("pcfast"), JRT(TEXT("Upgrade PC: RAM + VGA")), JRT(TEXT("Game lancar: tarif wajar PC naik ke Rp 4.000/jam, kepuasan pemain PC +5.")), 80000 * FMath::Max(1, Count(EJRUnitType::PC)), Done(bFastPC) });
		Out.Add({ TEXT("av"), JRT(TEXT("Antivirus berlisensi")), JRT(TEXT("PC yang hang jarang kena virus.")), 80000, Done(bAntivirus) });
		if (!HypeName.IsEmpty())
		{
			Out.Add({ TEXT("hype"), JRPrintf(JRT(TEXT("Install %s di semua PC")), *HypeName), JRT(TEXT("Gamer online datang lebih banyak selama game ini hype.")), 60000, Done(bHypeInstalled) });
		}
	}
	// Level Juragan: barang yang belum dimiliki baru bisa dibeli setelah levelnya cukup.
	const int32 Lv = JuraganLevel();
	for (FJRShopItem& It : Out)
	{
		const int32 Need = ItemMinLevel(It.Id);
		if (It.Lock.IsEmpty() && Lv < Need) It.Lock = JRPrintf(JRT(TEXT("Butuh Level Juragan %d")), Need);
	}
	return Out;
}

FString AJRGameMode::ItemName(FName Id) const
{
	if (Id == TEXT("psbekas")) return JRT(TEXT("TV + PS1 bekas"));
	if (Id == TEXT("pcbekas")) return JRT(TEXT("PC warnet bekas"));
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
	if (Phase == EJRGamePhase::Intro || Phase == EJRGamePhase::Menu) return false;
	for (const FJRShopItem& It : GetShopItems())
	{
		if (It.Id != Id) continue;
		if (!It.Lock.IsEmpty() || Money < It.Cost) return false;
		Money -= It.Cost;
		// Layanan langsung berlaku; barang fisik diantar kurir besok pagi.
		if (Id == TEXT("move")) { MoveToEra2(); return true; }
		if (Id == TEXT("move3")) { MoveToEra(3); return true; }
		if (Id == TEXT("billing"))
		{
			bBilling = true;
			SpawnBillingPC();
			AddToast(JRT(TEXT("Komputer billing terpasang di samping kasir. Nyalakan unit dari sana!")), Good);
			return true;
		}
		if (Id == TEXT("kulkas") || Id == TEXT("konter") || Id == TEXT("printer"))
		{
			if (Id == TEXT("kulkas")) { bKulkas = true; Drinks = FMath::Max(Drinks, (int32)KratSize); }
			else if (Id == TEXT("konter")) bKonter = true;
			else { bPrinter = true; Paper = FMath::Max(Paper, (int32)RimSize); }
			SpawnServices();
			AddToast(Id == TEXT("konter") ? FString(JRT(TEXT("Konter pulsa terpasang di depan tangga. Isi saldo dulu lewat HP > Toko.")))
				: JRPrintf(JRT(TEXT("%s terpasang di depan tangga lantai 1.")), *It.Name), Good);
			return true;
		}
		if (Id == TEXT("saldo"))
		{
			Saldo += It.Cost;
			AddToast(JRPrintf(JRT(TEXT("Saldo konter sekarang %s")), *JRFormatRp(Saldo)), Good);
			return true;
		}
		if (Id == TEXT("krat") || Id == TEXT("kertas")) Stats.Stock += It.Cost;	// modal stok, tetap diantar kurir
		if (Id == TEXT("ps2")) { bPS2 = true; PricePS = FMath::Max(PricePS, 3000); }
		else if (Id == TEXT("duduk1") || Id == TEXT("duduk2"))
		{
			SeatTier = Id == TEXT("duduk1") ? 1 : 2;
			for (AJRStation* S : Stations) if (S) S->ApplySeatTier(SeatTier);
		}
		else if (Id == TEXT("pcfast")) { bFastPC = true; PricePC = FMath::Max(PricePC, 4000); }
		else if (Id == TEXT("poster") || Id == TEXT("tanaman") || Id == TEXT("kipas") || Id == TEXT("neon") || Id == TEXT("ac"))
		{
			Decor.AddUnique(Id);
			SpawnDecor();
		}
		else if (Id == TEXT("av")) bAntivirus = true;
		else if (Id == TEXT("net")) bFastNet = true;
		else if (Id == TEXT("hype")) bHypeInstalled = true;
		else
		{
			if (IsUsedItem(Id)) LoakOffer = NAME_None;
			PendingOrders.Add(Id);
			AddToast(JRPrintf(JRT(TEXT("Dipesan: %s (%s). Kurir datang besok pagi.")), *It.Name, *JRFormatRp(It.Cost)), Good);
			return true;
		}
		AddToast(JRPrintf(JRT(TEXT("Dibeli: %s (%s)")), *It.Name, *JRFormatRp(It.Cost)), Good);
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
		AddToast(JRT(TEXT("Stik original dibagikan ke semua TV")), Good);
	}
	else if (Id == TEXT("daya") || Id == TEXT("daya2"))
	{
		CapVA = Id == TEXT("daya") ? 2200 : 4400;
		AddToast(JRPrintf(JRT(TEXT("Petugas listrik datang. Daya naik ke %d VA")), CapVA), Good);
	}
	else if (Id == TEXT("krat") || Id == TEXT("kertas"))
	{
		CarryBox = Id;
		AddToast(Id == TEXT("krat") ? FString(JRT(TEXT("Bawa krat minuman ke kulkas di depan tangga"))) : FString(JRT(TEXT("Bawa kertas ke printer di depan tangga"))));
	}
	else
	{
		CarryBox = Id;
		AddToast(JRPrintf(JRT(TEXT("Bawa %s ke tempat bertanda hijau")), *ItemName(Id)));
	}
}

bool AJRGameMode::GetInstallSlot(FVector& OutLoc, FRotator& OutRot) const
{
	if (CarryBox.IsNone()) return false;
	if (CarryBox == TEXT("krat") || CarryBox == TEXT("kertas"))
	{
		// Penanda hijau di depan kulkas / printer
		const EJRService K = CarryBox == TEXT("krat") ? EJRService::Kulkas : EJRService::Printer;
		if (!HasService(K) || Era < 3) return false;
		OutLoc = ServiceSpot(K) - FVector(40.f, 0.f, 0.f);
		OutRot = FRotator(0.f, 90.f, 0.f);
		return true;
	}
	const EJRUnitType Type = TypeForItem(CarryBox);
	const int32 I = Count(Type);
	if (I >= MaxFor(Type)) return false;
	OutLoc = SlotLocation(Type, I, OutRot);
	return true;
}

void AJRGameMode::InstallBox()
{
	if (CarryBox.IsNone()) return;
	if (CarryBox == TEXT("krat") || CarryBox == TEXT("kertas"))
	{
		ServiceInteract(CarryBox == TEXT("krat") ? EJRService::Kulkas : EJRService::Printer);
		return;
	}
	const EJRUnitType Type = TypeForItem(CarryBox);
	if (Count(Type) >= MaxFor(Type)) return;
	SpawnStation(Type);
	if (IsUsedItem(CarryBox)) Stations.Last()->MarkUsed();
	JRSound::PlayAt(this, TEXT("SFX_Pasang"), Stations.Last()->GetActorLocation() + FVector(0, 0, 80));
	CarryBox = NAME_None;
	AddToast(JRPrintf(JRT(TEXT("%s terpasang dan siap dipakai")), *Stations.Last()->DisplayName), Good);
}

void AJRGameMode::AdjustPrice(EJRUnitType Type, int32 Delta)
{
	int32& Price = Type == EJRUnitType::PS ? PricePS : Type == EJRUnitType::VIP ? PriceVIP : PricePC;
	Price = Type == EJRUnitType::VIP ? FMath::Clamp(Price + Delta, 3000, 20000) : FMath::Clamp(Price + Delta, 1000, 8000);
}

void AJRGameMode::CheckDemoOver()
{
	if (!bDemo || !(Day >= DemoLastDay || (Era == 1 && Gate1Open()))) return;
	bDemoOver = true;
	DemoSummary = {
		JRPrintf(JRT(TEXT("Kamu bertahan %d hari sebagai juragan rental.")), Day),
		JRPrintf(JRT(TEXT("Kas akhir %s, reputasi %d, %d unit.")), *JRFormatRp(Money), FMath::RoundToInt(Rep), Stations.Num()),
		Gate1Open() ? FString(JRT(TEXT("Gerbang 1 terbuka! Ruko kecil, karyawan, dan cerita lanjutan menunggu di versi lengkap.")))
			: FString(JRT(TEXT("Di versi lengkap: pindah ke ruko, rekrut karyawan, internet fiber, dan misteri TV 1 sampai tamat."))),
	};
}

void AJRGameMode::Confirm()
{
	if (bDemoOver) return;	// layar akhir demo yang menentukan langkah berikutnya
	if (IsOpening()) { SkipOpening(); return; }	// LANJUT/Enter saat adegan pembuka = lewati adegan
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
	bShowMoment = false;
	Day++;
	Hour = 8.f;
	NextAutoSaveHour = 8.f + AutoSaveEveryHours;
	Stats = FJRDayStats();
	bQuiet = false;
	bFightToday = false;
	bTourAccepted = bTourRunning = bTourDone = false;
	TourPlayers = 0;
	if (IsTourDay() && !bBalance) OfferTournament();
	NewDailyMissions();
	if ((Day - 1) % 7 == 0 || !Missions.ContainsByPredicate([](const FJRMission& M) { return M.bWeekly; })) NewWeeklyMission();
	LoakOffer = NAME_None;
	if (Day >= 2 && FMath::FRand() < 0.35f)
	{
		LoakOffer = (Count(EJRUnitType::PC) > 0 && FMath::RandBool()) ? FName(TEXT("pcbekas")) : FName(TEXT("psbekas"));
		AddToast(JRPrintf(JRT(TEXT("Ada tawaran loak hari ini: %s. Cek HP > Toko.")), *ItemName(LoakOffer)));
	}
	if (PendingNetPlan >= 0)
	{
		NetPlan = PendingNetPlan;
		PendingNetPlan = -1;
		AddToast(JRPrintf(JRT(TEXT("Teknisi %s selesai memasang. Internet baru aktif!")), NetName(NetPlan)), Good);
	}
	// Tamu spesial: paling banyak satu per hari, masing-masing sekali seumur permainan.
	GuestAt = -1.f; GuestWhich = -1;
	{
		TArray<int32> Ready;
		if (!(GuestsDone & 1) && Rep >= 50.f) Ready.Add(0);						// streamer
		if (!(GuestsDone & 2) && Day >= 6) Ready.Add(1);							// Pak Lurah sidak
		if (!(GuestsDone & 4) && Day >= 8 && Stations.Num() >= 5) Ready.Add(2);	// juragan sebelah
		if (Ready.Num() > 0 && FMath::FRand() < 0.35f)
		{
			GuestWhich = Ready[FMath::RandRange(0, Ready.Num() - 1)];
			GuestAt = FMath::FRandRange(14.f, 19.f);
		}
	}
	RtAt = (Era >= 2 && FMath::FRand() < 0.5f) ? FMath::FRandRange(22.f, 23.5f) : -1.f;
	Phase = EJRGamePhase::Playing;
	RollMorning();	// kabar pagi + pilih rencana hari ini

	// Kurir datang
	if (PendingOrders.Num() > 0)
	{
		for (const FName& Id : PendingOrders) SpawnDelivery(Id);
		AddToast(JRPrintf(JRT(TEXT("Kurir datang! %d kardus di depan pintu.")), PendingOrders.Num()), Good);
		PendingOrders.Reset();
	}
	if (bHypeInstalled && !HypeName.IsEmpty()) AddToast(JRPrintf(JRT(TEXT("%s terpasang. Gamer online mulai antre!")), *HypeName), Good);

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
	if (Repaid > 0.f) AddToast(JRPrintf(JRT(TEXT("%d orang datang bayar utang plus bonus: +%s")), N, *JRFormatRp(Repaid)), Good);
	if (Lost > 0.f) AddToast(JRPrintf(JRT(TEXT("Utang %s sepertinya tidak akan kembali")), *JRFormatRp(Lost)), Bad);
}

void AJRGameMode::AddToast(const FString& Text, const FLinearColor& Color)
{
	Toasts.Insert({ Text, Color, 5.f }, 0);
	if (Toasts.Num() > 5) Toasts.SetNum(5);
}
