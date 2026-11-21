// Paket 1 permainan lanjutan: Level Juragan (XP), upgrade unit Lv 1-5, dan bisnis sampingan Game Center
// (kulkas minuman, konter pulsa & top-up, printer & fotokopi).

#include "JRGameMode.h"
#include "Misc/CommandLine.h"
#include "JRLang.h"

#include "Engine/World.h"
#include "JRService.h"
#include "JRSound.h"
#include "JRStation.h"
#include "JRVisitor.h"
#include "JRFixtures.h"

namespace JRLocal_Progression
{
	const FLinearColor Good(0.36f, 0.79f, 0.54f);
	const FLinearColor Bad(0.94f, 0.42f, 0.36f);
	const FLinearColor Gold(1.f, 0.78f, 0.3f);

	struct FBuyer { const TCHAR* Name; const TCHAR* Mesh; };
	// Nama diterjemahkan saat dipakai (JRT di inisialisasi statis membuat build rilis crash).
	// Tiga pertama juga pelanggan printer (anak sekolah, mahasiswa, pekerja kantoran).
	const FBuyer Buyers[] = {
		{ TEXT("Anak SMA"), TEXT("SK_Anak_SMA") },
		{ TEXT("Mahasiswa"), TEXT("SK_Mahasiswa") },
		{ TEXT("Mas-mas kantoran"), TEXT("SK_Kantoran") },
		{ TEXT("Gamer online"), TEXT("SK_Gamer") },
		{ TEXT("Bapak-bapak"), TEXT("SK_Bapak") },
		{ TEXT("Emak-emak"), TEXT("SK_Emak") },
	};
}
using namespace JRLocal_Progression;

/* ---------- Nama rental ---------- */

void AJRGameMode::SetRentalName(const FString& NewName)
{
	FString N = NewName.TrimStartAndEnd();
	N.ReplaceInline(TEXT("\n"), TEXT(""));
	if (N.Len() > 20) N = N.Left(20);
	if (N.IsEmpty()) N = TEXT("Juragan");
	RentalName = N;
	bNeedName = false;
	// Bangun ulang ruangan agar papan nama luar dan dalam memakai nama baru.
	if (Room)
	{
		Room->Destroy();
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Room = GetWorld()->SpawnActor<AJRRoom>(FVector::ZeroVector, FRotator::ZeroRotator, SP);
		Room->Build(Layout(), Era);
	}
	AddToast(JRPrintf(JRT(TEXT("Papan nama baru terpasang: %s")), *RentalName), Good);
}

/* ---------- Viralin: komentar warganet setiap tutup hari ---------- */

