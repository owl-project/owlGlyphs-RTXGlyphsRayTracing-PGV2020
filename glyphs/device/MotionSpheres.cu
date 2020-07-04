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

#include "glyphs/device/GlyphsGeom.h"
#include "glyphs/device/PerRayData.h"
#include "glyphs/device/RayGenData.h"

#include "glyphs/device/Camera.h"
#include "glyphs/device/roundedCone.h"

namespace glyphs {
  namespace device {
    

    OPTIX_INTERSECT_PROGRAM(MotionSpheres)()
    {
      int primID = optixGetPrimitiveIndex();

      const auto& self
        = owl::getProgramData<GlyphsGeom>();

      owl::Ray ray(optixGetWorldRayOrigin(),
                   optixGetWorldRayDirection(),
                   optixGetRayTmin(),
                   optixGetRayTmax());
      const Link link = self.links[primID];
      if (link.prev < 0) return;

      float tmp_hit_t = ray.tmax;

      vec3f pa = link.pos;
      float ra = link.rad;
      vec3f pb = link.prev<0 ? pa : self.links[link.prev].pos;
      vec3f accel = link.accel;

      PerRayData& prd = owl::getPRD<PerRayData>();
      Random& rnd = *prd.rnd;

      // Compute random sphere position in time for motion blur
      float dt = 4e-3f;
      vec3f va = (pa-pb)/0.02f;
      vec3f p = 0.5f*(pa+pb);
      float a = dot(normalize(va),accel); // acceleration in direction of arrow

      float r = (rnd() - 0.5f);
      vec3f pc = p+r*dt*(va+r*dt*normalize(va)*a);
      vec3f normal;
      if (intersectSphere2(pc, ra, ray, tmp_hit_t, normal)) {
        if (optixReportIntersection(tmp_hit_t, 0)) {
          prd.primID = primID;
          prd.meshID = -1;
          prd.t = tmp_hit_t;
          prd.Ng = normal;
        }
      }
    }

    
    OPTIX_BOUNDS_PROGRAM(MotionSpheres)(const void  *geomData,
                                        box3f       &primBounds,
                                        const int    primID)
    {
      const GlyphsGeom &self = *(const GlyphsGeom*)geomData;
      const Link &link = self.links[primID];
      
      vec3f pa = link.pos;
      primBounds
        = box3f()
        .including(pa-self.radius)
        .including(pa+self.radius);

      if (link.prev >= 0) {
        // add space that connects to previous point:
        vec3f pb = self.links[link.prev].pos;
        primBounds
          = primBounds
          .including(pb-self.radius)
          .including(pb+self.radius);
      }
    }

    OPTIX_CLOSEST_HIT_PROGRAM(MotionSpheres)()
    {
      /* nothing to do, since we already modify in intersect prog */
    }    

  }
}

