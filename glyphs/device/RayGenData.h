// ======================================================================== //
// Copyright 2018-2020 Ingo Wald                                            //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "glyphs/device/FrameState.h"
#include "glyphs/device/GlyphsGeom.h"

namespace glyphs {
  namespace device {

    struct RayGenData {
      int deviceIndex;
      int deviceCount;
      OptixTraversableHandle world;
      vec2ui      fbSize;
      uint32_t   *colorBufferPtr;
#ifdef __CUDA_ARCH__
      float4     *accumBufferPtr;
#else
      vec4f      *accumBufferPtr;
#endif
      Link       *linkBuffer;
      FrameState *frameStateBuffer;
    };
  
  }
}
