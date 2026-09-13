#include "Lostsense/Gameplay/Progression/SkillGraph.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <utility>

namespace Lostsense::Gameplay {
namespace {

constexpr std::uint64_t SkillModifierPrefix = 1ULL << 62U;
constexpr std::uint16_t MaximumTreeId = (1U << 14U) - 1U;
constexpr std::uint32_t MaximumPointCost = 1'000'000U;
constexpr std::size_t MaximumNodeModifiers = 65535U;

[[nodiscard]] bool
IsModifierOperationValid(const Stats::ModifierOperation operation) noexcept {
  switch (operation) {
  case Stats::ModifierOperation::Additive:
  case Stats::ModifierOperation::Multiplicative:
    return true;
  }
  return false;
}

} // namespace

SkillTreeRuntime::SkillTreeRuntime(Combat::Combatant &owner,
                                   AbilityRuntime &abilities,
                                   AbilityLoadout &loadout,
                                   SkillTreeDefinition definition,
                                   const std::uint32_t initialPoints)
    : owner_{owner}, abilities_{abilities}, loadout_{loadout},
      totalPoints_{initialPoints}, unspentPoints_{initialPoints} {
  definitionValid_ = owner_.Id().IsValid() && abilities_.IsValid() &&
                     loadout_.IsValid() &&
                     loadout_.AbilityAuthority() == &abilities_ &&
                     InitializeDefinition(std::move(definition));
}

const SkillNodeDefinition *
SkillTreeRuntime::FindNode(const SkillNodeId id) const noexcept {
  const auto it = nodes_.find(id);
  return it == nodes_.end() ? nullptr : &it->second;
}

bool SkillTreeRuntime::IsAllocated(const SkillNodeId id) const noexcept {
  return allocated_.contains(id);
}

bool SkillTreeRuntime::HasMutation(const AbilityMutationId id) const noexcept {
  return id.IsValid() && mutationCounts_.contains(id);
}

bool SkillTreeRuntime::HasTag(const GameplayTagId id) const noexcept {
  return id.IsValid() && tagCounts_.contains(id);
}

bool SkillTreeRuntime::GrantPoints(const std::uint32_t points) noexcept {
  if (!definitionValid_ || points == 0U ||
      totalPoints_ > std::numeric_limits<std::uint32_t>::max() - points ||
      unspentPoints_ > std::numeric_limits<std::uint32_t>::max() - points) {
    return false;
  }
  totalPoints_ += points;
  unspentPoints_ += points;
  return true;
}

SkillOperationResult SkillTreeRuntime::Allocate(const SkillNodeId id) {
  if (!definitionValid_) {
    return SkillOperationResult::InvalidRuntime;
  }
  const SkillNodeDefinition *node = FindNode(id);
  if (node == nullptr) {
    return SkillOperationResult::UnknownNode;
  }
  if (allocated_.contains(id)) {
    return SkillOperationResult::AlreadyAllocated;
  }
  if (!IsNodeClassAllowed(*node)) {
    return SkillOperationResult::WrongClass;
  }
  if (unspentPoints_ < node->PointCost) {
    return SkillOperationResult::InsufficientPoints;
  }
  for (const SkillNodeId prerequisite : node->Prerequisites) {
    if (!allocated_.contains(prerequisite)) {
      return SkillOperationResult::MissingPrerequisite;
    }
  }
  if (!ExclusiveGroupAvailable(*node)) {
    return SkillOperationResult::ExclusiveGroupLimit;
  }

  const Combat::CombatantState combatState = owner_.CaptureState();
  const AbilityRuntimeState abilityState = abilities_.CaptureState();

  unspentPoints_ -= node->PointCost;
  allocated_.insert(id);
  const auto rollbackLocal = [&]() {
    allocated_.erase(id);
    unspentPoints_ += node->PointCost;
    const bool combatRestored = owner_.RestoreState(combatState);
    const bool abilityRestored = abilities_.RestoreState(abilityState);
    if (!combatRestored || !abilityRestored) {
      definitionValid_ = false;
      return false;
    }
    return true;
  };

  if (!InstallNodeModifiers(*node)) {
    return rollbackLocal() ? SkillOperationResult::ModifierFailure
                           : SkillOperationResult::InternalFailure;
  }
  if (node->UnlockAbility.IsValid() &&
      !abilities_.Unlock(node->UnlockAbility)) {
    return rollbackLocal() ? SkillOperationResult::AbilityUnlockFailed
                           : SkillOperationResult::InternalFailure;
  }

  AddDerivedReferences(*node);
  if (node->ExclusiveGroup.IsValid()) {
    ++exclusiveAllocationCounts_[node->ExclusiveGroup];
  }
  return SkillOperationResult::Success;
}

SkillOperationResult SkillTreeRuntime::Refund(const SkillNodeId id) {
  if (!definitionValid_) {
    return SkillOperationResult::InvalidRuntime;
  }
  const SkillNodeDefinition *node = FindNode(id);
  if (node == nullptr) {
    return SkillOperationResult::UnknownNode;
  }
  if (!allocated_.contains(id)) {
    return SkillOperationResult::NotAllocated;
  }
  if (HasAllocatedDependents(id)) {
    return SkillOperationResult::DependencyActive;
  }
  if (node->UnlockAbility.IsValid() &&
      loadout_.UsesAbility(node->UnlockAbility)) {
    return SkillOperationResult::AbilityEquipped;
  }

  const Combat::CombatantState combatState = owner_.CaptureState();
  const AbilityRuntimeState abilityState = abilities_.CaptureState();
  const auto rollbackExternal = [&]() {
    const bool combatRestored = owner_.RestoreState(combatState);
    const bool abilityRestored = abilities_.RestoreState(abilityState);
    if (!combatRestored || !abilityRestored) {
      definitionValid_ = false;
      return false;
    }
    return true;
  };

  if (node->UnlockAbility.IsValid() &&
      !abilities_.Revoke(node->UnlockAbility)) {
    return rollbackExternal() ? SkillOperationResult::AbilityRevokeFailed
                              : SkillOperationResult::InternalFailure;
  }
  if (!RemoveNodeModifiers(*node)) {
    return rollbackExternal() ? SkillOperationResult::ModifierFailure
                              : SkillOperationResult::InternalFailure;
  }

  RemoveDerivedReferences(*node);
  allocated_.erase(id);
  unspentPoints_ += node->PointCost;
  if (node->ExclusiveGroup.IsValid()) {
    auto count = exclusiveAllocationCounts_.find(node->ExclusiveGroup);
    if (count == exclusiveAllocationCounts_.end() || count->second == 0U) {
      definitionValid_ = false;
      return SkillOperationResult::InternalFailure;
    }
    --count->second;
  }
  return SkillOperationResult::Success;
}

SkillTreeRuntimeState SkillTreeRuntime::CaptureState() const {
  SkillTreeRuntimeState state;
  state.TotalPoints = totalPoints_;
  state.UnspentPoints = unspentPoints_;
  state.AllocatedNodes.assign(allocated_.begin(), allocated_.end());
  return state;
}

bool SkillTreeRuntime::RestoreState(const SkillTreeRuntimeState &state) {
  if (!definitionValid_ || !ValidateState(state)) {
    return false;
  }

  const std::set<SkillNodeId> target(state.AllocatedNodes.begin(),
                                     state.AllocatedNodes.end());
  for (const SkillNodeId id : allocated_) {
    if (target.contains(id)) {
      continue;
    }
    const SkillNodeDefinition &node = nodes_.at(id);
    if (node.UnlockAbility.IsValid() &&
        loadout_.UsesAbility(node.UnlockAbility)) {
      return false;
    }
  }

  const Combat::CombatantState combatState = owner_.CaptureState();
  const AbilityRuntimeState abilityState = abilities_.CaptureState();
  const SkillTreeRuntimeState skillState = CaptureState();

  for (auto it = topologicalOrder_.rbegin(); it != topologicalOrder_.rend();
       ++it) {
    const SkillNodeId id = *it;
    if (!allocated_.contains(id) || target.contains(id)) {
      continue;
    }
    const SkillNodeDefinition &node = nodes_.at(id);
    if ((node.UnlockAbility.IsValid() &&
         !abilities_.Revoke(node.UnlockAbility)) ||
        !RemoveNodeModifiers(node)) {
      static_cast<void>(Rollback(combatState, abilityState, skillState));
      return false;
    }
    allocated_.erase(id);
  }

  for (const SkillNodeId id : topologicalOrder_) {
    if (!target.contains(id) || allocated_.contains(id)) {
      continue;
    }
    const SkillNodeDefinition &node = nodes_.at(id);
    allocated_.insert(id);
    if (!InstallNodeModifiers(node) ||
        (node.UnlockAbility.IsValid() &&
         !abilities_.Unlock(node.UnlockAbility))) {
      static_cast<void>(Rollback(combatState, abilityState, skillState));
      return false;
    }
  }

  totalPoints_ = state.TotalPoints;
  unspentPoints_ = state.UnspentPoints;
  RebuildDerivedReferences();
  return true;
}

bool SkillTreeRuntime::InitializeDefinition(SkillTreeDefinition definition) {
  if (!definition.Id.IsValid() || definition.Id.Value > MaximumTreeId ||
      definition.Nodes.empty()) {
    return false;
  }
  treeId_ = definition.Id;

  for (const ClassId id : definition.SupportedClasses) {
    if (!id.IsValid() || !supportedClasses_.insert(id).second) {
      return false;
    }
  }
  if (!supportedClasses_.empty() &&
      !supportedClasses_.contains(abilities_.OwnerClass())) {
    return false;
  }

  for (const SkillExclusiveGroupDefinition &group :
       definition.ExclusiveGroups) {
    if (!group.Id.IsValid() || group.MaxAllocated == 0U ||
        !exclusiveGroups_.emplace(group.Id, group).second) {
      return false;
    }
  }

  std::set<AbilityId> grantedAbilities;
  for (SkillNodeDefinition &node : definition.Nodes) {
    if (!node.Id.IsValid() || !IsNodeCategoryValid(node.Category) ||
        node.PointCost == 0U || node.PointCost > MaximumPointCost ||
        (node.RequiredClass.IsValid() &&
         !IsClassSupported(node.RequiredClass)) ||
        (node.ExclusiveGroup.IsValid() &&
         !exclusiveGroups_.contains(node.ExclusiveGroup)) ||
        node.AttributeModifiers.size() > MaximumNodeModifiers ||
        !nodes_.emplace(node.Id, node).second) {
      return false;
    }

    std::set<SkillNodeId> prerequisites;
    for (const SkillNodeId prerequisite : node.Prerequisites) {
      if (!prerequisite.IsValid() || prerequisite == node.Id ||
          !prerequisites.insert(prerequisite).second) {
        return false;
      }
    }
    std::set<SkillNodeId> connections;
    for (const SkillNodeId connection : node.Connections) {
      if (!connection.IsValid() || connection == node.Id ||
          !connections.insert(connection).second) {
        return false;
      }
    }
    for (const SkillAttributeModifier &modifier : node.AttributeModifiers) {
      if (!modifier.Attribute.IsValid() ||
          !IsModifierOperationValid(modifier.Operation) ||
          !std::isfinite(modifier.Magnitude) ||
          (modifier.Operation == Stats::ModifierOperation::Multiplicative &&
           modifier.Magnitude < 0.0)) {
        return false;
      }
    }
    std::set<AbilityMutationId> mutations;
    for (const AbilityMutationId mutation : node.AbilityMutations) {
      if (!mutation.IsValid() || !mutations.insert(mutation).second) {
        return false;
      }
    }
    std::set<GameplayTagId> tags;
    for (const GameplayTagId tag : node.Tags) {
      if (!tag.IsValid() || !tags.insert(tag).second) {
        return false;
      }
    }
    if (node.UnlockAbility.IsValid() &&
        (abilities_.FindDefinition(node.UnlockAbility) == nullptr ||
         !grantedAbilities.insert(node.UnlockAbility).second)) {
      return false;
    }
  }

  for (const auto &[id, node] : nodes_) {
    static_cast<void>(id);
    for (const SkillNodeId prerequisite : node.Prerequisites) {
      if (!nodes_.contains(prerequisite)) {
        return false;
      }
    }
    for (const SkillNodeId connection : node.Connections) {
      if (!nodes_.contains(connection)) {
        return false;
      }
    }
  }

  return ValidateConnections() && ValidatePrerequisiteGraph();
}

bool SkillTreeRuntime::IsNodeCategoryValid(
    const SkillNodeCategory category) const noexcept {
  switch (category) {
  case SkillNodeCategory::Minor:
  case SkillNodeCategory::Major:
  case SkillNodeCategory::Keystone:
  case SkillNodeCategory::Transformation:
  case SkillNodeCategory::Hybrid:
  case SkillNodeCategory::ClassMechanic:
    return true;
  }
  return false;
}

bool SkillTreeRuntime::IsClassSupported(const ClassId id) const noexcept {
  return id.IsValid() &&
         (supportedClasses_.empty() || supportedClasses_.contains(id));
}

bool SkillTreeRuntime::IsNodeClassAllowed(
    const SkillNodeDefinition &node) const noexcept {
  return !node.RequiredClass.IsValid() ||
         node.RequiredClass == abilities_.OwnerClass();
}

bool SkillTreeRuntime::ValidateConnections() const noexcept {
  for (const auto &[id, node] : nodes_) {
    for (const SkillNodeId connected : node.Connections) {
      const auto other = nodes_.find(connected);
      if (other == nodes_.end() ||
          std::find(other->second.Connections.begin(),
                    other->second.Connections.end(),
                    id) == other->second.Connections.end()) {
        return false;
      }
    }
  }
  return true;
}

bool SkillTreeRuntime::ValidatePrerequisiteGraph() {
  std::map<SkillNodeId, std::uint32_t> indegree;
  dependents_.clear();
  std::queue<SkillNodeId> ready;
  std::size_t roots = 0U;

  for (const auto &[id, node] : nodes_) {
    const auto degree = static_cast<std::uint32_t>(node.Prerequisites.size());
    indegree.emplace(id, degree);
    if (degree == 0U) {
      ready.push(id);
      ++roots;
    }
    for (const SkillNodeId prerequisite : node.Prerequisites) {
      dependents_[prerequisite].push_back(id);
    }
  }
  if (roots == 0U) {
    return false;
  }

  topologicalOrder_.clear();
  topologicalOrder_.reserve(nodes_.size());
  while (!ready.empty()) {
    const SkillNodeId current = ready.front();
    ready.pop();
    topologicalOrder_.push_back(current);
    const auto found = dependents_.find(current);
    if (found == dependents_.end()) {
      continue;
    }
    for (const SkillNodeId dependent : found->second) {
      std::uint32_t &degree = indegree.at(dependent);
      if (degree == 0U) {
        return false;
      }
      --degree;
      if (degree == 0U) {
        ready.push(dependent);
      }
    }
  }
  return topologicalOrder_.size() == nodes_.size();
}

bool SkillTreeRuntime::ValidateState(
    const SkillTreeRuntimeState &state) const noexcept {
  if (state.UnspentPoints > state.TotalPoints) {
    return false;
  }
  std::set<SkillNodeId> allocated;
  std::uint64_t spent = 0U;
  std::map<SkillExclusiveGroupId, std::uint32_t> groupCounts;
  for (const SkillNodeId id : state.AllocatedNodes) {
    const SkillNodeDefinition *node = FindNode(id);
    if (node == nullptr || !allocated.insert(id).second ||
        !IsNodeClassAllowed(*node)) {
      return false;
    }
    spent += node->PointCost;
    if (spent > std::numeric_limits<std::uint32_t>::max()) {
      return false;
    }
    if (node->ExclusiveGroup.IsValid()) {
      ++groupCounts[node->ExclusiveGroup];
    }
  }
  if (spent + state.UnspentPoints != state.TotalPoints) {
    return false;
  }
  for (const SkillNodeId id : allocated) {
    const SkillNodeDefinition &node = nodes_.at(id);
    for (const SkillNodeId prerequisite : node.Prerequisites) {
      if (!allocated.contains(prerequisite)) {
        return false;
      }
    }
  }
  for (const auto &[group, count] : groupCounts) {
    const auto definition = exclusiveGroups_.find(group);
    if (definition == exclusiveGroups_.end() ||
        count > definition->second.MaxAllocated) {
      return false;
    }
  }
  return true;
}

bool SkillTreeRuntime::HasAllocatedDependents(
    const SkillNodeId id) const noexcept {
  const auto found = dependents_.find(id);
  if (found == dependents_.end()) {
    return false;
  }
  return std::any_of(found->second.begin(), found->second.end(),
                     [this](const SkillNodeId dependent) {
                       return allocated_.contains(dependent);
                     });
}

bool SkillTreeRuntime::ExclusiveGroupAvailable(
    const SkillNodeDefinition &node) const noexcept {
  if (!node.ExclusiveGroup.IsValid()) {
    return true;
  }
  const auto group = exclusiveGroups_.find(node.ExclusiveGroup);
  if (group == exclusiveGroups_.end()) {
    return false;
  }
  const auto count = exclusiveAllocationCounts_.find(node.ExclusiveGroup);
  const std::uint32_t allocatedCount =
      count == exclusiveAllocationCounts_.end() ? 0U : count->second;
  return allocatedCount < group->second.MaxAllocated;
}

Stats::ModifierId
SkillTreeRuntime::ModifierIdFor(const SkillNodeId node,
                                const std::size_t index) const noexcept {
  if (!treeId_.IsValid() || treeId_.Value > MaximumTreeId || !node.IsValid() ||
      index >= MaximumNodeModifiers) {
    return {};
  }
  const std::uint64_t encoded =
      (static_cast<std::uint64_t>(treeId_.Value) << 48U) |
      (static_cast<std::uint64_t>(node.Value) << 16U) |
      (static_cast<std::uint64_t>(index) + 1U);
  return Stats::ModifierId{SkillModifierPrefix | encoded};
}

bool SkillTreeRuntime::InstallNodeModifiers(const SkillNodeDefinition &node) {
  for (std::size_t index = 0; index < node.AttributeModifiers.size(); ++index) {
    const SkillAttributeModifier &source = node.AttributeModifiers[index];
    const Stats::ModifierId id = ModifierIdFor(node.Id, index);
    if (!id.IsValid() || owner_.Attributes().HasModifier(id) ||
        !owner_.AddAttributeModifier({id, source.Attribute, source.Operation,
                                      Stats::ModifierSource::SkillTree,
                                      source.Magnitude})) {
      return false;
    }
  }
  return true;
}

bool SkillTreeRuntime::RemoveNodeModifiers(
    const SkillNodeDefinition &node) noexcept {
  for (std::size_t index = 0; index < node.AttributeModifiers.size(); ++index) {
    const Stats::ModifierId id = ModifierIdFor(node.Id, index);
    if (!id.IsValid() || !owner_.RemoveAttributeModifier(id)) {
      return false;
    }
  }
  return true;
}

void SkillTreeRuntime::AddDerivedReferences(const SkillNodeDefinition &node) {
  for (const AbilityMutationId mutation : node.AbilityMutations) {
    ++mutationCounts_[mutation];
  }
  for (const GameplayTagId tag : node.Tags) {
    ++tagCounts_[tag];
  }
}

void SkillTreeRuntime::RemoveDerivedReferences(
    const SkillNodeDefinition &node) noexcept {
  for (const AbilityMutationId mutation : node.AbilityMutations) {
    const auto it = mutationCounts_.find(mutation);
    if (it != mutationCounts_.end()) {
      if (it->second <= 1U) {
        mutationCounts_.erase(it);
      } else {
        --it->second;
      }
    }
  }
  for (const GameplayTagId tag : node.Tags) {
    const auto it = tagCounts_.find(tag);
    if (it != tagCounts_.end()) {
      if (it->second <= 1U) {
        tagCounts_.erase(it);
      } else {
        --it->second;
      }
    }
  }
}

void SkillTreeRuntime::RebuildDerivedReferences() {
  mutationCounts_.clear();
  tagCounts_.clear();
  exclusiveAllocationCounts_.clear();
  for (const SkillNodeId id : allocated_) {
    const SkillNodeDefinition &node = nodes_.at(id);
    AddDerivedReferences(node);
    if (node.ExclusiveGroup.IsValid()) {
      ++exclusiveAllocationCounts_[node.ExclusiveGroup];
    }
  }
}

bool SkillTreeRuntime::Rollback(
    const Combat::CombatantState &combatState,
    const AbilityRuntimeState &abilityState,
    const SkillTreeRuntimeState &skillState) noexcept {
  const bool combatRestored = owner_.RestoreState(combatState);
  const bool abilityRestored = abilities_.RestoreState(abilityState);
  totalPoints_ = skillState.TotalPoints;
  unspentPoints_ = skillState.UnspentPoints;
  allocated_.clear();
  allocated_.insert(skillState.AllocatedNodes.begin(),
                    skillState.AllocatedNodes.end());
  RebuildDerivedReferences();
  return combatRestored && abilityRestored;
}

} // namespace Lostsense::Gameplay
