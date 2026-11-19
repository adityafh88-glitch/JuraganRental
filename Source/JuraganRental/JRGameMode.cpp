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
#include "JRRadio.h"
#include "JRStation.h"
#include "JRVisitor.h"
#include "JRSaveGame.h"
#include "JRMenuWidget.h"
#include "JRPhoneWidget.h"
#include "Kismet/GameplayStatics.h"

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
using namespace JRLocal_GameMode;

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
		Era = 2; Money = 1000000.f; Rep = 45.f; CapVA = 2900; OwnedPS = 4; OwnedPC = 3;
	}
	else if (bTest)
	{
		OwnedPS = 3; OwnedPC = 2; CapVA = 2200; // supaya kompor bisa jalan bersama 5 unit
	}
	BuildWorld();
	bMenuTest = FParse::Param(FCommandLine::Get(), TEXT("jrmenutest"));
	bBalance = FParse::Param(FCommandLine::Get(), TEXT("jrbalance"));
	bMysteryTest = FParse::Param(FCommandLine::Get(), TEXT("jrmysterytest"));
	if (bMysteryTest) { Phase = EJRGamePhase::Playing; Hour = 23.f; }
	if (bBalance)
	{
		Phase = EJRGamePhase::Playing;
		UGameplayStatics::SetGlobalTimeDilation(this, 6.f);
	}
	if (bMenuTest) SetTickableWhenPaused(true); // uji menu jeda tetap berjalan saat game dijeda
	if (!bTest && !bBalance && !bMysteryTest && !FParse::Param(FCommandLine::Get(), TEXT("jrera2"))) Phase = EJRGamePhase::Menu;

	if (bTest)
	{
		bAutoTest = true;
		Phase = EJRGamePhase::Playing;
		Hour = 13.5f;
		SpawnDelivery(TEXT("ps")); // uji kurir: kardus TV di depan pintu
		if (Era == 2) { HypeName = HypeGames[0]; HypeDays = 3; bHypeInstalled = true; }
		bStaffTest = Era == 2 && FParse::Param(FCommandLine::Get(), TEXT("jrstaff"));
		if (bStaffTest) { for (bool& B : Staff) B = true; SpawnStaffActors(); }
		if (Era == 2 && FParse::Param(FCommandLine::Get(), TEXT("jrnet"))) ChooseNetPlan(2);
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
			if (Era == 2) Decor.Add(TEXT("ac"));
			SpawnDecor();
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("jrtiers")))
		{
			SeatTier = Era == 2 ? 2 : 1;
			bFastPC = Era == 2;
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
	Room->Build(L.HalfX, L.HalfY, L.DoorY, Era == 2);
	Kasir = World->SpawnActor<AJRKasir>(L.Kasir(), FRotator::ZeroRotator, SP);
	Radio = World->SpawnActor<AJRRadio>(L.Kasir() + FVector(0, -6, 100), FRotator::ZeroRotator, SP); // di atas meja, antara laci dan buku utang
	Mcb = World->SpawnActor<AJRMcb>(L.Mcb(), FRotator(0.f, 90.f, 0.f), SP);
	Stove = World->SpawnActor<AJRStove>(L.Stove(), FRotator(0.f, 90.f, 0.f), SP);
	Router = L.bRouter ? World->SpawnActor<AJRRouter>(L.Router(), FRotator(0.f, 180.f, 0.f), SP) : nullptr;
	if (!SlotMarker) SlotMarker = World->SpawnActor<AJRSlotMarker>(FVector::ZeroVector, FRotator::ZeroRotator, SP);

	QueueUsed.Init(false, L.Queue.Num());
	for (int32 i = 0; i < OwnedPS; ++i) SpawnStation(EJRUnitType::PS);
	for (int32 i = 0; i < OwnedPC; ++i) SpawnStation(EJRUnitType::PC);
	SpawnStaffActors();
	SpawnDecor();
}

void AJRGameMode::SpawnDecor()
{
	for (AJRDecor* D : DecorActors) if (D) D->Destroy();
	DecorActors.Reset();
	const FJRLayout& L = Layout();
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const FName& Id : Decor)
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
	// Simpan jumlah unit dan isi kardus, lalu bongkar garasi.
	OwnedPS = Count(EJRUnitType::PS);
	OwnedPC = Count(EJRUnitType::PC);
	TArray<FName> BoxIds;
	for (AJRDeliveryBox* B : Boxes) if (B) BoxIds.Add(B->ItemId);
	ClearWorld();

	Era = 2;
	CapVA = FMath::Max(CapVA, 2900); // ruko: listrik bisnis
	BuildWorld();
	for (const FName& Id : BoxIds) SpawnDelivery(Id);

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn()) Pawn->SetActorLocation(Layout().PlayerStart(), false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(FRotator::ZeroRotator);
	}
	AddToast(TEXT("Selamat datang di ruko baru! Sewa dan internet ditagih tiap tutup toko."), Good);
}

void AJRGameMode::ClearWorld()
{
	for (AJRCustomer* C : Customers) if (C) C->Destroy();
	Customers.Reset();
	for (AJRDeliveryBox* B : Boxes) if (B) B->Destroy();
	Boxes.Reset();
	for (AJRStation* S : Stations) if (S) S->Destroy();
	Stations.Reset();
	for (AActor* A : TArray<AActor*>{ Room, Kasir, Radio, Mcb, Stove, Router }) if (A) A->Destroy();
	for (AJRVisitor* V : StaffActors) if (V) V->Destroy();
	StaffActors.Reset();
	for (AJRDecor* D : DecorActors) if (D) D->Destroy();
	DecorActors.Reset();
}

/* ---------- Provider internet ---------- */

const TCHAR* AJRGameMode::NetName(int32 Plan)
{
	switch (Plan) { case 1: return TEXT("KilatNet 20 Mbps"); case 2: return TEXT("FiberNusa 50 Mbps"); default: return TEXT("NetRakyat 5 Mbps"); }
}

int32 AJRGameMode::NetCost(int32 Plan)
{
	switch (Plan) { case 1: return 45000; case 2: return 80000; default: return 25000; }
}

