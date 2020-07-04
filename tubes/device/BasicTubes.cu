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

#include "tubes/device/TubesGeom.h"
#include "tubes/device/PerRayData.h"
#include "tubes/device/RayGenData.h"
#include "tubes/device/Camera.h"

namespace tubes {
  namespace device {
    
    inline __device__
    float sign(float & val)
    {
      return val < 0.0f ? -1.0f : 1.0;
    }


    inline __device__
    bool intersectSphere2(const vec3f   pa,
                          const float   ra,
                          const owl::Ray ray,
                          float& hit_t,
                          vec3f& isec_normal)
    {
// #if STATS_ON
//       STATS_COUNT_LINK_TEST();
// #endif
      const vec3f  oc = ray.origin - pa;
      const float  a = dot((vec3f)ray.direction, (vec3f)ray.direction);
      const float  b = dot(oc, (vec3f)ray.direction);
      const float  c = dot(oc, oc) - ra * ra;
      const float  discriminant = b * b - a * c;

      if (discriminant < 0.f) return false;

      {
        float temp = (-b - sqrtf(discriminant)) / a;
        if (temp < hit_t && temp > ray.tmin) {
          hit_t = temp;
          isec_normal = ray.origin + hit_t * ray.direction - pa;
          return true;
        }
      }

      {
        float temp = (-b + sqrtf(discriminant)) / a;
        if (temp < hit_t && temp > ray.tmin) {
          hit_t = temp;
          isec_normal = ray.origin + hit_t * ray.direction - pa;
          return true;
        }
      }
      return false;
    }


    //Correct the intersection program.
    /*! ray-cylinder intersector : ref code from shadertoy.com/view/4lcSRn */
    inline __device__ bool intersectCylinder(const vec3f   pa,
                                             const vec3f   pb,
                                             const float   ra,
                                             const owl::Ray ray,
                                             float &hit_t,
                                             vec3f &isec_normal)
    {
#if STATS_ON
      STATS_COUNT_LINK_TEST();
#endif
      const vec3f  ba = pb - pa;  
      const vec3f  oc = ray.origin - pa;

      float baba = dot(ba,ba);
      float bard = dot(ba,ray.direction);
      float baoc = dot(ba,oc);

      float k2 = baba                       - bard*bard;
      float k1 = baba*dot(oc,ray.direction) - baoc*bard;
      float k0 = baba*dot(oc,oc)            - baoc*baoc - ra*ra*baba;
    
      float h = k1*k1 - k2*k0;
   
      if (h < 0.f) return false;
      
      h = sqrtf(h);
      float t = (-k1-h)/k2;
      
      // body
      float y = baoc + t*bard;
      if( y>0.0 && y<baba ){
        hit_t = t;
        isec_normal = (oc+t*ray.direction - ba*y/baba)/ra;
        return true;
      }

      // caps
      t = ( ((y<0.0) ? 0.0 : baba) - baoc)/bard;
      if( abs(k1+k2*t)<h )
        {
          hit_t = t;
          isec_normal =  ba*sign(y)/baba;
          return true;
        }
      return false;
    }



    /* ray - rounded cone intersection. */
    inline __device__
        bool intersectRoundedCone(
            const vec3f  pa, const vec3f  pb,
            const float  ra, const float  rb,
            const owl::Ray ray,
            float& hit_t,
            vec3f& isec_normal)
    {
        const vec3f& ro = ray.origin;
        const vec3f& rd = ray.direction;

        vec3f  ba = pb - pa;
        vec3f  oa = ro - pa;
        vec3f  ob = ro - pb;
        float  rr = ra - rb;
        float  m0 = dot(ba, ba);
        float  m1 = dot(ba, oa);
        float  m2 = dot(ba, rd);
        float  m3 = dot(rd, oa);
        float  m5 = dot(oa, oa);
        float  m6 = dot(ob, rd);
        float  m7 = dot(ob, ob);

        float d2 = m0 - rr * rr;

        float k2 = d2 - m2 * m2;
        float k1 = d2 * m3 - m1 * m2 + m2 * rr * ra;
        float k0 = d2 * m5 - m1 * m1 + m1 * rr * ra * 2.0 - m0 * ra * ra;

        float h = k1 * k1 - k0 * k2;
        if (h < 0.0) return false;
        float t = (-sqrtf(h) - k1) / k2;

        float y = m1 - ra * rr + t * m2;
        if (y > 0.0 && y < d2)
        {
            hit_t = t;
            isec_normal = normalize(d2 * (oa + t * rd) - ba * y);
            return true;
        }

        // Caps. 
        float h1 = m3 * m3 - m5 + ra * ra;
        if (h1 > 0.0)
        {
            t = -m3 - sqrtf(h1);
            hit_t = t;
            isec_normal = normalize((oa + t * rd) / ra);
            return true;
        }
        return false;
    }
    OPTIX_INTERSECT_PROGRAM(basicTubes_intersect)()
    {
        const int primID = optixGetPrimitiveIndex();
        const auto& self
            = owl::getProgramData<TubesGeom>();

        owl::Ray ray(optixGetWorldRayOrigin(),
            optixGetWorldRayDirection(),
            optixGetRayTmin(),
            optixGetRayTmax());
        const Link link = self.links[primID];
        if (link.prev < 0) return;

        float tmp_hit_t = ray.tmax;

        vec3f pb, pa; float ra, rb;
        pa = link.pos;
        ra = link.rad;
        if (link.prev >= 0) {
            rb = self.links[link.prev].rad;
            pb = self.links[link.prev].pos;
            vec3f normal;

            if (intersectRoundedCone(pa, pb, ra,rb, ray, tmp_hit_t, normal))
            {
                if (optixReportIntersection(tmp_hit_t, primID)) {
                    PerRayData& prd = owl::getPRD<PerRayData>();
                    prd.linkID = primID;
                    prd.t = tmp_hit_t;
                    prd.isec_normal = normal;
                }
            }
        }
    }

