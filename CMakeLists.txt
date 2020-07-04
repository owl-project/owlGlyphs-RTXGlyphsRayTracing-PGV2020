# ======================================================================== #
# Copyright 2018-2020 The Contributors                                     #
#                                                                          #
# Licensed under the Apache License, Version 2.0 (the "License");          #
# you may not use this file except in compliance with the License.         #
# You may obtain a copy of the License at                                  #
#                                                                          #
#     http://www.apache.org/licenses/LICENSE-2.0                           #
#                                                                          #
# Unless required by applicable law or agreed to in writing, software      #
# distributed under the License is distributed on an "AS IS" BASIS,        #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
# See the License for the specific language governing permissions and      #
# limitations under the License.                                           #
# ======================================================================== #

cmake_policy(SET CMP0048 NEW)
project(owlTubes VERSION 0.1.0)

cmake_minimum_required(VERSION 2.8)
# iw, 1/3/20: yes, i know this is 'old' cmake style, but on CentOS7
# this is the only way that works, since the 'correct' way of setting
# the cxx11 standard doesnt propagate those flags to nvcc for the cuda
# programs. TODO: check to see if this can be fixed in the
# configure_cuda script instead.
if (NOT WIN32)
set (CMAKE_CXX_FLAGS "--std=c++11")
set (CUDA_PROPAGATE_HOST_FLAGS ON)
endif()

set(owl_dir ${CMAKE_CURRENT_SOURCE_DIR}/../owl)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${owl_dir}/owl/common/cmake/")

include(configure_owl)
include(configure_cuda)
#include(configure_optix)
#include(configure_build_type)

# -------------------------------------------------------
if (NOT WIN32) 
set(CMAKE_CXX_FLAGS "-std=c++11")
endif()
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# -------------------------------------------------------
include_directories("${OWL_INCLUDES}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
add_subdirectory(${owl_dir}
  external_owl
  EXCLUDE_FROM_ALL)
add_subdirectory(${owl_dir}/owl/common/viewerWidget
  external_owl_viewerWidget
  EXCLUDE_FROM_ALL)

# -------------------------------------------------------
add_subdirectory(tubes)
add_subdirectory(tools)

