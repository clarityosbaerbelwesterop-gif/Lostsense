#include "Lostsense/Stats/AttributeSet.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

namespace Lostsense::Stats {
namespace {

[[nodiscard]] bool
IsDefinitionValid(const AttributeDefinition &definition) noexcept {
  return definition.Id.IsValid() && std::isfinite(definition.DefaultBase) &&
         std::isfinite(definition.Minimum) &&
         std::isfinite(definition.Maximum) &&
         definition.Minimum <= definition.Maximum &&
         definition.DefaultBase >= definition.Minimum &&
         definition.DefaultBase <= definition.Maximum;
}

[[nodiscard]] bool
IsOperationValid(const ModifierOperation operation) noexcept {
  switch (operation) {
  case ModifierOperation::Additive:
  case ModifierOperation::Multiplicative:
    return true;
  }
  return false;
}

[[nodiscard]] bool IsSourceValid(const ModifierSource source) noexcept {
  switch (source) {
  case ModifierSource::System:
  case ModifierSource::Equipment:
  case ModifierSource::SkillTree:
  case ModifierSource::StatusEffect:
  case ModifierSource::Temporary:
    return true;
  }
  return false;
}

} // namespace

double AttributeSet::EvaluateEntry(const Entry &entry) noexcept {
  long double value = static_cast<long double>(entry.Base);
  for (const auto &[id, modifier] : entry.Modifiers) {
    static_cast<void>(id);
    if (modifier.Operation == ModifierOperation::Additive) {
      value += static_cast<long double>(modifier.Magnitude);
    }
  }
  for (const auto &[id, modifier] : entry.Modifiers) {
    static_cast<void>(id);
    if (modifier.Operation == ModifierOperation::Multiplicative) {
      value *= static_cast<long double>(modifier.Magnitude);
    }
  }

  const long double minimum =
      static_cast<long double>(entry.Definition.Minimum);
  const long double maximum =
      static_cast<long double>(entry.Definition.Maximum);
  if (!std::isfinite(value)) {
    return value < 0.0L ? entry.Definition.Minimum : entry.Definition.Maximum;
  }
  return static_cast<double>(std::clamp(value, minimum, maximum));
}

bool AttributeSet::Define(AttributeDefinition definition) {
  if (!IsDefinitionValid(definition) || Contains(definition.Id)) {
    return false;
  }
  entries_.emplace(definition.Id,
                   Entry{definition, definition.DefaultBase, {}});
  return true;
}

bool AttributeSet::Contains(const AttributeId id) const noexcept {
  return entries_.contains(id);
}

bool AttributeSet::SetBase(const AttributeId id, const double value) noexcept {
  const auto entry = entries_.find(id);
  if (entry == entries_.end() || !std::isfinite(value)) {
    return false;
  }
  entry->second.Base = std::clamp(value, entry->second.Definition.Minimum,
                                  entry->second.Definition.Maximum);
  return true;
}

double AttributeSet::GetBase(const AttributeId id) const noexcept {
  const auto entry = entries_.find(id);
  return entry == entries_.end() ? 0.0 : entry->second.Base;
}

double AttributeSet::Get(const AttributeId id) const noexcept {
  const auto entry = entries_.find(id);
  return entry == entries_.end() ? 0.0 : EvaluateEntry(entry->second);
}

bool AttributeSet::AddModifier(const AttributeModifier modifier) {
  const auto entry = entries_.find(modifier.Attribute);
  if (!modifier.Id.IsValid() || entry == entries_.end() ||
      HasModifier(modifier.Id) || !IsOperationValid(modifier.Operation) ||
      !IsSourceValid(modifier.Source) || !std::isfinite(modifier.Magnitude) ||
      (modifier.Operation == ModifierOperation::Multiplicative &&
       modifier.Magnitude < 0.0)) {
    return false;
  }

  return entry->second.Modifiers.emplace(modifier.Id, modifier).second;
}

bool AttributeSet::RemoveModifier(const ModifierId id) noexcept {
  for (auto &[attribute, entry] : entries_) {
    static_cast<void>(attribute);
    if (entry.Modifiers.erase(id) != 0U) {
      return true;
    }
  }
  return false;
}

std::size_t
AttributeSet::RemoveModifiersBySource(const ModifierSource source) noexcept {
  std::size_t removed = 0;
  for (auto &[attribute, entry] : entries_) {
    static_cast<void>(attribute);
    for (auto modifier = entry.Modifiers.begin();
         modifier != entry.Modifiers.end();) {
      if (modifier->second.Source == source) {
        modifier = entry.Modifiers.erase(modifier);
        ++removed;
      } else {
        ++modifier;
      }
    }
  }
  return removed;
}

bool AttributeSet::HasModifier(const ModifierId id) const noexcept {
  for (const auto &[attribute, entry] : entries_) {
    static_cast<void>(attribute);
    if (entry.Modifiers.contains(id)) {
      return true;
    }
  }
  return false;
}

std::size_t AttributeSet::ModifierCount() const noexcept {
  std::size_t count = 0;
  for (const auto &[attribute, entry] : entries_) {
    static_cast<void>(attribute);
    count += entry.Modifiers.size();
  }
  return count;
}

AttributeSetState AttributeSet::CaptureState() const {
  AttributeSetState state;
  state.BaseValues.reserve(entries_.size());
  state.Modifiers.reserve(ModifierCount());
  for (const auto &[id, entry] : entries_) {
    state.BaseValues.push_back(AttributeBaseState{id, entry.Base});
    for (const auto &[modifierId, modifier] : entry.Modifiers) {
      static_cast<void>(modifierId);
      state.Modifiers.push_back(modifier);
    }
  }
  return state;
}

bool AttributeSet::RestoreState(const AttributeSetState &state) {
  if (state.BaseValues.size() != entries_.size()) {
    return false;
  }

  AttributeSet restored = *this;
  for (auto &[id, entry] : restored.entries_) {
    static_cast<void>(id);
    entry.Base = entry.Definition.DefaultBase;
    entry.Modifiers.clear();
  }

  std::set<AttributeId> restoredBases;
  for (const AttributeBaseState &base : state.BaseValues) {
    const auto entry = restored.entries_.find(base.Id);
    if (!restoredBases.insert(base.Id).second ||
        entry == restored.entries_.end() || !std::isfinite(base.Value) ||
        base.Value < entry->second.Definition.Minimum ||
        base.Value > entry->second.Definition.Maximum) {
      return false;
    }
    entry->second.Base = base.Value;
  }
  for (const AttributeModifier &modifier : state.Modifiers) {
    if (!restored.AddModifier(modifier)) {
      return false;
    }
  }

  *this = std::move(restored);
  return true;
}

} // namespace Lostsense::Stats
