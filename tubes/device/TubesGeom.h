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

#include <assert.h>
#include "tubes/device/common.h"

namespace tubes {
  namespace device {

    // ------------------------------------------------------------------
    // Cylinder geometry
    // ------------------------------------------------------------------

    struct Cylinder
    {
      float radius;
      vec3f p1; // TODO: only the y-axis is significant
      vec3f p2; // TODO: only the y-axis is significant
    };

    inline __both__ box3f get_bounds(const Cylinder &cyl)
    {
      assert(cyl.p1.x==0.f&&cyl.p1.y==0.f&&cyl.p1.z==0.f&&cyl.p2.x==0.f&&cyl.p2.z==0.f);
      box3f bounds;
      bounds.extend(vec3f(-cyl.radius,0.f,-cyl.radius));
      bounds.extend(vec3f(-cyl.radius,0.f,+cyl.radius));
      bounds.extend(vec3f(+cyl.radius,0.f,+cyl.radius));
      bounds.extend(vec3f(+cyl.radius,0.f,-cyl.radius));
      bounds.extend(vec3f(-cyl.radius,cyl.p2.y,-cyl.radius));
      bounds.extend(vec3f(-cyl.radius,cyl.p2.y,+cyl.radius));
      bounds.extend(vec3f(+cyl.radius,cyl.p2.y,+cyl.radius));
      bounds.extend(vec3f(+cyl.radius,cyl.p2.y,-cyl.radius));
      return bounds;
    }

    // ------------------------------------------------------------------
    // Cone geometry
    // ------------------------------------------------------------------

    struct Cone
    {
      vec3f pos; //TODO: implicit, part of instance
      float radius;
      float height; //TODO: _could_ go into scale, not sure yet..
    };

    inline __both__ box3f get_bounds(const Cone &cone)
    {
      box3f bounds;
      bounds.extend(vec3f(-cone.radius,cone.pos.y,-cone.radius));
      bounds.extend(vec3f(-cone.radius,cone.pos.y,+cone.radius));
      bounds.extend(vec3f(+cone.radius,cone.pos.y,+cone.radius));
      bounds.extend(vec3f(+cone.radius,cone.pos.y,-cone.radius));
      bounds.extend(vec3f(0.f,cone.pos.y+cone.height,0.f));
      return bounds;
    }

    // ------------------------------------------------------------------
    // Compound arrow geometry
    // ------------------------------------------------------------------

    struct Arrow {
      float length;
      float thickness;
    };

    struct ArrowParts {
      Cylinder cyl;
      Cone cone;
    };

    inline __both__ ArrowParts disassemble(const Arrow &arrow)
    {
      Cylinder cyl{
        arrow.thickness*.5f,
        vec3f(0.f),
        vec3f(0.f,arrow.length-.2f,0.f)};

      float cone_height=0.2f;
      Cone cone{
        vec3f(0,arrow.length-cone_height,0),
        arrow.thickness*.7f,
        cone_height
        };

      return {cyl,cone};
    }

    inline __both__ box3f get_bounds(const Arrow &arrow)
    {
      ArrowParts parts = disassemble(arrow);
      box3f cyl_bounds = get_bounds(parts.cyl);
      box3f cone_bounds = get_bounds(parts.cone);
      return cyl_bounds.extend(cone_bounds);
    }

    struct ArrowsGeom {
      Arrow *arrows;
    };
  }
}
