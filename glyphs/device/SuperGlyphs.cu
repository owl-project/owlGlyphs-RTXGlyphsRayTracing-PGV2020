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
#include "glyphs/device/Super.h"

#include "glyphs/device/Camera.h"
#include "glyphs/device/roundedCone.h"

#define DBG     0
#define NEWTON  1

namespace glyphs {
  namespace device {

#if TESSELLATE_SUPER_GLYPHS
#   define SUPER_GLYPHS_MAIN OPTIX_CLOSEST_HIT_PROGRAM(SuperGlyphs)()
#elif USER_GEOM_SUPER_GLYPHS
#   define SUPER_GLYPHS_MAIN OPTIX_INTERSECT_PROGRAM(SuperGlyphs)()
#else
#   define SUPER_GLYPHS_MAIN OPTIX_ANY_HIT_PROGRAM(SuperGlyphs)()
#endif

#if USER_GEOM_SUPER_GLYPHS
    OPTIX_BOUNDS_PROGRAM(SuperGlyphs)(const void* geomData,
        box3f& primBounds,
        const int    primID)
    {
      primBounds = box3f(vec3f(-1.f,-1.f,-1.f),
                         vec3f(+1.f,+1.f,+1.f));
    }

    OPTIX_CLOSEST_HIT_PROGRAM(SuperGlyphs)()
    { }
#endif

    __device__
    inline bool refine(const SuperGeomData& self, float& t, vec3f& n)
    {
      const vec3f& rst = self.rst[0];
      const vec3f& ABC = self.ABC[0];
      super::Quadric sq{rst.x,rst.y,rst.z,ABC.x,ABC.y,ABC.z};

      vec3f ori = optixGetObjectRayOrigin();
      vec3f dir = vec3f(optixGetObjectRayDirection());
      vec3f pos = ori + dir * t;

#if NEWTON
      // Refinement with Newton's method
      float f = super::f(sq,pos);
      vec3f df = super::df(sq,pos);
      t += 1e-2f;
      for (int i=0; i<50; ++i) {
        pos = ori + dir * t;
        f = super::f(sq,pos);
        df = super::df(sq,pos);
        if (f < 1e-3f) {
          n = super::normal(sq,pos);
          return true;
        }
        float dt = .5f * (f / dot(df,dir));
        t -= dt;
      }
#if DBG
      const vec2i pixelID = owl::getLaunchIndex();
      return pixelID.x % 2 == 0 && pixelID.y % 2 == 0;
#else
      return false;
#endif
#else
      // Refinement with secant method
      float f = super::f(sq,pos);
      float t1 = t;
      t += 1e-3f;
      while (f > 1e-3f) {
        float oldF = f;
        pos = ori + dir * t;
        f = super::f(sq,pos);
        if (f >= oldF) {
#if DBG
          const vec2i pixelID = owl::getLaunchIndex();
          return pixelID.x % 2 == 0 && pixelID.y % 2 == 0;
#else
          return false;
#endif
        }
        float dt = (f * (t-t1)) / (f-oldF);
        t1 = t;
        t -= dt;
      }
      n = super::normal(sq,pos);
      return true;
#endif
    }

    SUPER_GLYPHS_MAIN
    {
      PerRayData& prd = owl::getPRD<PerRayData>();
      const SuperGeomData& self = owl::getProgramData<SuperGeomData>();
    
      int   primID = optixGetPrimitiveIndex();
      const vec3i index  = self.index[primID];
        
#if USER_GEOM_SUPER_GLYPHS
      vec3f Ng;
      vec3f col(.5f);
#else
      // compute normal:
      const vec3f& v1     = self.vertex[index.x];
      const vec3f& v2     = self.vertex[index.y];
      const vec3f& v3     = self.vertex[index.z];
      vec3f Ng            = normalize(cross(v2 - v1, v3 - v1));

      const float u = optixGetTriangleBarycentrics().x;
      const float v = optixGetTriangleBarycentrics().y;

      vec3f col = (self.color)
        ? ((1.f - u - v) * self.color[index.x]
           + u * self.color[index.y]
           + v * self.color[index.z])
        : vec3f(.5f);
#endif

      float t    = optixGetRayTmax();

#if !USER_GEOM_SUPER_GLYPHS

#if !TESSELLATE_SUPER_GLYPHS
      if (!refine(self,t,Ng))
        optixIgnoreIntersection();
#endif

      // Multiplication by two is an implementation detail (ignore!)
      prd.primID = optixGetInstanceIndex()*2;
      // we currently have all triangles baked into a single mesh:
      prd.meshID = -1;
      prd.color = col;
      prd.t = t;
      prd.Ng = Ng;
#else
      // Guess a first root by intersecting the bounding sphere
      // of the super quadric
      owl::Ray ray(optixGetObjectRayOrigin(),
                   optixGetObjectRayDirection(),
                   optixGetRayTmin(),
                   optixGetRayTmax());

      //printf("%f %f %f  %f %f %f\n",ray.origin.x,ray.origin.y,ray.origin.z,ray.direction.x,ray.direction.y,ray.direction.z);
      bool sph = intersectSphere2(vec3f(0.f),1.f,ray,t,Ng);

      // Refine
      if (sph) {
        if (refine(self,t,Ng) && optixReportIntersection(t, 0)) {
          // Multiplication by two is an implementation detail (ignore!)
          prd.primID = optixGetInstanceIndex()*2;
          // we currently have all triangles baked into a single mesh:
          prd.meshID = -1;
          prd.color = col;
          prd.t = t;
          prd.Ng = Ng;
        }
      }
#endif
    }

  }
}

