#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LostsensePlayerController.generated.h"

UCLASS()
class LOSTSENSEGAME_API ALostsensePlayerController final
    : public APlayerController {
  GENERATED_BODY()

public:
  ALostsensePlayerController();

protected:
  virtual void BeginPlay() override;
  virtual void SetupInputComponent() override;

private:
  void SaveDevelopmentCharacter();
  void LoadDevelopmentCharacter();
  void ResetDevelopmentEncounter();
  FString DevelopmentSavePath() const;

  FString StartupCharacterSnapshot;
};
