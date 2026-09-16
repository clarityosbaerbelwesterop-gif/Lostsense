#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "LostsenseDeepMoverActor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ELostsenseDeepMoverKind : uint8 {
  LanternRailCart,
  ServiceCage,
  VentilationRotor
};

UCLASS()
class LOSTSENSEGAME_API ALostsenseDeepMoverActor final : public AActor {
  GENERATED_BODY()

public:
  ALostsenseDeepMoverActor();

  void Configure(ELostsenseDeepMoverKind InKind, FVector InPrimaryOffset,
                 FVector InAlternateOffset = FVector::ZeroVector);

protected:
  virtual void Tick(float DeltaSeconds) override;

private:
  UPROPERTY(VisibleAnywhere)
  TObjectPtr<UStaticMeshComponent> Visual;

  UPROPERTY(VisibleInstanceOnly)
  ELostsenseDeepMoverKind Kind = ELostsenseDeepMoverKind::LanternRailCart;

  bool bConfigured = false;
  FVector Origin = FVector::ZeroVector;
  FVector PrimaryOffset = FVector::ZeroVector;
  FVector AlternateOffset = FVector::ZeroVector;
};
