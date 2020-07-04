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

#include "glyphs/device/common.h"

namespace glyphs {
  namespace device {

    struct Link {
      vec3f pos;
      
      /*! per vertex color, 8-bit RGBA */
      unsigned col = (unsigned)-1;
      
      /* current node radius*/
      float rad;

      /* acceleration of glyph */
      vec3f accel = vec3f();

      /*! previous node in the graph; either a valid index in the
          geom's control point buffer, or '-1' to indicate 'no
          predecessor' (ie, this is then the first control point in a
          node/stream line */      
      int   prev;
    };
    
    /*! the device-side glyphs geometry */
    struct GlyphsGeom {
      /*! list of all control points (a buffer) */
      Link         *links;
      float         radius;
      int           numLinks;
    };

    struct TrianglesGeomData
    {
        vec3f*  color;
        vec3i* index;
        vec3f* vertex;
    };

    struct SuperGeomData
    {
        vec3f* rst;
        vec3f* ABC;
        vec3f*  color;
        vec3i* index;
        vec3f* vertex;
    };

  }
}