const TCHAR* AJRGameMode::NetDesc(int32 Plan)
{
	switch (Plan)
	{
	case 1: return TEXT("Jarang lemot. Pemain PC sedikit lebih puas.");
	case 2: return TEXT("Hampir tidak pernah lemot. Pemain PC jauh lebih puas, gamer lebih sering datang.");
	default: return TEXT("Murah, tapi sering lemot saat ramai.");
	}
}

void AJRGameMode::ChooseNetPlan(int32 Plan)
{
	if (Era < 2) return;
	PendingNetPlan = Plan == NetPlan ? -1 : Plan;
	AddToast(PendingNetPlan < 0 ? FString::Printf(TEXT("Tetap pakai %s"), NetName(NetPlan))
		: FString::Printf(TEXT("Pindah ke %s mulai besok (%s/hari)"), NetName(Plan), *JRFormatRp(NetCost(Plan))));
}

/* ---------- Berantem dan tamu spesial ---------- */

void AJRGameMode::FightEvent(AJRCustomer* A, AJRCustomer* B)
{
	bFightToday = true;
	A->bRaging = true;
	B->bRaging = true;
	A->Mood = TEXT("!!");
	B->Mood = TEXT("!!");
	JRSound::PlayAt(this, TEXT("SFX_Stik_Banting"), A->GetActorLocation());
	JRSound::Voice(this, VoiceCode[A->TypeIndex], TEXT("Rage"), A->GetActorLocation(), true);
	TWeakObjectPtr<AJRCustomer> WA(A), WB(B);
	const FString Where = A->Station && B->Station ? FString::Printf(TEXT("%s dan %s"), *A->Station->DisplayName, *B->Station->DisplayName) : FString(TEXT("TV"));
	auto Calm = [WA, WB]() { for (AJRCustomer* X : { WA.Get(), WB.Get() }) if (X) { X->bRaging = false; X->Mood.Reset(); } };
	PushChoice(TEXT("Berantem!"),
		FString::Printf(TEXT("%s dan %s di %s saling dorong. \"Curang lu! Pakai jurus itu terus!\" Pelanggan lain mulai menonton."), *A->DisplayName, *B->DisplayName, *Where),
		{ TEXT("Lerai dan tegur"), TEXT("Usir keduanya (bayar setengah)"), TEXT("Biarkan saja") },
		{
			[this, WA, WB, Calm]()
			{
				Calm();
				for (AJRCustomer* X : { WA.Get(), WB.Get() }) if (X) X->SatAdj -= 5.f;
				AddToast(TEXT("Mereka salaman lalu lanjut main. Suasana kembali tenang."), Good);
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
				AddToast(TEXT("Keduanya diusir. Warga senang rental ini tertib. Reputasi +1"), Good);
			},
			[this, WA, Calm]()
			{
				Calm();
				if (AJRCustomer* X = WA.Get()) if (X->Station) X->Station->bBroken = true;
				for (AJRCustomer* X : Customers) if (X && X->State == EJRCustomerState::Playing) X->SatAdj -= 5.f;
				Stats.RepDelta -= 2.f;
				AddToast(TEXT("Stik terlempar dan rusak. Pelanggan lain terganggu. Reputasi -2"), Bad);
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
	JRSound::Play2D(this, TEXT("SFX_Bel_Pintu"), 0.8f);
	static const TCHAR* Mesh[] = { TEXT("SK_Gamer"), TEXT("SK_Bapak"), TEXT("SK_Kantoran") };
	TWeakObjectPtr<AJRVisitor> WV(SpawnMysteryVisitor(Mesh[Which], Inside));
	auto Bye = [WV, Door]() { if (WV.IsValid()) WV->LeaveTo(Door); };

	if (Which == 0)
	{
		PushChoice(TEXT("Tamu spesial: Bang Jago"),
			TEXT("Streamer game \"Bang Jago\" masuk sambil membawa ponsel di tongkat. \"Halo guys, kita live dari rental yang lagi rame dibicarain nih! Bang, boleh numpang main sejam?\""),
			{ TEXT("Silakan, gratis untuk Bang Jago"), TEXT("Boleh, tarif normal ya") },
			{
				[this, Bye]() { Bye(); ViralDays = 2; Stats.RepDelta += 3.f; AddToast(TEXT("Live-nya ditonton ribuan orang. Rentalmu VIRAL: pengunjung melonjak 2 hari! Reputasi +3"), Good); },
				[this, Bye]() { Bye(); Money += 10000.f; Stats.Income += 10000.f; Stats.RepDelta += 1.f; AddToast(TEXT("Bang Jago bayar Rp 10.000 dan menyebut rentalmu di live. Reputasi +1"), Good); }
			});
	}
	else if (Which == 1)
	{
		const bool bNice = DecorBonus() >= 4;
		PushChoice(TEXT("Pak Lurah sidak"),
			bNice ? TEXT("Pak Lurah keliling melihat-lihat. \"Wah, rapi dan nyaman. Ada tanaman, ada hiasan. Begini ini contoh usaha anak muda kampung kita!\"")
				: TEXT("Pak Lurah keliling melihat-lihat. \"Usahanya bagus, Mas, tapi ruangannya masih polos. Coba dikasih hiasan atau tanaman biar warga betah.\""),
			{ TEXT("Terima kasih, Pak Lurah") },
			{
				[this, Bye, bNice]()
				{
					Bye();
					Stats.RepDelta += bNice ? 4.f : 0.f;
					AddToast(bNice ? TEXT("Pak Lurah memuji rentalmu di grup warga. Reputasi +4") : TEXT("Saran Pak Lurah: beli dekorasi lewat HP > Toko."),
						bNice ? Good : FLinearColor(0.37f, 0.70f, 1.f));
				}
			});
	}
	else
	{
		const int32 Offer = 140000;
		PushChoice(TEXT("Juragan rental sebelah"),
			FString::Printf(TEXT("Juragan rental dari kampung sebelah mampir. \"Rame juga ya tempatmu. Begini, saya lagi butuh unit cepat. Satu TV-mu saya beli %s, tunai sekarang.\""), *JRFormatRp(Offer)),
			{ TEXT("Jual satu TV"), TEXT("Tolak, unitnya masih dipakai") },
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
							AddToast(FString::Printf(TEXT("Satu TV terjual %s. Tempatnya bisa diisi unit baru."), *JRFormatRp(Offer)), Good);
							return;
						}
					}
					AddToast(TEXT("Semua TV sedang dipakai. Juragan sebelah pulang dengan kecewa."));
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
	default: return 35000;
	}
}

const TCHAR* AJRGameMode::StaffName(EJRStaff S)
{
	switch (S)
	{
	case EJRStaff::Kasir: return TEXT("Mas Kasir");
	case EJRStaff::Teknisi: return TEXT("Teknisi");
	default: return TEXT("Koki Kantin");
	}
}

const TCHAR* AJRGameMode::StaffDesc(EJRStaff S)
{
	switch (S)
	{
	case EJRStaff::Kasir: return TEXT("Menagih pelanggan yang antre bayar, jadi tidak ada yang kabur.");
	case EJRStaff::Teknisi: return TEXT("Memperbaiki stik rusak, PC hang, dan virus.");
	default: return TEXT("Memasak dan mengantar mi pesanan (cek daya dulu).");
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
	AddToast(B ? FString::Printf(TEXT("%s mulai bekerja. Gaji %s/hari"), StaffName(S), *JRFormatRp(StaffWage(S)))
		: FString::Printf(TEXT("%s diberhentikan"), StaffName(S)), B ? Good : FLinearColor(0.37f, 0.70f, 1.f));
	SpawnStaffActors();
}

void AJRGameMode::SpawnStaffActors()
{
	for (AJRVisitor* V : StaffActors) if (V) V->Destroy();
	StaffActors.Reset();
	const FJRLayout& L = Layout();
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	struct FPost { EJRStaff Who; const TCHAR* Mesh; FVector Loc; float Yaw; };
	const FPost Posts[] = {
		{ EJRStaff::Kasir, TEXT("SK_Mahasiswa"), L.Kasir() + FVector(75.f, -30.f, 0.f), 180.f },			// di belakang meja kasir
		{ EJRStaff::Teknisi, TEXT("SK_Gamer"), FVector(L.HalfX - 120.f, 0.2f * L.HalfY, 0.f), 180.f },	// dekat dinding belakang
		{ EJRStaff::Koki, TEXT("SK_Bapak"), L.Stove() + FVector(0.f, 75.f, 0.f), -90.f },				// di depan kompor
	};
	for (const FPost& P : Posts)
	{
		if (!HasStaff(P.Who)) continue;
		if (AJRVisitor* V = GetWorld()->SpawnActor<AJRVisitor>(P.Loc, FRotator(0.f, P.Yaw, 0.f), SP))
		{
			V->Setup(P.Mesh, TEXT("A_Idle"));
			StaffActors.Add(V);
		}
	}
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
	// Teknisi: perbaiki unit rusak setelah 10 detik (virus 18 detik)
	if (HasStaff(EJRStaff::Teknisi))
	{
		for (AJRStation* S : Stations)
		{
			if (!S || !S->bBroken) { RepairTimers.Remove(S); continue; }
			float& T = RepairTimers.FindOrAdd(S);
			T += DeltaSeconds;
			if (T >= (S->bVirus ? 18.f : 10.f))
			{
				S->bBroken = false;
				S->bVirus = false;
				RepairTimers.Remove(S);
				StaffDone[1]++;
				AddToast(FString::Printf(TEXT("Teknisi membereskan %s"), *S->DisplayName), Good);
			}
		}
	}
	// Koki: masak lalu antar ke pemesan pertama
	if (HasStaff(EJRStaff::Koki) && !bCarryMie)
	{
		AJRStation* Orderer = nullptr;
		for (AJRStation* S : Stations) if (S && S->Customer && S->Customer->bOrder && S->Customer->State == EJRCustomerState::Playing) { Orderer = S; break; }
		float& T = StaffTimer[(int32)EJRStaff::Koki];
		if (!Orderer) T = 0.f;
		else if (StoveState == EJRStoveState::Idle && CarryBox.IsNone() && PowerVA + 600 <= CapVA) { T += DeltaSeconds; if (T >= 2.f) { T = 0.f; StartCook(); } }
		else if (StoveState == EJRStoveState::Ready)
		{
			T += DeltaSeconds;
			if (T >= 4.f)
			{
				T = 0.f;
				StoveState = EJRStoveState::Idle;
				AJRCustomer* C = Orderer->Customer;
				C->bOrder = false;
				C->bAte = true;
				C->Bill += MiePrice;
				StaffDone[2]++;
				JRSound::PlayAt(this, TEXT("SFX_Notif"), Orderer->GetActorLocation(), 0.7f);
				AddToast(FString::Printf(TEXT("Koki mengantar mi ke %s. +%s masuk tagihan"), *Orderer->DisplayName, *JRFormatRp(MiePrice)), Good);
			}
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
	if (bMysteryTest && MysteryStep >= 5 && Choices.Num() == 0)
	{
		MysteryTestTimer += DeltaSeconds;
		if (MysteryTestTimer > 1.5f)
		{
			UE_LOG(LogTemp, Display, TEXT("JR_MYSTERY_DONE step=%d gold=%d money=%.0f"), MysteryStep, bGoldCassette ? 1 : 0, Money);
			FGenericPlatformMisc::RequestExit(false);
		}
		return;
	}
	if (MysteryStep >= 5 || Choices.Num() > 0 || bAutoTest || bBalance) return;
	if (bMysteryTest)
	{
		MysteryTestTimer += DeltaSeconds;
		if (MysteryTestTimer < 3.f) return;
		MysteryTestTimer = 0.f;
		RunMysteryStep();
		return;
	}
	// Tiap bab punya jamnya sendiri, menjelang tutup.
	static const float StepHour[] = { 23.2f, 23.4f, 23.0f, 22.4f, 23.5f };
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
	const FString TvName = Tv1 ? Tv1->DisplayName : FString(TEXT("TV 1"));
	TWeakObjectPtr<AJRStation> WT(Tv1);

	switch (Step)
	{
	case 0:
	{
		FlickerTime = 2.5f;
		if (Tv1 && !Tv1->Customer) Tv1->bHaunted = true;
		if (Tv1) JRSound::PlayAt(this, TEXT("SFX_TV_Nyala"), Tv1->GetActorLocation() + FVector(0, 0, 100), 0.5f);
		PushChoice(FString::Printf(TEXT("%s menyala sendiri"), *TvName),
			FString::Printf(TEXT("Lampu berkedip. Tidak ada yang main, tapi layar %s menyala biru dan menampilkan tulisan \"LANJUTKAN?\" dengan logo game naga yang tidak kamu kenal."), *TvName),
			{ TEXT("Matikan TV-nya"), TEXT("Biarkan saja") },
			{
				[this, WT]() { if (WT.IsValid()) WT->bHaunted = false; AddToast(TEXT("TV dimatikan. Colokannya... memang sudah dicabut dari tadi?")); },
				[this, WT]() { AddToast(TEXT("Layar itu terus berkedip sampai toko tutup.")); }
			});
		break;
	}
	case 1:
	{
		TWeakObjectPtr<AJRVisitor> WV(SpawnMysteryVisitor(TEXT("SK_Mahasiswa"), Inside));
		PushChoice(TEXT("Pemuda berjaket"),
			FString::Printf(TEXT("Seorang pemuda berjaket lusuh berdiri di pintu. \"Bang, masih buka? Saya cuma mau cek save-an saya... di %s.\" Ia menyodorkan uang kertas seribu bergambar lama, tahun 1999."), *TvName),
			{ TEXT("Silakan, gratis saja"), TEXT("Maaf, sudah mau tutup") },
			{
				[this, WV, Door]()
				{
					if (WV.IsValid()) WV->LeaveTo(Door);
					Stats.RepDelta += 1.f;
					AddToast(TEXT("Ia hanya menatap layar sebentar, tersenyum, lalu pergi tanpa bicara."));
				},
				[this, WV, Door]()
				{
					if (WV.IsValid()) WV->LeaveTo(Door);
					AddToast(TEXT("Ia mengangguk pelan. Uang seribu lamanya tertinggal di meja kasir."));
				}
			});
		break;
	}
	case 2:
	{
		PushChoice(TEXT("Tulisan di buku utang"),
			FString::Printf(TEXT("Di halaman paling belakang buku utang ada tulisan pensil yang bukan tulisanmu:\n\"Bayu - %s - belum tamat - 1999\""), *TvName),
			{ TEXT("Tanya Pak RT besok"), TEXT("Tutup bukunya") },
			{
				[this]() { AddToast(TEXT("Pak RT pasti tahu sejarah tempat ini.")); },
				[this]() { AddToast(TEXT("Kamu menutup buku utang. Tulisan itu tidak bisa dihapus.")); }
			});
		break;
	}
	case 3:
	{
		TWeakObjectPtr<AJRVisitor> WR(SpawnMysteryVisitor(TEXT("SK_Pak_RT"), Inside));
		PushChoice(TEXT("Cerita Pak RT"),
			TEXT("\"Bayu? Wah, lama sekali itu. Dulu, sebelum jadi tempatmu, di sini ada rental PS kecil punya Pak Darto. Bayu itu bocah langganan, tiap malam main sampai jam dua belas. Waktu rentalnya tutup tahun '99, dia nangis karena game naganya belum tamat. Katanya sekarang sudah kerja di kota.\""),
			{ TEXT("Jadi dia masih hidup?"), TEXT("Terima kasih, Pak") },
			{
				[this, WR, Door]() { if (WR.IsValid()) WR->LeaveTo(Door); AddToast(TEXT("\"Ya masih, Mas! Memangnya kenapa?\" Pak RT tertawa sambil pulang.")); },
				[this, WR, Door]() { if (WR.IsValid()) WR->LeaveTo(Door); AddToast(TEXT("Pak RT pulang. Kamu melirik TV itu lagi.")); }
			});
		break;
	}
	default:
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
			AddToast(bStay ? TEXT("Kaset Emas dipajang di meja kasir. +Rp 50.000, reputasi +3")
				: TEXT("Paginya ada Kaset Emas dan amplop di depan pintu. +Rp 50.000, reputasi +3"), Good);
		};
		PushChoice(TEXT("Bayu kembali"),
			FString::Printf(TEXT("Pria berkemeja rapi masuk. \"Bang... saya Bayu. Dulu tiap malam saya main di sini, di %s. Boleh saya tamatin game itu sekali ini aja? Sudah dua puluh tahun lebih kepikiran.\""), *TvName),
			{ TEXT("Silakan, Mas"), TEXT("Maaf, sudah tutup") },
			{
				[this, WV, Door, Reward]()
				{
					if (WV.IsValid()) WV->LeaveTo(Door);
					Reward(true);
					PushChoice(TEXT("TAMAT"),
						TEXT("Layar menampilkan tulisan TAMAT. Bayu tertawa, matanya basah. Ia menaruh sebuah kaset berlapis emas di meja kasir: \"Buat rental ini. Terima kasih sudah menghidupkan tempat ini lagi.\""),
						{ TEXT("Sama-sama, Mas Bayu") }, { []() {} });
				},
				[WV, Door, Reward]()
				{
					if (WV.IsValid()) WV->LeaveTo(Door);
					Reward(false);
				}
			});
		break;
	}
	}

	if (bMysteryTest && MysteryStep >= 5)
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

bool AJRGameMode::HasSave()
{
	return UGameplayStatics::DoesSaveGameExist(SlotName(), 0);
}

FString AJRGameMode::SaveSummary()
{
	const UJRSaveGame* S = HasSave() ? Cast<UJRSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0)) : nullptr;
	if (!S) return FString();
	return FString::Printf(TEXT("Hari %d  -  %s  -  %s"), S->Day, S->Era == 2 ? TEXT("Ruko Kecil") : TEXT("Garasi"), *JRFormatRp(S->Money));
}

void AJRGameMode::SaveProgress()
{
	UJRSaveGame* S = Cast<UJRSaveGame>(UGameplayStatics::CreateSaveGameObject(UJRSaveGame::StaticClass()));
	S->SavedAt = FDateTime::Now();
	S->Era = Era; S->Day = Day; S->Money = Money; S->Rep = Rep; S->CapVA = CapVA;
	S->PricePS = PricePS; S->PricePC = PricePC;
	S->OwnedPS = Count(EJRUnitType::PS); S->OwnedPC = Count(EJRUnitType::PC);
	S->bStikOri = bStikOri; S->bPS2 = bPS2; S->bAntivirus = bAntivirus; S->bFastNet = bFastNet; S->bPaketMalam = bPaketMalam;
	S->HypeName = HypeName; S->HypeDays = HypeDays; S->bHypeInstalled = bHypeInstalled; S->Regulars = Regulars;
	S->Debts = Debts;
	S->PendingOrders = PendingOrders;
	for (const AJRDeliveryBox* B : Boxes) if (B) S->Boxes.Add(B->ItemId);
	S->ReportLines = ReportLines;
	S->MysteryStep = MysteryStep; S->MysteryDay = MysteryDay; S->bGoldCassette = bGoldCassette;
	for (bool B : Staff) S->Staff.Add(B);
	S->SeatTier = SeatTier; S->bFastPC = bFastPC; S->Decor = Decor;
	S->GuestsDone = GuestsDone; S->ViralDays = ViralDays;
	S->NetPlan = NetPlan; S->PendingNetPlan = PendingNetPlan;
	for (const AJRStation* X : Stations) if (X && X->bUsed) (X->Type == EJRUnitType::PS ? S->UsedPS : S->UsedPC)++;
	if (UGameplayStatics::SaveGameToSlot(S, SlotName(), 0))
	{
		ReportLines.Add(TEXT("Permainan tersimpan otomatis."));
	}
	else
	{
		AddToast(TEXT("Gagal menyimpan permainan!"), Bad);
	}
}

bool AJRGameMode::LoadProgress()
{
	const UJRSaveGame* S = HasSave() ? Cast<UJRSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0)) : nullptr;
	if (!S) return false;
	ClearWorld();
	Era = S->Era; Day = S->Day; Money = S->Money; Rep = S->Rep; CapVA = S->CapVA;
	PricePS = S->PricePS; PricePC = S->PricePC;
	OwnedPS = S->OwnedPS; OwnedPC = S->OwnedPC;
	bStikOri = S->bStikOri; bPS2 = S->bPS2; bAntivirus = S->bAntivirus; bFastNet = S->bFastNet; bPaketMalam = S->bPaketMalam;
	HypeName = S->HypeName; HypeDays = S->HypeDays; bHypeInstalled = S->bHypeInstalled; Regulars = S->Regulars;
	Debts = S->Debts;
	PendingOrders = S->PendingOrders;
	MysteryStep = S->MysteryStep; MysteryDay = S->MysteryDay; bGoldCassette = S->bGoldCassette;
	for (int32 i = 0; i < (int32)EJRStaff::Count; ++i) Staff[i] = S->Staff.IsValidIndex(i) && S->Staff[i];
	SeatTier = S->SeatTier; bFastPC = S->bFastPC; Decor = S->Decor;
	GuestsDone = S->GuestsDone; ViralDays = S->ViralDays;
	NetPlan = FMath::Max(S->NetPlan, bFastNet ? 1 : 0); PendingNetPlan = S->PendingNetPlan;
	CarryBox = NAME_None; bCarryMie = false; StoveState = EJRStoveState::Idle; bMcbTripped = false; bNetSlow = false;
	Choices.Reset();
	Hour = 24.f;
	BuildWorld();
	for (const FName& Id : S->Boxes) SpawnDelivery(Id);
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
	ReportLines = S->ReportLines;
	ReportLines.Remove(TEXT("Permainan tersimpan otomatis."));
	ReportLines.Insert(FString::Printf(TEXT("Permainan dimuat (disimpan %s)."), *S->SavedAt.ToString(TEXT("%d/%m/%Y %H:%M"))), 0);
	Phase = EJRGamePhase::Report;
	return true;
}

