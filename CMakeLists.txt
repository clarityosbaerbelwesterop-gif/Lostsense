cmake_minimum_required(VERSION 3.20)
project(Lostsense LANGUAGES CXX)

option(LOSTSENSE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(LOSTSENSE_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(lostsense_apply_quality target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(LOSTSENSE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(LOSTSENSE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
        if(LOSTSENSE_ENABLE_SANITIZERS)
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endfunction()

add_library(lostsense_core
    Source/Lostsense/Core/DeterministicRandom.cpp
)
target_include_directories(lostsense_core PUBLIC Source/Lostsense)
lostsense_apply_quality(lostsense_core)

add_library(lostsense_stats
    Source/Lostsense/Stats/AttributeSet.cpp
    Source/Lostsense/Stats/CombatAttributes.cpp
)
target_include_directories(lostsense_stats PUBLIC Source/Lostsense)
target_link_libraries(lostsense_stats PUBLIC lostsense_core)
lostsense_apply_quality(lostsense_stats)

add_library(lostsense_combat
    Source/Lostsense/Combat/Combatant.cpp
    Source/Lostsense/Combat/DamageCalculator.cpp
    Source/Lostsense/Combat/VitalPools.cpp
)
target_include_directories(lostsense_combat PUBLIC Source/Lostsense)
target_link_libraries(lostsense_combat PUBLIC lostsense_stats lostsense_core)
lostsense_apply_quality(lostsense_combat)

add_library(lostsense_gameplay
    Source/Lostsense/Gameplay/Abilities/AbilityRuntime.cpp
    Source/Lostsense/Gameplay/Content/FirstSliceContent.cpp
    Source/Lostsense/Gameplay/Effects/EffectRuntime.cpp
    Source/Lostsense/Gameplay/Events/GameplayEvents.cpp
    Source/Lostsense/Gameplay/Items/Equipment.cpp
    Source/Lostsense/Gameplay/Items/Inventory.cpp
    Source/Lostsense/Gameplay/Items/ItemCatalog.cpp
    Source/Lostsense/Gameplay/Items/LootRuntime.cpp
    Source/Lostsense/Gameplay/Loadout/AbilityLoadout.cpp
    Source/Lostsense/Gameplay/Persistence/CharacterPersistence.cpp
    Source/Lostsense/Gameplay/Persistence/SaveCodec.cpp
    Source/Lostsense/Gameplay/Progression/SkillGraph.cpp
    Source/Lostsense/Gameplay/Story/StoryRuntime.cpp
    Source/Lostsense/Gameplay/World/DeepRouteState.cpp
)
target_include_directories(lostsense_gameplay PUBLIC Source/Lostsense)
target_link_libraries(lostsense_gameplay PUBLIC lostsense_combat lostsense_stats lostsense_core)
lostsense_apply_quality(lostsense_gameplay)

enable_testing()

function(lostsense_test name source libraries)
    add_executable(${name} ${source})
    target_include_directories(${name} PRIVATE Source/Lostsense Tests)
    target_link_libraries(${name} PRIVATE ${libraries})
    lostsense_apply_quality(${name})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

lostsense_test(deterministic_random_tests Tests/Core/DeterministicRandomTests.cpp lostsense_core)
lostsense_test(attribute_set_tests Tests/Stats/AttributeSetTests.cpp lostsense_stats)
lostsense_test(damage_calculator_tests Tests/Combat/DamageCalculatorTests.cpp lostsense_combat)
lostsense_test(vital_pools_tests Tests/Combat/VitalPoolsTests.cpp lostsense_combat)
lostsense_test(combatant_tests Tests/Combat/CombatantTests.cpp lostsense_combat)
lostsense_test(effect_runtime_tests Tests/Gameplay/EffectRuntimeTests.cpp lostsense_gameplay)
lostsense_test(ability_runtime_tests Tests/Gameplay/AbilityRuntimeTests.cpp lostsense_gameplay)
lostsense_test(skill_graph_tests Tests/Gameplay/SkillGraphTests.cpp lostsense_gameplay)
lostsense_test(ability_loadout_tests Tests/Gameplay/AbilityLoadoutTests.cpp lostsense_gameplay)
lostsense_test(item_runtime_tests Tests/Gameplay/ItemRuntimeTests.cpp lostsense_gameplay)
lostsense_test(loot_runtime_tests Tests/Gameplay/LootRuntimeTests.cpp lostsense_gameplay)
lostsense_test(gameplay_event_tests Tests/Gameplay/GameplayEventTests.cpp lostsense_gameplay)
lostsense_test(persistence_tests Tests/Gameplay/PersistenceTests.cpp lostsense_gameplay)
lostsense_test(first_slice_content_tests Tests/Gameplay/FirstSliceContentTests.cpp lostsense_gameplay)
lostsense_test(story_runtime_tests Tests/Gameplay/StoryRuntimeTests.cpp lostsense_gameplay)
lostsense_test(deep_route_state_tests Tests/Gameplay/DeepRouteStateTests.cpp lostsense_gameplay)