void AJRGameMode::MakeNetizenPosts(float AvgSat)
{
	// Nama akun diambil acak; isi postingan menyesuaikan kejadian dan kinerja hari ini.
	static const TCHAR* Akun[] = { TEXT("@bocilgaming_"), TEXT("@mas.kantoran"), TEXT("@warga_rt05"), TEXT("@gamer.malam"),
		TEXT("@kuliahsambilmain"), TEXT("@emak.bijak"), TEXT("@jokirank_id"), TEXT("@anaksma.gabut"), TEXT("@bapak2pes") };
	const FString N = RentalName;
	TArray<FString> Lines;
	for (EJRMoment M : Stats.Moments)
	{
		switch (M)
		{
		case EJRMoment::Emak: Lines.Add(JRPrintf(JRT(TEXT("Lagi seru main bola di %s, tiba-tiba ada suara 'PULANG!' dari pintu... emak siapa itu wkwk")), *N)); break;
		case EJRMoment::RageQuit: Lines.Add(JRPrintf(JRT(TEXT("Ada yang kalah penalti terus banting stik di %s. Bang, sabar bang wkwk")), *N)); break;
		case EJRMoment::Fight: Lines.Add(JRPrintf(JRT(TEXT("Hampir ribut gara-gara offside di %s. Untung yang jaga sigap")), *N)); break;
		case EJRMoment::RunAway: Lines.Add(JRPrintf(JRT(TEXT("Tadi liat orang kabur dari %s belum bayar. Kasirnya ke mana bang??")), *N)); break;
		case EJRMoment::Trip: Lines.Add(JRPrintf(JRT(TEXT("JEGLEK! Lampu %s mati pas lagi final, save-an ilang")), *N)); break;
		case EJRMoment::PakRt: Lines.Add(JRT(TEXT("Pak RT keliling malam-malam negur rental yang berisik. Kena juga ya wkwk"))); break;
		case EJRMoment::Upsell: Lines.Add(JRPrintf(JRT(TEXT("\"Bang, nambah sejam!\" ...dan itu sudah yang ketiga kalinya. %s bikin nagih wkwk")), *N)); break;
		case EJRMoment::Streamer: Lines.Add(JRPrintf(JRT(TEXT("Bang Jago live dari %s! Rental kecil tapi vibes-nya juara")), *N)); break;
		case EJRMoment::Lurah: Lines.Add(JRPrintf(JRT(TEXT("Pak Lurah mampir ke %s. Rental sekarang resmi jadi tempat nongkrong warga")), *N)); break;
		case EJRMoment::Tour: Lines.Add(JRPrintf(JRT(TEXT("Turnamen PS di %s pecah banget! Tahun depan ikut lagi")), *N)); break;
		default: break;
		}
	}
	// Kinerja hari ini
	if (AvgSat >= 70.f) Lines.Add(JRPrintf(JRT(TEXT("Main di %s tuh enak: stik aman, cepat dilayani. 10/10 balik lagi besok")), *N));
	else if (AvgSat < 40.f) Lines.Add(JRPrintf(JRT(TEXT("Antre lama di %s, unitnya rusak mulu. Kecewa bang")), *N));
	if (Trash.Num() >= 5) Lines.Add(JRPrintf(JRT(TEXT("%s rame sih, tapi sampahnya di mana-mana. Tolong dong bersihin")), *N));
	if (Decor.Contains(TEXT("ac"))) Lines.Add(JRPrintf(JRT(TEXT("AC di %s dingin banget, betah main sampai sore")), *N));
	if (Lines.Num() == 0) Lines.Add(JRPrintf(JRT(TEXT("Baru tahu ada %s di sini. Kapan-kapan mampir ah")), *N));

	// Ambil paling banyak 3 postingan, terbaru di depan
	for (int32 i = 0; i < 3 && Lines.Num() > 0; ++i)
	{
		const int32 Pick = FMath::RandRange(0, Lines.Num() - 1);
		FJRPost P;
		P.Author = Akun[FMath::RandRange(0, UE_ARRAY_COUNT(Akun) - 1)];
		P.Text = Lines[Pick];
		P.Likes = FMath::RandRange(5, 40) + FMath::RoundToInt(Rep * FMath::FRandRange(1.f, 4.f));
		P.Day = Day;
		Posts.Insert(P, 0);
		Lines.RemoveAt(Pick);
	}
	if (Posts.Num() > 30) Posts.SetNum(30);
}

/* ---------- Level Juragan ---------- */

int32 AJRGameMode::XpForLevel(int32 Level)
{
	const int32 N = FMath::Clamp(Level, 1, MaxJuraganLevel) - 1;
	return 60 * N * N + 90 * N;	// L2 150, L4 810, L7 2.700, L10 5.670, L13 9.720
}

int32 AJRGameMode::JuraganLevel() const
{
	int32 L = 1;
	while (L < MaxJuraganLevel && Xp >= XpForLevel(L + 1)) ++L;
	return L;
}

