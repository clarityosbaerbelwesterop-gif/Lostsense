#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Lostsense::Combat {

enum class DamageType : std::uint8_t {
  Physical,
  Fire,
  Frost,
  Lightning,
  Poison,
  Arcane,
  Abyssal,
  Count
};

inline constexpr std::size_t DamageTypeCount =
    static_cast<std::size_t>(DamageType::Count);
using DamageValues = std::array<double, DamageTypeCount>;

struct OffensiveStats final {
  double IncreasedDamagePercent{0.0};
  double CriticalChance{0.0};
  double CriticalMultiplier{1.5};
  double ArmorPenetration{0.0};
  DamageValues ResistancePenetration{};
};

struct DefensiveStats final {
  double Armor{0.0};
  DamageValues Resistances{};
  double ResistanceCap{0.75};
  double BlockChance{0.0};
  double BlockMitigation{0.5};
};

struct DamageRequest final {
  DamageValues BaseDamage{};
  OffensiveStats Attacker{};
  DefensiveStats Defender{};

  // A deterministic sample in [0, 1). Generate it on the authoritative caller.
  double CriticalRoll{0.0};
  double BlockRoll{0.0};
  bool CanCritical{true};
  bool CanBlock{true};
};

struct DamageResult final {
  DamageValues AppliedByType{};
  double TotalApplied{0.0};
  bool WasCritical{false};
  bool WasBlocked{false};
};

class DamageCalculator final {
public:
  // Pipeline: sanitize input -> increased damage -> critical -> per-type
  // mitigation -> block. Invalid/negative values are safely clamped.
  [[nodiscard]] static DamageResult
  Calculate(const DamageRequest &request) noexcept;
};

} // namespace Lostsense::Combat
