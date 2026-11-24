// Tawaran iklan berhadiah (rewarded). Selalu pilihan pemain ("Nanti saja" selalu ada), paling banyak sekali
// per jenis per hari, dan hanya muncul kalau iklannya sudah siap (JRAds::IsReady). Tidak ada iklan paksa.

#include "JRGameMode.h"
#include "JRLang.h"

#include "JRAds.h"
#include "JRSound.h"

namespace JRLocal_RewardAds
{
	const FLinearColor Good(0.36f, 0.79f, 0.54f);
	const FLinearColor Info(0.37f, 0.70f, 1.f);
	float EraScale(int32 Era) { return Era >= 3 ? 3.f : Era == 2 ? 1.5f : 0.6f; }
}
using namespace JRLocal_RewardAds;

bool AJRGameMode::OfferAd(int32 Bit, const FString& Title, const FString& Body, TFunction<void()> Reward)
{
	if ((AdOffers & Bit) || PromoId > 0 || bBalance || IsTutorial() || !JRAds::IsReady()) return false;
	AdOffers |= Bit;
	TWeakObjectPtr<AJRGameMode> WeakThis(this);
	PushChoice(Title, Body, { JRT(TEXT("Tonton iklan (dapat hadiah)")), JRT(TEXT("Nanti saja")) },
		{ [WeakThis, Reward]()
		  {
			  JRAds::Show([WeakThis, Reward](bool bEarned)
			  {
				  AJRGameMode* GM = WeakThis.Get();
				  if (!GM) return;
				  if (bEarned) { Reward(); JRSound::Play2D(GM, TEXT("SFX_Kaching"), 0.7f); GM->Celebrate(); }
				  else GM->AddToast(JRT(TEXT("Iklan belum selesai ditonton, hadiah tidak didapat")), Info);
			  });
		  },
		  []() {} }, true);
	if (bAutoTest) UE_LOG(LogTemp, Display, TEXT("JR_ADOFFER %d"), Bit);
	return true;
}

void AJRGameMode::OfferAdEndOfDay()
{
	// Dipanggil di akhir EndDay (setelah laporan dan simpanan): untung x1,5 atau kiriman Emak saat kas minus
	const float K = EraScale(Era);
	if (Money < 0.f)
	{
		const float Help = FMath::RoundToFloat(130000.f * K / 1000.f) * 1000.f;
		OfferAd(8, JRT(TEXT("Kas minus")),
			JRPrintf(JRT(TEXT("Kas sedang minus. Tonton satu iklan pendek, dan Emak mengirim uang %s untuk modal besok.")), *JRFormatRp(Help)),
			[this, Help]()
			{
				Money += Help;
				AddToast(JRPrintf(JRT(TEXT("Emak transfer %s. \"Semangat, Nak!\"")), *JRFormatRp(Help)), Good);
				ReportLines.Remove(JRT(TEXT("Permainan tersimpan otomatis.")));	// SaveProgress menambahkannya lagi
				SaveProgress();
			});
		return;
	}
	if (LastNet <= 0.f) return;
	const float Bonus = FMath::RoundToFloat(FMath::Min(LastNet * 0.5f, 100000.f * K) / 500.f) * 500.f;
	if (Bonus < 1000.f) return;
	OfferAd(1, JRT(TEXT("Bonus juragan")),
		JRPrintf(JRT(TEXT("Untung bersih hari ini %s. Tonton satu iklan pendek untuk bonus +%s (untung x1,5)?")), *JRFormatRp(LastNet), *JRFormatRp(Bonus)),
		[this, Bonus]()
		{
			Money += Bonus;
			ReportLines.Add(JRPrintf(JRT(TEXT("Bonus iklan: +%s")), *JRFormatRp(Bonus)));
			AddToast(JRPrintf(JRT(TEXT("Bonus juragan +%s masuk kas!")), *JRFormatRp(Bonus)), Good);
			ReportLines.Remove(JRT(TEXT("Permainan tersimpan otomatis.")));	// SaveProgress menambahkannya lagi
			SaveProgress();
		});
}

void AJRGameMode::OfferAdCourier()
{
	if (Phase != EJRGamePhase::Playing || PendingOrders.Num() == 0) return;
	OfferAd(2, JRT(TEXT("Kurir kilat")),
		JRT(TEXT("Pesananmu baru diantar besok pagi. Tonton satu iklan pendek supaya kurir langsung datang sekarang?")),
		[this]()
		{
			const int32 N = PendingOrders.Num();
			for (const FName& Id : PendingOrders) SpawnDelivery(Id);
			PendingOrders.Reset();
			AddToast(JRPrintf(JRT(TEXT("Kurir kilat datang! %d kardus di depan pintu.")), N), Good);
		});
}

void AJRGameMode::OfferAdMcb()
{
	OfferAd(4, JRT(TEXT("MCB jeglek!")),
		JRT(TEXT("Listrik padam dan pelanggan mulai gelisah. Tonton satu iklan pendek: tukang listrik langsung menaikkan MCB dan menjaga daya aman selama 1 jam?")),
		[this]()
		{
			ResetMcb();
			McbGuardUntil = Hour + 1.f;
			AddToast(JRT(TEXT("Tukang listrik siaga 1 jam: MCB tidak akan turun.")), Good);
		});
}
