#pragma once

#include "CoreMinimal.h"
#include "Lostsense/Gameplay/Items/LootRuntime.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LostsenseRuntimeSubsystem.generated.h"

namespace Lostsense::Combat {
class Combatant;
struct DamageSpec;
} // namespace Lostsense::Combat

namespace Lostsense::Gameplay {
class EffectRuntime;
struct GeneratedLootEntry;
} // namespace Lostsense::Gameplay

UENUM(BlueprintType)
enum class ELostsensePresentationEventType : uint8 {
  DamageApplied,
  CombatantDied,
  EffectApplied,
  EffectRemoved,
  AbilityActivated,
  ItemDropped,
  ItemPickedUp,
  ItemEquipped,
  ItemUnequipped,
  SkillAllocated,
  SkillRefunded
};

USTRUCT(BlueprintType)
struct FLostsensePresentationEvent {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  int64 Sequence = 0;

  UPROPERTY(BlueprintReadOnly)
  ELostsensePresentationEventType Type =
      ELostsensePresentationEventType::DamageApplied;

  UPROPERTY(BlueprintReadOnly)
  int64 SourceId = 0;

  UPROPERTY(BlueprintReadOnly)
  int64 TargetId = 0;

  UPROPERTY(BlueprintReadOnly)
  int64 ContentId = 0;

  UPROPERTY(BlueprintReadOnly)
  double Value = 0.0;

  UPROPERTY(BlueprintReadOnly)
  int32 Quantity = 0;
};

UCLASS()
class LOSTSENSEGAME_API ULostsenseRuntimeSubsystem final
    : public UGameInstanceSubsystem {
  GENERATED_BODY()

public:
  ULostsenseRuntimeSubsystem();
  virtual ~ULostsenseRuntimeSubsystem() override;

  virtual void Initialize(FSubsystemCollectionBase &Collection) override;
  virtual void Deinitialize() override;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Runtime")
  bool IsPortableRuntimeReady() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Runtime")
  double GetPlayerHealth() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Runtime")
  double GetPlayerMaximumHealth() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Runtime")
  double GetPlayerResource() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Runtime")
  double GetPlayerMaximumResource() const;

  UFUNCTION(BlueprintPure, Category = "Lostsense|Progression")
  int32 GetUnspentSkillPoints() const;

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Progression")
  bool GrantSkillPoints(int32 Points);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Progression")
  bool AllocateSkillNode(int32 SkillNodeId);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Loadout")
  bool EquipAbilityInSlot(int32 SlotIndex, int32 AbilityId);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Inventory")
  bool EquipFirstInventoryItem();

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Save")
  bool SaveCharacterToText(FString &OutPayload) const;

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Save")
  bool LoadCharacterFromText(const FString &Payload);

  UFUNCTION(BlueprintCallable, Category = "Lostsense|Presentation")
  TArray<FLostsensePresentationEvent> DrainPresentationEvents();

  void AdvancePlayerTime(float DeltaSeconds);
  bool ActivatePlayerAbility(uint32 AbilityId);
  bool ActivatePlayerAbilityAgainst(
      uint32 AbilityId, Lostsense::Combat::Combatant &Target,
      Lostsense::Gameplay::EffectRuntime &TargetEffects);
  bool ActivatePlayerLoadoutSlot(
      int32 SlotIndex, Lostsense::Combat::Combatant *Target = nullptr,
      Lostsense::Gameplay::EffectRuntime *TargetEffects = nullptr);
  bool ResolveEnemyBasicAttack(Lostsense::Combat::Combatant &Enemy,
                               const Lostsense::Combat::DamageSpec &Damage);
  TArray<Lostsense::Gameplay::GeneratedLootEntry>
  GenerateLoot(uint32 LootTableId, uint32 ItemLevel, uint64 SourceCombatantId);
  bool PickupGeneratedLoot(const Lostsense::Gameplay::GeneratedLootEntry &Loot);

private:
  struct FPortableRuntime;
  TUniquePtr<FPortableRuntime> PortableRuntime;
};
