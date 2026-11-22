// Jalur jalan pelanggan dan staf: A* di atas grid lantai agar tidak menembus meja kasir, kompor, meja TV, dan perabot.
// Grid dibangun dari kotak batas komponen yang menghalangi pemain (BlockAll), per lantai, dan dibangun ulang
// paling cepat tiap beberapa detik (unit baru dibeli, pindah era) saat ada yang meminta jalur.

#include "JRGameMode.h"

#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "JRCustomer.h"
#include "JRDelivery.h"
#include "JRFixtures.h"
#include "JRVisitor.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace JRLocal_Nav
{
	constexpr float Cell = 25.f;		// ukuran sel grid (cm)
	constexpr float Agent = 24.f;		// jari-jari badan NPC: jarak minimal ke penghalang
	constexpr float Rebuild = 4.f;		// detik minimal antar pembangunan ulang grid
	constexpr float OutsideX = 320.f;	// grid mencakup halaman depan (pelanggan datang dari jalan)
}
using namespace JRLocal_Nav;

void AJRGameMode::BuildNav() const
{
	const FJRLayout& L = Layout();
	Nav.Era = Era;
	Nav.BuiltAt = GetWorld()->GetTimeSeconds();
	Nav.Origin = FVector2D(-(L.HalfX + OutsideX), -(L.HalfY + Cell));
	Nav.NX = FMath::CeilToInt((2.f * L.HalfX + OutsideX + Cell) / Cell);
	Nav.NY = FMath::CeilToInt((2.f * L.HalfY + 2.f * Cell) / Cell);
	const int32 Floors = L.HasUpstairs() ? 2 : 1;
	for (int32 F = 0; F < 2; ++F) Nav.Blocked[F].Init(0, F < Floors ? Nav.NX * Nav.NY : 0);

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const AActor* A = *It;
		// Yang bergerak atau mudah dilangkahi tidak dihitung sebagai penghalang.
		if (A->IsA<APawn>() || A->IsA<AJRCustomer>() || A->IsA<AJRVisitor>() || A->IsA<AJRDeliveryBox>() || A->IsA<AJRTrash>() || A->IsA<AJRSlotMarker>()) continue;
		TInlineComponentArray<UPrimitiveComponent*> Comps(A);
		for (const UPrimitiveComponent* P : Comps)
		{
			if (!P || !P->IsRegistered() || !P->IsCollisionEnabled() || P->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block) continue;
			const FBox B = P->Bounds.GetBox();
			// Langit, tanah, dan rumah tetangga: tidak ada perabot yang lebar dan panjangnya lebih dari 5 m.
			if (B.GetSize().X > 500.f && B.GetSize().Y > 500.f) continue;
			for (int32 F = 0; F < Floors; ++F)
			{
				const float Z0 = F == 0 ? 0.f : L.FloorH;
				if (B.Max.Z < Z0 + 20.f || B.Min.Z > Z0 + 150.f) continue;	// lantai, plafon, dan barang di lantai lain
				const int32 X0 = FMath::Max(0, FMath::FloorToInt((B.Min.X - Agent - Nav.Origin.X) / Cell));
				const int32 X1 = FMath::Min(Nav.NX - 1, FMath::FloorToInt((B.Max.X + Agent - Nav.Origin.X) / Cell));
				const int32 Y0 = FMath::Max(0, FMath::FloorToInt((B.Min.Y - Agent - Nav.Origin.Y) / Cell));
				const int32 Y1 = FMath::Min(Nav.NY - 1, FMath::FloorToInt((B.Max.Y + Agent - Nav.Origin.Y) / Cell));
				for (int32 X = X0; X <= X1; ++X)
				{
					for (int32 Y = Y0; Y <= Y1; ++Y)
					{
						// Tandai hanya bila titik tengah sel benar-benar di dalam kotak yang diperbesar
						const float CX = Nav.Origin.X + (X + 0.5f) * Cell, CY = Nav.Origin.Y + (Y + 0.5f) * Cell;
						if (CX >= B.Min.X - Agent && CX <= B.Max.X + Agent && CY >= B.Min.Y - Agent && CY <= B.Max.Y + Agent)
							Nav.Blocked[F][X * Nav.NY + Y] = 1;
					}
				}
			}
		}
	}
	int32 Count = 0;
	for (uint8 V : Nav.Blocked[0]) Count += V;
	UE_LOG(LogTemp, Display, TEXT("JR_NAV grid %dx%d lantai=%d terhalang=%d"), Nav.NX, Nav.NY, Floors, Count);
	// Uji -jrnavdump: peta grid lantai 1 ke Saved/nav_grid.txt (# terhalang, baris = X dari jalan ke belakang)
	if (FParse::Param(FCommandLine::Get(), TEXT("jrnavdump")))
	{
		FString Map;
		for (int32 X = 0; X < Nav.NX; ++X)
		{
			for (int32 Y = 0; Y < Nav.NY; ++Y) Map.AppendChar(Nav.Blocked[0][X * Nav.NY + Y] ? TEXT('#') : TEXT('.'));
			Map += TEXT("\n");
		}
		FFileHelper::SaveStringToFile(Map, *(FPaths::ProjectSavedDir() / TEXT("nav_grid.txt")));
	}
}

