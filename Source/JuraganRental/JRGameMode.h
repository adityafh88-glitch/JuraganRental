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
class AJRRadio;

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
	// Catatan untuk keseimbangan ekonomi (-jrbalance)
	int32 AngryQueue = 0, AngryStart = 0, Ran = 0, Breaks = 0, Trips = 0, Paid = 0;
	int32 MieSold = 0, Started = 0, Repairs = 0;
	float Bonus = 0.f;	// hadiah misi hari ini
	float Side = 0.f;	// untung bisnis sampingan (minuman, top-up, print)
	int32 DrinksSold = 0, TopUps = 0, Prints = 0, WalkInLost = 0;
	bool bFridgeWarned = false;
	int32 Xp = 0;		// XP yang didapat hari ini
	int32 Upsells = 0;	// "Bang, nambah sejam!" dilayani
	int32 ExactChange = 0;	// kembalian pas di kasir
	TArray<EJRMoment> Moments;	// kejadian penting hari ini (urutan terjadi)
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
	// Era 3: lantai 2 dan tangga di sisi kiri (-Y), naik ke arah belakang (+X)
	float FloorH = 0.f;		// tinggi lantai 2 (0 = satu lantai)
	TArray<float> VipY;		// posisi Y booth VIP di lantai 2 (di X = HalfX - 110)
	float StairX0 = 0.f, StairX1 = 0.f, StairW = 160.f;
	FVector McbPos = FVector::ZeroVector;	// pengganti posisi MCB bawaan (jika tertutup tangga)
	bool HasUpstairs() const { return FloorH > 0.f; }
	FVector StairBottom() const { return FVector(StairX0 - 60.f, -HalfY + StairW / 2, 0.f); }
	FVector StairTop() const { return FVector(StairX1 + 50.f, -HalfY + StairW / 2, FloorH); }

	FVector Kasir() const { return FVector(-(HalfX - 210.f), -0.65f * HalfY, 0.f); }
	FVector Mcb() const { return !McbPos.IsZero() ? McbPos : FVector(0.4f * HalfX, -(HalfY - 7.f), 130.f); }
	FVector Stove() const { return FVector(-0.13f * HalfX, -(HalfY - 34.f), 0.f); }
	FVector Router() const { return FVector(HalfX - 12.f, 0.55f * HalfY, 175.f); }
	FVector DoorOutside() const { return FVector(-(HalfX + 200.f), DoorY, 0.f); }
	FVector PlayerStart() const { return FVector(-0.36f * HalfX, 0.f, 110.f); }
	int32 MaxUnits() const { return PsY.Num(); }
	int32 MaxVip() const { return VipY.Num(); }
};

