#include "Lostsense/Gameplay/Story/StoryRuntime.h"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <unordered_set>

namespace Lostsense::Gameplay {
namespace {
constexpr std::uint32_t ReturnedObjective = 80001U;
constexpr std::uint32_t BellObjective = 80002U;
constexpr std::uint32_t MarksObjective = 80003U;
constexpr std::uint32_t ForemanObjective = 80004U;
constexpr std::uint32_t ShaftObjective = 80005U;
constexpr std::uint32_t TonguelessObjective = 80006U;

[[nodiscard]] bool IsBeatValid(const FirstSliceStoryBeat beat) {
  return static_cast<std::size_t>(beat) <
         static_cast<std::size_t>(FirstSliceStoryBeat::Count);
}

[[nodiscard]] bool ParseUnsigned(const std::string_view token,
                                 std::uint32_t &value) {
  if (token.empty()) {
    return false;
  }
  const char *begin = token.data();
  const char *end = token.data() + token.size();
  const auto result = std::from_chars(begin, end, value);
  return result.ec == std::errc{} && result.ptr == end;
}
} // namespace

FirstSliceStoryRuntime::FirstSliceStoryRuntime(
    std::vector<StoryObjectiveDefinition> definitions)
    : definitions_(std::move(definitions)) {
  std::unordered_set<std::uint32_t> ids;
  valid_ = !definitions_.empty();
  state_.SchemaVersion = CurrentSchemaVersion;
  state_.Objectives.reserve(definitions_.size());

  for (const StoryObjectiveDefinition &definition : definitions_) {
    if (definition.Id == 0U || !IsBeatValid(definition.CompletionBeat) ||
        !ids.insert(definition.Id).second) {
      valid_ = false;
      break;
    }
    if (std::any_of(definition.Prerequisites.begin(),
                    definition.Prerequisites.end(),
                    [](const FirstSliceStoryBeat beat) {
                      return !IsBeatValid(beat);
                    })) {
      valid_ = false;
      break;
    }
    state_.Objectives.push_back({definition.Id, ObjectiveState::Locked});
  }

  if (valid_) {
    RefreshObjectives();
  }
}

bool FirstSliceStoryRuntime::IsValid() const { return valid_; }

bool FirstSliceStoryRuntime::HasBeat(const FirstSliceStoryBeat beat) const {
  return valid_ && IsBeatValid(beat) &&
         state_.Beats[static_cast<std::size_t>(beat)];
}

ObjectiveState
FirstSliceStoryRuntime::Objective(const std::uint32_t objectiveId) const {
  const auto found =
      std::find_if(state_.Objectives.begin(), state_.Objectives.end(),
                   [objectiveId](const StoryObjectiveState &objective) {
                     return objective.Id == objectiveId;
                   });
  return found != state_.Objectives.end() ? found->State
                                          : ObjectiveState::Locked;
}

FirstSliceStoryState FirstSliceStoryRuntime::CaptureState() const {
  return state_;
}

bool FirstSliceStoryRuntime::CompleteBeat(const FirstSliceStoryBeat beat) {
  if (!valid_ || !IsBeatValid(beat)) {
    return false;
  }
  const std::size_t index = static_cast<std::size_t>(beat);
  if (state_.Beats[index]) {
    return false;
  }
  if (index > 0U && !state_.Beats[index - 1U]) {
    return false;
  }
  state_.Beats[index] = true;
  RefreshObjectives();
  return true;
}

bool FirstSliceStoryRuntime::RestoreState(const FirstSliceStoryState &state) {
  if (!valid_ || !ValidateState(state)) {
    return false;
  }
  state_ = state;
  RefreshObjectives();
  return true;
}

bool FirstSliceStoryRuntime::ValidateState(
    const FirstSliceStoryState &state) const {
  if (state.SchemaVersion != CurrentSchemaVersion ||
      state.Objectives.size() != definitions_.size()) {
    return false;
  }

  for (std::size_t index = 1U; index < state.Beats.size(); ++index) {
    if (state.Beats[index] && !state.Beats[index - 1U]) {
      return false;
    }
  }

  for (std::size_t index = 0; index < definitions_.size(); ++index) {
    const StoryObjectiveDefinition &definition = definitions_[index];
    const StoryObjectiveState &objective = state.Objectives[index];
    if (objective.Id != definition.Id) {
      return false;
    }
    const auto rawState = static_cast<std::uint8_t>(objective.State);
    if (rawState > static_cast<std::uint8_t>(ObjectiveState::Completed)) {
      return false;
    }

    const bool completed =
        state.Beats[static_cast<std::size_t>(definition.CompletionBeat)];
    const bool prerequisitesMet = std::all_of(
        definition.Prerequisites.begin(), definition.Prerequisites.end(),
        [&state](const FirstSliceStoryBeat beat) {
          return state.Beats[static_cast<std::size_t>(beat)];
        });
    const ObjectiveState expected =
        completed ? ObjectiveState::Completed
                  : (prerequisitesMet ? ObjectiveState::Active
                                      : ObjectiveState::Locked);
    if (objective.State != expected) {
      return false;
    }
  }
  return true;
}

void FirstSliceStoryRuntime::RefreshObjectives() {
  for (std::size_t index = 0; index < definitions_.size(); ++index) {
    const StoryObjectiveDefinition &definition = definitions_[index];
    StoryObjectiveState &objective = state_.Objectives[index];

    if (HasBeat(definition.CompletionBeat)) {
      objective.State = ObjectiveState::Completed;
      continue;
    }

    const bool prerequisitesMet = std::all_of(
        definition.Prerequisites.begin(), definition.Prerequisites.end(),
        [this](const FirstSliceStoryBeat beat) { return HasBeat(beat); });
    objective.State =
        prerequisitesMet ? ObjectiveState::Active : ObjectiveState::Locked;
  }
}

std::vector<StoryObjectiveDefinition> BuildFirstSliceObjectives() {
  return {
      {ReturnedObjective,
       FirstSliceStoryBeat::MetMara,
       {FirstSliceStoryBeat::ReturnedAwakened}},
      {BellObjective,
       FirstSliceStoryBeat::BellgraveDepartureAllowed,
       {FirstSliceStoryBeat::MetMara, FirstSliceStoryBeat::MetHadrun}},
      {MarksObjective,
       FirstSliceStoryBeat::FoundNinthDescentPlate,
       {FirstSliceStoryBeat::BellgraveDepartureAllowed,
        FirstSliceStoryBeat::EnteredRavelwood}},
      {ForemanObjective,
       FirstSliceStoryBeat::EnteredUpperVaur,
       {FirstSliceStoryBeat::FoundNinthDescentPlate}},
      {ShaftObjective,
       FirstSliceStoryBeat::EnteredCoinlessShaft,
       {FirstSliceStoryBeat::EnteredUpperVaur}},
      {TonguelessObjective,
       FirstSliceStoryBeat::BellgraveChanged,
       {FirstSliceStoryBeat::OdranDefeated,
        FirstSliceStoryBeat::NinthDescentRecordRecovered}},
  };
}

bool FirstSliceStoryCodec::Serialize(const FirstSliceStoryState &state,
                                     std::string &outPayload) {
  if (state.SchemaVersion != FirstSliceStoryRuntime::CurrentSchemaVersion) {
    return false;
  }

  std::ostringstream stream;
  stream << "LOSTSENSE_STORY " << state.SchemaVersion << '\n';
  stream << "BEATS ";
  for (const bool beat : state.Beats) {
    stream << (beat ? '1' : '0');
  }
  stream << '\n';
  stream << "OBJECTIVES " << state.Objectives.size() << '\n';
  for (const StoryObjectiveState &objective : state.Objectives) {
    const auto rawState = static_cast<std::uint8_t>(objective.State);
    if (objective.Id == 0U ||
        rawState > static_cast<std::uint8_t>(ObjectiveState::Completed)) {
      return false;
    }
    stream << "OBJECTIVE " << objective.Id << ' '
           << static_cast<unsigned int>(rawState) << '\n';
  }
  stream << "END\n";
  outPayload = stream.str();
  return true;
}

bool FirstSliceStoryCodec::Deserialize(const std::string_view payload,
                                       FirstSliceStoryState &outState) {
  if (payload.empty() || payload.size() > 64U * 1024U) {
    return false;
  }

  std::istringstream stream{std::string(payload)};
  std::string token;
  std::uint32_t schema = 0U;
  if (!(stream >> token) || token != "LOSTSENSE_STORY" || !(stream >> schema) ||
      schema != FirstSliceStoryRuntime::CurrentSchemaVersion) {
    return false;
  }

  if (!(stream >> token) || token != "BEATS") {
    return false;
  }
  std::string beats;
  if (!(stream >> beats) ||
      beats.size() != static_cast<std::size_t>(FirstSliceStoryBeat::Count)) {
    return false;
  }

  FirstSliceStoryState candidate;
  candidate.SchemaVersion = schema;
  for (std::size_t index = 0; index < beats.size(); ++index) {
    if (beats[index] != '0' && beats[index] != '1') {
      return false;
    }
    candidate.Beats[index] = beats[index] == '1';
  }

  if (!(stream >> token) || token != "OBJECTIVES") {
    return false;
  }
  std::uint32_t objectiveCount = 0U;
  if (!(stream >> objectiveCount) || objectiveCount > 128U) {
    return false;
  }
  candidate.Objectives.reserve(objectiveCount);
  std::unordered_set<std::uint32_t> ids;
  for (std::uint32_t index = 0U; index < objectiveCount; ++index) {
    std::string idToken;
    std::string stateToken;
    if (!(stream >> token) || token != "OBJECTIVE" || !(stream >> idToken) ||
        !(stream >> stateToken)) {
      return false;
    }
    std::uint32_t id = 0U;
    std::uint32_t rawState = 0U;
    if (!ParseUnsigned(idToken, id) || !ParseUnsigned(stateToken, rawState) ||
        id == 0U ||
        rawState > static_cast<std::uint32_t>(ObjectiveState::Completed) ||
        !ids.insert(id).second) {
      return false;
    }
    candidate.Objectives.push_back({id, static_cast<ObjectiveState>(rawState)});
  }

  if (!(stream >> token) || token != "END") {
    return false;
  }
  std::string trailing;
  if (stream >> trailing) {
    return false;
  }

  outState = std::move(candidate);
  return true;
}

} // namespace Lostsense::Gameplay
