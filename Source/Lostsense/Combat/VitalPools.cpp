#include "Lostsense/Combat/VitalPools.h"

#include <algorithm>
#include <cmath>

namespace Lostsense::Combat {
namespace {

[[nodiscard]] bool IsValidAmount(const double amount) noexcept {
  return std::isfinite(amount) && amount >= 0.0;
}

[[nodiscard]] double ValidMaximumOrZero(const double maximum) noexcept {
  return IsValidAmount(maximum) ? maximum : 0.0;
}

} // namespace

HealthPool::HealthPool(const double maximum) noexcept
    : maximum_{ValidMaximumOrZero(maximum)}, current_{maximum_},
      dead_{maximum_ == 0.0} {}

bool HealthPool::SetMaximum(const double maximum) noexcept {
  if (!IsValidAmount(maximum)) {
    return false;
  }
  maximum_ = maximum;
  current_ = std::min(current_, maximum_);
  if (current_ == 0.0) {
    dead_ = true;
  }
  return true;
}

DamageApplication HealthPool::ApplyDamage(const double amount) noexcept {
  DamageApplication result;
  if (!IsValidAmount(amount)) {
    return result;
  }

  result.Requested = amount;
  result.WasValid = true;
  result.WasAlreadyDead = dead_;
  if (dead_) {
    result.Overkill = amount;
    return result;
  }

  result.Applied = std::min(amount, current_);
  result.Overkill = amount - result.Applied;
  current_ -= result.Applied;
  if (current_ == 0.0) {
    dead_ = true;
    result.BecameDead = true;
  }
  return result;
}

HealingApplication HealthPool::Heal(const double amount) noexcept {
  HealingApplication result;
  if (!IsValidAmount(amount)) {
    return result;
  }

  result.Requested = amount;
  result.WasValid = true;
  result.WasDead = dead_;
  if (dead_) {
    result.Overheal = amount;
    return result;
  }

  const double missing = maximum_ - current_;
  result.Applied = std::min(amount, missing);
  result.Overheal = amount - result.Applied;
  current_ += result.Applied;
  return result;
}

bool HealthPool::Revive(const double health) noexcept {
  if (!dead_ || !std::isfinite(health) || health <= 0.0 || maximum_ <= 0.0) {
    return false;
  }
  current_ = std::min(health, maximum_);
  dead_ = false;
  return true;
}

HealthState HealthPool::CaptureState() const noexcept {
  return {maximum_, current_, dead_};
}

bool HealthPool::RestoreState(const HealthState &state) noexcept {
  if (!IsValidAmount(state.Maximum) || !IsValidAmount(state.Current) ||
      state.Current > state.Maximum || state.Dead != (state.Current == 0.0)) {
    return false;
  }
  maximum_ = state.Maximum;
  current_ = state.Current;
  dead_ = state.Dead;
  return true;
}

ResourcePool::ResourcePool(const double maximum) noexcept
    : maximum_{ValidMaximumOrZero(maximum)}, current_{maximum_} {}

bool ResourcePool::SetMaximum(const double maximum) noexcept {
  if (!IsValidAmount(maximum)) {
    return false;
  }
  maximum_ = maximum;
  current_ = std::min(current_, maximum_);
  return true;
}

ResourceConsumption ResourcePool::TryConsume(const double amount) noexcept {
  ResourceConsumption result;
  if (!IsValidAmount(amount)) {
    return result;
  }
  result.Requested = amount;
  result.WasValid = true;
  if (amount > current_) {
    return result;
  }
  current_ -= amount;
  result.Consumed = amount;
  result.Succeeded = true;
  return result;
}

ResourceRestoration ResourcePool::Restore(const double amount) noexcept {
  ResourceRestoration result;
  if (!IsValidAmount(amount)) {
    return result;
  }
  result.Requested = amount;
  result.WasValid = true;
  const double missing = maximum_ - current_;
  result.Restored = std::min(amount, missing);
  result.Overflow = amount - result.Restored;
  current_ += result.Restored;
  return result;
}

ResourceState ResourcePool::CaptureState() const noexcept {
  return {maximum_, current_};
}

bool ResourcePool::RestoreState(const ResourceState &state) noexcept {
  if (!IsValidAmount(state.Maximum) || !IsValidAmount(state.Current) ||
      state.Current > state.Maximum) {
    return false;
  }
  maximum_ = state.Maximum;
  current_ = state.Current;
  return true;
}

} // namespace Lostsense::Combat
