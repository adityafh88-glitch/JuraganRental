// Tujuan jangka panjang: 16 lencana dan 4 ending. Progres disimpan per pemain (slot JRSettings),
// tidak per simpanan, jadi tetap ada walau mulai permainan baru. Uji otomatis/bot/rekaman promosi
// tidak menyimpan progres ini ke disk.

#include "JRGameMode.h"
#include "JRLang.h"

#include "JRSettings.h"
#include "JRSound.h"

namespace JRLocal_Badges
{
	const FLinearColor Gold(1.f, 0.78f, 0.3f);
	struct FBadgeDef { const TCHAR* Id; const TCHAR* Title; const TCHAR* Desc; };
	// Teks diterjemahkan saat dipakai (JRT di inisialisasi statis membuat build rilis crash).
	const FBadgeDef Badges[] = {
		{ TEXT("pertama"), TEXT("Pelanggan pertama"), TEXT("Terima bayaran pertama") },	// JRNOWRAP
		{ TEXT("ramai"), TEXT("Rental rame"), TEXT("30 pelanggan bayar dalam sehari") },	// JRNOWRAP
		{ TEXT("sejam"), TEXT("Bang, nambah sejam!"), TEXT("25 kali pelanggan nambah jam") },	// JRNOWRAP
		{ TEXT("teliti"), TEXT("Kasir teliti"), TEXT("20 kali kembalian pas") },	// JRNOWRAP
		{ TEXT("mi"), TEXT("Raja mi instan"), TEXT("Jual 50 mangkuk mi") },	// JRNOWRAP
		{ TEXT("ruko"), TEXT("Pindah ke ruko"), TEXT("Buka rental di ruko kecil") },	// JRNOWRAP
		{ TEXT("gc"), TEXT("Game Center 2 lantai"), TEXT("Buka Game Center") },	// JRNOWRAP
		{ TEXT("lv10"), TEXT("Juragan Lv 10"), TEXT("Capai Level Juragan 10") },	// JRNOWRAP
		{ TEXT("minggu"), TEXT("Seminggu buka"), TEXT("Bertahan sampai hari ke-7") },	// JRNOWRAP
		{ TEXT("bulan"), TEXT("Sebulan buka"), TEXT("Bertahan sampai hari ke-30") },	// JRNOWRAP
		{ TEXT("bintang"), TEXT("Bintang lima"), TEXT("Kepuasan rata-rata 80 dalam sehari") },	// JRNOWRAP
		{ TEXT("adem"), TEXT("Listrik aman"), TEXT("20 pelanggan sehari tanpa MCB turun") },	// JRNOWRAP
		{ TEXT("viral"), TEXT("Viral di Viralin"), TEXT("Rentalmu viral") },	// JRNOWRAP
		{ TEXT("emas"), TEXT("Kaset emas"), TEXT("Tamatkan misteri TV 1") },	// JRNOWRAP
		{ TEXT("naga"), TEXT("Server Naga"), TEXT("Tamatkan misteri Naga Online") },	// JRNOWRAP
		{ TEXT("turnamen"), TEXT("Tuan rumah turnamen"), TEXT("Gelar turnamen sampai final") },	// JRNOWRAP
	};
	const FBadgeDef Endings[] = {
		{ TEXT("sejati"), TEXT("Juragan Sejati"), TEXT("Game Center Rp 10 juta dan reputasi 85") },	// JRNOWRAP
		{ TEXT("legenda"), TEXT("Legenda Kampung"), TEXT("Reputasi 98 saat tutup toko") },	// JRNOWRAP
		{ TEXT("raja"), TEXT("Raja Turnamen"), TEXT("Gelar 4 turnamen sampai final") },	// JRNOWRAP
		{ TEXT("tikar"), TEXT("Gulung Tikar"), TEXT("Kas minus 3 hari berturut-turut") },	// JRNOWRAP
	};
}
using namespace JRLocal_Badges;

