cmake_minimum_required(VERSION 3.20)
project(spartak_fx_research LANGUAGES CXX)

include(CTest)

# The release subproject is the single verified build definition.
add_subdirectory(build_release)
