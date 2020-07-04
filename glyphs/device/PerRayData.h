// ======================================================================== //
// Copyright 2018-2019 Ingo Wald                                            //
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

#include "glyphs/device/common.h"

namespace glyphs {
  namespace device {
    
    typedef owl::common::LCG<16> Random;
    
    struct PerRayData {
      /*! primitive ID, -1 means 'no hit', otherwise this is either the
         link (meshID=-1) or triangle mesh (meshID=0) */
      int primID;
      /*! ID of the triangle mehs we hit; if -1, this means we hit the
          glyphs - eventullay this should allow to support multiple
          meshes with different materials */
      int meshID;
      
      /*! distance to hit point */
      float t;      
      /* geometry normal at intersection.*/
      vec3f Ng;

      /* interpolated surface color, if available */
      vec3f color;

      /* linear congruential generator */
      Random* rnd;
    };

  }
}