int32 AJRGameMode::NumBadges() { return UE_ARRAY_COUNT(Badges); }
int32 AJRGameMode::NumEndings() { return UE_ARRAY_COUNT(Endings); }

bool AJRGameMode::RecordsProgress() const
{
	return !bAutoTest && !bBalance && !bMenuTest && !bMysteryTest && PromoId == 0;
}

TArray<FString> AJRGameMode::BadgeLines()
{
	const UJRSettingsSave* S = JRSettings::Get();
	TArray<FString> Out;
	Out.Add(JRPrintf(JRT(TEXT("LENCANA %d/%d")), S->Badges.Num(), NumBadges()));
	for (const FBadgeDef& B : Badges)
	{
		const bool bHave = S->Badges.Contains(B.Id);
		Out.Add(FString::Printf(TEXT("%s %s - %s"), bHave ? TEXT("[v]") : TEXT("[  ]"), JRT(B.Title), JRT(B.Desc)));
	}
	Out.Add(JRPrintf(JRT(TEXT("ENDING %d/%d")), S->Endings.Num(), NumEndings()));
	for (const FBadgeDef& E : Endings)
	{
		const bool bHave = S->Endings.Contains(E.Id);
		Out.Add(bHave ? FString::Printf(TEXT("[v] %s - %s"), JRT(E.Title), JRT(E.Desc)) : FString::Printf(TEXT("[  ] ??? - %s"), JRT(E.Desc)));
	}
	return Out;
}

void AJRGameMode::UnlockBadge(const TCHAR* Id)
{
	UJRSettingsSave* S = JRSettings::Get();
	if (S->Badges.Contains(Id)) return;
	for (const FBadgeDef& B : Badges)
	{
		if (FCString::Strcmp(B.Id, Id) != 0) continue;
		S->Badges.Add(Id);
		if (RecordsProgress()) JRSettings::Save();
		AddToast(JRPrintf(JRT(TEXT("LENCANA BARU: %s")), JRT(B.Title)), Gold);
		JRSound::Play2D(this, TEXT("SFX_Kaset_Emas"), 0.5f);
		UE_LOG(LogTemp, Display, TEXT("JR_BADGE %s (%d/%d)"), Id, S->Badges.Num(), NumBadges());
		return;
	}
}

void AJRGameMode::UnlockEnding(const TCHAR* Id, const FString& Story, TArray<FString> Labels, TArray<TFunction<void()>> Actions)
{
	UJRSettingsSave* S = JRSettings::Get();
	const bool bNew = !S->Endings.Contains(Id);
	if (bNew)
	{
		S->Endings.Add(Id);
		if (RecordsProgress()) JRSettings::Save();
	}
	for (const FBadgeDef& E : Endings)
	{
		if (FCString::Strcmp(E.Id, Id) != 0) continue;
		UE_LOG(LogTemp, Display, TEXT("JR_ENDING %s baru=%d (%d/%d)"), Id, bNew ? 1 : 0, S->Endings.Num(), NumEndings());
		if (Labels.Num() == 0) { Labels.Add(JRT(TEXT("Lanjut main"))); Actions.Add([]() {}); }
		const FString Body = Story + TEXT("\n\n") + JRPrintf(JRT(TEXT("Ending terbuka: %d dari %d. Lihat semuanya di HP > Info.")), S->Endings.Num(), NumEndings());
		PushChoice(JRPrintf(JRT(TEXT("ENDING: %s")), JRT(E.Title)), Body, MoveTemp(Labels), MoveTemp(Actions));
		JRSound::Play2D(this, TEXT("SFX_Kaset_Emas"), 0.9f);
		return;
	}
}

void AJRGameMode::CountLife(EJRLife Kind)
{
	UJRSettingsSave* S = JRSettings::Get();
	switch (Kind)
	{
	case EJRLife::Paid: S->LifePaid++; break;
	case EJRLife::Upsell: S->LifeUpsell++; break;
	case EJRLife::Mie: S->LifeMie++; break;
	case EJRLife::Exact: S->LifeExact++; break;
	case EJRLife::Tour: S->LifeTours++; break;
	}
	CheckBadges(false);
}