void AJRGameMode::AddXp(int32 Amount)
{
	if (Amount <= 0) return;
	const int32 Old = JuraganLevel();
	Xp += Amount;
	Stats.Xp += Amount;
	const int32 New = JuraganLevel();
	if (New <= Old) return;

	// Sebutkan apa saja yang baru terbuka.
	FString Opened;
	auto Append = [&Opened](const FString& S) { Opened += (Opened.IsEmpty() ? TEXT("") : TEXT(",")) + S; };
	for (const FJRShopItem& It : GetShopItems())
	{
		const int32 M = ItemMinLevel(It.Id);
		if (M > Old && M <= New) Append(It.Name);
	}
	for (int32 Lv = 2; Lv <= MaxUnitLevel; ++Lv)
	{
		if (UnitLevelReq(Lv) > Old && UnitLevelReq(Lv) <= New) Append(JRPrintf(JRT(TEXT("upgrade unit Lv %d")), Lv));
	}
	JRSound::Play2D(this, TEXT("SFX_Ding"), 0.8f);
	AddMoment(EJRMoment::LevelUp);
	// Penjelasan sekali saat pertama kali naik level (dari Level 1).
	if (Old <= 1 && !bBalance && (!bAutoTest || FParse::Param(FCommandLine::Get(), TEXT("jrlvtip"))))
	{
		PushChoice(JRPrintf(JRT(TEXT("Naik ke Level Juragan %d!")), New),
			JRT(TEXT("Level Juragan naik dari XP: pelanggan yang bayar, mi dan jasa sampingan, misi harian, misi mingguan, dan turnamen. Setiap level membuka barang baru di HP > Toko dan upgrade unit. Pantau XP-mu di HP > Unit.")),
			{ JRT(TEXT("Siap, Juragan!")) }, { []() {} });
	}
	AddToast(Opened.IsEmpty() ? JRPrintf(JRT(TEXT("Naik ke Level Juragan %d!")), New)
		: JRPrintf(JRT(TEXT("Naik ke Level Juragan %d! Terbuka: %s")), New, *Opened), Gold);
}

int32 AJRGameMode::ItemMinLevel(FName Id)
{
	static const TPair<const TCHAR*, int32> Table[] = {
		{ TEXT("tanaman"), 2 }, { TEXT("kipas"), 2 }, { TEXT("daya"), 2 },
		{ TEXT("neon"), 3 }, { TEXT("duduk1"), 3 },
		{ TEXT("ps2"), 4 }, { TEXT("daya2"), 4 }, { TEXT("av"), 4 }, { TEXT("hype"), 4 },
		{ TEXT("pcfast"), 5 }, { TEXT("billing"), 5 },
		{ TEXT("ac"), 6 }, { TEXT("duduk2"), 6 }, { TEXT("vip"), 6 },
		{ TEXT("kulkas"), 7 }, { TEXT("konter"), 8 }, { TEXT("printer"), 10 },
	};
	for (const auto& P : Table) if (Id == P.Key) return P.Value;
	return 1;
}

/* ---------- Upgrade unit ---------- */

int32 AJRGameMode::UnitUpgradeCost(EJRUnitType Type, int32 ToLevel)
{
	static const int32 CostPS[] = { 60000, 140000, 260000, 420000 };
	static const int32 CostPC[] = { 80000, 180000, 320000, 500000 };
	static const int32 CostVIP[] = { 200000, 400000, 700000, 1100000 };
	const int32 I = FMath::Clamp(ToLevel, 2, MaxUnitLevel) - 2;
	return Type == EJRUnitType::PS ? CostPS[I] : Type == EJRUnitType::VIP ? CostVIP[I] : CostPC[I];
}

