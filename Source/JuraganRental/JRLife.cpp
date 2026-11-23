// Hidup juragan: kejadian keluarga dan kampung (paling banyak satu per hari, dialog pilihan), "Suasana rumah"
// (FamilyMood 0-100 yang berubah sesuai pilihan), dan "Impian juragan" (tujuan belanja pribadi di HP > Toko:
// jawaban untuk "duitnya buat apa?"). Impian terakhir membuka ending "Anak Berbakti".

#include "JRGameMode.h"
#include "JRLang.h"
#include "Misc/CommandLine.h"

#include "JRCustomer.h"
#include "JRStation.h"
#include "JRSound.h"

namespace JRLocal_Life
{
	const FLinearColor Good(0.36f, 0.79f, 0.54f);
	const FLinearColor Bad(0.94f, 0.42f, 0.36f);
	const FLinearColor Info(0.37f, 0.70f, 1.f);
	constexpr int32 NumFamily = 13;
	constexpr int32 NumDreams = 5;
}
using namespace JRLocal_Life;

/* ---------- Suasana rumah ---------- */

FString AJRGameMode::MoodName() const
{
	return FamilyMood >= 75 ? JRT(TEXT("Hangat")) : FamilyMood >= 45 ? JRT(TEXT("Biasa")) : FamilyMood >= 25 ? JRT(TEXT("Kurang akur")) : JRT(TEXT("Tegang"));
}

void AJRGameMode::AddMood(int32 Delta)
{
	const int32 Old = FamilyMood;
	FamilyMood = FMath::Clamp(FamilyMood + Delta, 0, 100);
	if (FamilyMood == Old) return;
	AddToast(JRPrintf(JRT(TEXT("Suasana rumah %s%d: %s")), Delta > 0 ? TEXT("+") : TEXT(""), FamilyMood - Old, *MoodName()), Delta > 0 ? Good : Bad);
}

float AJRGameMode::MoodSpawnMul() const
{
	// Rumah hangat: Emak ikut promosi ke ibu-ibu arisan. Rumah tegang: juragan kurang semangat.
	return FamilyMood >= 75 ? 1.06f : FamilyMood < 25 ? 0.94f : 1.f;
}

/* ---------- Kejadian keluarga dan kampung ---------- */

void AJRGameMode::ScheduleFamily()
{
	FamilyAt = -1.f;
	FamilyWhich = -1;
	// Suasana rumah pelan-pelan kembali ke biasa
	FamilyMood += FamilyMood > 55 ? -2 : FamilyMood < 45 ? 2 : 0;
	if (Day < 2 || PromoId > 0 || bBalance) return;
	if (FMath::FRand() >= 0.45f && !FParse::Param(FCommandLine::Get(), TEXT("jrfamily"))) return;
	TArray<int32> Pool;
	for (int32 i = 0; i < NumFamily; ++i)
	{
		if (i == LastFamily) continue;
		if (i == 1 && Era >= 3) continue;							// atap bocor: garasi dan ruko saja
		if (i == 8 && Era >= 2) continue;							// token listrik: garasi (meteran prabayar rumah)
		if (i == 11 && Era < 2) continue;							// pajak reklame: papan nama ruko
		if ((i == 4 || i == 12) && Count(EJRUnitType::PS) < 2) continue;	// Bapak pinjam TV / sewa bawa pulang
		if (i == 12 && bRentOut) continue;
		Pool.Add(i);
	}
	if (News == EJRNews::Hujan && LastFamily != 1 && Era < 3) Pool.Add(1), Pool.Add(1);	// hujan: atap bocor lebih sering
	FamilyWhich = Pool[FMath::RandRange(0, Pool.Num() - 1)];
	FamilyAt = FamilyWhich == 7 ? FMath::FRandRange(20.f, 22.f) : FMath::FRandRange(10.f, 18.f);	// komplain berisik: malam
}

void AJRGameMode::TickFamily()
{
	if (FamilyAt < 0.f || Hour < FamilyAt || Choices.Num() > 0) return;
	const int32 Which = FamilyWhich;
	FamilyAt = -1.f;
	FamilyWhich = -1;
	FamilyEvent(Which);
}

