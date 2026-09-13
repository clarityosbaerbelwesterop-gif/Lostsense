#include "Lostsense/Gameplay/Loadout/AbilityLoadout.h"
#include "TestHarness.h"

#include <vector>

namespace {
using namespace Lostsense;
using namespace Lostsense::Gameplay;
using Tests::TestSuite;

constexpr ClassId Knight{1U};
constexpr ClassId Wizard{2U};
constexpr AbilityId Slash{100U};
constexpr AbilityId EchoStrike{101U};
constexpr AbilityId LockedSkill{102U};
constexpr AbilityId WizardSkill{103U};
constexpr AbilityId UltimateSkill{104U};

AbilityLoadoutSlotMask Slots(const AbilityLoadoutSlot first,
                             const AbilityLoadoutSlot second) noexcept {
  return static_cast<AbilityLoadoutSlotMask>(AbilityLoadoutSlotBit(first) |
                                             AbilityLoadoutSlotBit(second));
}

std::vector<AbilityDefinition> Definitions() {
  AbilityDefinition slash;
  slash.Id = Slash;
  slash.RequiredClass = Knight;
  slash.AllowedLoadoutSlots =
      Slots(AbilityLoadoutSlot::Active1, AbilityLoadoutSlot::Active2);

  AbilityDefinition echo;
  echo.Id = EchoStrike;
  echo.RequiredClass = Knight;
  echo.AllowedLoadoutSlots =
      Slots(AbilityLoadoutSlot::Active1, AbilityLoadoutSlot::Active2);
  echo.AllowDuplicateInLoadout = true;

  AbilityDefinition locked;
  locked.Id = LockedSkill;
  locked.RequiredClass = Knight;
  locked.AllowedLoadoutSlots =
      AbilityLoadoutSlotBit(AbilityLoadoutSlot::Active2);

  AbilityDefinition wizard;
  wizard.Id = WizardSkill;
  wizard.RequiredClass = Wizard;
  wizard.AllowedLoadoutSlots =
      AbilityLoadoutSlotBit(AbilityLoadoutSlot::Active3);

  AbilityDefinition ultimate;
  ultimate.Id = UltimateSkill;
  ultimate.RequiredClass = Knight;
  ultimate.AllowedLoadoutSlots =
      AbilityLoadoutSlotBit(AbilityLoadoutSlot::Ultimate);

  return {slash, echo, locked, wizard, ultimate};
}

struct Fixture final {
  Combat::Combatant Owner{Combat::CombatantId{1U},
                          Combat::CombatantKind::Player};
  EffectRuntime Effects{Owner, std::vector<EffectDefinition>{}};
  Core::DeterministicRandom Random{10U, 3U};
  AbilityRuntime Abilities{Owner, Effects, Random, Knight, Definitions()};
  AbilityLoadout Loadout{Abilities};
};

void TestEquipValidation(TestSuite &suite) {
  Fixture fixture;
  suite.Expect(fixture.Loadout.IsValid(),
               "loadout accepts valid ability runtime");
  suite.Expect(fixture.Abilities.OwnerClass() == Knight,
               "ability runtime exposes authoritative owner class");
  suite.Expect(fixture.Abilities.Unlock(Slash), "slash unlocks");
  suite.Expect(fixture.Abilities.Unlock(EchoStrike), "echo strike unlocks");
  suite.Expect(fixture.Abilities.Unlock(UltimateSkill), "ultimate unlocks");

  suite.Expect(fixture.Loadout.Equip(AbilityLoadoutSlot::Active1, Slash) ==
                   LoadoutResult::Success,
               "owned ability equips into allowed slot");
  suite.Expect(fixture.Loadout.AbilityAt(AbilityLoadoutSlot::Active1) == Slash,
               "equipped ability is queryable");
  suite.Expect(fixture.Loadout.UsesAbility(Slash),
               "loadout reports equipped ability usage");

  suite.Expect(fixture.Loadout.Equip(AbilityLoadoutSlot::Active2, Slash) ==
                   LoadoutResult::DuplicateNotAllowed,
               "duplicate restriction blocks a second copy");
  suite.Expect(fixture.Loadout.Equip(AbilityLoadoutSlot::Active3, Slash) ==
                   LoadoutResult::SlotNotAllowed,
               "authored slot restriction is enforced");
  suite.Expect(fixture.Loadout.Equip(AbilityLoadoutSlot::Active1,
                                     LockedSkill) == LoadoutResult::NotUnlocked,
               "locked ability cannot enter loadout");
  suite.Expect(
      fixture.Loadout.Equip(AbilityLoadoutSlot::Active1, AbilityId{999U}) ==
          LoadoutResult::UnknownAbility,
      "unknown ability IDs are rejected");
  suite.Expect(fixture.Loadout.Equip(
                   static_cast<AbilityLoadoutSlot>(AbilityLoadoutSlotCount),
                   Slash) == LoadoutResult::InvalidSlot,
               "out-of-range loadout slot is rejected");

  suite.Expect(fixture.Loadout.Equip(AbilityLoadoutSlot::Ultimate,
                                     UltimateSkill) == LoadoutResult::Success,
               "ultimate respects dedicated slot compatibility");
  suite.Expect(
      fixture.Loadout.Equip(AbilityLoadoutSlot::Active3, UltimateSkill) ==
          LoadoutResult::SlotNotAllowed,
      "ultimate cannot enter an incompatible active slot");
}

void TestDuplicatePolicy(TestSuite &suite) {
  Fixture fixture;
  suite.Expect(fixture.Abilities.Unlock(Slash), "slash unlocks for duplicates");
  suite.Expect(fixture.Abilities.Unlock(EchoStrike),
               "duplicate-capable ability unlocks");

  suite.Expect(fixture.Loadout.Equip(AbilityLoadoutSlot::Active1, EchoStrike) ==
                   LoadoutResult::Success,
               "duplicate-capable ability equips first copy");
  suite.Expect(fixture.Loadout.Equip(AbilityLoadoutSlot::Active2, EchoStrike) ==
                   LoadoutResult::Success,
               "duplicate-capable ability equips second copy");

  suite.Expect(fixture.Loadout.Unequip(AbilityLoadoutSlot::Active2),
               "occupied slot unequips");
  suite.Expect(!fixture.Loadout.Unequip(AbilityLoadoutSlot::Active2),
               "empty slot cannot be unequipped twice");
}

void TestCaptureRestoreAndCorruption(TestSuite &suite) {
  Fixture fixture;
  suite.Expect(fixture.Abilities.Unlock(Slash), "slash unlocks for restore");
  suite.Expect(fixture.Abilities.Unlock(EchoStrike),
               "echo unlocks for restore");
  suite.Expect(fixture.Loadout.Equip(AbilityLoadoutSlot::Active1, Slash) ==
                   LoadoutResult::Success,
               "restore fixture equips slash");
  suite.Expect(fixture.Loadout.Equip(AbilityLoadoutSlot::Active2, EchoStrike) ==
                   LoadoutResult::Success,
               "restore fixture equips echo");

  const AbilityLoadoutState saved = fixture.Loadout.CaptureState();
  suite.Expect(fixture.Loadout.Unequip(AbilityLoadoutSlot::Active1),
               "loadout mutates after capture");
  suite.Expect(fixture.Loadout.RestoreState(saved),
               "valid loadout state restores");
  suite.Expect(fixture.Loadout.AbilityAt(AbilityLoadoutSlot::Active1) == Slash,
               "restore recreates first slot exactly");

  AbilityLoadoutState corrupt = saved;
  corrupt.Slots[static_cast<std::size_t>(AbilityLoadoutSlot::Active2)] =
      AbilityId{999U};
  const AbilityLoadoutState before = fixture.Loadout.CaptureState();
  suite.Expect(!fixture.Loadout.RestoreState(corrupt),
               "unknown ability in persisted loadout is rejected");
  suite.Expect(fixture.Loadout.CaptureState().Slots == before.Slots,
               "rejected restore leaves loadout unchanged");

  AbilityLoadoutState invalidSlot = saved;
  invalidSlot.Slots[static_cast<std::size_t>(AbilityLoadoutSlot::Active2)] =
      Slash;
  suite.Expect(!fixture.Loadout.RestoreState(invalidSlot),
               "persisted duplicate restriction is rejected");
}

void TestAbilityDefinitionLoadoutMaskValidation(TestSuite &suite) {
  Fixture fixture;
  AbilityDefinition invalid;
  invalid.Id = AbilityId{500U};
  invalid.AllowedLoadoutSlots = static_cast<AbilityLoadoutSlotMask>(1U << 15U);
  Core::DeterministicRandom random{20U, 5U};
  AbilityRuntime runtime{
      fixture.Owner, fixture.Effects, random, Knight, {invalid}};
  suite.Expect(!runtime.IsValid(),
               "ability definition rejects unknown loadout slot bits");

  suite.Expect(!fixture.Abilities.Unlock(WizardSkill),
               "wrong-class ability cannot become owned/unlocked");
}

} // namespace

int main() {
  TestSuite suite{"ability loadout"};
  TestEquipValidation(suite);
  TestDuplicatePolicy(suite);
  TestCaptureRestoreAndCorruption(suite);
  TestAbilityDefinitionLoadoutMaskValidation(suite);
  return suite.Finish();
}