void AJRGameMode::BeginNewGame()
{
	Phase = EJRGamePhase::Intro;
}

/* ---------- Mode uji otomatis ---------- */

void AJRGameMode::TickAutoTest(float DeltaSeconds)
{
	AutoTestTime += DeltaSeconds;

	// Pemain otomatis: nyalakan unit, perbaiki, terima bayaran, naikkan MCB, restart router.
	for (AJRStation* S : Stations)
	{
		if (S->bBroken && !bStaffTest) { S->bBroken = false; S->bVirus = false; }
		if (S->Customer && S->Customer->State == EJRCustomerState::WaitStart) StartSession(S);
	}
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
	if (NextPayer() && FMath::FRand() < 0.02f && !bStaffTest) CollectNextPayer();
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
	// Uji karyawan: rusakkan satu unit dan selesaikan dua sesi agar teknisi dan kasir pasti bekerja.
	if (bStaffTest && AutoTestTime > 5.f && AutoTestTime - DeltaSeconds <= 5.f && Stations.Num() > 0) Stations[0]->bBroken = true;
	if (bStaffTest && ((AutoTestTime > 9.f && AutoTestTime - DeltaSeconds <= 9.f) || (AutoTestTime > 12.f && AutoTestTime - DeltaSeconds <= 12.f)))
	{
		for (AJRCustomer* C : Customers) if (C->State == EJRCustomerState::Playing && !C->bRaging) { EndSession(C); break; }
	}
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
			if (!Hud->IsPhoneOpen())
			{
				Hud->SetPhoneOpen(true);
				int32 Tab = 0;
				if (FParse::Value(FCommandLine::Get(), TEXT("jrphonetab="), Tab) && Hud->GetPhone()) Hud->GetPhone()->SetTab(Tab);
			}
			if (AutoTestTime > 28.5f)
			{
				const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoTest/JR_Shot_HP.png"));
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
			int32 Used = 0; FString Names;
			for (const AJRStation* X : Stations) if (X->bUsed) { Used++; Names += X->DisplayName + TEXT(";"); }
			UE_LOG(LogTemp, Display, TEXT("JR_LOAK used=%d %s"), Used, *Names);
		}
		UE_LOG(LogTemp, Display, TEXT("JR_STAFF kasir=%d teknisi=%d koki=%d wages=%d"), StaffDone[0], StaffDone[1], StaffDone[2], TotalWages());
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
	if (!CarryBox.IsNone()) { if (Chance(6.f)) InstallBox(); }
	else if (Boxes.Num() > 0 && !bCarryMie && Chance(6.f)) PickBox(Boxes[0]);
}