void AJRGameMode::FamilyEvent(int32 Which)
{
	LastFamily = Which;
	const int32 E = FMath::Clamp(Era, 1, 3);
	const float K = E == 1 ? 0.6f : E == 2 ? 1.5f : 3.f;	// skala biaya per era (garasi: modal masih tipis)
	auto Pay = [this](float Amount) { Money -= Amount; Stats.Stock += Amount; };	// dicatat sebagai belanja hari ini
	auto Earn = [this](float Amount) { Money += Amount; Stats.Side += Amount; };
	JRSound::Play2D(this, TEXT("SFX_Notif"), 0.6f);
	if (bAutoTest) UE_LOG(LogTemp, Display, TEXT("JR_FAMILY kejadian=%d hari=%d jam=%.2f mood=%d"), Which, Day, Hour, FamilyMood);

	switch (Which)
	{
	case 0:
	{
		const float Spp = 50000.f * K;
		PushChoice(JRT(TEXT("Adik minta uang SPP")),
			JRPrintf(JRT(TEXT("Adikmu datang dengan muka memelas. \"Kak, besok terakhir bayar SPP. Kurang %s. Emak lagi nggak pegang uang...\"")), *JRFormatRp(Spp)),
			{ JRPrintf(JRT(TEXT("Kasih %s")), *JRFormatRp(Spp)), JRT(TEXT("\"Tunggu rental rame ya, Dek\"")) },
			{ [this, Pay, Spp]() { Pay(Spp); AddMood(12); AddXp(2); AddToast(JRT(TEXT("Adik lompat kegirangan. \"Makasih, Kak! Nanti aku bantu bersih-bersih!\"")), Good); },
			  [this]() { AddMood(-10); } });
		break;
	}
	case 1:
	{
		const float Fix = 40000.f * K;
		PushChoice(JRT(TEXT("Atap bocor!")),
			JRT(TEXT("Air hujan menetes dari genteng, tepat di atas salah satu TV. Pelanggan mulai geser-geser kursi.")),
			{ JRPrintf(JRT(TEXT("Panggil tukang (%s)")), *JRFormatRp(Fix)), JRT(TEXT("Tadah pakai ember saja")) },
			{ [this, Pay, Fix]() { Pay(Fix); Stats.RepDelta += 1.f; AddToast(JRT(TEXT("Tukang datang naik ke genteng. Beres, pelanggan aman.")), Good); },
			  [this]()
			  {
				  for (AJRStation* S : Stations)
				  {
					  if (S && S->Type == EJRUnitType::PS && !S->bBroken && !S->Customer) { S->bBroken = true; AddToast(JRPrintf(JRT(TEXT("%s kena tetesan air dan mati. Perbaiki dulu sebelum dipakai.")), *S->DisplayName), Bad); return; }
				  }
				  Stats.RepDelta -= 1.f;
				  AddToast(JRT(TEXT("Ember penuh, lantai becek. Pelanggan menggerutu.")), Bad);
			  } });
		break;
	}
	case 2:
	{
		const float Fee = 25000.f * K;
		PushChoice(JRT(TEXT("Iuran kampung")),
			JRPrintf(JRT(TEXT("Pak RT keliling membawa map. \"Mas, iuran ronda dan kebersihan bulan ini %s ya. Usaha yang tertib iurannya, warga juga senang mampir.\"")), *JRFormatRp(Fee)),
			{ JRT(TEXT("Bayar iuran")), JRT(TEXT("\"Besok ya, Pak\"")) },
			{ [this, Pay, Fee]() { Pay(Fee); Stats.RepDelta += 3.f; AddToast(JRT(TEXT("Namamu ditulis di papan iuran balai warga. Reputasi +3")), Good); },
			  [this]() { Stats.RepDelta -= 2.f; AddToast(JRT(TEXT("Pak RT menghela napas. Kabarnya cepat menyebar di grup warga. Reputasi -2")), Bad); } });
		break;
	}
	case 3:
	{
		const float Sale = 15000.f + 10000.f * E;
		PushChoice(JRT(TEXT("Gorengan buatan Emak")),
			JRT(TEXT("Emak datang membawa sebaskom bakwan dan tahu isi yang masih hangat. \"Nak, Emak goreng kebanyakan. Mau dijual di kasir atau dibagi ke pelanggan?\"")),
			{ JRPrintf(JRT(TEXT("Jual di kasir (+%s)")), *JRFormatRp(Sale)), JRT(TEXT("Bagi gratis ke pelanggan")) },
			{ [this, Earn, Sale]() { Earn(Sale); AddMood(6); AddToast(JRT(TEXT("Gorengan Emak ludes dalam setengah jam!")), Good); },
			  [this]() { Stats.RepDelta += 2.f; AddMood(8); AddToast(JRT(TEXT("Pelanggan senang dapat gorengan gratis. Reputasi +2")), Good); } });
		break;
	}
	case 4:
		PushChoice(JRT(TEXT("Bapak pinjam TV")),
			JRT(TEXT("Bapak berdiri di pintu. \"Le, TV di rumah rusak. Sinetron favorit Bapak tayang sebentar lagi. Pinjam satu TV-mu sampai malam, ya?\"")),
			{ JRT(TEXT("Pinjamkan satu TV")), JRT(TEXT("\"Lagi rame, Pak. Besok ya\"")) },
			{ [this]()
			  {
				  // TV paling ujung yang sedang kosong dibawa pulang; kembali besok pagi (dipasang lagi seperti kardus kurir).
				  int32 Pick = INDEX_NONE;
				  for (int32 i = 0; i < Stations.Num(); ++i)
					  if (Stations[i] && Stations[i]->Type == EJRUnitType::PS && Stations[i]->IsFree() && (Pick == INDEX_NONE || Stations[i]->Index > Stations[Pick]->Index)) Pick = i;
				  if (Pick != INDEX_NONE)
				  {
					  AJRStation* S = Stations[Pick];
					  const FName Back = S->bUsed ? FName(TEXT("psbekas")) : FName(TEXT("ps"));
					  Stations.RemoveAt(Pick);
					  S->Destroy();
					  PendingOrders.Add(Back);
				  }
				  AddMood(14);
				  AddToast(JRT(TEXT("Bapak senyum lebar membawa TV pulang. Besok pagi TV-nya dikembalikan.")), Good);
			  },
			  [this]() { AddMood(-8); } });
		break;
	case 5:
	{
		const float Loan = 100000.f * K;
		PushChoice(JRT(TEXT("Teman lama pinjam uang")),
			JRPrintf(JRT(TEXT("Teman SMA-mu mampir. \"Bro, motor gue masuk bengkel. Pinjam %s dulu, minggu depan gue balikin plus traktir.\"")), *JRFormatRp(Loan)),
			{ JRPrintf(JRT(TEXT("Pinjamkan %s")), *JRFormatRp(Loan)), JRT(TEXT("Tolak dengan halus")) },
			{ [this, Loan]() { Money -= Loan; Debts.Add(Loan); Stats.Utang += Loan; Stats.RepDelta += 1.f; AddToast(JRT(TEXT("Dicatat di buku utang. Semoga dia menepati janji...")), Info); },
			  [this]() { AddToast(JRT(TEXT("\"Santai, bro. Gue ngerti.\" Dia pamit pelan.")), Info); } });
		break;
	}
	case 6:
		PushChoice(JRT(TEXT("Kucing kampung")),
			JRT(TEXT("Seekor kucing oranye masuk dan tidur di atas meja kasir. Bocil-bocil langsung mengelilinginya.")),
			{ JRT(TEXT("Kasih makan (Rp 5.000)")), JRT(TEXT("Usir pelan-pelan")) },
			{ [this, Pay]()
			  {
				  Pay(5000.f);
				  AddMood(4);
				  Stats.RepDelta += 1.f;
				  if (FMath::FRand() < 0.3f) { ViralDays = FMath::Max(ViralDays, 1); AddToast(JRT(TEXT("Video bocil main bareng kucing di rentalmu viral! Besok lebih ramai.")), Good); }
				  else AddToast(JRT(TEXT("Si Oyen betah. Pelanggan bilang rentalmu jadi adem. Reputasi +1")), Good);
			  },
			  [this]() { AddToast(JRT(TEXT("Kucingnya pergi, bocil-bocil kecewa.")), Info); } });
		break;
	case 7:
	{
		const float Treat = 10000.f;
		PushChoice(JRT(TEXT("Tetangga komplain")),
			JRT(TEXT("Bu tetangga mengetuk pintu. \"Mas, sudah malam. Teriakan anak-anak main bola kedengaran sampai kamar, bayi saya nggak bisa tidur.\"")),
			{ JRPrintf(JRT(TEXT("Minta maaf, antar mi hangat (%s)")), *JRFormatRp(Treat)), JRT(TEXT("\"Namanya juga rental, Bu\"")) },
			{ [this, Pay, Treat]() { Pay(Treat); Stats.RepDelta += 2.f; AddToast(JRT(TEXT("Bu tetangga luluh. \"Ya sudah, jangan sampai larut ya, Mas.\" Reputasi +2")), Good); },
			  [this]() { Stats.RepDelta -= 3.f; AddToast(JRT(TEXT("Bu tetangga cerita ke grup ibu-ibu. Reputasi -3")), Bad); } });
		break;
	}
	case 8:
	{
		const float Token = 50000.f;
		PushChoice(JRT(TEXT("Token listrik bunyi")),
			JRT(TEXT("Meteran listrik rumah berbunyi tit-tit-tit. Pulsa token tinggal sedikit, padahal semua TV sedang menyala.")),
			{ JRPrintf(JRT(TEXT("Isi token sekarang (%s)")), *JRFormatRp(Token)), JRT(TEXT("Nanti saja, masih ada")) },
			{ [this, Token]() { Money -= Token; Stats.Elec += Token; AddToast(JRT(TEXT("Token terisi. Listrik aman sampai tutup.")), Good); },
			  [this]() { if (!bMcbTripped) Trip(); AddToast(JRT(TEXT("Listrik padam! Ternyata token habis. Isi dulu lewat MCB.")), Bad); } });
		break;
	}
	case 9:
	{
		const float Med = 30000.f;
		PushChoice(JRT(TEXT("Emak masuk angin")),
			JRT(TEXT("Emak terbaring sambil memijat kepala. \"Nggak apa-apa, Nak, cuma masuk angin...\" Tapi wajahnya pucat.")),
			{ JRPrintf(JRT(TEXT("Belikan obat dan jamu (%s)")), *JRFormatRp(Med)), JRT(TEXT("Minta adik yang kerokin")) },
			{ [this, Pay, Med]() { Pay(Med); AddMood(15); AddToast(JRT(TEXT("Sore harinya Emak sudah bisa tersenyum lagi.")), Good); },
			  [this]() { AddMood(-6); } });
		break;
	}
	case 10:
	{
		const float Cake = 40000.f * K;
		PushChoice(JRT(TEXT("Adik ulang tahun")),
			JRT(TEXT("Hari ini adikmu ulang tahun. Dari tadi ia mondar-mandir di depan rental, pura-pura tidak menunggu apa-apa.")),
			{ JRPrintf(JRT(TEXT("Traktir kue dan main gratis sejam (%s)")), *JRFormatRp(Cake)), JRT(TEXT("Ucapkan selamat saja")) },
			{ [this, Pay, Cake]() { Pay(Cake); AddMood(16); AddXp(2); AddToast(JRT(TEXT("Adik meniup lilin di depan TV 1. Semua pelanggan ikut nyanyi!")), Good); },
			  [this]() { AddMood(-5); } });
		break;
	}
	case 12:
	{
		const float Fee = 40000.f * K;
		PushChoice(JRT(TEXT("Sewa PS bawa pulang")),
			JRPrintf(JRT(TEXT("Mas-mas kos datang. \"Bang, PS-nya bisa disewa bawa pulang semalam? Mau main sama anak-anak kos. Saya bayar %s.\"")), *JRFormatRp(Fee)),
			{ JRPrintf(JRT(TEXT("Sewakan satu TV + PS (+%s)")), *JRFormatRp(Fee)), JRT(TEXT("\"Nggak bisa dibawa pulang, Mas\"")) },
			{ [this, Earn, Fee]()
			  {
				  int32 Pick = INDEX_NONE;
				  for (int32 i = 0; i < Stations.Num(); ++i)
					  if (Stations[i] && Stations[i]->Type == EJRUnitType::PS && Stations[i]->IsFree() && (Pick == INDEX_NONE || Stations[i]->Index > Stations[Pick]->Index)) Pick = i;
				  if (Pick == INDEX_NONE) { AddToast(JRT(TEXT("Semua TV sedang dipakai. Penyewa bilang lain kali saja.")), Info); return; }
				  AJRStation* S = Stations[Pick];
				  PendingOrders.Add(S->bUsed ? FName(TEXT("psbekas")) : FName(TEXT("ps")));
				  Stations.RemoveAt(Pick);
				  S->Destroy();
				  Earn(Fee);
				  bRentOut = true;
				  AddToast(JRT(TEXT("PS dibawa pulang. Besok pagi dikembalikan, semoga stiknya utuh...")), Good);
			  },
			  [this]() { AddToast(JRT(TEXT("\"Oh gitu ya, Bang. Ya udah main di sini aja deh.\"")), Info); } });
		break;
	}
	default:
	{
		const float Tax = 75000.f;
		PushChoice(JRT(TEXT("Petugas pajak reklame")),
			JRPrintf(JRT(TEXT("Petugas berseragam memotret papan namamu. \"Papan reklame ini belum berizin, Pak. Bayar retribusi %s sekarang, atau kami proses denda.\"")), *JRFormatRp(Tax)),
			{ JRT(TEXT("Bayar retribusi")), JRT(TEXT("Coba tawar")) },
			{ [this, Pay, Tax]() { Pay(Tax); AddToast(JRT(TEXT("Stiker izin reklame ditempel. Aman.")), Info); },
			  [this, Pay, Tax]()
			  {
				  if (FMath::RandBool()) { Pay(Tax * 0.5f); AddToast(JRT(TEXT("Petugas luluh setelah ditraktir kopi. Bayar setengah saja.")), Good); }
				  else { Pay(Tax * 2.f); Stats.RepDelta -= 1.f; AddToast(JRT(TEXT("Petugas tersinggung. Kena denda dua kali lipat!")), Bad); }
			  } });
		break;
	}
	}
}

