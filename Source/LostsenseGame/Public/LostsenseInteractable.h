#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "LostsenseInteractable.generated.h"

class ALostsenseKnightCharacter;

UINTERFACE(MinimalAPI)
class ULostsenseInteractable : public UInterface {
  GENERATED_BODY()
};

class LOSTSENSEGAME_API ILostsenseInteractable {
  GENERATED_BODY()

public:
  virtual FText GetInteractionPrompt() const = 0;
  virtual bool
  CanInteract(const ALostsenseKnightCharacter &Interactor) const = 0;
  virtual bool Interact(ALostsenseKnightCharacter &Interactor) = 0;
};