UCLASS()
class JURAGANRENTAL_API AJRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJRGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
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
	bool bFastNet = false;		// (simpanan lama) setara KilatNet
	/* Provider internet ruko: 0 NetRakyat, 1 KilatNet, 2 FiberNusa */
	int32 NetPlan = 0;
	int32 PendingNetPlan = -1;	// ganti provider mulai besok

	/* Tawaran loak hari ini: "psbekas" / "pcbekas" (None = tidak ada) */
	FName LoakOffer;
	static bool IsPCItem(FName Id) { return Id == TEXT("pc") || Id == TEXT("pcbekas"); }
	static bool IsUsedItem(FName Id) { return Id == TEXT("psbekas") || Id == TEXT("pcbekas"); }
	static const TCHAR* NetName(int32 Plan);
	static int32 NetCost(int32 Plan);
	static const TCHAR* NetDesc(int32 Plan);
	void ChooseNetPlan(int32 Plan);
	float InternetToday() const { return Layout().bRouter ? (float)NetCost(NetPlan) : 0.f; }
	bool bPaketMalam = false;
	int32 SeatTier = 0;			// 0 standar, 1 empuk/busa, 2 gaming
	TArray<FName> Decor;		// dekorasi yang sudah dibeli
	int32 DecorBonus() const;	// tambahan kepuasan dari dekorasi
	int32 DecorVA() const;		// daya yang dipakai dekorasi (kipas, neon, AC)
	bool bFastPC = false;		// PC kencang: tarif wajar PC naik
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
	void PutBackMie();					// taruh lagi di kompor (pemesan sudah pergi)
	bool CanOfferMie(const AJRStation* Station) const;	// pelanggan yang main dan belum makan mau dibawakan mi

	/** Waktu berjalan hanya jika toko buka dan tidak ada kejadian yang menunggu pilihan. */
	bool IsPlaying() const { return Phase == EJRGamePhase::Playing && Choices.Num() == 0; }
	int32 PriceVIP = 8000;
	int32 PriceFor(EJRUnitType Type) const { const int32 P = Type == EJRUnitType::PS ? PricePS : Type == EJRUnitType::VIP ? PriceVIP : PricePC; return bBerkah ? P / 2 : P; }
	int32 FairFor(EJRUnitType Type) const { return Type == EJRUnitType::PS ? (bPS2 ? 3000 : 2000) : Type == EJRUnitType::VIP ? 8000 : (bFastPC ? 4000 : 3000); }
	bool Gate2Open() const;
	/** Jalur jalan pelanggan (lewat tangga jika berpindah lantai). */
	void MakePath(const FVector& From, const FVector& To, TArray<FVector>& Out) const;
	/** Panjang jalan sebenarnya (lewat tangga bila beda lantai), untuk memilih tugas terdekat. */
	float PathLen(const FVector& From, const FVector& To) const;
	/** Grid lantai untuk jalur NPC (dibangun ulang otomatis; lihat JRNav.cpp). */
	struct FJRNavGrid { FVector2D Origin = FVector2D::ZeroVector; int32 NX = 0, NY = 0, Era = -1; float BuiltAt = -100.f; TArray<uint8> Blocked[2]; };
	mutable FJRNavGrid Nav;
	void BuildNav() const;
	bool NavRoute(const FVector& From, const FVector& To, int32 Floor, TArray<FVector>& Out) const;
	static EJRUnitType TypeForItem(FName Id) { return Id == TEXT("vip") ? EJRUnitType::VIP : IsPCItem(Id) ? EJRUnitType::PC : EJRUnitType::PS; }
	int32 MaxFor(EJRUnitType Type) const { return Type == EJRUnitType::VIP ? Layout().MaxVip() : Layout().MaxUnits(); }
	int32 TotalUnits() const { return Stations.Num(); }
	const TArray<TObjectPtr<AJRStation>>& GetStations() const { return Stations; }
	bool bBilling = false;		// sistem billing: nyalakan unit dari komputer kasir
	const FJRDayStats& GetStats() const { return Stats; }

	/* ---------- Kejadian dengan pilihan ---------- */
	TArray<FJRChoice> Choices;
	void PushChoice(const FString& Title, const FString& Body, TArray<FString> Labels, TArray<TFunction<void()>> Actions, bool bNeutral = false);
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

	/* ---------- Mode demo (Config/DefaultGame.ini [JuraganRental] bDemo) ---------- */
	bool bDemo = false;
	FString DemoStoreURL;
	static constexpr int32 DemoLastDay = 7;
	bool bDemoOver = false;
	TArray<FString> DemoSummary;
	void CheckDemoOver();

	/* ---------- Tutorial hari pertama (Main baru) ---------- */
	enum ETut : int32 { TutOff = -1, TutStart = 0, TutPay, TutMie, TutPhone, TutRepair, TutDone };
	int32 TutStep = TutOff;
	static constexpr int32 TutCount = 5;
	bool IsTutorial() const { return TutStep >= 0 && TutStep < TutDone; }
	/** Judul dan petunjuk langkah tutorial yang sedang aktif. */
	void TutorialText(bool bTouch, FString& OutTitle, FString& OutBody) const;
	void TutorialAdvance(int32 FromStep);
	void SkipTutorial();

	/* ---------- Turnamen akhir pekan (Era 2+, tiap hari ke-6, 13, 20, ...) ---------- */
	bool bTourAccepted = false;
	bool bTourRunning = false;
	bool bTourDone = false;
	int32 TourPlayers = 0;
	static constexpr int32 TourMax = 16;
	static constexpr int32 TourFee = 10000;
	static constexpr int32 TourPrize = 100000;
	bool IsTourDay() const { return Era >= 2 && Day % 7 == 6; }
	void OfferTournament();
	void TickTournament();
	void TournamentFinal();

	/* ---------- Kebersihan ---------- */
	void SpawnTrash(const FVector& At, int32 Kind);
	void PickTrash(class AJRTrash* T);
	int32 TrashCount() const { return Trash.Num(); }
	float TrashPenalty() const { return FMath::Min(15.f, 1.5f * Trash.Num()); }

	/* ---------- Misi harian (3) dan mingguan (1) ---------- */
	TArray<FJRMission> Missions;
	FString MissionText(const FJRMission& M) const;
	void NewDailyMissions();
	void NewWeeklyMission();
	void TickMissions(bool bEndOfDay);
	void CountRepair() { Stats.Repairs++; }

	/* ---------- Karyawan (Era 2) ---------- */
	bool Staff[(int32)EJRStaff::Count] = { false, false, false, false };
	static int32 StaffWage(EJRStaff S);
	static const TCHAR* StaffName(EJRStaff S);
	static const TCHAR* StaffDesc(EJRStaff S);
	bool HasStaff(EJRStaff S) const { return Staff[(int32)S]; }
	void ToggleStaff(EJRStaff S);
	int32 TotalWages() const;

	/* ---------- Level Juragan (XP): isi toko terbuka bertahap ---------- */
	int32 Xp = 0;
	static constexpr int32 MaxJuraganLevel = 20;
	static int32 XpForLevel(int32 Level);	// total XP untuk mencapai level ini
	/* ---------- Lencana dan ending (JRBadges.cpp) ---------- */
	static int32 NumBadges();
	static int32 NumEndings();
	static TArray<FString> BadgeLines();	// untuk HP > Info
	bool RecordsProgress() const;			// uji otomatis/bot/promosi tidak menyimpan progres
	void UnlockBadge(const TCHAR* Id);
	void UnlockEnding(const TCHAR* Id, const FString& Story, TArray<FString> Labels = TArray<FString>(), TArray<TFunction<void()>> Actions = TArray<TFunction<void()>>());
	void CountLife(EJRLife Kind);
	void CheckBadges(bool bEndOfDay);
	void CheckEndings();
	int32 NegDays = 0;		// hari berturut-turut kas minus saat tutup
	int32 SeenEndings = 0;	// bit ending yang sudah tampil di simpanan ini

	/* ---------- Kabar pagi dan rencana hari ini (JRDaily.cpp) ---------- */
	EJRNews News = EJRNews::None;
	EJRPlan Plan = EJRPlan::None;
	static FString NewsTitle(EJRNews N);
	static FString NewsText(EJRNews N);
	static FString PlanTitle(EJRPlan P);
	static FString PlanEffect(EJRPlan P);
	int32 PlanCost(EJRPlan P) const;
	void RollMorning();
	void ChoosePlan(EJRPlan P);
	float DailySpawnMul(float H) const;
	float DailyTypeMul(int32 TypeIndex, float H) const;
	float DailyExtraHours(int32 TypeIndex) const;
	float DailyMieMul() const;
	int32 MiePriceNow() const;
	float DailyBreakMul() const;
	float DailySatBonus(const class AJRCustomer* C) const;

	int32 JuraganLevel() const;
	void AddXp(int32 Amount);
	static int32 ItemMinLevel(FName Id);	// level juragan minimal untuk membeli barang (1 = bebas)

	/* ---------- Upgrade unit (Lv 1-5) ---------- */
	static constexpr int32 MaxUnitLevel = 5;
	static int32 UnitLevelReq(int32 Level) { return Level <= 1 ? 1 : 1 + (Level - 1) * 3; }	// Lv2: L4, Lv3: L7, Lv4: L10, Lv5: L13
	static int32 UnitUpgradeCost(EJRUnitType Type, int32 ToLevel);
	static int32 LevelPriceBonus(EJRUnitType Type, int32 Level) { return (FMath::Clamp(Level, 1, 5) - 1) * (Type == EJRUnitType::VIP ? 2000 : 500); }
	static FString UnitLevelName(EJRUnitType Type, int32 Level);
	static constexpr int32 MaintPerLevel = 1500;	// perawatan per level per unit per hari
	int32 UnitPrice(const AJRStation* S) const;
	FString UnitUpgradeLock(const AJRStation* S) const;	// kosong = bisa diupgrade
	bool UpgradeUnit(AJRStation* S);
	int32 UnitLevelOf(EJRUnitType Type, int32 Index) const;
	int32 Maintenance() const;

	/* ---------- Bisnis sampingan Game Center (Era 3) ---------- */
	bool bKulkas = false, bKonter = false, bPrinter = false;
	int32 Drinks = 0;		// botol di kulkas
	int32 Paper = 0;		// lembar kertas di printer
	float Saldo = 0.f;		// saldo deposit konter pulsa
	static constexpr int32 DrinkPrice = 5000;
	static constexpr int32 KratSize = 24;
	static constexpr int32 MaxDrinks = 48;
	static constexpr int32 RimSize = 500;
	static constexpr int32 MaxPaper = 1000;
	static constexpr int32 SheetPrice = 500;
	FJRInteraction ServiceInteraction(EJRService Kind) const;
	void ServiceInteract(EJRService Kind);
	FString ServiceLabel(EJRService Kind, bool& bAlert) const;
	bool HasService(EJRService Kind) const { return Kind == EJRService::Kulkas ? bKulkas : Kind == EJRService::Konter ? bKonter : bPrinter; }

	/* ---------- Cerita misteri "TV 1" (5 bab, menjelang tutup) ---------- */
	int32 MysteryStep = 0;		// 0..4 = bab 1 (TV 1), 5..7 = bab 2 (Naga Online, Era 2+), 8 = semua tamat
	bool bNagaServer = false;	// hadiah bab 2: server Naga Online, gamer lebih sering datang
	bool NagaPaid = false;		// bab 2: sesi Bayu dibayar normal atau gratis
	int32 MysteryDay = 3;		// bab berikutnya paling cepat di hari ini
	bool bGoldCassette = false;	// hadiah tamat: kaset emas di meja kasir

	/* ---------- Kejadian: berantem dan tamu spesial ---------- */
	int32 GuestsDone = 0;		// bit 0 streamer, 1 Pak Lurah, 2 juragan sebelah
	int32 ViralDays = 0;		// rental viral: pengunjung melonjak

	/* ---------- Simpan dan muat ---------- */
	// Otomatis: tiap toko tutup, tiap 2 jam dalam game, saat keluar ke menu/keluar game,
	// dan saat aplikasi Android ditinggal ke latar belakang (HP dikunci, pindah aplikasi).
	static bool HasSave();
	/** Simpan sekarang di tengah hari (bila sedang buka toko). True bila tersimpan. */
	bool SaveNow();
	float SaveFlash = 0.f;		// detik tanda "Tersimpan" masih tampil di HUD
	static constexpr float AutoSaveEveryHours = 2.f;
	/** Ringkasan simpanan untuk menu, mis. "Hari 4 - Garasi - Rp 350.000". Kosong jika tidak ada. */
	static FString SaveSummary();
	bool LoadProgress();
	void BeginNewGame();		// dari menu utama: mulai hari pertama

	/* ---------- "Bang, nambah sejam!" dan momen hari ini ---------- */
	void ExtendSession(AJRStation* Station);

	/* ---------- Nama rental, Kartu Momen, Viralin ---------- */
	FString RentalName = TEXT("Juragan");
	bool bNeedName = false;		// Main baru: minta pemain menamai rental (setelah opening)
	void SetRentalName(const FString& NewName);

	/* ---------- Hidup juragan (JRLife.cpp) ---------- */
	int32 FamilyMood = 60;			// suasana rumah 0-100
	int32 DreamsDone = 0;			// bit per impian yang sudah terwujud
	FString MoodName() const;
	void AddMood(int32 Delta);
	float MoodSpawnMul() const;
	bool HasDream(int32 i) const;
	static void DreamInfo(int32 i, FName& OutId, FString& OutName, FString& OutDesc, int32& OutCost, int32& OutEra);
	void AddDreamItems(TArray<FJRShopItem>& Out) const;
	bool BuyDream(FName Id);
	FString NextDreamLine() const;
	void FamilyEvent(int32 Which);
	void CdJamEvent(class AJRStation* S);
	void OfferBerkah();
	void ReturnRentOut();
	bool bBerkah = false;		// Jumat Berkah: tarif setengah hari ini
	float CelebrateAt = -100.f;	// waktu dunia saat konfeti terakhir dimulai (naik level, impian, ending)
	void Celebrate() { CelebrateAt = GetWorld()->GetTimeSeconds(); }
	bool bRentOut = false;		// satu PS sedang disewa bawa pulang (kembali besok pagi)
	/** Bahasa diganti: bangun ulang ruangan agar papan nama/tulisan di dinding ikut berganti. */
	void OnLanguageChanged();
	TArray<FJRPost> Posts;		// komentar warganet, terbaru di depan
	bool bShowMoment = false;	// Kartu Momen tampil di layar laporan (hari yang baru selesai)
	float LastNet = 0.f;		// untung bersih hari terakhir
	float LastAvgSat = 50.f;	// kepuasan rata-rata hari terakhir
	int32 MomentStars() const { return FMath::Clamp(1 + FMath::FloorToInt((LastAvgSat - 30.f) / 12.5f), 1, 5); }
	void MakeNetizenPosts(float AvgSat);
	void AddMoment(EJRMoment M);
	static FString MomentText(EJRMoment M);

	/* ---------- Adegan pembuka (Main baru, Era 1) ---------- */
	bool IsOpening() const { return Opening.IsValid(); }
	const class AJROpening* GetOpening() const { return Opening.Get(); }
	void SkipOpening();

	/* ---------- Dipanggil pemain ---------- */
	void Confirm();				// Enter: buka toko / mulai hari berikutnya
	void StartSession(AJRStation* Station);
	void CollectNextPayer(bool bByPlayer = false);
	void CollectPayer(AJRCustomer* C, bool bByPlayer = false);
	/** Mini-game kasir: pelanggan bayar pakai uang besar, pemain memilih kembalian yang pas (JRDaily.cpp). */
	void ChangeGame(AJRCustomer* C);
	void ResetMcb();
	AJRCustomer* NextPayer() const;

	/* ---------- Dipanggil pelanggan ---------- */
	void TickPlaying(AJRCustomer* C, float DtH);
	void LeaveAngry(AJRCustomer* C, const FString& Message);
	void CountAngry(bool bQueue) { (bQueue ? Stats.AngryQueue : Stats.AngryStart)++; }
	void RunAway(AJRCustomer* C);
	void CustomerGone(AJRCustomer* C);

	void AddToast(const FString& Text, const FLinearColor& Color = FLinearColor(0.37f, 0.70f, 1.f));

	/* ---------- Menu developer (Config/DefaultGame.ini [JuraganRental] bDevMenu) ----------
	   Dibuka dengan mengetuk baris versi di HP > Atur sebanyak 7 kali. MATIKAN sebelum rilis produksi. */
	bool bDevMenu = false;
	void DevJumpEra(int32 NewEra);
	void DevAddMoney(float Amount);
	void DevLevelUp();
	void DevAddRep(float Amount);

	/* ---------- Rekaman promosi (-jrpromo=N -jrcapture, lihat JRPromo.cpp) ---------- */
	int32 PromoId = 0;			// 0 = tidak aktif; 2..5 = skenario video posting
	float PromoT = 0.f;			// detik sejak skenario mulai (langkah tetap 1/30 dtk dengan -benchmark -fps=30)
	int32 PromoFrame = 0;		// nomor frame rekaman berikutnya
	bool bPromoRec = false;		// sedang merekam
	TArray<FString> PromoCues;	// "frame nama_suara" untuk ditempel FFmpeg
	TArray<FJRChoice> PromoHeld;	// dialog ditahan sebentar agar kejadiannya terlihat dulu
	float PromoHeldUntil = -1.f;
	UPROPERTY()
	TObjectPtr<class ACameraActor> PromoCam;
	void PromoSetup();			// dipanggil di BeginPlay sebelum BuildWorld
	void TickPromo(float DeltaSeconds);
	void PromoCue(const FString& Sound);
	AJRCustomer* PromoSeat(AJRStation* S, int32 TypeIndex);
	bool PromoStart(AJRStation* S);

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
	bool Gate1Open() const { return Money >= 600000.f && Rep >= 40.f; }
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
	TObjectPtr<AJRRadio> Radio;

	UPROPERTY()
	TObjectPtr<class AJRBillingPC> BillingPC;
	void SpawnBillingPC();

	UPROPERTY()
	TObjectPtr<class AJRVisitor> KasirStaff;

	UPROPERTY()
	TObjectPtr<class AJRVisitor> TechStaff;

	UPROPERTY()
	TObjectPtr<class AJRVisitor> CookStaff;

	UPROPERTY()
	TObjectPtr<class AJRVisitor> CleanStaff;

	UPROPERTY()
	TArray<TObjectPtr<class AJRTrash>> Trash;

	int32 CleanState = 0;
	float CleanTimer = 0.f;
	TWeakObjectPtr<class AJRTrash> CleanJob;
	FVector CleanPost() const { return FVector(-(Layout().HalfX - 160.f), -0.25f * Layout().HalfY, 0.f); }

	// Status kerja: 0 di pos, 1 berjalan ke tugas, 2 mengerjakan, 3 kembali ke pos
	int32 TechState = 0, CookState = 0;
	float TechTimer = 0.f, CookTimer = 0.f;
	TWeakObjectPtr<AJRStation> TechJob, CookJob;
	FVector TechPost() const { return FVector(Layout().HalfX - 80.f, Layout().HalfY - 260.f, 0.f); }
	FVector CookPost() const { return Layout().Stove() + FVector(0.f, 75.f, 0.f); }
	FVector StaffSpot(const AJRStation* S) const;
	void SendStaff(class AJRVisitor* V, const FVector& To, float Yaw);
	void DestroyStaff();

	/* Level unit per tipe dan indeks (sumber kebenaran; dipakai ulang saat pindah era dan dimuat) */
	TArray<int32> LvPS, LvPC, LvVIP;
	TArray<int32>& LevelArray(EJRUnitType Type) { return Type == EJRUnitType::PS ? LvPS : Type == EJRUnitType::VIP ? LvVIP : LvPC; }

	/* Bisnis sampingan */
	UPROPERTY()
	TArray<TObjectPtr<class AJRService>> Services;
	void SpawnServices();
	FVector ServicePos(EJRService Kind) const;
	FVector ServiceSpot(EJRService Kind) const;	// tempat pembeli berdiri
	struct FJRWalkIn
	{
		TWeakObjectPtr<class AJRVisitor> V;
		EJRService Kind = EJRService::Konter;
		FString Name;
		int32 Amount = 0;	// nominal top-up (Rp) atau jumlah lembar
		int32 Fee = 0;		// biaya admin top-up
		float Wait = 0.f;	// detik menunggu sejak tiba
		bool bArrived = false;
		bool bHinted = false;
	};
	TArray<FJRWalkIn> WalkIns;
	const FJRWalkIn* WaitingAt(EJRService Kind) const;
	void SpawnWalkIn(EJRService Kind);
	void TickWalkIns(float DeltaSeconds, float DtH);
	void ServeWalkIn(EJRService Kind);
	void ClearWalkIns();
	void SellDrink();
	bool bPaket1Test = false;	// -jrpaket1: uji level, upgrade unit, dan bisnis sampingan

	UPROPERTY()
	TArray<TObjectPtr<class AJRDecor>> DecorActors;
	void SpawnDecor();

	UPROPERTY()
	TArray<TObjectPtr<AJRDeliveryBox>> Boxes;

	int32 OwnedPS = 2;
	int32 OwnedPC = 1;
	int32 OwnedVIP = 0;
	void MoveToEra(int32 NewEra);
	TArray<bool> QueueUsed;
	FJRDayStats Stats;
	float RtAt = -1.f;		// jam Pak RT datang malam ini (-1 = tidak)
	float AmbientTimer = 12.f;	// detik sampai motor lewat berikutnya
	bool bQuiet = false;	// suara dikecilkan setelah ditegur Pak RT

	// Mode uji otomatis (-jrautotest, -jrera2): main sendiri, ambil screenshot, lalu keluar.
	void TickAutoTest(float DeltaSeconds);
	void SaveProgress(bool bMidDay = false);
	float NextAutoSaveHour = 10.f;
	// Siang ke malam: matahari dan cahaya langit mengikuti jam dalam game.
	void TickSky();
	TWeakObjectPtr<class ADirectionalLight> SkySun;
	TWeakObjectPtr<class ASkyLight> SkyFill;
	bool bSkyInit = false;
	float SunBase = 0.f, FillBase = 0.f, SunYaw = 0.f, LastSkyHour = -99.f, LastCaptureDay = -1.f;
	FDelegateHandle AppBgHandle, AppQuitHandle;
	void OnAppBackground();
	void TickMystery(float DeltaSeconds);
	void TickStaff(float DeltaSeconds);
	void TickTutorial(float DeltaSeconds);
	bool TutorialTarget(FVector& Out) const;

	UPROPERTY()
	TObjectPtr<class AJRTutorialArrow> TutArrow;
	void FightEvent(AJRCustomer* A, AJRCustomer* B);
	void TickGuests();
	void ScheduleFamily();
	void TickFamily();
	float FamilyAt = -1.f;		// jam kejadian keluarga hari ini (-1 = tidak ada)
	int32 FamilyWhich = -1;
	int32 LastFamily = -1;		// jangan mengulang kejadian yang sama dua kali berturut-turut
	int32 FamilyTestNext = 0;	// -jrfamily: putar semua kejadian berurutan
	void GuestEvent(int32 Which);
	bool bFightToday = false;
	float GuestAt = -1.f;		// jam tamu spesial datang hari ini (-1 = tidak)
	int32 GuestWhich = -1;
	void SpawnStaffActors();
	float StaffTimer[(int32)EJRStaff::Count] = { 0.f, 0.f, 0.f, 0.f };
	TMap<TWeakObjectPtr<AJRStation>, float> RepairTimers;
	int32 StaffDone[(int32)EJRStaff::Count] = { 0, 0, 0, 0 };
	bool bStaffTest = false;	// -jrstaff: uji otomatis dengan semua karyawan (bot tidak mengerjakan tugas mereka)
	void RunMysteryStep();
	float FlickerTime = 0.f;
	bool bMysteryTest = false;	// -jrmysterytest: jalankan semua bab berurutan lalu keluar
	float MysteryTestTimer = 0.f;
	class AJRVisitor* SpawnMysteryVisitor(const TCHAR* Mesh, const FVector& Where);
	void ClearWorld();
	static const TCHAR* SlotName();
	/** Salinan simpanan kedua (lihat SaveProgress) dan pemuat yang memilih salinan terbaru. */
	static FString BackupPath();
	static class UJRSaveGame* LoadSaveObject();
public:
	/** Catatan hasil simpan terakhir, untuk penguji (menu utama, menu developer aktif). */
	static const FString& SaveNote();
	bool IsDevMenu() const { return bDevMenu; }
private:
	bool bMenuTest = false;		// -jrmenutest: potret menu utama dan pengaturan
	TWeakObjectPtr<class AJROpening> Opening;

	// -jrbalance: bot pemain yang cukup cekatan main beberapa hari (waktu dipercepat) untuk mengukur ekonomi.
	bool bBalance = false;
	int32 BalanceLoggedDay = 0;
	float BalanceReportTime = 0.f;
	void TickBalance(float DeltaSeconds);
	void BalanceBetweenDays(float DeltaSeconds);
	float MenuTestTime = 0.f;
	int32 MenuTestStep = 0;
	bool bAutoTest = false;
	float AutoTestTime = 0.f;
	int32 AutoTestShot = 0;
	int32 AutoTestMieDelivered = 0;
	int32 AutoTestEvents = 0;
	int32 AutoTestInstalled = 0;
	int32 UpsellForced = 0;	// uji: berapa pelanggan sudah dipaksa habis waktunya
	float AutoChoiceTimer = 0.f;
};
