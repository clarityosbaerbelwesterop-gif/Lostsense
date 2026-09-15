#include "LostsenseNpcActor.h"

#include "LostsenseKnightCharacter.h"
#include "LostsenseStorySubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace {
ELostsenseStoryBeat BeatForNpc(const ELostsenseNpcIdentity Identity) {
  switch (Identity) {
  case ELostsenseNpcIdentity::MaraVenn:
    return ELostsenseStoryBeat::MetMara;
  case ELostsenseNpcIdentity::HadrunPike:
    return ELostsenseStoryBeat::MetHadrun;
  case ELostsenseNpcIdentity::TamsinCoil:
    return ELostsenseStoryBeat::BellgraveDepartureAllowed;
  }
  return ELostsenseStoryBeat::ReturnedAwakened;
}

FText LineForNpc(const ELostsenseNpcIdentity Identity, const int32 Index) {
  switch (Identity) {
  case ELostsenseNpcIdentity::MaraVenn:
    return Index == 0
               ? FText::FromString(TEXT(
                     "Seven years sealed below, and you still carry the Ninth Descent plate marks. The bell rang this morning. I remember it. The town does not."))
               : FText::FromString(TEXT(
                     "Find Hadrun. He knows what the Bellwardens buried, even if he hates admitting it."));
  case ELostsenseNpcIdentity::HadrunPike:
    return Index == 0
               ? FText::FromString(TEXT(
                     "If you are truly the Returned, prove you can still hold a measure. Guard the strike, then answer it. After that I open the Ravelwood road."))
               : FText::FromString(TEXT(
                     "Weeping Cut reaches the old survey line. Bring back anything marked Ninth Descent."));
  case ELostsenseNpcIdentity::TamsinCoil:
    return Index == 0
               ? FText::FromString(TEXT(
                     "Bring me what the shaft spits out. I can tell Bellwarden bronze from Charter scrap without asking whose ledger it came from."))
               : FText::FromString(TEXT(
                     "The Ninth Cage brake is older than the Charter. That should worry you more than it worries them."));
  }
  return FText::GetEmpty();
}
} // namespace

ALostsenseNpcActor::ALostsenseNpcActor() {
  PrimaryActorTick.bCanEverTick = false;

  Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
  SetRootComponent(Visual);
  Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  Visual->SetCollisionObjectType(ECC_WorldDynamic);

  static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
      TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
  if (CylinderMesh.Succeeded()) {
    Visual->SetStaticMesh(CylinderMesh.Object);
  }
  SetActorScale3D(FVector(0.48F, 0.48F, 0.95F));
}

void ALostsenseNpcActor::Configure(const ELostsenseNpcIdentity identity) {
  Identity = identity;
  DialogueIndex = 0;
  bConfigured = true;
}

FText ALostsenseNpcActor::GetInteractionPrompt() const {
  return FText::Format(FText::FromString(TEXT("Talk to {0}")), GetDisplayName());
}

bool ALostsenseNpcActor::CanInteract(
    const ALostsenseKnightCharacter &Interactor) const {
  return bConfigured && !Interactor.IsPendingKillPending() &&
         FVector::DistSquared(GetActorLocation(), Interactor.GetActorLocation()) <=
             FMath::Square(240.0F);
}

bool ALostsenseNpcActor::Interact(ALostsenseKnightCharacter &Interactor) {
  if (!CanInteract(Interactor)) {
    return false;
  }

  UGameInstance *GameInstance = GetGameInstance();
  ULostsenseStorySubsystem *Story =
      GameInstance != nullptr
          ? GameInstance->GetSubsystem<ULostsenseStorySubsystem>()
          : nullptr;
  if (Story == nullptr || !Story->IsStoryReady()) {
    return false;
  }

  static_cast<void>(Story->CompleteBeat(BeatForNpc(Identity)));
  DialogueIndex = FMath::Min(DialogueIndex + 1, 1);
  return true;
}

ELostsenseNpcIdentity ALostsenseNpcActor::GetIdentity() const { return Identity; }

FText ALostsenseNpcActor::GetDisplayName() const {
  switch (Identity) {
  case ELostsenseNpcIdentity::MaraVenn:
    return FText::FromString(TEXT("Mara Venn"));
  case ELostsenseNpcIdentity::HadrunPike:
    return FText::FromString(TEXT("Hadrun Pike"));
  case ELostsenseNpcIdentity::TamsinCoil:
    return FText::FromString(TEXT("Tamsin Coil"));
  }
  return FText::FromString(TEXT("Unknown"));
}

FText ALostsenseNpcActor::GetCurrentLine() const {
  return LineForNpc(Identity, DialogueIndex);
}