FString AJRGameMode::UnitLevelName(EJRUnitType Type, int32 Level)
{
	const int32 L = FMath::Clamp(Level, 1, MaxUnitLevel);
	if (L == 1) return JRT(TEXT("Standar"));
	if (Type == EJRUnitType::PS)
	{
		switch (L)
		{
		case 2: return JRT(TEXT("TV LED 32 inci"));
		case 3: return JRT(TEXT("TV LED 40 inci"));
		case 4: return JRT(TEXT("TV 43 inci + stik nirkabel"));
		default: return JRT(TEXT("TV 4K 50 inci"));
		}
	}
	if (Type == EJRUnitType::PC)
	{
		switch (L)
		{
		case 2: return JRT(TEXT("SSD: loading kilat"));
		case 3: return JRT(TEXT("VGA baru: grafis lancar"));
		case 4: return JRT(TEXT("Monitor LCD 144 Hz"));
		default: return JRT(TEXT("PC rakitan RGB"));
		}
	}
	switch (L)
	{
	case 2: return JRT(TEXT("Soundbar"));
	case 3: return JRT(TEXT("TV 4K 55 inci"));
	case 4: return JRT(TEXT("Sofa recliner"));
	default: return JRT(TEXT("PlayStasiun 5"));
	}
}

int32 AJRGameMode::UnitLevelOf(EJRUnitType Type, int32 Index) const
{
	const TArray<int32>& A = Type == EJRUnitType::PS ? LvPS : Type == EJRUnitType::VIP ? LvVIP : LvPC;
	return A.IsValidIndex(Index) ? FMath::Clamp(A[Index], 1, MaxUnitLevel) : 1;
}

int32 AJRGameMode::UnitPrice(const AJRStation* S) const
{
	return S ? PriceFor(S->Type) + LevelPriceBonus(S->Type, S->UpLevel) : 0;
}

FString AJRGameMode::UnitUpgradeLock(const AJRStation* S) const
{
	if (!S) return TEXT("-");
	if (S->UpLevel >= MaxUnitLevel) return JRT(TEXT("Level maksimal"));
	const int32 Req = UnitLevelReq(S->UpLevel + 1);
	if (JuraganLevel() < Req) return JRPrintf(JRT(TEXT("Butuh Level Juragan %d")), Req);
	const int32 Cost = UnitUpgradeCost(S->Type, S->UpLevel + 1);
	if (Money < Cost) return JRPrintf(JRT(TEXT("Kas kurang (%s)")), *JRFormatRp(Cost));
	return FString();
}

bool AJRGameMode::UpgradeUnit(AJRStation* S)
{
	if (!S || !UnitUpgradeLock(S).IsEmpty()) return false;
	const int32 To = S->UpLevel + 1;
	Money -= UnitUpgradeCost(S->Type, To);
	TArray<int32>& A = LevelArray(S->Type);
	while (A.Num() <= S->Index) A.Add(1);
	A[S->Index] = To;
	S->ApplyUpLevel(To);
	JRSound::PlayAt(this, TEXT("SFX_Pasang"), S->GetActorLocation() + FVector(0, 0, 80));
	AddToast(JRPrintf(JRT(TEXT("%s naik ke Lv %d: %s. Tarif +%s/jam")), *S->DisplayName, To, *UnitLevelName(S->Type, To),
		*JRFormatRp(LevelPriceBonus(S->Type, To))), Good);
	return true;
}

int32 AJRGameMode::Maintenance() const
{
	int32 M = 0;
	for (const AJRStation* S : Stations) if (S) M += (S->UpLevel - 1) * MaintPerLevel;
	return M;
}

/* ---------- Bisnis sampingan ---------- */

FVector AJRGameMode::ServicePos(EJRService Kind) const
{
	// Berjajar di depan tangga (lantai 1, sisi kiri), menghadap ke tengah ruangan.
	return FVector(120.f + 170.f * (int32)Kind, -(Layout().HalfY - 260.f), 0.f);
}

FVector AJRGameMode::ServiceSpot(EJRService Kind) const
{
	return ServicePos(Kind) + FVector(0.f, 100.f, 0.f);
}

void AJRGameMode::SpawnServices()
{
	for (AJRService* S : Services) if (S) S->Destroy();
	Services.Reset();
	if (Era < 3) return;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < (int32)EJRService::Count; ++i)
	{
		const EJRService K = (EJRService)i;
		if (!HasService(K)) continue;
		if (AJRService* S = GetWorld()->SpawnActor<AJRService>(ServicePos(K), FRotator(0.f, 90.f, 0.f), SP))
		{
			S->Setup(K);
			Services.Add(S);
		}
	}
}