void AJRGameMode::ReturnRentOut()
{
	if (!bRentOut) return;
	bRentOut = false;
	if (FMath::FRand() < 0.25f)
	{
		const float Fix = 20000.f * (Era == 1 ? 0.6f : Era == 2 ? 1.5f : 3.f);
		Money -= Fix;
		Stats.Stock += Fix;
		AddToast(JRPrintf(JRT(TEXT("PS sewaan kembali, tapi analog stiknya kendor. Ganti stik %s.")), *JRFormatRp(Fix)), Bad);
	}
	else AddToast(JRT(TEXT("PS sewaan kembali dengan selamat. Penyewa titip salam, katanya mau sewa lagi.")), Good);
}

/* ---------- CD macet: trik legendaris PS1/PS2 ---------- */

void AJRGameMode::CdJamEvent(AJRStation* S)
{
	if (!S) return;
	JRSound::PlayAt(this, TEXT("SFX_Stik_Rusak"), S->GetActorLocation() + FVector(0, 0, 100));
	TWeakObjectPtr<AJRStation> WS(S);
	auto Fixed = [this, WS](const FString& Msg)
	{
		if (bAutoTest) UE_LOG(LogTemp, Display, TEXT("JR_CDJAM hasil=jalan"));
		if (AJRStation* X = WS.Get()) { X->bBroken = false; if (AJRCustomer* C = X->Customer.Get()) C->Emote(JRT(TEXT("Jalan lagi!")), FColor(120, 230, 150)); }
		AddXp(1);
		AddToast(Msg, Good);
	};
	auto Stuck = [this, WS](const FString& Msg) { if (bAutoTest) UE_LOG(LogTemp, Display, TEXT("JR_CDJAM hasil=macet")); if (AJRStation* X = WS.Get()) AddToast(Msg + TEXT(" ") + JRPrintf(JRT(TEXT("Tahan aksi di %s untuk memperbaiki.")), *X->DisplayName), Bad); };
	PushChoice(JRPrintf(JRT(TEXT("CD macet di %s!")), *S->DisplayName),
		JRT(TEXT("Layar berhenti di tulisan loading. Pelanggan memandangmu penuh harap. Pakai jurus apa?")),
		{ JRT(TEXT("Lap CD pakai kaos")), JRT(TEXT("Balik PS-nya (trik legendaris)")), JRT(TEXT("Buka dan perbaiki dengan benar")) },
		{ [Fixed, Stuck]() { if (FMath::FRand() < 0.6f) Fixed(JRT(TEXT("Dilap-lap, ditiup, dimasukkan lagi... JALAN! Pelanggan tepuk tangan."))); else Stuck(JRT(TEXT("CD-nya baret parah, lap saja tidak cukup."))); },
		  [Fixed, Stuck]() { if (FMath::FRand() < 0.45f) Fixed(JRT(TEXT("PS dibalik, dan entah bagaimana loading-nya lanjut. Ilmu rental turun-temurun!"))); else Stuck(JRT(TEXT("Dibalik malah berbunyi krek. Harus dibongkar."))); },
		  [Stuck]() { Stuck(JRT(TEXT("Oke, cara yang benar."))); } }, true);
}

