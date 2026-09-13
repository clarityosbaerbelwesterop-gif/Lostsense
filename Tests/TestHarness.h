#pragma once

#include <cmath>
#include <iostream>
#include <string_view>

namespace Lostsense::Tests {

class TestSuite final {
public:
  explicit TestSuite(const std::string_view name) noexcept : name_{name} {}

  void Expect(const bool condition, const std::string_view description) {
    if (!condition) {
      std::cerr << "FAIL [" << name_ << "]: " << description << '\n';
      ++failures_;
    }
  }

  void ExpectNear(const double actual, const double expected,
                  const std::string_view description,
                  const double tolerance = 0.000001) {
    if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance) {
      std::cerr << "FAIL [" << name_ << "]: " << description << " (expected "
                << expected << ", got " << actual << ")\n";
      ++failures_;
    }
  }

  [[nodiscard]] int Finish() const {
    if (failures_ == 0) {
      std::cout << name_ << ": all tests passed.\n";
    }
    return failures_ == 0 ? 0 : 1;
  }

private:
  std::string_view name_;
  int failures_{0};
};

} // namespace Lostsense::Tests