void AJRGameMode::BalanceBetweenDays(float DeltaSeconds)
{
	if (Phase == EJRGamePhase::Intro) { Phase = EJRGamePhase::Playing; return; }
	if (Phase != EJRGamePhase::Report) return;
	const int32 Units = Count(EJRUnitType::PS) + Count(EJRUnitType::PC);
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
		const bool bGate2 = Era == 2 && Money >= 3000000.f && Rep >= 60.f && Units >= 10;
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
	S->ApplySeatTier(SeatTier);
	Stations.Add(S);
}

/* ---------- Loop utama ---------- */

void AJRGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (FJRToast& T : Toasts) T.TimeLeft -= DeltaSeconds;
	Toasts.RemoveAll([](const FJRToast& T) { return T.TimeLeft <= 0.f; });

	// Mode uji: potret dialog pertama lalu pilih opsi pertama.
	if ((bAutoTest || bBalance || bMysteryTest) && Choices.Num() > 0)
	{
		AutoChoiceTimer += DeltaSeconds;
		if (bMysteryTest && AutoChoiceTimer > 0.8f && AutoChoiceTimer - DeltaSeconds <= 0.8f)
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / FString::Printf(TEXT("AutoTest/JR_Misteri_%d.png"), MysteryStep)), true, false);
		}
		if (bAutoTest && FParse::Param(FCommandLine::Get(), TEXT("jrguests")) && AutoChoiceTimer > 0.6f && AutoChoiceTimer - DeltaSeconds <= 0.6f)
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

	if (!IsPlaying()) return;
	if (bAutoTest) TickAutoTest(DeltaSeconds);
	TickMystery(DeltaSeconds);
	TickStaff(DeltaSeconds);
	if (bBalance) TickBalance(DeltaSeconds);

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
	static const float SlowPerHour[] = { 0.12f, 0.04f, 0.012f };
	if (Era == 2 && !bNetSlow && !bAutoTest && FMath::FRand() < SlowPerHour[FMath::Clamp(NetPlan, 0, 2)] * DtH)
	{
		bNetSlow = true;
		JRSound::Play2D(this, TEXT("SFX_Modem"), 0.6f);
		for (AJRCustomer* X : Customers)
		{
			if (X->State == EJRCustomerState::Playing && X->Used == EJRUnitType::PC && JRSound::Voice(this, VoiceCode[X->TypeIndex], TEXT("Lemot"), X->GetActorLocation())) break;
		}
		AddToast(TEXT("Internet lemot! Gamer mulai ngomel. Tahan E di router (dinding belakang) untuk restart."), Bad);
	}
	TickGuests();
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
	if (Era == 2 && bPaketMalam && H >= 21.f) Base *= 1.8f;
	if (bQuiet && H >= 22.f) Base *= 0.5f;
	const float RepMul = 0.5f + Rep / 100.f;
	const float PriceMul = FMath::Pow((float(FairFor(EJRUnitType::PS)) / PricePS + float(FairFor(EJRUnitType::PC)) / PricePC) / 2.f, 1.5f);
	float R = (ViralDays > 0 ? 1.5f : 1.f) * (1.f + DecorBonus() / 100.f) * Base * RepMul * PriceMul * FMath::Min(Era == 2 ? 2.6f : 1.8f, 0.55f + Stations.Num() * 0.18f);
	if (Era == 2)
	{
		R *= 1.3f * (1.f + 0.04f * Regulars); // ruko di jalan besar
		if (bHypeInstalled && !HypeName.IsEmpty()) R *= 1.3f;
		if (NetPlan == 2) R *= 1.1f; // gamer cari warnet fiber
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
	// Antrean sudah panjang: sebagian calon pelanggan lewat saja (tanpa mengurangi reputasi).
	int32 Waiting = 0;
	for (bool bUsed : QueueUsed) Waiting += bUsed ? 1 : 0;
	if (!FindFree(Want) && Waiting >= 2 && FMath::FRand() < 0.15f * Waiting) return;

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
	if (C->TypeIndex == Bocil && FMath::FRand() < (bAutoTest ? 1.f : 0.2f))
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

	const float BreakPerHour = (S->bUsed ? 1.6f : 1.f) * (S->Type == EJRUnitType::PS ? (bStikOri ? 0.07f : 0.22f) : 0.18f);
	if (FMath::FRand() < BreakPerHour * DtH)
	{
		S->bBroken = true;
		C->Breakdowns++;
		Stats.Breaks++;
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
	// Berantem: dua pemain PS bersebelahan (paling banyak sekali sehari)
	if (!bFightToday && S->Type == EJRUnitType::PS && C->TypeIndex != Bocil && !bBalance && !bAutoTest && Choices.Num() == 0
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
		if (S->Type == EJRUnitType::PS && C->TypeIndex != Bocil && FMath::FRand() < (bAutoTest ? 0.5f : 0.06f))
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
				Stats.RepDelta += 1.f;
				B->bOrder = false;
				B->Mood = TEXT("T_T");
				if (WE.IsValid()) JRSound::Voice(this, TEXT("Emak"), TEXT("Jewer"), WE->GetActorLocation(), true);
				JRSound::Voice(this, TEXT("Bocil"), TEXT("Ketahuan"), B->GetActorLocation(), true);
				AddToast(FString::Printf(TEXT("Bocil dijewer pulang. Emak bayar setengah: %s. Reputasi warga +1"), *JRFormatRp(Paid)), Good);
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
				AddToast(TEXT("Emak pergi sambil ngomel. Reputasi warga -1"), Bad);
			}
		});
}

