// Gameplay harian: "Kabar pagi" (kejadian acak yang mengubah jenis dan jumlah pengunjung hari itu),
// "Rencana hari ini" (pilih 1 dari 3 kartu tiap pagi, berlaku sehari), dan mini-game "Kembalian pas" di kasir.

#include "JRGameMode.h"
#include "JRLang.h"

#include "JRCustomer.h"
#include "JRFixtures.h"
#include "JRSound.h"

namespace JRLocal_Daily
{
	const FLinearColor Good(0.36f, 0.79f, 0.54f);
	constexpr int32 Bocil = 0, Sma = 1, Mahasiswa = 2, Pekerja = 3, Gamer = 4, Bapak = 5;
}
using namespace JRLocal_Daily;

FString AJRGameMode::NewsTitle(EJRNews N)
{
	switch (N)
	{
	case EJRNews::Gajian: return JRT(TEXT("Hari gajian"));
	case EJRNews::LiburSekolah: return JRT(TEXT("Libur sekolah"));
	case EJRNews::Hujan: return JRT(TEXT("Hujan seharian"));
	case EJRNews::TanggalTua: return JRT(TEXT("Tanggal tua"));
	case EJRNews::BolaMalam: return JRT(TEXT("Laga bola besar malam ini"));
	default: return JRT(TEXT("Hari biasa"));
	}
}

FString AJRGameMode::NewsText(EJRNews N)
{
	switch (N)
	{
	case EJRNews::Gajian: return JRT(TEXT("Tanggal muda! Mas-mas kantoran dan mahasiswa baru gajian dan kiriman, mainnya lebih lama."));
	case EJRNews::LiburSekolah: return JRT(TEXT("Sekolah libur. Bocil dan anak SMA datang dari pagi sampai sore."));
	case EJRNews::Hujan: return JRT(TEXT("Hujan dari pagi. Pengunjung lebih sedikit, tapi yang datang betah lama dan doyan mi."));
	case EJRNews::TanggalTua: return JRT(TEXT("Dompet warga menipis. Pengunjung lebih sepi dan jarang jajan."));
	case EJRNews::BolaMalam: return JRT(TEXT("Ada laga bola besar malam ini. Mulai pukul 19.00 TV PS bakal diserbu."));
	default: return JRT(TEXT("Tidak ada kabar istimewa. Saatnya buka toko seperti biasa."));
	}
}

int32 AJRGameMode::PlanCost(EJRPlan P) const
{
	const int32 E = FMath::Clamp(Era, 1, 3);
	switch (P)
	{
	case EJRPlan::Brosur: return 15000 * E;
	case EJRPlan::Genset: return 20000 * E;
	case EJRPlan::StikBaru: return 10000 * E;
	default: return 0;
	}
}

FString AJRGameMode::PlanTitle(EJRPlan P)
{
	switch (P)
	{
	case EJRPlan::Brosur: return JRT(TEXT("Sebar brosur"));
	case EJRPlan::Pelajar: return JRT(TEXT("Promo pelajar"));
	case EJRPlan::MiPedas: return JRT(TEXT("Mi pedas level 5"));
	case EJRPlan::Genset: return JRT(TEXT("Sewa genset"));
	case EJRPlan::StikBaru: return JRT(TEXT("Stik dan kabel baru"));
	case EJRPlan::Begadang: return JRT(TEXT("Paket begadang"));
	case EJRPlan::Bersih: return JRT(TEXT("Bersih-bersih total"));
	case EJRPlan::Tutor: return JRT(TEXT("Ajari bocil main"));
	default: return FString();
	}
}

FString AJRGameMode::PlanEffect(EJRPlan P)
{
	switch (P)
	{
	case EJRPlan::Brosur: return JRT(TEXT("pengunjung +25%"));
	case EJRPlan::Pelajar: return JRT(TEXT("bocil dan anak SMA datang 2x, lebih puas"));
	case EJRPlan::MiPedas: return JRT(TEXT("mi dipesan 2x, harga mi +Rp 1.000"));
	case EJRPlan::Genset: return JRT(TEXT("MCB tidak akan turun hari ini"));
	case EJRPlan::StikBaru: return JRT(TEXT("unit jarang rusak, pelanggan lebih puas"));
	case EJRPlan::Begadang: return JRT(TEXT("pengunjung malam (21.00+) +60%"));
	case EJRPlan::Bersih: return JRT(TEXT("semua sampah hilang, reputasi +1"));
	case EJRPlan::Tutor: return JRT(TEXT("XP hari ini +25%"));
	default: return FString();
	}
}

