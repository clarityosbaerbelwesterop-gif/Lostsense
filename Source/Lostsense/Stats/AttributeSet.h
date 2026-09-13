#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <vector>

namespace Lostsense::Stats {

struct AttributeId final {
  std::uint32_t Value{0};

  constexpr AttributeId() noexcept = default;
  explicit constexpr AttributeId(const std::uint32_t value) noexcept
      : Value{value} {}

  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const AttributeId &, const AttributeId &) noexcept = default;
};

struct ModifierId final {
  std::uint64_t Value{0};

  constexpr ModifierId() noexcept = default;
  explicit constexpr ModifierId(const std::uint64_t value) noexcept
      : Value{value} {}

  [[nodiscard]] constexpr bool IsValid() const noexcept { return Value != 0U; }
  [[nodiscard]] friend constexpr auto
  operator<=>(const ModifierId &, const ModifierId &) noexcept = default;
};

enum class ModifierOperation : std::uint8_t {
  Additive,
  Multiplicative,
};

enum class ModifierSource : std::uint8_t {
  System,
  Equipment,
  SkillTree,
  StatusEffect,
  Temporary,
};

struct AttributeDefinition final {
  AttributeId Id{};
  double DefaultBase{0.0};
  double Minimum{-std::numeric_limits<double>::max()};
  double Maximum{std::numeric_limits<double>::max()};
};

struct AttributeModifier final {
  ModifierId Id{};
  AttributeId Attribute{};
  ModifierOperation Operation{ModifierOperation::Additive};
  ModifierSource Source{ModifierSource::System};
  // Additive values are flat deltas. Multiplicative values are factors, so
  // 1.20 means +20% and 0.80 means -20%.
  double Magnitude{0.0};
};

struct AttributeBaseState final {
  AttributeId Id{};
  double Value{0.0};
};

struct AttributeSetState final {
  std::vector<AttributeBaseState> BaseValues{};
  std::vector<AttributeModifier> Modifiers{};
};

// A deterministic, definition-driven stat container. Modifier IDs establish
// stable evaluation order and double as serialization/removal handles.
class AttributeSet final {
public:
  [[nodiscard]] bool Define(AttributeDefinition definition);
  [[nodiscard]] bool Contains(AttributeId id) const noexcept;

  [[nodiscard]] bool SetBase(AttributeId id, double value) noexcept;
  [[nodiscard]] double GetBase(AttributeId id) const noexcept;
  [[nodiscard]] double Get(AttributeId id) const noexcept;

  [[nodiscard]] bool AddModifier(AttributeModifier modifier);
  [[nodiscard]] bool RemoveModifier(ModifierId id) noexcept;
  [[nodiscard]] std::size_t
  RemoveModifiersBySource(ModifierSource source) noexcept;
  [[nodiscard]] bool HasModifier(ModifierId id) const noexcept;
  [[nodiscard]] std::size_t ModifierCount() const noexcept;

  [[nodiscard]] AttributeSetState CaptureState() const;
  [[nodiscard]] bool RestoreState(const AttributeSetState &state);

private:
  struct Entry final {
    AttributeDefinition Definition{};
    double Base{0.0};
    std::map<ModifierId, AttributeModifier> Modifiers{};
  };

  [[nodiscard]] static double EvaluateEntry(const Entry &entry) noexcept;

  std::map<AttributeId, Entry> entries_{};
};

} // namespace Lostsense::Stats
