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

cuda_compile_and_embed(embedded_common_programs device/common.cu)
cuda_compile_and_embed(embedded_ArrowGlyphs_programs device/ArrowGlyphs.cu)
cuda_compile_and_embed(embedded_MotionSpheres_programs device/MotionSpheres.cu)
cuda_compile_and_embed(embedded_SphereGlyphs_programs device/SphereGlyphs.cu)
cuda_compile_and_embed(embedded_SuperGlyphs_programs device/SuperGlyphs.cu)

include_directories(${GLUT_INCLUDE_DIR})
add_executable(owlGlyphsViewer
  ${embedded_common_programs}
  ${embedded_ArrowGlyphs_programs}
  ${embedded_MotionSpheres_programs}
  ${embedded_SphereGlyphs_programs}
  ${embedded_SuperGlyphs_programs}
  device/RayGenData.h
  ArrowGlyphs.h
  ArrowGlyphs.cpp
  viewer.cpp
  Glyphs.h
  Glyphs.cpp
  OptixGlyphs.h
  OptixGlyphs.cpp
  MotionSpheres.h
  MotionSpheres.cpp
  SphereGlyphs.h
  SphereGlyphs.cpp
  SuperGlyphs.h
  SuperGlyphs.cpp
  Triangles.h
  Triangles.cpp
  )

target_link_libraries(owlGlyphsViewer
  ${OWL_VIEWER_LIBRARIES}
  )
