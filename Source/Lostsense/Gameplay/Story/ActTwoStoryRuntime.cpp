#include "ActTwoStoryRuntime.h"

#include <array>
#include <charconv>

namespace Lostsense::Gameplay {
namespace {
constexpr std::uint32_t Bit(ActTwoStoryBeat beat) noexcept {
  return 1U << static_cast<std::uint32_t>(beat);
}

constexpr std::uint32_t FinalMask =
    (1U << (static_cast<std::uint32_t>(ActTwoStoryBeat::WitnessRootResolved) +
            1U)) -
    1U;

bool ParseUnsigned(std::string_view text, std::uint32_t &value) noexcept {
  if (text.empty()) {
    return false;
  }
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value);
  return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
} // namespace

bool ActTwoStoryRuntime::UnlockFromActOne(
    const FirstSliceStoryRuntime &actOne) {
  if (!actOne.HasBeat(FirstSliceStoryBeat::BellgraveChanged)) {
    return false;
  }
  State_.Unlocked = true;
  return true;
}

bool ActTwoStoryRuntime::HasBeat(ActTwoStoryBeat beat) const noexcept {
  const auto index = static_cast<std::uint32_t>(beat);
  return index <= static_cast<std::uint32_t>(ActTwoStoryBeat::WitnessRootResolved) &&
         (State_.CompletedMask & Bit(beat)) != 0U;
}

bool ActTwoStoryRuntime::CompleteBeat(ActTwoStoryBeat beat) {
  if (!State_.Unlocked) {
    return false;
  }
  const auto index = static_cast<std::uint32_t>(beat);
  if (index > static_cast<std::uint32_t>(ActTwoStoryBeat::WitnessRootResolved)) {
    return false;
  }
  if (beat == ActTwoStoryBeat::WitnessRootResolved) {
    return State_.RootDecision != WitnessRootDecision::None &&
           (State_.CompletedMask & Bit(ActTwoStoryBeat::MotherVeyrDefeated)) != 0U
               ? (State_.CompletedMask |= Bit(beat), true)
               : false;
  }
  if (index > 0U) {
    const auto required = (1U << index) - 1U;
    if ((State_.CompletedMask & required) != required) {
      return false;
    }
  }
  State_.CompletedMask |= Bit(beat);
  return true;
}

bool ActTwoStoryRuntime::ResolveWitnessRoot(WitnessRootDecision decision) {
  if (!State_.Unlocked || decision == WitnessRootDecision::None ||
      HasBeat(ActTwoStoryBeat::WitnessRootResolved) ||
      !HasBeat(ActTwoStoryBeat::MotherVeyrDefeated)) {
    return false;
  }
  State_.RootDecision = decision;
  return CompleteBeat(ActTwoStoryBeat::WitnessRootResolved);
}

ObjectiveState
ActTwoStoryRuntime::Objective(std::uint32_t objectiveId) const noexcept {
  if (!State_.Unlocked || objectiveId < 80010U || objectiveId > 80014U) {
    return ObjectiveState::Locked;
  }

  ActTwoStoryBeat completionBeat{};
  switch (objectiveId) {
  case 80010U:
    completionBeat = ActTwoStoryBeat::DomaIreMet;
    break;
  case 80011U:
    completionBeat = ActTwoStoryBeat::InfectedVillagerStabilized;
    break;
  case 80012U:
    completionBeat = ActTwoStoryBeat::GiltfenRootTunnelOpened;
    break;
  case 80013U:
    completionBeat = ActTwoStoryBeat::ThornChoirDiscovered;
    break;
  case 80014U:
    completionBeat = ActTwoStoryBeat::WitnessRootResolved;
    break;
  default:
    return ObjectiveState::Locked;
  }

  if (HasBeat(completionBeat)) {
    return ObjectiveState::Completed;
  }
  if (objectiveId == 80010U || Objective(objectiveId - 1U) == ObjectiveState::Completed) {
    return ObjectiveState::Active;
  }
  return ObjectiveState::Locked;
}

bool ActTwoStoryRuntime::RestoreState(const ActTwoStoryState &state) noexcept {
  if (!ValidState(state)) {
    return false;
  }
  State_ = state;
  return true;
}

bool ActTwoStoryRuntime::ValidState(const ActTwoStoryState &state) noexcept {
  if ((state.CompletedMask & ~FinalMask) != 0U) {
    return false;
  }
  if (!state.Unlocked) {
    return state.CompletedMask == 0U &&
           state.RootDecision == WitnessRootDecision::None;
  }

  bool gap = false;
  for (std::uint32_t index = 0;
       index <= static_cast<std::uint32_t>(ActTwoStoryBeat::WitnessRootResolved);
       ++index) {
    const bool set = (state.CompletedMask & (1U << index)) != 0U;
    if (!set) {
      gap = true;
    } else if (gap) {
      return false;
    }
  }

  const bool resolved =
      (state.CompletedMask & Bit(ActTwoStoryBeat::WitnessRootResolved)) != 0U;
  if (resolved != (state.RootDecision != WitnessRootDecision::None)) {
    return false;
  }
  return static_cast<std::uint32_t>(state.RootDecision) <=
         static_cast<std::uint32_t>(WitnessRootDecision::Burn);
}

bool ActTwoStoryCodec::Serialize(const ActTwoStoryState &state,
                                 std::string &output) {
  ActTwoStoryRuntime validator;
  if (!validator.RestoreState(state)) {
    return false;
  }
  output = "A2S1|" + std::to_string(state.Unlocked ? 1U : 0U) + "|" +
           std::to_string(state.CompletedMask) + "|" +
           std::to_string(static_cast<std::uint32_t>(state.RootDecision));
  return true;
}

bool ActTwoStoryCodec::Deserialize(std::string_view text,
                                   ActTwoStoryState &output) {
  if (text.size() > 128U || !text.starts_with("A2S1|")) {
    return false;
  }
  std::array<std::uint32_t, 3> values{};
  auto remainder = text.substr(5);
  for (std::size_t index = 0; index < values.size(); ++index) {
    const auto separator = remainder.find('|');
    const bool final = index + 1U == values.size();
    if ((!final && separator == std::string_view::npos) ||
        (final && separator != std::string_view::npos)) {
      return false;
    }
    const auto token = final ? remainder : remainder.substr(0, separator);
    if (!ParseUnsigned(token, values[index])) {
      return false;
    }
    if (!final) {
      remainder.remove_prefix(separator + 1U);
    }
  }
  if (values[0] > 1U ||
      values[2] > static_cast<std::uint32_t>(WitnessRootDecision::Burn)) {
    return false;
  }

  ActTwoStoryState candidate{values[0] == 1U, values[1],
                             static_cast<WitnessRootDecision>(values[2])};
  ActTwoStoryRuntime validator;
  if (!validator.RestoreState(candidate)) {
    return false;
  }
  output = candidate;
  return true;
}

} // namespace Lostsense::Gameplay
