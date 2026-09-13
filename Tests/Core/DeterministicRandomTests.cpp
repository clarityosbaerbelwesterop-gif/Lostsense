#include "Lostsense/Core/DeterministicRandom.h"
#include "TestHarness.h"

#include <array>
#include <cstdint>

namespace {

using Lostsense::Core::DeterministicRandom;
using Lostsense::Tests::TestSuite;

void TestGoldenSequence(TestSuite &suite) {
  DeterministicRandom random{42U, 54U};
  constexpr std::array<std::uint32_t, 6> Expected{0xA15C02B7U, 0x7B47F409U,
                                                  0xBA1D3330U, 0x83D2F293U,
                                                  0xBFA4784BU, 0xCBED606EU};
  for (const std::uint32_t expected : Expected) {
    suite.Expect(random.NextUInt32() == expected,
                 "PCG stream matches its cross-platform golden sequence");
  }
}

void TestSeedAndStreamIdentity(TestSuite &suite) {
  DeterministicRandom first{987654321U, 7U};
  DeterministicRandom second{987654321U, 7U};
  DeterministicRandom otherStream{987654321U, 8U};
  bool streamDiffers = false;
  for (int sample = 0; sample < 32; ++sample) {
    const std::uint32_t value = first.NextUInt32();
    suite.Expect(value == second.NextUInt32(),
                 "equal seed and stream reproduce every sample");
    streamDiffers = streamDiffers || value != otherStream.NextUInt32();
  }
  suite.Expect(streamDiffers,
               "independent stream IDs produce another sequence");
}

void TestSnapshotAndValidation(TestSuite &suite) {
  DeterministicRandom random{123U, 99U};
  static_cast<void>(random.NextUInt64());
  const auto snapshot = random.CaptureState();
  const std::uint64_t expected = random.NextUInt64();
  static_cast<void>(random.NextUInt64());
  suite.Expect(random.RestoreState(snapshot), "valid RNG state restores");
  suite.Expect(random.NextUInt64() == expected,
               "restored RNG state resumes the exact stream");

  const auto beforeInvalidRestore = random.CaptureState();
  suite.Expect(!random.RestoreState({123U, 2U}),
               "even PCG increment is rejected as corrupt state");
  suite.Expect(random.CaptureState() == beforeInvalidRestore,
               "rejected RNG state does not mutate the stream");
}

void TestRanges(TestSuite &suite) {
  DeterministicRandom random{456U, 12U};
  for (int sample = 0; sample < 10000; ++sample) {
    const double unit = random.NextUnit();
    suite.Expect(unit >= 0.0 && unit < 1.0,
                 "unit sample remains in the half-open unit interval");
    suite.Expect(random.NextBounded(7U) < 7U,
                 "bounded sample remains below its exclusive upper bound");
  }

  const auto beforeZeroBound = random.CaptureState();
  suite.Expect(random.NextBounded(0U) == 0U,
               "empty integer range has a safe zero result");
  suite.Expect(random.CaptureState() == beforeZeroBound,
               "empty integer range does not consume randomness");
}

} // namespace

int main() {
  TestSuite suite{"deterministic random"};
  TestGoldenSequence(suite);
  TestSeedAndStreamIdentity(suite);
  TestSnapshotAndValidation(suite);
  TestRanges(suite);
  return suite.Finish();
}
