// Antarmuka untuk objek yang bisa diinteraksi pemain dengan tombol E.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JRTypes.h"
#include "JRInteractable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UJRInteractable : public UInterface
{
	GENERATED_BODY()
};

class JURAGANRENTAL_API IJRInteractable
{
	GENERATED_BODY()

public:
	/** Apa yang bisa dilakukan pemain saat melihat objek ini. */
	virtual FJRInteraction GetInteraction() const { return FJRInteraction(); }

	/** Dipanggil saat E ditekan (Press) atau selesai ditahan (Hold). */
	virtual void Interact() {}
};
