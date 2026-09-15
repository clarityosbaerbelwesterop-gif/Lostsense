#include "LostsenseStoryGateActor.h"

#include "LostsenseKnightCharacter.h"

#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"

ALostsenseStoryGateActor::ALostsenseStoryGateActor() {
  PrimaryActorTick.bCanEverTick = false;
  Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
  SetRootComponent(Trigger);
  Trigger->SetBoxExtent(FVector(320.0F, 520.0F, 240.0F));
  Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
  Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ALostsenseStoryGateActor::Configure(
    const ELostsenseStoryBeat InBeat, const ELostsenseStoryBeat InRequiredBeat,
    const bool bInRequiresBeat) {
  Beat = InBeat;
  RequiredBeat = InRequiredBeat;
  bRequiresBeat = bInRequiresBeat;
}

void ALostsenseStoryGateActor::BeginPlay() {
  Super::BeginPlay();
  Trigger->OnComponentBeginOverlap.AddDynamic(
      this, &ALostsenseStoryGateActor::OnGateOverlap);
}

void ALostsenseStoryGateActor::OnGateOverlap(
    UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
    UPrimitiveComponent *OtherComponent, const int32 OtherBodyIndex,
    const bool bFromSweep, const FHitResult &SweepResult) {
  static_cast<void>(OverlappedComponent);
  static_cast<void>(OtherComponent);
  static_cast<void>(OtherBodyIndex);
  static_cast<void>(bFromSweep);
  static_cast<void>(SweepResult);

  if (bCommitted || Cast<ALostsenseKnightCharacter>(OtherActor) == nullptr) {
    return;
  }

  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseStorySubsystem *Story =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
          : nullptr;
  if (Story == nullptr ||
      (bRequiresBeat && !Story->HasBeat(RequiredBeat))) {
    return;
  }

  if (Story->CompleteBeat(Beat) || Story->HasBeat(Beat)) {
    bCommitted = true;
    SetActorEnableCollision(false);
  }
}
