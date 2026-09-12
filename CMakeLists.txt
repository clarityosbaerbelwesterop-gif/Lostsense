cmake_minimum_required(VERSION 3.20)
project(Lostsense VERSION 0.1.0 LANGUAGES CXX)

option(LOSTSENSE_ENABLE_SANITIZERS "Enable address and undefined-behaviour sanitizers" OFF)

add_library(lostsense_combat
    Source/Lostsense/Combat/DamageCalculator.cpp
)
target_include_directories(lostsense_combat PUBLIC Source)
target_compile_features(lostsense_combat PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(lostsense_combat PRIVATE /W4 /permissive-)
else()
    target_compile_options(lostsense_combat PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if(LOSTSENSE_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(lostsense_combat PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(lostsense_combat PUBLIC -fsanitize=address,undefined)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(lostsense_combat_tests Tests/Combat/DamageCalculatorTests.cpp)
    target_link_libraries(lostsense_combat_tests PRIVATE lostsense_combat)
    add_test(NAME combat_damage_pipeline COMMAND lostsense_combat_tests)
endif()
