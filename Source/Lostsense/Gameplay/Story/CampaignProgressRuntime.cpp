#include "CampaignProgressRuntime.h"

#include <array>
#include <charconv>

namespace Lostsense::Gameplay {
namespace {
bool ParseUnsigned(std::string_view text, std::uint32_t &value) noexcept {
  if (text.empty()) {
    return false;
  }
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value);
  return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
} // namespace

bool CampaignProgressRuntime::Begin() {
  if (State_.Started || !CampaignCatalog::IsCanonical()) {
    return false;
  }
  State_.Started = true;
  State_.NextQuestIndex = FirstContinuationQuestIndex;
  return true;
}

bool CampaignProgressRuntime::CompleteQuest(const std::uint32_t questId) {
  if (!State_.Started || IsFinished()) {
    return false;
  }
  const auto &quests = CampaignCatalog::Quests();
  if (State_.NextQuestIndex >= quests.size() ||
      quests[State_.NextQuestIndex].Id != questId) {
    return false;
  }
  ++State_.NextQuestIndex;
  return true;
}

ObjectiveState
CampaignProgressRuntime::Objective(const std::uint32_t questId) const noexcept {
  if (!State_.Started) {
    return ObjectiveState::Locked;
  }
  const auto index = CampaignCatalog::QuestIndex(questId);
  if (index == static_cast<std::size_t>(-1) ||
      index < FirstContinuationQuestIndex) {
    return ObjectiveState::Locked;
  }
  if (index < State_.NextQuestIndex) {
    return ObjectiveState::Completed;
  }
  return index == State_.NextQuestIndex ? ObjectiveState::Active
                                        : ObjectiveState::Locked;
}

std::uint32_t CampaignProgressRuntime::CurrentQuestId() const noexcept {
  const auto &quests = CampaignCatalog::Quests();
  return State_.Started && State_.NextQuestIndex < quests.size()
             ? quests[State_.NextQuestIndex].Id
             : 0U;
}

bool CampaignProgressRuntime::IsFinished() const noexcept {
  return State_.Started &&
         State_.NextQuestIndex == CampaignCatalog::Quests().size();
}

bool CampaignProgressRuntime::RestoreState(
    const CampaignProgressState &state) noexcept {
  if (!ValidState(state)) {
    return false;
  }
  State_ = state;
  return true;
}

bool CampaignProgressRuntime::ValidState(
    const CampaignProgressState &state) noexcept {
  if (state.SchemaVersion != CurrentSchemaVersion ||
      !CampaignCatalog::IsCanonical()) {
    return false;
  }
  if (!state.Started) {
    return state.NextQuestIndex == 0U;
  }
  return state.NextQuestIndex >= FirstContinuationQuestIndex &&
         state.NextQuestIndex <= CampaignCatalog::Quests().size();
}

bool CampaignProgressCodec::Serialize(const CampaignProgressState &state,
                                      std::string &output) {
  CampaignProgressRuntime validator;
  if (!validator.RestoreState(state)) {
    return false;
  }
  output = "CPS1|" + std::to_string(state.Started ? 1U : 0U) + "|" +
           std::to_string(state.NextQuestIndex);
  return true;
}

bool CampaignProgressCodec::Deserialize(std::string_view text,
                                        CampaignProgressState &output) {
  if (text.size() > 64U || !text.starts_with("CPS1|")) {
    return false;
  }
  std::array<std::uint32_t, 2> values{};
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
  if (values[0] > 1U) {
    return false;
  }
  const CampaignProgressState candidate{
      CampaignProgressRuntime::CurrentSchemaVersion, values[0] == 1U,
      values[1]};
  CampaignProgressRuntime validator;
  if (!validator.RestoreState(candidate)) {
    return false;
  }
  output = candidate;
  return true;
}

} // namespace Lostsense::Gameplay