void AJRGameMode::RollMorning()
{
	News = EJRNews::None;
	Plan = EJRPlan::None;
	if (Day < 2 || PromoId > 0) return;	// hari pertama: tutorial saja

	// Kabar pagi: 35% hari biasa, sisanya kejadian (bola besar hanya kalau ada TV PS)
	TArray<EJRNews> Pool = { EJRNews::Gajian, EJRNews::LiburSekolah, EJRNews::Hujan, EJRNews::TanggalTua };
	if (Count(EJRUnitType::PS) > 0) Pool.Add(EJRNews::BolaMalam);
	if (FMath::FRand() >= 0.35f) News = Pool[FMath::RandRange(0, Pool.Num() - 1)];

	// Tiga kartu rencana acak (genset hanya berguna kalau daya pas-pasan)
	TArray<EJRPlan> Deck = { EJRPlan::Brosur, EJRPlan::Pelajar, EJRPlan::MiPedas, EJRPlan::StikBaru, EJRPlan::Begadang, EJRPlan::Bersih, EJRPlan::Tutor };
	if (Era < 3) Deck.Add(EJRPlan::Genset);
	TArray<EJRPlan> Offer;
	while (Offer.Num() < 3 && Deck.Num() > 0)
	{
		const EJRPlan P = Deck[FMath::RandRange(0, Deck.Num() - 1)];
		Offer.Add(P);
		Deck.Remove(P);
	}

	TArray<FString> Labels;
	TArray<TFunction<void()>> Actions;
	for (EJRPlan P : Offer)
	{
		const int32 Cost = PlanCost(P);
		Labels.Add(Cost > 0 ? JRPrintf(JRT(TEXT("%s (%s): %s")), *PlanTitle(P), *JRFormatRp(Cost), *PlanEffect(P))
			: JRPrintf(JRT(TEXT("%s: %s")), *PlanTitle(P), *PlanEffect(P)));
		Actions.Add([this, P]() { ChoosePlan(P); });
	}
	const FString Body = NewsText(News) + TEXT("\n\n") + JRT(TEXT("Pilih rencana hari ini:"));
	PushChoice(JRPrintf(JRT(TEXT("Kabar pagi hari ke-%d: %s")), Day, *NewsTitle(News)), Body, MoveTemp(Labels), MoveTemp(Actions));
	if (bAutoTest || bBalance) UE_LOG(LogTemp, Display, TEXT("JR_DAILY hari=%d kabar=%d kartu=%d,%d,%d"), Day, (int32)News, Offer.Num() > 0 ? (int32)Offer[0] : -1, Offer.Num() > 1 ? (int32)Offer[1] : -1, Offer.Num() > 2 ? (int32)Offer[2] : -1);
}

void AJRGameMode::ChoosePlan(EJRPlan P)
{
	const int32 Cost = PlanCost(P);
	if (Cost > 0 && Money < Cost)
	{
		AddToast(JRPrintf(JRT(TEXT("Kas kurang untuk %s. Hari ini tanpa rencana khusus.")), *PlanTitle(P)), FLinearColor(0.94f, 0.42f, 0.36f));
		Plan = EJRPlan::None;
		return;
	}
	Plan = P;
	Money -= Cost;
	Stats.Stock += Cost;
	if (P == EJRPlan::Bersih)
	{
		for (AJRTrash* T : TArray<TObjectPtr<AJRTrash>>(Trash)) if (T) T->Destroy();
		Trash.Reset();
		Rep = FMath::Min(100.f, Rep + 1.f);
		Stats.RepDelta += 1.f;
	}
	JRSound::Play2D(this, TEXT("SFX_Kaching"), 0.5f);
	AddToast(JRPrintf(JRT(TEXT("Rencana hari ini: %s (%s)")), *PlanTitle(P), *PlanEffect(P)), Good);
	if (bAutoTest || bBalance) UE_LOG(LogTemp, Display, TEXT("JR_DAILY rencana=%d biaya=%d kas=%.0f"), (int32)P, Cost, Money);
}

float AJRGameMode::DailySpawnMul(float H) const
{
	float M = 1.f;
	switch (News)
	{
	case EJRNews::Gajian: M *= 1.2f; break;
	case EJRNews::LiburSekolah: M *= H < 17.f ? 1.35f : 1.f; break;
	case EJRNews::Hujan: M *= 0.8f; break;
	case EJRNews::TanggalTua: M *= 0.85f; break;
	case EJRNews::BolaMalam: M *= H >= 19.f ? 1.5f : 1.f; break;
	default: break;
	}
	if (Plan == EJRPlan::Brosur) M *= 1.25f;
	if (Plan == EJRPlan::Begadang && H >= 21.f) M *= 1.6f;
	return M;
}

float AJRGameMode::DailyTypeMul(int32 TypeIndex, float H) const
{
	float M = 1.f;
	const bool bPelajar = TypeIndex == Bocil || TypeIndex == Sma;
	if (News == EJRNews::Gajian && (TypeIndex == Pekerja || TypeIndex == Mahasiswa)) M *= 2.f;
	if (News == EJRNews::LiburSekolah && bPelajar) M *= 3.f;
	if (News == EJRNews::BolaMalam && H >= 19.f && (TypeIndex == Sma || TypeIndex == Bapak || TypeIndex == Pekerja)) M *= 2.f;
	if (Plan == EJRPlan::Pelajar && bPelajar) M *= 2.f;
	return M;
}

float AJRGameMode::DailyExtraHours(int32 TypeIndex) const
{
	if (TypeIndex == Bocil) return 0.f;	// bocil tetap sejam (dicari emak)
	if (News == EJRNews::Hujan) return 1.f;
	if (News == EJRNews::Gajian && (TypeIndex == Pekerja || TypeIndex == Mahasiswa)) return 0.5f;
	return 0.f;
}

