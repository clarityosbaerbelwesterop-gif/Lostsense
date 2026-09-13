#include "Lostsense/Gameplay/Items/Inventory.h"

#include <limits>
#include <utility>

namespace Lostsense::Gameplay {

Inventory::Inventory(const ItemCatalog &catalog,
                     const std::uint32_t capacity) noexcept
    : catalog_{catalog}, capacity_{capacity},
      valid_{catalog.Validate() && capacity > 0U} {}

std::uint32_t Inventory::OccupiedSlots() const noexcept {
  return OccupiedSlotsFor(stacks_, instances_);
}

std::uint32_t Inventory::StackQuantity(const ItemId item) const noexcept {
  const auto it = stacks_.find(item);
  return it == stacks_.end() ? 0U : it->second;
}

bool Inventory::ContainsInstance(const ItemInstanceId id) const noexcept {
  return instances_.contains(id);
}

const ItemInstance *
Inventory::FindInstance(const ItemInstanceId id) const noexcept {
  const auto it = instances_.find(id);
  return it == instances_.end() ? nullptr : &it->second;
}

InventoryResult Inventory::AddStack(const ItemId item,
                                    const std::uint32_t quantity) {
  if (!valid_) {
    return InventoryResult::InvalidRuntime;
  }
  const ItemDefinition *definition = catalog_.FindItem(item);
  if (definition == nullptr) {
    return InventoryResult::InvalidItem;
  }
  if (quantity == 0U) {
    return InventoryResult::InvalidQuantity;
  }
  if (definition->MaximumStackSize <= 1U) {
    return InventoryResult::WrongStorageKind;
  }
  const std::uint32_t current = StackQuantity(item);
  if (quantity > std::numeric_limits<std::uint32_t>::max() - current) {
    return InventoryResult::InvalidQuantity;
  }
  auto proposed = stacks_;
  proposed[item] = current + quantity;
  if (OccupiedSlotsFor(proposed, instances_) > capacity_) {
    return InventoryResult::CapacityExceeded;
  }
  stacks_ = std::move(proposed);
  return InventoryResult::Success;
}

InventoryResult Inventory::RemoveStack(const ItemId item,
                                       const std::uint32_t quantity) {
  if (!valid_) {
    return InventoryResult::InvalidRuntime;
  }
  if (quantity == 0U) {
    return InventoryResult::InvalidQuantity;
  }
  const auto it = stacks_.find(item);
  if (it == stacks_.end() || quantity > it->second) {
    return InventoryResult::MissingItem;
  }
  it->second -= quantity;
  if (it->second == 0U) {
    stacks_.erase(it);
  }
  return InventoryResult::Success;
}

InventoryResult Inventory::AddInstance(ItemInstance instance) {
  if (!valid_) {
    return InventoryResult::InvalidRuntime;
  }
  const ItemDefinition *definition = catalog_.FindItem(instance.DefinitionId);
  if (definition == nullptr || !catalog_.ValidateInstance(instance)) {
    return InventoryResult::InvalidItem;
  }
  if (definition->MaximumStackSize != 1U) {
    return InventoryResult::WrongStorageKind;
  }
  if (instances_.contains(instance.InstanceId)) {
    return InventoryResult::DuplicateInstance;
  }
  if (OccupiedSlots() >= capacity_) {
    return InventoryResult::CapacityExceeded;
  }
  instances_.emplace(instance.InstanceId, std::move(instance));
  return InventoryResult::Success;
}

InventoryResult Inventory::TakeInstance(const ItemInstanceId id,
                                        ItemInstance &instance) {
  if (!valid_) {
    return InventoryResult::InvalidRuntime;
  }
  const auto it = instances_.find(id);
  if (it == instances_.end()) {
    return InventoryResult::MissingItem;
  }
  instance = std::move(it->second);
  instances_.erase(it);
  return InventoryResult::Success;
}

InventoryResult Inventory::TransferStackTo(Inventory &destination,
                                           const ItemId item,
                                           const std::uint32_t quantity) {
  if (!valid_ || !destination.valid_) {
    return InventoryResult::InvalidRuntime;
  }
  if (quantity == 0U) {
    return InventoryResult::InvalidQuantity;
  }
  if (&destination == this) {
    return StackQuantity(item) >= quantity ? InventoryResult::Success
                                           : InventoryResult::MissingItem;
  }
  if (StackQuantity(item) < quantity) {
    return InventoryResult::MissingItem;
  }

  const InventoryState sourceState = CaptureState();
  const InventoryState destinationState = destination.CaptureState();
  const InventoryResult removed = RemoveStack(item, quantity);
  if (removed != InventoryResult::Success) {
    return removed;
  }
  const InventoryResult added = destination.AddStack(item, quantity);
  if (added != InventoryResult::Success) {
    static_cast<void>(RestoreState(sourceState));
    static_cast<void>(destination.RestoreState(destinationState));
    return added;
  }
  return InventoryResult::Success;
}

InventoryResult Inventory::TransferInstanceTo(Inventory &destination,
                                              const ItemInstanceId id) {
  if (!valid_ || !destination.valid_) {
    return InventoryResult::InvalidRuntime;
  }
  if (&destination == this) {
    return ContainsInstance(id) ? InventoryResult::Success
                                : InventoryResult::MissingItem;
  }

  const InventoryState sourceState = CaptureState();
  const InventoryState destinationState = destination.CaptureState();
  ItemInstance instance;
  const InventoryResult taken = TakeInstance(id, instance);
  if (taken != InventoryResult::Success) {
    return taken;
  }
  const InventoryResult added = destination.AddInstance(std::move(instance));
  if (added != InventoryResult::Success) {
    static_cast<void>(RestoreState(sourceState));
    static_cast<void>(destination.RestoreState(destinationState));
    return added;
  }
  return InventoryResult::Success;
}

InventoryState Inventory::CaptureState() const {
  InventoryState state;
  state.Capacity = capacity_;
  state.Stacks.reserve(stacks_.size());
  state.Instances.reserve(instances_.size());
  for (const auto &[item, quantity] : stacks_) {
    state.Stacks.push_back({item, quantity});
  }
  for (const auto &[id, instance] : instances_) {
    static_cast<void>(id);
    state.Instances.push_back(instance);
  }
  return state;
}

bool Inventory::RestoreState(const InventoryState &state) {
  if (!valid_ || !ValidateState(state)) {
    return false;
  }
  std::map<ItemId, std::uint32_t> restoredStacks;
  std::map<ItemInstanceId, ItemInstance> restoredInstances;
  for (const ItemStackState &stack : state.Stacks) {
    restoredStacks.emplace(stack.Item, stack.Quantity);
  }
  for (const ItemInstance &instance : state.Instances) {
    restoredInstances.emplace(instance.InstanceId, instance);
  }
  stacks_ = std::move(restoredStacks);
  instances_ = std::move(restoredInstances);
  return true;
}

std::uint32_t Inventory::OccupiedSlotsFor(
    const std::map<ItemId, std::uint32_t> &stacks,
    const std::map<ItemInstanceId, ItemInstance> &instances) const noexcept {
  std::uint64_t occupied = instances.size();
  for (const auto &[item, quantity] : stacks) {
    const ItemDefinition *definition = catalog_.FindItem(item);
    if (definition == nullptr || definition->MaximumStackSize <= 1U) {
      return std::numeric_limits<std::uint32_t>::max();
    }
    occupied += (static_cast<std::uint64_t>(quantity) +
                 definition->MaximumStackSize - 1U) /
                definition->MaximumStackSize;
    if (occupied > std::numeric_limits<std::uint32_t>::max()) {
      return std::numeric_limits<std::uint32_t>::max();
    }
  }
  return static_cast<std::uint32_t>(occupied);
}

bool Inventory::ValidateState(const InventoryState &state) const noexcept {
  if (state.Capacity != capacity_) {
    return false;
  }
  std::map<ItemId, std::uint32_t> stacks;
  std::map<ItemInstanceId, ItemInstance> instances;
  for (const ItemStackState &stack : state.Stacks) {
    const ItemDefinition *definition = catalog_.FindItem(stack.Item);
    if (definition == nullptr || definition->MaximumStackSize <= 1U ||
        stack.Quantity == 0U || stacks.contains(stack.Item)) {
      return false;
    }
    stacks.emplace(stack.Item, stack.Quantity);
  }
  for (const ItemInstance &instance : state.Instances) {
    const ItemDefinition *definition = catalog_.FindItem(instance.DefinitionId);
    if (definition == nullptr || definition->MaximumStackSize != 1U ||
        !catalog_.ValidateInstance(instance) ||
        !instances.emplace(instance.InstanceId, instance).second) {
      return false;
    }
  }
  return OccupiedSlotsFor(stacks, instances) <= capacity_;
}

} // namespace Lostsense::Gameplay
