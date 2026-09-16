#include "LostsenseMenuProjection.h"

#include "LostsenseRuntimeSubsystem.h"

#include "Lostsense/Gameplay/Content/FirstSliceContent.h"
#include "Lostsense/Gameplay/Persistence/SaveCodec.h"

#include <algorithm>
#include <set>
#include <string>

namespace {
using namespace Lostsense::Gameplay;
using namespace Lostsense::Gameplay::FirstSlice;

FString ItemName(const ItemId Item) {
  if (Item == GravesongLongsword) {
    return TEXT("Gravesong Longsword");
  }
  if (Item == BellguardSabre) {
    return TEXT("Bellguard Sabre");
  }
  if (Item == MarchSpear) {
    return TEXT("March Spear");
  }
  if (Item == TonguelessShield) {
    return TEXT("Tongueless Shield");
  }
  if (Item == OdransClapper) {
    return TEXT("Odran's Clapper");
  }
  if (Item == BellwardenCuirass) {
    return TEXT("Bellwarden Cuirass");
  }
  if (Item == VaurGoldThread) {
    return TEXT("Vaur Gold Thread");
  }
  return FString::Printf(TEXT("Unknown item %u"), Item.Value);
}

FString RarityName(const RarityId Rarity) {
  if (Rarity == CommonRarity) {
    return TEXT("COMMON");
  }
  if (Rarity == RareRarity) {
    return TEXT("RARE");
  }
  if (Rarity == LegendaryRarity) {
    return TEXT("LEGENDARY");
  }
  if (Rarity == BossUniqueRarity) {
    return TEXT("BOSS UNIQUE");
  }
  return TEXT("UNRANKED");
}

FString SkillName(const SkillNodeId Node) {
  if (Node == FirstMeasure) {
    return TEXT("First Measure");
  }
  if (Node == BellstepNode) {
    return TEXT("Bellstep");
  }
  if (Node == ResolveWell) {
    return TEXT("Resolve Well");
  }
  if (Node == MeasureBreakerNode) {
    return TEXT("Measure Breaker");
  }
  if (Node == GuardDensity) {
    return TEXT("Guard Density");
  }
  if (Node == MarchSweepNode) {
    return TEXT("March Sweep");
  }
  if (Node == AnsweringGuardNode) {
    return TEXT("Answering Guard");
  }
  if (Node == StandWhereItFallsNode) {
    return TEXT("Stand Where It Falls");
  }
  if (Node == HollowMeasureNode) {
    return TEXT("Hollow Measure");
  }
  if (Node == HeavyMeasure) {
    return TEXT("Heavy Measure");
  }
  if (Node == BellwardVitality) {
    return TEXT("Bellward Vitality");
  }
  if (Node == ClappersReturnNode) {
    return TEXT("Clapper's Return");
  }
  return FString::Printf(TEXT("Scar node %u"), Node.Value);
}

FString SkillDetail(const SkillNodeId Node) {
  if (Node == FirstMeasure) {
    return TEXT("Attack Power +1");
  }
  if (Node == BellstepNode) {
    return TEXT("Unlock Bellstep // Active I");
  }
  if (Node == ResolveWell) {
    return TEXT("Maximum Resolve +10");
  }
  if (Node == MeasureBreakerNode) {
    return TEXT("Unlock Measure Breaker // Active II");
  }
  if (Node == GuardDensity) {
    return TEXT("Armor +4");
  }
  if (Node == MarchSweepNode) {
    return TEXT("Unlock March Sweep // Active III");
  }
  if (Node == AnsweringGuardNode) {
    return TEXT("Unlock Answering Guard // Active IV");
  }
  if (Node == StandWhereItFallsNode) {
    return TEXT("Oath: Armor +5 // exclusive choice");
  }
  if (Node == HollowMeasureNode) {
    return TEXT("Oath: Damage +8% // exclusive choice");
  }
  if (Node == HeavyMeasure) {
    return TEXT("Attack Power +3");
  }
  if (Node == BellwardVitality) {
    return TEXT("Maximum Vitality +10");
  }
  if (Node == ClappersReturnNode) {
    return TEXT("Transformation hook: Clapper's Return");
  }
  return TEXT("Authored Scar Atlas node");
}

FString SkillCategory(const SkillNodeCategory Category) {
  switch (Category) {
  case SkillNodeCategory::Minor:
    return TEXT("MINOR");
  case SkillNodeCategory::Major:
    return TEXT("MAJOR");
  case SkillNodeCategory::Keystone:
    return TEXT("KEYSTONE");
  case SkillNodeCategory::Transformation:
    return TEXT("TRANSFORMATION");
  case SkillNodeCategory::Hybrid:
    return TEXT("HYBRID");
  case SkillNodeCategory::ClassMechanic:
    return TEXT("CLASS");
  }
  return TEXT("SCAR");
}

bool DecodeCharacter(const ULostsenseRuntimeSubsystem &Runtime,
                     CharacterSaveState &OutState) {
  FString Payload;
  if (!Runtime.SaveCharacterToText(Payload)) {
    return false;
  }
  const std::string Utf8(TCHAR_TO_UTF8(*Payload));
  return SaveCodec::Deserialize(Utf8, OutState).Status ==
         SaveDecodeStatus::Success;
}
} // namespace

