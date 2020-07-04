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

add_definitions(${OWL_CXX_FLAGS})

include_directories(${OWL_INCLUDES})
include_directories(${OWL_VIEWER_INCLUDES})

#cuda_compile_and_embed(embedded_tubes_programs device/tubes.cu)
cuda_compile_and_embed(embedded_common_programs device/common.cu)
cuda_compile_and_embed(embedded_InstanceTubes_programs device/InstanceTubes.cu)

add_executable(owlTubesViewer
  ${embedded_common_programs}
  ${embedded_InstanceTubes_programs}
  viewer.cpp
  device/RayGenData.h
  Tubes.h
  Tubes.cpp
  OptixTubes.h
  OptixTubes.cpp
  InstanceTubes.h
  InstanceTubes.cpp
  )
target_link_libraries(owlTubesViewer
  ${OWL_LIBRARIES}
  ${OWL_VIEWER_LIBRARIES}
  )