void AJRGameMode::RageQuitEvent(AJRCustomer* C)
{
	AJRStation* S = C->Station;
	C->bRaging = true;
	if (S) S->bBroken = true;
	JRSound::PlayAt(this, TEXT("SFX_Stik_Banting"), C->GetActorLocation());
	JRSound::Voice(this, VoiceCode[C->TypeIndex], TEXT("Rage"), C->GetActorLocation(), true);
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
				Stats.RepDelta -= 1.f;
				AddToast(TEXT("Ganti rugi masuk tagihan. Reputasi -1"), Bad);
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
	JRSound::Voice(this, TEXT("Gamer"), TEXT("Joki"), C->GetActorLocation(), true);
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

void AJRGameMode::PutBackMie()
{
	if (!bCarryMie || StoveState != EJRStoveState::Idle) return;
	bCarryMie = false;
	StoveState = EJRStoveState::Ready;
	AddToast(TEXT("Mi ditaruh lagi di kompor. Siap untuk pesanan berikutnya"));
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
	if (C->TypeIndex != Bocil && !C->bFined && FMath::FRand() < 0.07f)
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
					Stats.RepDelta -= 0.25f;
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
	Stats.Paid++;
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
	const float S = 75.f - 0.3f * FMath::Max(0.f, C->WaitMinutes - 10.f) - 12.f * C->Breakdowns - 40.f * (PriceRatio - 1.f)
		+ (C->bAte ? 5.f : 0.f) - 10.f * C->MissedOrders + C->SatAdj
		+ 6.f * SeatTier + (C->Used == EJRUnitType::PC && bFastPC ? 5.f : 0.f)	// tempat duduk nyaman, PC kencang
		+ DecorBonus()	// suasana ruangan
		+ (C->Used == EJRUnitType::PC && Era == 2 ? (NetPlan == 2 ? 5.f : NetPlan == 1 ? 2.f : 0.f) : 0.f);	// internet kencang
	return FMath::RoundToInt(FMath::Clamp(S, 0.f, 100.f));
}

/* ---------- Listrik dan internet ---------- */

void AJRGameMode::ComputePower()
{
	int32 U = (Era == 2 ? 250 : 100) + DecorVA(); // lampu (+ kulkas di ruko) + kipas/neon/AC
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
	Stats.Trips++;
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
	for (AJRStation* S : Stations) { S->Customer = nullptr; S->bHaunted = false; }
	if (Room) Room->SetLightsOn(true);
	FlickerTime = 0.f;
	bMcbTripped = false;
	bNetSlow = false;
	StoveState = EJRStoveState::Idle;
	bCarryMie = false;
	if (!CarryBox.IsNone()) { SpawnDelivery(CarryBox); CarryBox = NAME_None; }
	Choices.Reset();
	const FJRLayout& L = Layout();
	const int32 Wages = TotalWages();
	const float Net = InternetToday();
	Money -= Stats.Elec + L.Rent + Net + Wages;

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
	ReportLines.Add(FString::Printf(TEXT("Pelanggan datang: %d"), Stats.Customers));
	if (ViralDays > 0 && --ViralDays == 0) ReportLines.Add(TEXT("Efek viral dari streamer mulai reda."));
	ReportLines.Add(FString::Printf(TEXT("Pemasukan sewa + jajan: %s"), *JRFormatRp(Stats.Income)));
	ReportLines.Add(FString::Printf(TEXT("Belanja bahan kantin: %s"), *JRFormatRp(-Stats.Stock)));
	ReportLines.Add(FString::Printf(TEXT("Tagihan listrik: %s"), *JRFormatRp(-Stats.Elec)));
	if (Era == 2)
	{
		ReportLines.Add(FString::Printf(TEXT("Sewa ruko: %s   Internet %s: %s"), *JRFormatRp(-L.Rent), NetName(NetPlan), *JRFormatRp(-Net)));
	}
	if (Wages > 0) ReportLines.Add(FString::Printf(TEXT("Gaji karyawan: %s"), *JRFormatRp(-Wages)));
	ReportLines.Add(FString::Printf(TEXT("Untung bersih: %s"), *JRFormatRp(Stats.Income - Stats.Elec - Stats.Stock - L.Rent - Net - Wages)));
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
			: FString::Printf(TEXT("Target Gerbang 1: Rp 600.000 dan reputasi 40. Sekarang %s, reputasi %d."), *JRFormatRp(Money), FMath::RoundToInt(Rep)));
	}
	else
	{
		const int32 Units = Count(EJRUnitType::PS) + Count(EJRUnitType::PC);
		ReportLines.Add(Money >= 3000000.f && Rep >= 60.f && Units >= 10
			? FString(TEXT("Gerbang 2 tercapai! Game Center 2 lantai menunggu di Era 3."))
			: FString::Printf(TEXT("Target Gerbang 2: Rp 3 juta, reputasi 60, 10 unit. Sekarang %s, reputasi %d, %d unit."), *JRFormatRp(Money), FMath::RoundToInt(Rep), Units));
	}
	SaveProgress();
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
	const int32 FullLoad = (Era == 2 ? 250 : 100) + DecorVA()
		+ (Count(EJRUnitType::PS) + PipePS) * VaPS + (Count(EJRUnitType::PC) + PipePC) * VaPC;
	const int32 CapSoon = InPipeline(TEXT("daya2")) > 0 ? 4400 : InPipeline(TEXT("daya")) > 0 ? 2200 : CapVA;
	const FString PowerLockPS = FullLoad + VaPS > CapSoon ? FString::Printf(TEXT("Daya kurang (%d/%d VA). Tambah daya dulu"), FullLoad + VaPS, CapSoon) : FString();
	const FString PowerLockPC = FullLoad + VaPC > CapSoon ? FString::Printf(TEXT("Daya kurang (%d/%d VA). Tambah daya dulu"), FullLoad + VaPC, CapSoon) : FString();
	auto Once = [this](FName Id, bool bOwned) { return bOwned ? TEXT("Sudah dibeli") : InPipeline(Id) > 0 ? TEXT("Sudah dipesan") : TEXT(""); };
	auto Done = [](bool bOwned) { return bOwned ? TEXT("Sudah dibeli") : TEXT(""); };

	if (Era == 1)
	{
		Out.Add({ TEXT("move"), TEXT("Pindah ke Ruko Kecil"),
			TEXT("Muat 6 TV dan 6 PC, daya 2.200 VA, router internet. Sewa Rp 75.000/hari + internet Rp 25.000/hari."), 450000,
			!Gate1Open() ? TEXT("Syarat: kas Rp 600.000 dan reputasi 40") : Phase == EJRGamePhase::Playing ? TEXT("Pindahan saat toko tutup") : TEXT("") });
	}
	Out.Add({ TEXT("ps"), bPS2 ? TEXT("TV 21\" + PlayStasiun 2") : TEXT("TV tabung + PlayStasiun 1"), TEXT("Tambah 1 unit rental. Diantar kurir besok pagi."), bPS2 ? 140000 : 90000, bFullPS ? FString(TEXT("Tempat penuh")) : PowerLockPS });
	Out.Add({ TEXT("pc"), TEXT("PC warnet"), TEXT("Tambah 1 bilik PC. Diantar kurir besok pagi."), 250000, bFullPC ? FString(TEXT("Tempat penuh")) : PowerLockPC });
	Out.Add({ TEXT("stik"), TEXT("Stik original"), TEXT("Stik jauh lebih jarang rusak."), 50000, Once(TEXT("stik"), bStikOri) });
	if (!LoakOffer.IsNone())
	{
		const bool bPC = LoakOffer == TEXT("pcbekas");
		Out.Insert({ LoakOffer, bPC ? TEXT("LOAK: PC warnet bekas") : TEXT("LOAK: TV + PlayStasiun 1 bekas"),
			TEXT("Murah, tapi lebih sering rusak. Tawaran hanya hari ini!"), bPC ? 140000 : 50000,
			bPC ? (bFullPC ? FString(TEXT("Tempat penuh")) : PowerLockPC) : (bFullPS ? FString(TEXT("Tempat penuh")) : PowerLockPS) }, 0);
	}
	auto DecorItem = [this, &Out](FName Id, const TCHAR* Name, const TCHAR* Desc, int32 Cost)
	{
		Out.Add({ Id, Name, Desc, Cost, Decor.Contains(Id) ? FString(TEXT("Sudah dipasang")) : FString() });
	};
	DecorItem(TEXT("poster"), TEXT("Dekor: poster game"), TEXT("Dua poster di dinding belakang. Kepuasan +2."), 30000);
	DecorItem(TEXT("tanaman"), TEXT("Dekor: tanaman pot"), TEXT("Tiga pot tanaman dekat pintu. Kepuasan +2."), 40000);
	DecorItem(TEXT("kipas"), TEXT("Dekor: kipas angin berdiri"), TEXT("Dua kipas, ruangan tidak gerah. Kepuasan +3, daya 100 VA."), 60000);
	DecorItem(TEXT("neon"), TEXT("Dekor: lampu neon RGB"), TEXT("Lis lampu ungu-biru ala game center. Kepuasan +3, daya 30 VA."), 80000);
	if (Era == 2)
	{
		DecorItem(TEXT("ac"), TEXT("Dekor: AC split"), TEXT("Sejuk seperti warnet kota. Kepuasan +8, daya 600 VA."), 400000);
		if (!Decor.Contains(TEXT("ac")) && FullLoad + 600 > CapSoon) Out.Last().Lock = FString::Printf(TEXT("Daya kurang (%d/%d VA). Tambah daya dulu"), FullLoad + 600, CapSoon);
	}
	Out.Add({ TEXT("duduk1"), TEXT("Bantal & kursi busa"), TEXT("Semua unit lebih nyaman: kepuasan pelanggan +6."), 100000, Done(SeatTier >= 1) });
	if (Era == 1)
	{
		Out.Add({ TEXT("daya"), TEXT("Tambah daya ke 2.200 VA"), TEXT("MCB tidak gampang turun."), 150000, Once(TEXT("daya"), CapVA >= 2200) });
	}
	else
	{
		Out.Add({ TEXT("daya2"), TEXT("Tambah daya ke 4.400 VA"), TEXT("Cukup untuk ruko penuh + kompor."), 350000, Once(TEXT("daya2"), CapVA >= 4400) });
		Out.Add({ TEXT("ps2"), TEXT("PlayStasiun 2 untuk semua TV"), TEXT("Harga wajar rental PS naik ke Rp 3.000/jam. 180 VA per unit."), 90000 * FMath::Max(1, Count(EJRUnitType::PS)), Done(bPS2) });
		Out.Add({ TEXT("duduk2"), TEXT("Bean bag & kursi gaming"), TEXT("Kepuasan pelanggan +12 (menggantikan busa)."), 250000,
			SeatTier >= 2 ? FString(TEXT("Sudah dibeli")) : SeatTier < 1 ? FString(TEXT("Beli bantal & kursi busa dulu")) : FString() });
		Out.Add({ TEXT("pcfast"), TEXT("Upgrade PC: RAM + VGA"), TEXT("Game lancar: tarif wajar PC naik ke Rp 4.000/jam, kepuasan pemain PC +5."), 80000 * FMath::Max(1, Count(EJRUnitType::PC)), Done(bFastPC) });
		Out.Add({ TEXT("av"), TEXT("Antivirus berlisensi"), TEXT("PC yang hang jarang kena virus."), 80000, Done(bAntivirus) });
		if (!HypeName.IsEmpty())
		{
			Out.Add({ TEXT("hype"), FString::Printf(TEXT("Install %s di semua PC"), *HypeName), TEXT("Gamer online datang lebih banyak selama game ini hype."), 60000, Done(bHypeInstalled) });
		}
	}
	return Out;
}