bool AJRGameMode::NavRoute(const FVector& From, const FVector& To, int32 Floor, TArray<FVector>& Out) const
{
	if (Nav.Blocked[Floor].Num() != Nav.NX * Nav.NY || Nav.NX == 0) return false;
	const TArray<uint8>& G = Nav.Blocked[Floor];
	auto CellOf = [this](const FVector& P, int32& X, int32& Y)
	{
		X = FMath::FloorToInt((P.X - Nav.Origin.X) / Cell);
		Y = FMath::FloorToInt((P.Y - Nav.Origin.Y) / Cell);
		return X >= 0 && Y >= 0 && X < Nav.NX && Y < Nav.NY;
	};
	auto Free = [this, &G](int32 X, int32 Y) { return X >= 0 && Y >= 0 && X < Nav.NX && Y < Nav.NY && G[X * Nav.NY + Y] == 0; };
	auto Center = [this](int32 X, int32 Y, float Z) { return FVector(Nav.Origin.X + (X + 0.5f) * Cell, Nav.Origin.Y + (Y + 0.5f) * Cell, Z); };
	// Titik awal/tujuan bisa berada di dalam penghalang (kursi di depan TV, antrean di samping meja): pakai sel bebas terdekat.
	auto Nearest = [&Free](int32& X, int32& Y)
	{
		if (Free(X, Y)) return true;
		for (int32 R = 1; R <= 8; ++R)
		{
			int32 BX = -1, BY = -1, BD = MAX_int32;
			for (int32 DX = -R; DX <= R; ++DX)
				for (int32 DY = -R; DY <= R; ++DY)
				{
					if (FMath::Max(FMath::Abs(DX), FMath::Abs(DY)) != R || !Free(X + DX, Y + DY)) continue;
					const int32 D = DX * DX + DY * DY;
					if (D < BD) { BD = D; BX = X + DX; BY = Y + DY; }
				}
			if (BX >= 0) { X = BX; Y = BY; return true; }
		}
		return false;
	};
	int32 SX, SY, TX, TY;
	if (!CellOf(From, SX, SY) || !CellOf(To, TX, TY)) return false;
	if (!Nearest(SX, SY) || !Nearest(TX, TY)) return false;

	// A* 8 arah; jalan diagonal hanya bila kedua sisi lurusnya bebas (tidak memotong sudut meja).
	const int32 N = Nav.NX * Nav.NY;
	TArray<float> Cost; Cost.Init(TNumericLimits<float>::Max(), N);
	TArray<int32> Parent; Parent.Init(-1, N);
	TArray<uint8> Closed; Closed.Init(0, N);
	struct FNode { int32 I; float F; };
	TArray<FNode> Open;
	auto Less = [](const FNode& A, const FNode& B) { return A.F < B.F; };
	auto H = [TX, TY](int32 X, int32 Y) { const int32 DX = FMath::Abs(X - TX), DY = FMath::Abs(Y - TY); return (float)FMath::Max(DX, DY) + 0.414f * FMath::Min(DX, DY); };
	const int32 Start = SX * Nav.NY + SY, Goal = TX * Nav.NY + TY;
	Cost[Start] = 0.f;
	Open.HeapPush({ Start, H(SX, SY) }, Less);
	bool bFound = false;
	while (Open.Num() > 0)
	{
		FNode Cur;
		Open.HeapPop(Cur, Less);
		if (Closed[Cur.I]) continue;
		Closed[Cur.I] = 1;
		if (Cur.I == Goal) { bFound = true; break; }
		const int32 CX = Cur.I / Nav.NY, CY = Cur.I % Nav.NY;
		for (int32 DX = -1; DX <= 1; ++DX)
			for (int32 DY = -1; DY <= 1; ++DY)
			{
				if ((DX == 0 && DY == 0) || !Free(CX + DX, CY + DY)) continue;
				if (DX != 0 && DY != 0 && (!Free(CX + DX, CY) || !Free(CX, CY + DY))) continue;
				const int32 NI = (CX + DX) * Nav.NY + (CY + DY);
				if (Closed[NI]) continue;
				const float NC = Cost[Cur.I] + ((DX != 0 && DY != 0) ? 1.414f : 1.f);
				if (NC < Cost[NI])
				{
					Cost[NI] = NC;
					Parent[NI] = Cur.I;
					Open.HeapPush({ NI, NC + H(CX + DX, CY + DY) }, Less);
				}
			}
	}
	if (!bFound) return false;

	TArray<int32> Cells;
	for (int32 I = Goal; I != -1; I = Parent[I]) Cells.Insert(I, 0);

	// Rapikan: lompati titik yang bisa dicapai lurus tanpa melewati sel terhalang.
	auto Clear = [&](const FVector& A, const FVector& B)
	{
		const float D = FVector::Dist2D(A, B);
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(D / (Cell * 0.4f)));
		for (int32 i = 1; i < Steps; ++i)
		{
			int32 X, Y;
			if (!CellOf(FMath::Lerp(A, B, (float)i / Steps), X, Y) || !Free(X, Y)) return false;
		}
		return true;
	};
	const float Z = From.Z;
	TArray<FVector> Pts;
	for (int32 I : Cells) Pts.Add(Center(I / Nav.NY, I % Nav.NY, Z));
	FVector Anchor = Pts[0];
	int32 K = 0;
	while (K < Pts.Num() - 1)
	{
		int32 Far = K + 1;
		for (int32 J = Pts.Num() - 1; J > K + 1; --J) if (Clear(Pts[K], Pts[J])) { Far = J; break; }
		Out.Add(Pts[Far]);
		K = Far;
	}
	// Bila awal sudah di sel bebas dan tujuan lurus terlihat, cukup langsung (tanpa mampir ke tengah sel).
	if (Out.Num() > 0 && FVector::Dist2D(Out.Last(), To) < Cell * 0.75f) Out.Pop();
	if (Out.Num() > 0 && Clear(From, Out[0]) == false && Pts.Num() > 0) Out.Insert(Anchor, 0);
	Out.Add(To);
	return true;
}

