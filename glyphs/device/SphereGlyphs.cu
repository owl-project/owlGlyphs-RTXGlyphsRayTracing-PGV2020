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

    // ------------------------------------------------------------------
    // RTX programs for sphere glyphs
    // ------------------------------------------------------------------

    OPTIX_INTERSECT_PROGRAM(SphereGlyphs)()
    {
      int instID = optixGetInstanceIndex();

      const auto& self
        = owl::getProgramData<GlyphsGeom>();

      owl::Ray ray(optixGetObjectRayOrigin(),
                   optixGetObjectRayDirection(),
                   optixGetRayTmin(),
                   optixGetRayTmax());

      const Link link = self.links[instID];
      if (link.prev < 0) return;

      if (link.prev >= 0) {
        float tmp_hit_t = ray.tmax;
        vec3f normal;
        if (intersectInstanceSphereRTGem(vec3f(0.f), 1.f, ray, tmp_hit_t, normal)) {
          if (optixReportIntersection(tmp_hit_t, 0)) {
            PerRayData& prd = owl::getPRD<PerRayData>();
            prd.primID = instID;
            prd.meshID = -1;
            prd.t = tmp_hit_t;
            prd.Ng = optixTransformNormalFromObjectToWorldSpace(normal);
          }
        }
      }
    }

    OPTIX_BOUNDS_PROGRAM(SphereGlyphs)(const void* geomData,
        box3f& primBounds,
        const int    primID)
    {
      primBounds = box3f(vec3f(-1.f,-1.f,-1.f),
                         vec3f(+1.f,+1.f,+1.f));
    }

    OPTIX_CLOSEST_HIT_PROGRAM(SphereGlyphs)()
    {  }

  }
}
