cmake_minimum_required(VERSION 3.20)
project(Lostsense VERSION 0.10.0 LANGUAGES CXX)

option(LOSTSENSE_ENABLE_SANITIZERS
       "Enable address and undefined-behaviour sanitizers" OFF)
option(LOSTSENSE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(lostsense_project_options INTERFACE)
target_compile_features(lostsense_project_options INTERFACE cxx_std_20)

if(MSVC)
  target_compile_options(lostsense_project_options INTERFACE /W4 /permissive-)
  if(LOSTSENSE_WARNINGS_AS_ERRORS)
    target_compile_options(lostsense_project_options INTERFACE /WX)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(
    lostsense_project_options
    INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  if(LOSTSENSE_WARNINGS_AS_ERRORS)
    target_compile_options(lostsense_project_options INTERFACE -Werror)
  endif()
endif()

if(LOSTSENSE_ENABLE_SANITIZERS)
  if(MSVC)
    message(WARNING "LOSTSENSE_ENABLE_SANITIZERS is not configured for MSVC")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
      lostsense_project_options
      INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(lostsense_project_options
                        INTERFACE -fsanitize=address,undefined)
  else()
    message(WARNING "Sanitizers are unsupported by the selected compiler")
  endif()
endif()

add_library(lostsense_core
            Source/Lostsense/Core/DeterministicRandom.cpp)
target_include_directories(lostsense_core PUBLIC Source)
target_link_libraries(lostsense_core PUBLIC lostsense_project_options)

add_library(
  lostsense_combat
  Source/Lostsense/Combat/Combatant.cpp
  Source/Lostsense/Combat/DamageCalculator.cpp
  Source/Lostsense/Combat/VitalPools.cpp
  Source/Lostsense/Stats/AttributeSet.cpp
  Source/Lostsense/Stats/CombatAttributes.cpp)
target_include_directories(lostsense_combat PUBLIC Source)
target_link_libraries(lostsense_combat PUBLIC lostsense_core
                                           lostsense_project_options)

add_library(
  lostsense_gameplay
  Source/Lostsense/Gameplay/Effects/EffectRuntime.cpp
  Source/Lostsense/Gameplay/Content/FirstSliceContent.cpp
  Source/Lostsense/Gameplay/Abilities/AbilityRuntime.cpp
  Source/Lostsense/Gameplay/Loadout/AbilityLoadout.cpp
  Source/Lostsense/Gameplay/Progression/SkillGraph.cpp
  Source/Lostsense/Gameplay/Items/ItemCatalog.cpp
  Source/Lostsense/Gameplay/Items/Inventory.cpp
  Source/Lostsense/Gameplay/Items/Equipment.cpp
  Source/Lostsense/Gameplay/Items/LootRuntime.cpp
  Source/Lostsense/Gameplay/Events/GameplayEvents.cpp
  Source/Lostsense/Gameplay/Persistence/CharacterPersistence.cpp
  Source/Lostsense/Gameplay/Persistence/SaveCodec.cpp
  Source/Lostsense/Gameplay/Story/StoryRuntime.cpp
  Source/Lostsense/Gameplay/Story/ActTwoStoryRuntime.cpp
  Source/Lostsense/Gameplay/Story/CampaignCatalog.cpp
  Source/Lostsense/Gameplay/Story/CampaignProgressRuntime.cpp
  Source/Lostsense/Gameplay/Story/ActSevenChoiceRuntime.cpp
  Source/Lostsense/Gameplay/World/DeepRouteState.cpp)
target_include_directories(lostsense_gameplay PUBLIC Source)
target_link_libraries(lostsense_gameplay PUBLIC lostsense_combat
                                             lostsense_project_options)

include(CTest)
if(BUILD_TESTING)
  function(lostsense_add_test target source test_name)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE Tests)
    target_link_libraries(${target} PRIVATE lostsense_combat
                                            lostsense_project_options)
    add_test(NAME ${test_name} COMMAND ${target})
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 30)
  endfunction()

  lostsense_add_test(lostsense_damage_tests
                     Tests/Combat/DamageCalculatorTests.cpp
                     combat.damage_pipeline)
  lostsense_add_test(lostsense_vital_pool_tests
                     Tests/Combat/VitalPoolsTests.cpp combat.vital_pools)
  lostsense_add_test(lostsense_combatant_tests
                     Tests/Combat/CombatantTests.cpp combat.combatant)
  lostsense_add_test(lostsense_random_tests
                     Tests/Core/DeterministicRandomTests.cpp core.random)
  lostsense_add_test(lostsense_attribute_tests
                     Tests/Stats/AttributeSetTests.cpp stats.attributes)

  function(lostsense_add_gameplay_test target source test_name)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE Tests)
    target_link_libraries(${target} PRIVATE lostsense_gameplay
                                            lostsense_project_options)
    add_test(NAME ${test_name} COMMAND ${target})
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 30)
  endfunction()

  lostsense_add_gameplay_test(lostsense_effect_tests
                              Tests/Gameplay/EffectRuntimeTests.cpp
                              gameplay.effects)
  lostsense_add_gameplay_test(lostsense_ability_tests
                              Tests/Gameplay/AbilityRuntimeTests.cpp
                              gameplay.abilities)
  lostsense_add_gameplay_test(lostsense_loadout_tests
                              Tests/Gameplay/AbilityLoadoutTests.cpp
                              gameplay.loadout)
  lostsense_add_gameplay_test(lostsense_skill_graph_tests
                              Tests/Gameplay/SkillGraphTests.cpp
                              gameplay.skill_graph)
  lostsense_add_gameplay_test(lostsense_item_runtime_tests
                              Tests/Gameplay/ItemRuntimeTests.cpp
                              gameplay.items_inventory_equipment)
  lostsense_add_gameplay_test(lostsense_loot_runtime_tests
                              Tests/Gameplay/LootRuntimeTests.cpp
                              gameplay.deterministic_loot)
  lostsense_add_gameplay_test(lostsense_gameplay_event_tests
                              Tests/Gameplay/GameplayEventTests.cpp
                              gameplay.events)
  lostsense_add_gameplay_test(lostsense_persistence_tests
                              Tests/Gameplay/PersistenceTests.cpp
                              gameplay.persistence)
  lostsense_add_gameplay_test(lostsense_first_slice_content_tests
                              Tests/Gameplay/FirstSliceContentTests.cpp
                              gameplay.first_slice_content)
  lostsense_add_gameplay_test(lostsense_story_runtime_tests
                              Tests/Gameplay/StoryRuntimeTests.cpp
                              gameplay.story_runtime)
  lostsense_add_gameplay_test(lostsense_deep_route_state_tests
                              Tests/Gameplay/DeepRouteStateTests.cpp
                              gameplay.deep_route_state)
  lostsense_add_gameplay_test(lostsense_act_two_story_tests
                              Tests/Gameplay/ActTwoStoryRuntimeTests.cpp
                              gameplay.act_two_story)
  lostsense_add_gameplay_test(lostsense_campaign_catalog_tests
                              Tests/Gameplay/CampaignCatalogTests.cpp
                              gameplay.campaign_catalog)
  lostsense_add_gameplay_test(lostsense_campaign_progress_tests
                              Tests/Gameplay/CampaignProgressRuntimeTests.cpp
                              gameplay.campaign_progress)
  lostsense_add_gameplay_test(lostsense_act_seven_choice_tests
                              Tests/Gameplay/ActSevenChoiceRuntimeTests.cpp
                              gameplay.act_seven_choice)
endif()

find_program(LOSTSENSE_CLANG_FORMAT NAMES clang-format clang-format-18)
if(LOSTSENSE_CLANG_FORMAT)
  file(GLOB_RECURSE LOSTSENSE_FORMAT_FILES CONFIGURE_DEPENDS
       "${CMAKE_SOURCE_DIR}/Source/*.cpp" "${CMAKE_SOURCE_DIR}/Source/*.h"
       "${CMAKE_SOURCE_DIR}/Tests/*.cpp" "${CMAKE_SOURCE_DIR}/Tests/*.h")
  add_custom_target(
    format-check
    COMMAND ${LOSTSENSE_CLANG_FORMAT} --dry-run --Werror
            ${LOSTSENSE_FORMAT_FILES}
    COMMENT "Checking C++ formatting")
endif()