void AJRGameMode::MakePath(const FVector& From, const FVector& To, TArray<FVector>& Out) const
{
	Out.Reset();
	const FJRLayout& L = Layout();
	if (Nav.Era != Era || GetWorld()->GetTimeSeconds() - Nav.BuiltAt > Rebuild) BuildNav();
	auto Leg = [this, &Out](const FVector& A, const FVector& B, int32 Floor)
	{
		TArray<FVector> Part;
		if (NavRoute(A, B, Floor, Part)) Out.Append(Part); else Out.Add(B);	// tidak ketemu jalur: jalan lurus seperti dulu
	};
	if (L.HasUpstairs())
	{
		const bool bFromUp = From.Z > L.FloorH * 0.5f, bToUp = To.Z > L.FloorH * 0.5f;
		if (!bFromUp && bToUp) { Leg(From, L.StairBottom(), 0); Out.Add(L.StairTop()); Leg(L.StairTop(), To, 1); return; }
		if (bFromUp && !bToUp) { Leg(From, L.StairTop(), 1); Out.Add(L.StairBottom()); Leg(L.StairBottom(), To, 0); return; }
		Leg(From, To, bFromUp ? 1 : 0);
		return;
	}
	Leg(From, To, 0);
}

float AJRGameMode::PathLen(const FVector& From, const FVector& To) const
{
	// Perkiraan cepat untuk memilih staf terdekat: lewat tangga bila beda lantai, tanpa A*.
	const FJRLayout& L = Layout();
	TArray<FVector> Pts;
	if (L.HasUpstairs())
	{
		const bool bFromUp = From.Z > L.FloorH * 0.5f, bToUp = To.Z > L.FloorH * 0.5f;
		if (!bFromUp && bToUp) { Pts.Add(L.StairBottom()); Pts.Add(L.StairTop()); }
		else if (bFromUp && !bToUp) { Pts.Add(L.StairTop()); Pts.Add(L.StairBottom()); }
	}
	Pts.Add(To);
	float Len = 0.f;
	FVector P = From;
	for (const FVector& X : Pts) { Len += FVector::Dist2D(P, X); P = X; }
	return Len;
}