    OPTIX_INTERSECT_PROGRAM(tubes_intersect)()
    {
      const int primID =  optixGetPrimitiveIndex();
      const auto &self
        = owl::getProgramData<TubesGeom>();
        
      owl::Ray ray(optixGetWorldRayOrigin(),
                   optixGetWorldRayDirection(),
                   optixGetRayTmin(),
                   optixGetRayTmax());
      const Link link = self.links[primID];
      if(link.prev<0) return;

      float tmp_hit_t = ray.tmax;

      vec3f pb,pa; //float ra,rb;        
      pa = link.pos;               
      // ra = link.rad;

      PerRayData& prd = owl::getPRD<PerRayData>();
      vec3f normal;
      
      if (intersectSphere2(pa,self.radius,ray,
                           tmp_hit_t, normal)) {
        if (optixReportIntersection(tmp_hit_t, 1)) {
          prd.linkID = primID;
          prd.t = tmp_hit_t;
          prd.isec_normal = normal;
        }
      } else
        tmp_hit_t = ray.tmax;
      
      if (link.prev >= 0) {
        // rb = self.links[link.prev].rad;
        pb = self.links[link.prev].pos;        

        // prd.numIsecs++;
        if (intersectCylinder(pa,pb,self.radius,ray,tmp_hit_t,normal))       
          {
            if(optixReportIntersection(tmp_hit_t,primID)) {
              prd.linkID = primID;
              prd.t = tmp_hit_t;
              prd.isec_normal = normal;
            }
          }
      }
    }
    
    // Original Sphere boundingbox
    /*OPTIX_BOUNDS_PROGRAM(tubes_bounds)(const void  *geomData,
      box3f       &primBounds,
      const int    primID)
      {
      const TubesGeom &self = *(const TubesGeom*)geomData;
      const Link &link = self.links[primID];
      primBounds = box3f()
      .including(link.pos-self.radius)
      .including(link.pos+self.radius);
      }*/

      // Round Cone boundingBox
    OPTIX_BOUNDS_PROGRAM(basicTubes_bounds)(const void* geomData,
        box3f& primBounds,
        const int    primID)
    {
        const TubesGeom& self = *(const TubesGeom*)geomData;
        const Link& link = self.links[primID];

        vec3f pa = link.pos;
        float ra = link.rad;

        float rb = link.prev < 0 ? ra : self.links[link.prev].rad;
        vec3f pb = link.prev < 0 ? pa : self.links[link.prev].pos;

        vec3f a = pb - pa;
        vec3f ee = vec3f(1.0) - a * a / dot(a, a);
        vec3f e = ra * vec3f(sqrtf(ee.x), sqrtf(ee.y), sqrtf(ee.z));

        primBounds = box3f()
            .including(min(min(pa - e * ra, pb - e * rb), min(pa - ra, pb - rb)))  // be carefull with this.
            .including(max(max(pa + e * ra, pb + e * rb), max(pa + ra, pb + rb))); //
    }

    // Cylinder boundingBox    
    OPTIX_BOUNDS_PROGRAM(tubes_bounds)(const void  *geomData,
                                       box3f       &primBounds,
                                       const int    primID)
    {
      const TubesGeom &self = *(const TubesGeom*)geomData;
      const Link &link = self.links[primID];

      //vec3f pa = link.pos;                      //Not sure about the order of the links.
      //vec3f pb = self.links[link.prev].pos;
      //float ra = self.radius ;
      
      vec3f pa = link.pos;
      // float ra = link.rad;
      primBounds
        = box3f()
        .including(pa-self.radius)
        .including(pa+self.radius);
      // primBounds
      //   = box3f()
      //   .including(pa-ra)
      //   .including(pa+ra);

      if (link.prev >= 0) {
        // add space that connects to previous point:
        // float rb = self.links[link.prev].rad;
        vec3f pb = self.links[link.prev].pos;
        primBounds
          = primBounds
          .including(pb-self.radius)
          .including(pb+self.radius);
        // primBounds
        //   = primBounds
        //   .including(pb-rb)
        //   .including(pb+rb);
      }
    }

    OPTIX_CLOSEST_HIT_PROGRAM(tubes_closest_hit)()
    {
      // const float x = __uint_as_float(optixGetAttribute_0());
      // const float y = __uint_as_float(optixGetAttribute_1());
      // const float z = __uint_as_float(optixGetAttribute_2());
      
      // const int primID = optixGetPrimitiveIndex();
      // PerRayData &prd  = owl::getPRD<PerRayData>();
                        
      // prd.linkID       = primID;
      // prd.t            = optixGetRayTmax();
      // prd.isec_normal = vec3f(x,y,z);
    }    
  }
}