void AJRGameMode::CheckBadges(bool bEndOfDay)
{
	const UJRSettingsSave* S = JRSettings::Get();
	if (S->LifePaid >= 1) UnlockBadge(TEXT("pertama"));
	if (S->LifeUpsell >= 25) UnlockBadge(TEXT("sejam"));
	if (S->LifeExact >= 20) UnlockBadge(TEXT("teliti"));
	if (S->LifeMie >= 50) UnlockBadge(TEXT("mi"));
	if (S->LifeTours >= 1) UnlockBadge(TEXT("turnamen"));
	if (Stats.Paid >= 30) UnlockBadge(TEXT("ramai"));
	if (Era >= 2) UnlockBadge(TEXT("ruko"));
	if (Era >= 3) UnlockBadge(TEXT("gc"));
	if (JuraganLevel() >= 10) UnlockBadge(TEXT("lv10"));
	if (ViralDays > 0) UnlockBadge(TEXT("viral"));
	if (bGoldCassette) UnlockBadge(TEXT("emas"));
	if (bNagaServer) UnlockBadge(TEXT("naga"));
	if (!bEndOfDay) return;

	// Dinilai saat toko tutup (statistik hari itu sudah lengkap)
	if (Day >= 7) UnlockBadge(TEXT("minggu"));
	if (Day >= 30) UnlockBadge(TEXT("bulan"));
	if (Stats.Satisfaction.Num() >= 5)
	{
		float Avg = 0.f;
		for (int32 V : Stats.Satisfaction) Avg += V;
		if (Avg / Stats.Satisfaction.Num() >= 80.f) UnlockBadge(TEXT("bintang"));
	}
	if (Stats.Paid >= 20 && Stats.Trips == 0) UnlockBadge(TEXT("adem"));
}

void AJRGameMode::CheckEndings()
{
	// Dipanggil di akhir EndDay. Ending yang sudah pernah terbuka di simpanan ini tidak diulang.
	const UJRSettingsSave* S = JRSettings::Get();
	NegDays = Money < 0.f ? NegDays + 1 : 0;
	if (Era >= 3 && Money >= 10000000.f && Rep >= 85.f && !(SeenEndings & 1))
	{
		SeenEndings |= 1;
		UnlockEnding(TEXT("sejati"), JRT(TEXT("Dari garasi sempit sampai Game Center 2 lantai. Anak-anak kampung kini menyebut namamu dengan hormat: Juragan Sejati.")));
	}
	if (Rep >= 98.f && !(SeenEndings & 2))
	{
		SeenEndings |= 2;
		UnlockEnding(TEXT("legenda"), JRT(TEXT("Semua orang di kampung pernah main di rentalmu. Pak RT bahkan memasang namamu di papan pengumuman balai warga.")));
	}
	if (S->LifeTours >= 4 && !(SeenEndings & 4))
	{
		SeenEndings |= 4;
		UnlockEnding(TEXT("raja"), JRT(TEXT("Turnamenmu jadi agenda tetap kampung. Tim-tim dari kecamatan sebelah datang hanya untuk bertanding di rentalmu.")));
	}
	if (NegDays >= 3)
	{
		NegDays = 0;
		TArray<FString> Labels = { JRT(TEXT("Pinjam modal Emak (Rp 300.000)")) };
		TArray<TFunction<void()>> Actions = { [this]()
		{
			Money += 300000.f;
			AddToast(JRT(TEXT("Emak meminjamkan Rp 300.000. \"Jangan sampai bangkrut lagi ya, Nak.\"")), FLinearColor(0.36f, 0.79f, 0.54f));
		} };
		UnlockEnding(TEXT("tikar"), JRT(TEXT("Tiga hari berturut-turut kas minus. Rolling door ditutup, TV ditutupi kain. Tapi Emak tidak tega melihatmu menyerah...")), MoveTemp(Labels), MoveTemp(Actions));
	}
}
