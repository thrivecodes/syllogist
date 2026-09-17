cmake_minimum_required(VERSION 3.15)
project(syllogist VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Static core library
add_library(syllogist_core STATIC
    src/fact.c
    src/rule.c
    src/engine.c
    src/io.c
)

target_include_directories(syllogist_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(syllogist_core PRIVATE /W4)
else()
    target_compile_options(syllogist_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# CLI executable
add_executable(syllogist
    src/main.c
)

target_link_libraries(syllogist PRIVATE
    syllogist_core
)

target_include_directories(syllogist PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(syllogist PRIVATE /W4)
else()
    target_compile_options(syllogist PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Testing setup
enable_testing()

add_executable(test_fact
    tests/test_fact.c
)

target_link_libraries(test_fact PRIVATE
    syllogist_core
)

target_include_directories(test_fact PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

if(MSVC)
    target_compile_options(test_fact PRIVATE /W4)
else()
    target_compile_options(test_fact PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME test_fact COMMAND test_fact)

add_executable(test_rule
    tests/test_rule.c
)

target_link_libraries(test_rule PRIVATE
    syllogist_core
)

target_include_directories(test_rule PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

if(MSVC)
    target_compile_options(test_rule PRIVATE /W4)
else()
    target_compile_options(test_rule PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME test_rule COMMAND test_rule)

add_executable(test_engine
    tests/test_engine.c
)

target_link_libraries(test_engine PRIVATE
    syllogist_core
)

target_include_directories(test_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

if(MSVC)
    target_compile_options(test_engine PRIVATE /W4)
else()
    target_compile_options(test_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME test_engine COMMAND test_engine)