bool FLostsenseMenuProjection::CaptureInventory(
    const ULostsenseRuntimeSubsystem &Runtime,
    TArray<FLostsenseInventoryMenuEntry> &OutEntries) {
  OutEntries.Reset();
  CharacterSaveState State;
  if (!DecodeCharacter(Runtime, State)) {
    return false;
  }

  const ItemCatalog Catalog = FirstSlice::BuildItemCatalog();
  for (const EquippedItemState &Equipped : State.Equipment.Items) {
    FLostsenseInventoryMenuEntry Entry;
    Entry.InstanceId = static_cast<int64>(Equipped.Item.InstanceId.Value);
    Entry.ItemId = static_cast<int32>(Equipped.Item.DefinitionId.Value);
    Entry.bEquipped = true;
    Entry.Label = FString::Printf(
        TEXT("[EQUIPPED] %s  // IL %u // %s"),
        *ItemName(Equipped.Item.DefinitionId), Equipped.Item.ItemLevel,
        *RarityName(Equipped.Item.Rarity));
    OutEntries.Add(MoveTemp(Entry));
  }

  for (const ItemInstance &Instance : State.Inventory.Instances) {
    const ItemDefinition *Definition = Catalog.FindItem(Instance.DefinitionId);
    FLostsenseInventoryMenuEntry Entry;
    Entry.InstanceId = static_cast<int64>(Instance.InstanceId.Value);
    Entry.ItemId = static_cast<int32>(Instance.DefinitionId.Value);
    Entry.bEquippable =
        Definition != nullptr && !Definition->AllowedEquipmentSlots.empty();
    Entry.Label = FString::Printf(
        TEXT("%s%s  // IL %u // %s"), Entry.bEquippable ? TEXT("[GEAR] ") : TEXT(""),
        *ItemName(Instance.DefinitionId), Instance.ItemLevel,
        *RarityName(Instance.Rarity));
    OutEntries.Add(MoveTemp(Entry));
  }

  for (const ItemStackState &Stack : State.Inventory.Stacks) {
    FLostsenseInventoryMenuEntry Entry;
    Entry.ItemId = static_cast<int32>(Stack.Item.Value);
    Entry.Quantity = static_cast<int32>(FMath::Min<std::uint32_t>(
        Stack.Quantity, static_cast<std::uint32_t>(MAX_int32)));
    Entry.Label = FString::Printf(TEXT("[MATERIAL] %s  x%d"),
                                  *ItemName(Stack.Item), Entry.Quantity);
    OutEntries.Add(MoveTemp(Entry));
  }

  if (OutEntries.IsEmpty()) {
    FLostsenseInventoryMenuEntry Empty;
    Empty.Label = TEXT("Inventory empty // recover loot from the descent");
    OutEntries.Add(MoveTemp(Empty));
  }
  return true;
}

bool FLostsenseMenuProjection::CaptureScarAtlas(
    const ULostsenseRuntimeSubsystem &Runtime,
    TArray<FLostsenseScarMenuEntry> &OutEntries) {
  OutEntries.Reset();
  CharacterSaveState State;
  if (!DecodeCharacter(Runtime, State)) {
    return false;
  }

  std::set<std::uint32_t> Allocated;
  for (const SkillNodeId Node : State.SkillTree.AllocatedNodes) {
    Allocated.insert(Node.Value);
  }

  const SkillTreeDefinition Tree = FirstSlice::BuildKnightScarAtlas();
  for (const SkillNodeDefinition &Node : Tree.Nodes) {
    FLostsenseScarMenuEntry Entry;
    Entry.NodeId = static_cast<int32>(Node.Id.Value);
    Entry.Name = SkillName(Node.Id);
    Entry.Category = SkillCategory(Node.Category);
    Entry.Detail = SkillDetail(Node.Id);
    Entry.PointCost = static_cast<int32>(Node.PointCost);
    Entry.bAllocated = Allocated.contains(Node.Id.Value);

    const bool PrerequisitesMet = std::all_of(
        Node.Prerequisites.begin(), Node.Prerequisites.end(),
        [&Allocated](const SkillNodeId Requirement) {
          return Allocated.contains(Requirement.Value);
        });

    bool ExclusiveBlocked = false;
    if (!Entry.bAllocated && Node.ExclusiveGroup.IsValid()) {
      ExclusiveBlocked = std::any_of(
          Tree.Nodes.begin(), Tree.Nodes.end(),
          [&Node, &Allocated](const SkillNodeDefinition &Other) {
            return Other.Id != Node.Id &&
                   Other.ExclusiveGroup == Node.ExclusiveGroup &&
                   Allocated.contains(Other.Id.Value);
          });
    }
    Entry.bBlocked = ExclusiveBlocked;
    Entry.bAvailable = !Entry.bAllocated && !Entry.bBlocked &&
                       PrerequisitesMet &&
                       State.SkillTree.UnspentPoints >= Node.PointCost;
    OutEntries.Add(MoveTemp(Entry));
  }
  return true;
}
