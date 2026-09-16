#pragma once

#include "StoryRuntime.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace Lostsense::Gameplay {

enum class ActTwoStoryBeat : std::uint8_t {
  BlackSapTrailFound = 0,
  DomaIreMet,
  InfectedVillagerStabilized,
  GiltfenRootTunnelOpened,
  ThornChoirDiscovered,
  MotherVeyrDefeated,
  WitnessRootResolved,
};

enum class WitnessRootDecision : std::uint8_t { None = 0, Preserve, Burn };

struct ActTwoStoryState {
  bool Unlocked{false};
  std::uint32_t CompletedMask{0};
  WitnessRootDecision RootDecision{WitnessRootDecision::None};
};

class ActTwoStoryRuntime {
public:
  [[nodiscard]] bool UnlockFromActOne(const FirstSliceStoryRuntime &actOne);
  [[nodiscard]] bool IsUnlocked() const noexcept { return State_.Unlocked; }
  [[nodiscard]] bool HasBeat(ActTwoStoryBeat beat) const noexcept;
  [[nodiscard]] bool CompleteBeat(ActTwoStoryBeat beat);
  [[nodiscard]] bool ResolveWitnessRoot(WitnessRootDecision decision);
  [[nodiscard]] WitnessRootDecision RootDecision() const noexcept {
    return State_.RootDecision;
  }
  [[nodiscard]] ObjectiveState Objective(std::uint32_t objectiveId) const noexcept;
  [[nodiscard]] ActTwoStoryState CaptureState() const noexcept { return State_; }
  [[nodiscard]] bool RestoreState(const ActTwoStoryState &state) noexcept;

private:
  ActTwoStoryState State_{};
  [[nodiscard]] static bool ValidState(const ActTwoStoryState &state) noexcept;
};

class ActTwoStoryCodec {
public:
  [[nodiscard]] static bool Serialize(const ActTwoStoryState &state,
                                      std::string &output);
  [[nodiscard]] static bool Deserialize(std::string_view text,
                                        ActTwoStoryState &output);
};

} // namespace Lostsense::Gameplay