FString AJRGameMode::ItemName(FName Id) const
{
	if (Id == TEXT("psbekas")) return TEXT("TV + PS1 bekas");
	if (Id == TEXT("pcbekas")) return TEXT("PC warnet bekas");
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
		CapVA = Id == TEXT("daya") ? 2200 : 4400;
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
	const EJRUnitType Type = IsPCItem(CarryBox) ? EJRUnitType::PC : EJRUnitType::PS;
	const int32 I = Count(Type);
	if (I >= Layout().MaxUnits()) return false;
	OutLoc = SlotLocation(Type, I, OutRot);
	return true;
}

void AJRGameMode::InstallBox()
{
	if (CarryBox.IsNone()) return;
	const EJRUnitType Type = IsPCItem(CarryBox) ? EJRUnitType::PC : EJRUnitType::PS;
	if (Count(Type) >= Layout().MaxUnits()) return;
	SpawnStation(Type);
	if (IsUsedItem(CarryBox)) Stations.Last()->MarkUsed();
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
	bFightToday = false;
	LoakOffer = NAME_None;
	if (Day >= 2 && FMath::FRand() < 0.35f)
	{
		LoakOffer = (Count(EJRUnitType::PC) > 0 && FMath::RandBool()) ? FName(TEXT("pcbekas")) : FName(TEXT("psbekas"));
		AddToast(FString::Printf(TEXT("Ada tawaran loak hari ini: %s. Cek HP > Toko."), *ItemName(LoakOffer)));
	}
	if (PendingNetPlan >= 0)
	{
		NetPlan = PendingNetPlan;
		PendingNetPlan = -1;
		AddToast(FString::Printf(TEXT("Teknisi %s selesai memasang. Internet baru aktif!"), NetName(NetPlan)), Good);
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