const AJRGameMode::FJRWalkIn* AJRGameMode::WaitingAt(EJRService Kind) const
{
	for (const FJRWalkIn& W : WalkIns) if (W.Kind == Kind && W.bArrived && W.V.IsValid()) return &W;
	return nullptr;
}

FString AJRGameMode::ServiceLabel(EJRService Kind, bool& bAlert) const
{
	bAlert = false;
	switch (Kind)
	{
	// Dua baris pendek agar label tiga layanan yang berdampingan tidak saling menimpa.
	case EJRService::Kulkas:
		bAlert = Drinks <= 0;
		return FString(JRT(TEXT("MINUMAN"))) + TEXT("\n") + (Drinks <= 0 ? FString(JRT(TEXT("KOSONG!"))) : JRPrintf(JRT(TEXT("%d botol")), Drinks));
	case EJRService::Konter:
		bAlert = WaitingAt(Kind) != nullptr;
		return FString(JRT(TEXT("PULSA"))) + TEXT("\n") + (bAlert ? FString(JRT(TEXT("ADA PEMBELI!"))) : JRFormatRp(Saldo));
	default:
		bAlert = WaitingAt(Kind) != nullptr;
		return FString(JRT(TEXT("PRINT"))) + TEXT("\n") + (bAlert ? FString(JRT(TEXT("ADA PEMBELI!"))) : JRPrintf(JRT(TEXT("%d lembar")), Paper));
	}
}

FJRInteraction AJRGameMode::ServiceInteraction(EJRService Kind) const
{
	FJRInteraction Out;
	Out.Kind = EJRInteractKind::Info;
	const FJRWalkIn* W = WaitingAt(Kind);
	switch (Kind)
	{
	case EJRService::Kulkas:
		if (CarryBox == TEXT("krat")) { Out.Kind = EJRInteractKind::Press; Out.Prompt = JRPrintf(JRT(TEXT("Isi kulkas dengan krat minuman (+%d botol)")), KratSize); }
		else Out.Prompt = Drinks > 0 ? JRPrintf(JRT(TEXT("Kulkas: %d botol. Pelanggan membeli minum saat bayar di kasir")), Drinks)
			: FString(JRT(TEXT("Kulkas kosong. Pesan krat minuman lewat HP > Toko")));
		break;
	case EJRService::Konter:
		if (W && Saldo >= W->Amount)
		{
			Out.Kind = EJRInteractKind::Hold;
			Out.HoldTime = 1.2f;
			Out.Prompt = JRPrintf(JRT(TEXT("Tahan E: top-up %s untuk %s (untung %s)")), *JRFormatRp(W->Amount), *W->Name, *JRFormatRp(W->Fee));
		}
		else if (W) Out.Prompt = JRT(TEXT("Saldo konter kurang! Isi saldo lewat HP > Toko"));
		else Out.Prompt = JRPrintf(JRT(TEXT("Konter pulsa. Saldo %s")), *JRFormatRp(Saldo));
		break;
	default:
		if (CarryBox == TEXT("kertas")) { Out.Kind = EJRInteractKind::Press; Out.Prompt = JRPrintf(JRT(TEXT("Isi kertas printer (+%d lembar)")), RimSize); }
		else if (W && Paper >= W->Amount)
		{
			Out.Kind = EJRInteractKind::Hold;
			Out.HoldTime = 2.f;
			Out.Prompt = JRPrintf(JRT(TEXT("Tahan E: cetak %d lembar untuk %s (+%s)")), W->Amount, *W->Name, *JRFormatRp(W->Amount * SheetPrice));
		}
		else if (W) Out.Prompt = JRT(TEXT("Kertas kurang! Pesan kertas lewat HP > Toko"));
		else Out.Prompt = JRPrintf(JRT(TEXT("Printer: %d lembar kertas")), Paper);
		break;
	}
	return Out;
}

