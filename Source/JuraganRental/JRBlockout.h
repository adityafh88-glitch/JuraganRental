// Helper untuk membangun geometri greybox dari bentuk dasar engine.

#pragma once

#include "CoreMinimal.h"

class AActor;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

namespace JRBlockout
{
	enum class EShape : uint8 { Cube, Cylinder, Sphere };

	/** Warna dari kode hex sRGB, misalnya 0x6b4a2f. */
	JURAGANRENTAL_API FLinearColor Hex(uint32 Rgb);

	/** Tambah bagian berbentuk dasar saat runtime. Size dan Center dalam cm, relatif ke Parent. */
	JURAGANRENTAL_API UStaticMeshComponent* AddPart(AActor* Owner, USceneComponent* Parent, EShape Shape,
		const FVector& Size, const FVector& Center, const FLinearColor& Color, bool bCollide = true);

	/** Ganti warna bagian. Mengembalikan material dinamis agar warna bisa diubah lagi. */
	JURAGANRENTAL_API UMaterialInstanceDynamic* Paint(UStaticMeshComponent* Part, const FLinearColor& Color);
}
