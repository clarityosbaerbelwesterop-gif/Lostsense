#pragma once

#include "Lostsense/Combat/Combatant.h"
#include "Lostsense/Gameplay/Abilities/AbilityRuntime.h"
#include "Lostsense/Gameplay/GameplayTypes.h"
#include "Lostsense/Gameplay/Loadout/AbilityLoadout.h"
#include "Lostsense/Stats/AttributeSet.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace Lostsense::Gameplay {

enum class SkillNodeCategory : std::uint8_t {
  Minor,
  Major,
  Keystone,
  Transformation,
  Hybrid,
  ClassMechanic,
};

struct SkillAttributeModifier final {
  Stats::AttributeId Attribute{};
  Stats::ModifierOperation Operation{Stats::ModifierOperation::Additive};
  double Magnitude{0.0};
};

struct SkillExclusiveGroupDefinition final {
  SkillExclusiveGroupId Id{};
  std::uint32_t MaxAllocated{1U};
};

struct SkillNodeDefinition final {
  SkillNodeId Id{};
  SkillNodeCategory Category{SkillNodeCategory::Minor};
  std::uint32_t PointCost{1U};
  ClassId RequiredClass{};
  std::vector<SkillNodeId> Prerequisites{};
  std::vector<SkillNodeId> Connections{};
  SkillExclusiveGroupId ExclusiveGroup{};
  std::vector<SkillAttributeModifier> AttributeModifiers{};
  AbilityId UnlockAbility{};
  std::vector<AbilityMutationId> AbilityMutations{};
  std::vector<GameplayTagId> Tags{};
};

struct SkillTreeDefinition final {
  SkillTreeId Id{};
  std::vector<ClassId> SupportedClasses{};
  std::vector<SkillExclusiveGroupDefinition> ExclusiveGroups{};
  std::vector<SkillNodeDefinition> Nodes{};
};

struct SkillTreeRuntimeState final {
  std::uint32_t TotalPoints{0U};
  std::uint32_t UnspentPoints{0U};
  std::vector<SkillNodeId> AllocatedNodes{};
};

enum class SkillOperationResult : std::uint8_t {
  Success,
  InvalidRuntime,
  UnknownNode,
  AlreadyAllocated,
  NotAllocated,
  WrongClass,
  InsufficientPoints,
  MissingPrerequisite,
  ExclusiveGroupLimit,
  DependencyActive,
  AbilityEquipped,
  AbilityUnlockFailed,
  AbilityRevokeFailed,
  ModifierFailure,
  InvalidState,
  InternalFailure,
};

class SkillTreeRuntime final {
public:
  SkillTreeRuntime(Combat::Combatant &owner, AbilityRuntime &abilities,
                   AbilityLoadout &loadout, SkillTreeDefinition definition,
                   std::uint32_t initialPoints = 0U);

  [[nodiscard]] bool IsValid() const noexcept { return definitionValid_; }
  [[nodiscard]] SkillTreeId TreeId() const noexcept { return treeId_; }
  [[nodiscard]] std::uint32_t TotalPoints() const noexcept {
    return totalPoints_;
  }
  [[nodiscard]] std::uint32_t UnspentPoints() const noexcept {
    return unspentPoints_;
  }
  [[nodiscard]] const SkillNodeDefinition *
  FindNode(SkillNodeId id) const noexcept;
  [[nodiscard]] bool IsAllocated(SkillNodeId id) const noexcept;
  [[nodiscard]] bool HasMutation(AbilityMutationId id) const noexcept;
  [[nodiscard]] bool HasTag(GameplayTagId id) const noexcept;

  [[nodiscard]] bool GrantPoints(std::uint32_t points) noexcept;
  [[nodiscard]] SkillOperationResult Allocate(SkillNodeId id);
  [[nodiscard]] SkillOperationResult Refund(SkillNodeId id);

  [[nodiscard]] SkillTreeRuntimeState CaptureState() const;
  [[nodiscard]] bool RestoreState(const SkillTreeRuntimeState &state);

private:
  [[nodiscard]] bool InitializeDefinition(SkillTreeDefinition definition);
  [[nodiscard]] bool
  IsNodeCategoryValid(SkillNodeCategory category) const noexcept;
  [[nodiscard]] bool IsClassSupported(ClassId id) const noexcept;
  [[nodiscard]] bool
  IsNodeClassAllowed(const SkillNodeDefinition &node) const noexcept;
  [[nodiscard]] bool ValidatePrerequisiteGraph();
  [[nodiscard]] bool ValidateConnections() const noexcept;
  [[nodiscard]] bool
  ValidateState(const SkillTreeRuntimeState &state) const noexcept;
  [[nodiscard]] bool HasAllocatedDependents(SkillNodeId id) const noexcept;
  [[nodiscard]] bool
  ExclusiveGroupAvailable(const SkillNodeDefinition &node) const noexcept;
  [[nodiscard]] Stats::ModifierId
  ModifierIdFor(SkillNodeId node, std::size_t index) const noexcept;
  [[nodiscard]] bool InstallNodeModifiers(const SkillNodeDefinition &node);
  [[nodiscard]] bool
  RemoveNodeModifiers(const SkillNodeDefinition &node) noexcept;
  void AddDerivedReferences(const SkillNodeDefinition &node);
  void RemoveDerivedReferences(const SkillNodeDefinition &node) noexcept;
  void RebuildDerivedReferences();
  [[nodiscard]] bool Rollback(const Combat::CombatantState &combatState,
                              const AbilityRuntimeState &abilityState,
                              const SkillTreeRuntimeState &skillState) noexcept;

  Combat::Combatant &owner_;
  AbilityRuntime &abilities_;
  AbilityLoadout &loadout_;
  SkillTreeId treeId_{};
  std::set<ClassId> supportedClasses_{};
  std::map<SkillExclusiveGroupId, SkillExclusiveGroupDefinition>
      exclusiveGroups_{};
  std::map<SkillNodeId, SkillNodeDefinition> nodes_{};
  std::vector<SkillNodeId> topologicalOrder_{};
  std::map<SkillNodeId, std::vector<SkillNodeId>> dependents_{};
  std::set<SkillNodeId> allocated_{};
  std::map<SkillExclusiveGroupId, std::uint32_t> exclusiveAllocationCounts_{};
  std::map<AbilityMutationId, std::uint32_t> mutationCounts_{};
  std::map<GameplayTagId, std::uint32_t> tagCounts_{};
  std::uint32_t totalPoints_{0U};
  std::uint32_t unspentPoints_{0U};
  bool definitionValid_{false};
};

} // namespace Lostsense::Gameplay