void AJRGameMode::ServiceInteract(EJRService Kind)
{
	if (Kind == EJRService::Kulkas && CarryBox == TEXT("krat"))
	{
		Drinks = FMath::Min(MaxDrinks, Drinks + KratSize);
		CarryBox = NAME_None;
		JRSound::PlayAt(this, TEXT("SFX_Kardus"), ServicePos(Kind) + FVector(0, 0, 100));
		AddToast(JRPrintf(JRT(TEXT("Kulkas terisi: %d botol")), Drinks), Good);
		return;
	}
	if (Kind == EJRService::Printer && CarryBox == TEXT("kertas"))
	{
		Paper = FMath::Min(MaxPaper, Paper + RimSize);
		CarryBox = NAME_None;
		JRSound::PlayAt(this, TEXT("SFX_Kardus"), ServicePos(Kind) + FVector(0, 0, 100));
		AddToast(JRPrintf(JRT(TEXT("Kertas printer terisi: %d lembar")), Paper), Good);
		return;
	}
	ServeWalkIn(Kind);
}

void AJRGameMode::SpawnWalkIn(EJRService Kind)
{
	const FJRLayout& L = Layout();
	const int32 B = FMath::RandRange(0, Kind == EJRService::Printer ? 2 : UE_ARRAY_COUNT(Buyers) - 1);
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AJRVisitor* V = GetWorld()->SpawnActor<AJRVisitor>(FVector(-(L.HalfX - 100.f), L.DoorY, 0.f), FRotator::ZeroRotator, SP);
	if (!V) return;
	V->Setup(Buyers[B].Mesh, TEXT("A_Idle"));
	TArray<FVector> Pts;
	MakePath(V->GetActorLocation(), ServiceSpot(Kind), Pts);
	V->WalkPath(Pts, -90.f);	// menghadap konter/printer
	JRSound::PlayAt(this, TEXT("SFX_Bel_Pintu"), FVector(-(L.HalfX - 40.f), L.DoorY, 220.f), 0.5f);

	FJRWalkIn W;
	W.V = V;
	W.Kind = Kind;
	W.Name = JRT(Buyers[B].Name);
	if (Kind == EJRService::Konter)
	{
		static const int32 Nominal[] = { 10000, 20000, 25000, 50000, 100000 };
		W.Amount = Nominal[FMath::RandRange(0, UE_ARRAY_COUNT(Nominal) - 1)];
		W.Fee = W.Amount <= 25000 ? 2000 : 3000;
	}
	else W.Amount = FMath::RandRange(4, 30);
	WalkIns.Add(W);
}

void AJRGameMode::ServeWalkIn(EJRService Kind)
{
	const int32 I = WalkIns.IndexOfByPredicate([Kind](const FJRWalkIn& W) { return W.Kind == Kind && W.bArrived && W.V.IsValid(); });
	if (I == INDEX_NONE) return;
	FJRWalkIn W = WalkIns[I];
	if (Kind == EJRService::Konter)
	{
		if (Saldo < W.Amount) return;
		Saldo -= W.Amount;
		Money += W.Amount + W.Fee;	// nominal kembali jadi uang tunai, biaya admin = untung
		Stats.Income += W.Fee;
		Stats.Side += W.Fee;
		Stats.TopUps++;
		AddToast(JRPrintf(JRT(TEXT("Top-up %s untuk %s beres. Untung %s")), *JRFormatRp(W.Amount), *W.Name, *JRFormatRp(W.Fee)), Good);
	}
	else
	{
		if (Paper < W.Amount) return;
		Paper -= W.Amount;
		const int32 Pay = W.Amount * SheetPrice;
		Money += Pay;
		Stats.Income += Pay;
		Stats.Side += Pay;
		Stats.Prints++;
		AddToast(JRPrintf(JRT(TEXT("%d lembar tercetak untuk %s: +%s")), W.Amount, *W.Name, *JRFormatRp(Pay)), Good);
	}
	JRSound::PlayAt(this, TEXT("SFX_Kaching"), ServicePos(Kind) + FVector(0, 0, 110), 0.7f);
	AddXp(1);
	if (AJRVisitor* V = W.V.Get()) V->LeaveTo(Layout().DoorOutside());
	WalkIns.RemoveAt(I);
}

