#pragma once

namespace Lostsense::Combat {

struct HealthState final {
  double Maximum{0.0};
  double Current{0.0};
  bool Dead{true};
};

struct DamageApplication final {
  double Requested{0.0};
  double Applied{0.0};
  double Overkill{0.0};
  bool WasValid{false};
  bool WasAlreadyDead{false};
  bool BecameDead{false};
};

struct HealingApplication final {
  double Requested{0.0};
  double Applied{0.0};
  double Overheal{0.0};
  bool WasValid{false};
  bool WasDead{false};
};

class HealthPool final {
public:
  explicit HealthPool(double maximum = 0.0) noexcept;

  [[nodiscard]] double Maximum() const noexcept { return maximum_; }
  [[nodiscard]] double Current() const noexcept { return current_; }
  [[nodiscard]] bool IsDead() const noexcept { return dead_; }

  [[nodiscard]] bool SetMaximum(double maximum) noexcept;
  [[nodiscard]] DamageApplication ApplyDamage(double amount) noexcept;
  [[nodiscard]] HealingApplication Heal(double amount) noexcept;
  [[nodiscard]] bool Revive(double health) noexcept;

  [[nodiscard]] HealthState CaptureState() const noexcept;
  [[nodiscard]] bool RestoreState(const HealthState &state) noexcept;

private:
  double maximum_{0.0};
  double current_{0.0};
  bool dead_{true};
};

struct ResourceState final {
  double Maximum{0.0};
  double Current{0.0};
};

struct ResourceConsumption final {
  double Requested{0.0};
  double Consumed{0.0};
  bool WasValid{false};
  bool Succeeded{false};
};

struct ResourceRestoration final {
  double Requested{0.0};
  double Restored{0.0};
  double Overflow{0.0};
  bool WasValid{false};
};

class ResourcePool final {
public:
  explicit ResourcePool(double maximum = 0.0) noexcept;

  [[nodiscard]] double Maximum() const noexcept { return maximum_; }
  [[nodiscard]] double Current() const noexcept { return current_; }

  [[nodiscard]] bool SetMaximum(double maximum) noexcept;
  [[nodiscard]] ResourceConsumption TryConsume(double amount) noexcept;
  [[nodiscard]] ResourceRestoration Restore(double amount) noexcept;

  [[nodiscard]] ResourceState CaptureState() const noexcept;
  [[nodiscard]] bool RestoreState(const ResourceState &state) noexcept;

private:
  double maximum_{0.0};
  double current_{0.0};
};

} // namespace Lostsense::Combat
