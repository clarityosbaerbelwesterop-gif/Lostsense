#pragma once

#include "Lostsense/Gameplay/Items/ItemCatalog.h"

#include <cstdint>
#include <map>
#include <vector>

namespace Lostsense::Gameplay {

struct ItemStackState final {
  ItemId Item{};
  std::uint32_t Quantity{0U};
};

struct InventoryState final {
  std::uint32_t Capacity{0U};
  std::vector<ItemStackState> Stacks{};
  std::vector<ItemInstance> Instances{};
};

enum class InventoryResult : std::uint8_t {
  Success,
  InvalidRuntime,
  InvalidItem,
  InvalidQuantity,
  WrongStorageKind,
  CapacityExceeded,
  MissingItem,
  DuplicateInstance,
  InvalidState,
};

class Inventory final {
public:
  Inventory(const ItemCatalog &catalog, std::uint32_t capacity) noexcept;

  [[nodiscard]] bool IsValid() const noexcept { return valid_; }
  [[nodiscard]] std::uint32_t Capacity() const noexcept { return capacity_; }
  [[nodiscard]] std::uint32_t OccupiedSlots() const noexcept;
  [[nodiscard]] std::uint32_t StackQuantity(ItemId item) const noexcept;
  [[nodiscard]] bool ContainsInstance(ItemInstanceId id) const noexcept;
  [[nodiscard]] const ItemInstance *
  FindInstance(ItemInstanceId id) const noexcept;

  [[nodiscard]] InventoryResult AddStack(ItemId item, std::uint32_t quantity);
  [[nodiscard]] InventoryResult RemoveStack(ItemId item,
                                            std::uint32_t quantity);
  [[nodiscard]] InventoryResult AddInstance(ItemInstance instance);
  [[nodiscard]] InventoryResult TakeInstance(ItemInstanceId id,
                                             ItemInstance &instance);
  [[nodiscard]] InventoryResult
  TransferStackTo(Inventory &destination, ItemId item, std::uint32_t quantity);
  [[nodiscard]] InventoryResult TransferInstanceTo(Inventory &destination,
                                                   ItemInstanceId id);

  [[nodiscard]] InventoryState CaptureState() const;
  [[nodiscard]] bool RestoreState(const InventoryState &state);

private:
  [[nodiscard]] std::uint32_t OccupiedSlotsFor(
      const std::map<ItemId, std::uint32_t> &stacks,
      const std::map<ItemInstanceId, ItemInstance> &instances) const noexcept;
  [[nodiscard]] bool ValidateState(const InventoryState &state) const noexcept;

  const ItemCatalog &catalog_;
  std::uint32_t capacity_{0U};
  std::map<ItemId, std::uint32_t> stacks_{};
  std::map<ItemInstanceId, ItemInstance> instances_{};
  bool valid_{false};
};

} // namespace Lostsense::Gameplay