void AJRGameMode::TickWalkIns(float DeltaSeconds, float DtH)
{
	if (Era < 3) return;
	// Pembeli baru: paling banyak satu per layanan (yang berikutnya datang setelah dilayani).
	for (EJRService K : { EJRService::Konter, EJRService::Printer })
	{
		if (!HasService(K) || Hour >= 22.5f || WalkIns.ContainsByPredicate([K](const FJRWalkIn& W) { return W.Kind == K; })) continue;
		float Rate = K == EJRService::Konter ? 2.4f : (Hour >= 13.f && Hour < 18.f ? 2.6f : Hour < 9.f ? 0.5f : 1.3f);
		if (bPaket1Test) Rate *= 4.f;
		if (FMath::FRand() < Rate * DtH) SpawnWalkIn(K);
	}
	for (int32 i = WalkIns.Num() - 1; i >= 0; --i)
	{
		FJRWalkIn& W = WalkIns[i];
		AJRVisitor* V = W.V.Get();
		if (!V) { WalkIns.RemoveAt(i); continue; }
		if (!W.bArrived)
		{
			if (!V->IsWalking())
			{
				W.bArrived = true;
				AddToast(W.Kind == EJRService::Konter
					? JRPrintf(JRT(TEXT("%s mau top-up %s di konter pulsa")), *W.Name, *JRFormatRp(W.Amount))
					: JRPrintf(JRT(TEXT("%s mau print %d lembar di printer")), *W.Name, W.Amount));
			}
			continue;
		}
		W.Wait += DeltaSeconds;
		const bool bCan = W.Kind == EJRService::Konter ? Saldo >= W.Amount : Paper >= W.Amount;
		if (!bCan && !W.bHinted)
		{
			W.bHinted = true;
			AddToast(W.Kind == EJRService::Konter ? FString(JRT(TEXT("Saldo konter kurang! Isi saldo lewat HP > Toko")))
				: FString(JRT(TEXT("Kertas kurang! Pesan kertas lewat HP > Toko"))), Bad);
		}
		// Karyawan kasir ikut melayani konter dan printer bila pemain sibuk.
		if (bCan && HasStaff(EJRStaff::Kasir) && W.Wait >= (W.Kind == EJRService::Konter ? 4.f : 7.f))
		{
			ServeWalkIn(W.Kind);
			continue;
		}
		if (W.Wait > 45.f)
		{
			Stats.RepDelta -= 0.3f;
			Stats.WalkInLost++;
			AddToast(JRPrintf(JRT(TEXT("%s pergi, kelamaan menunggu di %s")), *W.Name,
				W.Kind == EJRService::Konter ? JRT(TEXT("konter pulsa")) : JRT(TEXT("printer"))), Bad);
			V->LeaveTo(Layout().DoorOutside());
			WalkIns.RemoveAt(i);
		}
	}
}

void AJRGameMode::ClearWalkIns()
{
	for (FJRWalkIn& W : WalkIns) if (AJRVisitor* V = W.V.Get()) V->Destroy();
	WalkIns.Reset();
}

void AJRGameMode::SellDrink()
{
	if (!bKulkas || Era < 3 || FMath::FRand() >= (bPaket1Test ? 0.8f : 0.35f)) return;
	if (Drinks > 0)
	{
		Drinks--;
		Money += DrinkPrice;
		Stats.Income += DrinkPrice;
		Stats.Side += DrinkPrice;
		Stats.DrinksSold++;
	}
	else if (!Stats.bFridgeWarned)
	{
		Stats.bFridgeWarned = true;
		AddToast(JRT(TEXT("Kulkas kosong! Pelanggan batal beli minum. Pesan krat lewat HP > Toko")), Bad);
	}
}