float AJRGameMode::DailyMieMul() const
{
	float M = News == EJRNews::Hujan ? 1.6f : News == EJRNews::TanggalTua ? 0.6f : 1.f;
	if (Plan == EJRPlan::MiPedas) M *= 2.f;
	return M;
}

int32 AJRGameMode::MiePriceNow() const
{
	return MiePrice + (Plan == EJRPlan::MiPedas ? 1000 : 0);
}

float AJRGameMode::DailyBreakMul() const
{
	return Plan == EJRPlan::StikBaru ? 0.4f : 1.f;
}

float AJRGameMode::DailySatBonus(const AJRCustomer* C) const
{
	float B = 0.f;
	if (Plan == EJRPlan::StikBaru) B += 4.f;
	if (Plan == EJRPlan::Pelajar && C && (C->TypeIndex == Bocil || C->TypeIndex == Sma)) B += 6.f;
	return B;
}

/* ---------- Mini-game kasir: kembalian pas ---------- */

void AJRGameMode::ChangeGame(AJRCustomer* C)
{
	const int32 Bill = FMath::RoundToInt(C->Bill);
	// Uang yang diserahkan: pecahan terkecil yang lebih besar dari tagihan (kembalian tidak pernah nol)
	static const int32 Notes[] = { 5000, 10000, 20000, 50000, 100000 };
	int32 Note = Notes[UE_ARRAY_COUNT(Notes) - 1];
	for (int32 N : Notes) if (N > Bill) { Note = N; break; }
	if (Note <= Bill) Note = (Bill / 50000 + 1) * 50000;
	const int32 Right = Note - Bill;
	// Dua jawaban salah yang menjebak: selisih 500 / 1.000 / 2.000 ke atas atau ke bawah
	TArray<int32> Opts = { Right };
	static const int32 Off[] = { -2000, -1000, -500, 500, 1000, 2000 };
	while (Opts.Num() < 3)
	{
		const int32 V = Right + Off[FMath::RandRange(0, UE_ARRAY_COUNT(Off) - 1)];
		if (V > 0 && !Opts.Contains(V)) Opts.Add(V);
	}
	for (int32 i = Opts.Num() - 1; i > 0; --i) Opts.Swap(i, FMath::RandRange(0, i));	// acak urutan

	TWeakObjectPtr<AJRCustomer> WC(C);
	TArray<FString> Labels;
	TArray<TFunction<void()>> Actions;
	for (int32 Given : Opts)
	{
		Labels.Add(JRFormatRp(Given));
		Actions.Add([this, WC, Given, Right]()
		{
			AJRCustomer* X = WC.Get();
			if (!X || X->State != EJRCustomerState::WaitPay) return;
			float Paid = X->Bill;
			if (Given == Right)
			{
				Stats.ExactChange++;
				CountLife(EJRLife::Exact);
				X->SatAdj += 6.f;
				AddXp(1);
				if (FMath::FRand() < 0.3f)
				{
					const float Tip = FMath::RandBool() ? 500.f : 1000.f;
					Paid += Tip;
					AddToast(JRPrintf(JRT(TEXT("PAS! %s senang, kasih tip %s")), *X->DisplayName, *JRFormatRp(Tip)), Good);
				}
				else AddToast(JRPrintf(JRT(TEXT("PAS! Kembalian %s tepat")), *JRFormatRp(Right)), Good);
				JRSound::Play2D(this, TEXT("SFX_Ding"), 0.6f);
			}
			else if (Given < Right)
			{
				X->SatAdj -= 12.f;
				Stats.RepDelta -= 0.25f;
				AddToast(JRPrintf(JRT(TEXT("Kembalian kurang! %s protes, \"Bang, kurang %s!\"")), *X->DisplayName, *JRFormatRp(Right - Given)), FLinearColor(0.94f, 0.42f, 0.36f));
			}
			else
			{
				Paid -= Given - Right;	// kelebihan kembalian = rugi
				AddToast(JRPrintf(JRT(TEXT("Kelebihan kembalian %s. Rugi deh!")), *JRFormatRp(Given - Right)), FLinearColor(0.94f, 0.42f, 0.36f));
			}
			if (bAutoTest) UE_LOG(LogTemp, Display, TEXT("JR_CHANGE beri=%d benar=%d pas=%d"), Given, Right, Given == Right ? 1 : 0);
			FinishPayment(X, Paid);
		});
	}
	if (bAutoTest) UE_LOG(LogTemp, Display, TEXT("JR_CHANGE tanya tagihan=%d uang=%d benar=%d"), Bill, Note, Right);
	PushChoice(JRT(TEXT("Kembalian pas?")),
		JRPrintf(JRT(TEXT("%s bayar tagihan %s pakai uang %s. Berapa kembaliannya?")), *C->DisplayName, *JRFormatRp(Bill), *JRFormatRp(Note)),
		MoveTemp(Labels), MoveTemp(Actions), true);	// tombol sama warna: jangan beri petunjuk jawaban
}
