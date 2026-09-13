#pragma once

#include "Lostsense/Combat/Combatant.h"
#include "Lostsense/Gameplay/Items/Inventory.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace Lostsense::Gameplay {

struct EquippedItemState final {
  EquipmentSlotId Slot{};
  ItemInstance Item{};
};

struct EquipmentState final {
  std::vector<EquippedItemState> Items{};
};

enum class EquipmentResult : std::uint8_t {
  Success,
  InvalidRuntime,
  InvalidSlot,
  MissingItem,
  WrongSlot,
  WrongClass,
  InventoryFailure,
  ModifierFailure,
  InvalidState,
};

class EquipmentRuntime final {
public:
  EquipmentRuntime(const ItemCatalog &catalog, Combat::Combatant &owner,
                   ClassId ownerClass) noexcept;

  [[nodiscard]] bool IsValid() const noexcept { return valid_; }
  [[nodiscard]] std::size_t EquippedCount() const noexcept {
    return equipped_.size();
  }
  [[nodiscard]] const ItemInstance *
  EquippedAt(EquipmentSlotId slot) const noexcept;
  [[nodiscard]] bool ContainsInstance(ItemInstanceId instance) const noexcept;
  [[nodiscard]] bool
  HasAbilityMutation(AbilityMutationId mutation) const noexcept;
  [[nodiscard]] bool GrantsUniqueEffect(EffectId effect) const noexcept;

  [[nodiscard]] EquipmentResult EquipFromInventory(Inventory &inventory,
                                                   ItemInstanceId instance,
                                                   EquipmentSlotId slot);
  [[nodiscard]] EquipmentResult UnequipToInventory(Inventory &inventory,
                                                   EquipmentSlotId slot);

  [[nodiscard]] EquipmentState CaptureState() const;
  [[nodiscard]] bool RestoreState(const EquipmentState &state,
                                  const Inventory &inventory);

private:
  [[nodiscard]] bool
  IsInstanceAllowed(const ItemInstance &instance) const noexcept;
  [[nodiscard]] bool IsSlotAllowed(const ItemDefinition &definition,
                                   EquipmentSlotId slot) const noexcept;
  [[nodiscard]] Stats::ModifierId
  ModifierIdFor(ItemInstanceId instance, std::size_t index) const noexcept;
  [[nodiscard]] bool InstallModifiers(const ItemInstance &instance);
  [[nodiscard]] bool RemoveModifiers(const ItemInstance &instance) noexcept;
  [[nodiscard]] bool ValidateState(const EquipmentState &state,
                                   const Inventory &inventory) const noexcept;

  const ItemCatalog &catalog_;
  Combat::Combatant &owner_;
  ClassId ownerClass_{};
  std::map<EquipmentSlotId, ItemInstance> equipped_{};
  bool valid_{false};
};

} // namespace Lostsense::Gameplay
