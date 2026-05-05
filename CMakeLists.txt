cmake_minimum_required(VERSION 3.20)

project(pob LANGUAGES CXX)

#Want only ISO C++20 with no GNU extensions or silent downgrades
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json so clangd can resolve
# include paths, language standard, and flags. Lives in build/.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#importing catch2 v3 for unit testing code
#run cmake -B build to clone it into build/_deps/; 
#cache subsequent configurations
include(FetchContent)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY
    https://github.com/catchorg/catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(catch2)

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)