#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Lostsense::Gameplay {

enum class FirstSliceStoryBeat : std::uint8_t {
  ReturnedAwakened,
  MetMara,
  MetHadrun,
  BellgraveDepartureAllowed,
  EnteredRavelwood,
  FoundNinthDescentPlate,
  EnteredUpperVaur,
  EnteredCoinlessShaft,
  OdranEncounterStarted,
  OdranDefeated,
  NinthDescentRecordRecovered,
  BellgraveChanged,
  Count
};

enum class ObjectiveState : std::uint8_t { Locked, Active, Completed };

struct StoryObjectiveDefinition final {
  std::uint32_t Id{0};
  FirstSliceStoryBeat CompletionBeat{FirstSliceStoryBeat::ReturnedAwakened};
  std::vector<FirstSliceStoryBeat> Prerequisites{};
};

struct StoryObjectiveState final {
  std::uint32_t Id{0};
  ObjectiveState State{ObjectiveState::Locked};

  friend bool operator==(const StoryObjectiveState &,
                         const StoryObjectiveState &) = default;
};

struct FirstSliceStoryState final {
  std::uint32_t SchemaVersion{1};
  std::array<bool, static_cast<std::size_t>(FirstSliceStoryBeat::Count)>
      Beats{};
  std::vector<StoryObjectiveState> Objectives{};

  friend bool operator==(const FirstSliceStoryState &,
                         const FirstSliceStoryState &) = default;
};

class FirstSliceStoryRuntime final {
public:
  static constexpr std::uint32_t CurrentSchemaVersion = 1;

  explicit FirstSliceStoryRuntime(
      std::vector<StoryObjectiveDefinition> definitions);

  [[nodiscard]] bool IsValid() const;
  [[nodiscard]] bool HasBeat(FirstSliceStoryBeat beat) const;
  [[nodiscard]] ObjectiveState Objective(std::uint32_t objectiveId) const;
  [[nodiscard]] FirstSliceStoryState CaptureState() const;

  [[nodiscard]] bool CompleteBeat(FirstSliceStoryBeat beat);
  [[nodiscard]] bool RestoreState(const FirstSliceStoryState &state);

private:
  [[nodiscard]] bool ValidateState(const FirstSliceStoryState &state) const;
  void RefreshObjectives();

  std::vector<StoryObjectiveDefinition> definitions_{};
  FirstSliceStoryState state_{};
  bool valid_{false};
};

[[nodiscard]] std::vector<StoryObjectiveDefinition> BuildFirstSliceObjectives();

class FirstSliceStoryCodec final {
public:
  [[nodiscard]] static bool Serialize(const FirstSliceStoryState &state,
                                      std::string &outPayload);
  [[nodiscard]] static bool Deserialize(std::string_view payload,
                                        FirstSliceStoryState &outState);
};

} // namespace Lostsense::Gameplay
