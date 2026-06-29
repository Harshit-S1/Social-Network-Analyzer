cmake_minimum_required(VERSION 3.15)
project(SocialNetworkAnalyzer)

# enforces C++17
set(CMAKE_CXX_STANDARD 17)

# using python3 explicitly to find pybind11 headers
execute_process(
    COMMAND python3 -m pybind11 --includes
    OUTPUT_VARIABLE PYBIND11_INCLUDES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# cleaning the output string to be used by CMake
separate_arguments(PYBIND11_INCLUDES)
string(REPLACE "-I" "" PYBIND11_INCLUDES "${PYBIND11_INCLUDES}")
include_directories(${PYBIND11_INCLUDES})

# defining our Python extension module
add_library(network_engine MODULE src/bindings.cpp src/Graph.cpp src/Analytics.cpp)

# preventing CMake from adding a lib prefix
set_target_properties(network_engine PROPERTIES PREFIX "")

# using python3 explicitly to get the correct OS extension (.so)
execute_process(
    COMMAND python3 -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))"
    OUTPUT_VARIABLE PYTHON_EXT_SUFFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# wrapping the variable in quotes for safety
set_target_properties(network_engine PROPERTIES SUFFIX "${PYTHON_EXT_SUFFIX}")

# optimization flags for the C++
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(network_engine PRIVATE -O3 -march=native)
endif()