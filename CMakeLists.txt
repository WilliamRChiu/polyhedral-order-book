cmake_minimum_required(VERSION 3.20)

project(pob LANGUAGES CXX)

#Want only ISO C++20 with no GNU extensions or silent downgrades
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json so clangd can resolve
# include paths, language standard, and flags. Lives in build/.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Static everywhere — single self-contained test binary, no rpath wrangling.
# Affects Catch2 + HiGHS + your own libs unless they specify STATIC/SHARED.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Make our cached BOOLs override option() calls inside fetched dependencies.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

include(FetchContent)

# Catch2 v3 — unit testing.
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(catch2)

# HiGHS — LP/MIP solver
set(ZLIB               OFF CACHE BOOL "" FORCE)  # not needed for in-process LP solving
set(ALL_TESTS          OFF CACHE BOOL "" FORCE)  # don't build HiGHS's own test suite

FetchContent_Declare(
    HiGHS
    GIT_REPOSITORY https://github.com/ERGO-Code/HiGHS.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(HiGHS)

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)

