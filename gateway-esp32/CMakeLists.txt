cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS
    "$ENV{IDF_PATH}/examples/common_components/protocol_examples_common"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Include only main and its transitive component dependencies.
idf_build_set_property(MINIMAL_BUILD ON)

project(BPG)