/* ---------- Jumat Berkah ---------- */

void AJRGameMode::OfferBerkah()
{
	bBerkah = false;
	const bool bTest = FParse::Param(FCommandLine::Get(), TEXT("jrberkah"));
	if (!bTest && (Day < 5 || Day % 7 != 5 || PromoId > 0 || bBalance)) return;	// tiap Jumat (sehari sebelum turnamen akhir pekan)
	PushChoice(JRT(TEXT("Jumat Berkah")),
		JRT(TEXT("Hari Jumat. Rental-rental di kampung sebelah pasang spanduk \"Harga Sedekah\". Mau ikut berbagi hari ini?")),
		{ JRT(TEXT("Pasang harga sedekah (tarif setengah, pengunjung +50%)")), JRT(TEXT("Tarif biasa saja")) },
		{ [this]() { bBerkah = true; AddToast(JRT(TEXT("Spanduk \"Jumat Berkah: Harga Sedekah\" terpasang. Siap-siap ramai!")), Good); if (bAutoTest) UE_LOG(LogTemp, Display, TEXT("JR_BERKAH aktif tarifPS=%d"), PriceFor(EJRUnitType::PS)); },
		  [this]() {} });
}

/* ---------- Impian juragan ---------- */

void AJRGameMode::DreamInfo(int32 i, FName& OutId, FString& OutName, FString& OutDesc, int32& OutCost, int32& OutEra)
{
	static const TCHAR* Ids[] = { TEXT("impian1"), TEXT("impian2"), TEXT("impian3"), TEXT("impian4"), TEXT("impian5") };
	static const int32 Costs[] = { 400000, 900000, 2000000, 6000000, 15000000 };
	static const int32 Eras[] = { 1, 1, 2, 3, 3 };
	OutId = Ids[i]; OutCost = Costs[i]; OutEra = Eras[i];
	switch (i)
	{
	case 0: OutName = JRT(TEXT("Impian: HP baru buat adik")); OutDesc = JRT(TEXT("Biar adik bisa ikut sekolah daring. Suasana rumah +20.")); break;
	case 1: OutName = JRT(TEXT("Impian: dapur baru buat Emak")); OutDesc = JRT(TEXT("Kompor dua tungku dan kulkas. Suasana rumah +20, mi matang lebih cepat.")); break;
	case 2: OutName = JRT(TEXT("Impian: motor juragan")); OutDesc = JRT(TEXT("Belanja ambil sendiri: kardus pesanan langsung tiba hari itu juga.")); break;
	case 3: OutName = JRT(TEXT("Impian: renovasi rumah Emak")); OutDesc = JRT(TEXT("Genteng baru, dinding dicat, teras keramik. Suasana rumah penuh, reputasi +5.")); break;
	default: OutName = JRT(TEXT("Impian: umrahkan Emak")); OutDesc = JRT(TEXT("Impian terbesar: memberangkatkan Emak ke Tanah Suci.")); break;
	}
}

