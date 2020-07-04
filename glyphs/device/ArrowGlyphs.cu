// ======================================================================== //
// Copyright 2019-2020 The Collaborators                                    //
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

    OPTIX_INTERSECT_PROGRAM(ArrowGlyphs)()
    {
      int primID = optixGetInstanceIndex();

      const auto& self
        = owl::getProgramData<GlyphsGeom>();

      owl::Ray ray(optixGetWorldRayOrigin(),
                   optixGetWorldRayDirection(),
                   optixGetRayTmin(),
                   optixGetRayTmax());
      const Link link = self.links[primID];
      if (link.prev < 0) return;

      float tmp_hit_t = ray.tmax;

      vec3f pb, pa, pc; float ra, rb, rc;
      if (link.prev >= 0) {
        pa = link.pos;
        ra = 0.0001f;
        rb = self.links[link.prev].rad;
        pb = self.links[link.prev].pos;
        vec3f va = pa-pb;
        float len = length(va);
        float tiplen = rb*4.f;
        rc = rb/3.f;
        float maxlen = 0.5*len;
        if (tiplen > maxlen) {
          rb *= maxlen/tiplen;
          if (rb < rc)
            rb = rc;
          tiplen = maxlen;
        }
        vec3f pc = pa-normalize(va)*tiplen;
        vec3f normal;

        if (intersectRoundedCone(pc, pa, rb, ra, ray, tmp_hit_t, normal)) {
          if (optixReportIntersection(tmp_hit_t, 0)) {
            PerRayData& prd = owl::getPRD<PerRayData>();
            prd.primID = primID;
            prd.meshID = -1;
            prd.t = tmp_hit_t;
            prd.Ng = normal;
          }
        }
        if (intersectCylinder(pc, pb, rc, ray, tmp_hit_t, normal)) {
          if (optixReportIntersection(tmp_hit_t, 0)) {
            PerRayData& prd = owl::getPRD<PerRayData>();
            prd.primID = primID;
            prd.meshID = -1;
            prd.t = tmp_hit_t;
            prd.Ng = normal;
          }
        }
      }
    }

    OPTIX_BOUNDS_PROGRAM(ArrowGlyphs)(const void* geomData,
        box3f& primBounds,
        const int    primID)
    {
      primBounds = box3f(vec3f(-1.f,-1.f,0.f),
                         vec3f(+1.f,+1.f,+1.f));
    }

    OPTIX_CLOSEST_HIT_PROGRAM(ArrowGlyphs)()
    {  }

  }
}