bool AJRGameMode::HasDream(int32 i) const { return (DreamsDone & (1 << i)) != 0; }

void AJRGameMode::AddDreamItems(TArray<FJRShopItem>& Out) const
{
	for (int32 i = 0; i < JRLocal_Life::NumDreams; ++i)
	{
		FName Id; FString Name, Desc; int32 Cost = 0, NeedEra = 1;
		DreamInfo(i, Id, Name, Desc, Cost, NeedEra);
		FString Lock;
		if (HasDream(i)) Lock = JRT(TEXT("Sudah terwujud"));
		else if (Era < NeedEra) Lock = NeedEra == 2 ? JRT(TEXT("Mulai di ruko")) : JRT(TEXT("Mulai di Game Center"));
		else if (i == 4 && !HasDream(3)) Lock = JRT(TEXT("Renovasi rumah Emak dulu"));
		Out.Add({ Id, Name, Desc, Cost, Lock });
	}
}

bool AJRGameMode::BuyDream(FName Id)
{
	for (int32 i = 0; i < JRLocal_Life::NumDreams; ++i)
	{
		FName DId; FString Name, Desc; int32 Cost = 0, NeedEra = 1;
		DreamInfo(i, DId, Name, Desc, Cost, NeedEra);
		if (DId != Id) continue;
		DreamsDone |= 1 << i;
		Celebrate();
		UnlockBadge(TEXT("impian"));
		JRSound::Play2D(this, TEXT("SFX_Kaset_Emas"), 0.8f);
		if (bAutoTest) UE_LOG(LogTemp, Display, TEXT("JR_DREAM %d terwujud kas=%.0f"), i, Money);
		switch (i)
		{
		case 0:
			AddMood(20);
			PushChoice(JRT(TEXT("Impian terwujud!")), JRT(TEXT("Adik memeluk kotak HP barunya erat-erat. \"Kak, nanti kalau aku sukses, gantian aku yang beliin Kakak!\"")), { JRT(TEXT("Lanjut jaga rental")) }, { []() {} });
			break;
		case 1:
			AddMood(20);
			PushChoice(JRT(TEXT("Impian terwujud!")), JRT(TEXT("Emak mencoba kompor barunya sambil bersenandung. Malam itu semua orang di rumah makan opor ayam.")), { JRT(TEXT("Lanjut jaga rental")) }, { []() {} });
			break;
		case 2:
			PushChoice(JRT(TEXT("Impian terwujud!")), JRT(TEXT("Motor bebek bekas yang masih kinclong terparkir di depan rental. Mulai sekarang belanja diambil sendiri, tidak perlu menunggu kurir besok.")), { JRT(TEXT("Lanjut jaga rental")) }, { []() {} });
			break;
		case 3:
			AddMood(100);
			Stats.RepDelta += 5.f;
			PushChoice(JRT(TEXT("Impian terwujud!")), JRT(TEXT("Rumah Emak kini bergenteng baru dan berteras keramik. Tetangga berdatangan memberi selamat. Reputasi +5")), { JRT(TEXT("Lanjut jaga rental")) }, { []() {} });
			break;
		default:
			AddMood(100);
			SeenEndings |= 8;
			UnlockEnding(TEXT("berbakti"), JRT(TEXT("Di bandara, Emak menggenggam tanganmu lama sekali. \"Dulu Emak cuma bisa doa supaya rental kecil di garasi itu laris. Ternyata Allah kasih lebih.\" Dari garasi sempit, kamu memberangkatkan Emak ke Tanah Suci.")));
			break;
		}
		return true;
	}
	return false;
}

FString AJRGameMode::NextDreamLine() const
{
	for (int32 i = 0; i < JRLocal_Life::NumDreams; ++i)
	{
		if (HasDream(i)) continue;
		FName Id; FString Name, Desc; int32 Cost = 0, NeedEra = 1;
		DreamInfo(i, Id, Name, Desc, Cost, NeedEra);
		if (Era < NeedEra) return FString();
		return JRPrintf(JRT(TEXT("%s (%s). Wujudkan lewat HP > Toko.")), *Name, *JRFormatRp(Cost));
	}
	return FString();
}
