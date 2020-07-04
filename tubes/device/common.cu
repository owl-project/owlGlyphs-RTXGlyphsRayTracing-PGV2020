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

#include <optix_device.h> // Only for test, can be remove later. 

namespace tubes {
  namespace device {
    


    inline __device__
    int32_t make_8bit(const float f)
    {
      return min(255,max(0,int(f*256.f)));
    }
    
    inline __device__
    int32_t make_rgba8(const vec4f color)
    {
      return
        (make_8bit(color.x) << 0) +
        (make_8bit(color.y) << 8) +
        (make_8bit(color.z) << 16);
    }

    inline __device__ vec3f random_in_unit_sphere(Random &rnd) {
      vec3f p;
      do {
        p = 2.0f*vec3f(rnd(),rnd(),rnd()) - vec3f(1, 1, 1);
      } while (dot(p,p) >= 1.0f);
      return p;
    }
    
    // ------------------------------------------------------------------
    // actual tubes stuff
    // ------------------------------------------------------------------

    /*
      OPTIX_INTERSECT_PROGRAM(tubes_intersect)()
      {
      const int primID = optixGetPrimitiveIndex();
      const auto &self
      = owl::getProgramData<TubesGeom>();
      
      owl::Ray ray(optixGetWorldRayOrigin(),
      optixGetWorldRayDirection(),
      optixGetRayTmin(),
      optixGetRayTmax());

      const Link link = self.links[primID];
      float tmp_hit_t = ray.tmax;
      if (intersectSphere(link,self.radius,ray,tmp_hit_t)) {
      optixReportIntersection(tmp_hit_t,primID);
      }
      }
    */
    


    inline __device__
    vec3f missColor(const Ray &ray)
    {
      const vec2i pixelID = owl::getLaunchIndex();
      const float t = pixelID.y / (float)optixGetLaunchDimensions().y;
      const vec3f c = (1.0f - t)*vec3f(1.0f, 1.0f, 1.0f) + t * vec3f(0.5f, 0.7f, 1.0f);
      return c;
    }

    
    
    inline __device__ vec3f traceRay(const RayGenData &self,
                                     owl::Ray &ray,
                                     Random &rnd,
                                     PerRayData &prd)
    {
      vec3f attenuation = 1.f;
      vec3f ambientLight(.8f);

      const FrameState *fs = &self.frameStateBuffer[0];
      int pathDepth = fs->shadeMode;
      /* code for tubes */
      if (pathDepth <= 1) {
        prd.instID = -1;
        // prd.numIsecs = 0;
        owl::traceRay(/*accel to trace against*/self.world,
                      /*the ray to trace*/ ray,
                      /*prd*/prd);

        // if (owl::getLaunchIndex()*2 == owl::getLaunchDims())
        //   printf("isecs %i\n",prd.numIsecs);
        if (prd.instID < 0)
          return missColor(ray);

        const Arrow arrow = self.arrowBuffer[prd.instID];

        vec3f N = prd.gn;

        if (dot(N,(vec3f)ray.direction)  > 0.f)
          N = -N;
        N = normalize(N);

        // hardcoded albedo for now:
        const vec3f albedo// = vec3f(0.628, 0.85, 0.511);
        = randomColor(1+prd.instID);
        vec3f color = albedo * (.2f+.6f*fabsf(dot(N,(vec3f)ray.direction)));
        return color;
      }


      
      /* iterative version of recursion, up to depth 50 */
      for (int depth=0;true;depth++) {
        prd.instID = -1;
        owl::trace(/*accel to trace against*/self.world,
                   /*the ray to trace*/ ray,
                   /*numRayTypes*/1,
                   /*prd*/prd,
                   0);
        // rtTrace(world, ray, prd, RT_VISIBILITY_ALL, RT_RAY_FLAG_DISABLE_ANYHIT);
        
        if (prd.instID == -1) {
          // miss...
          if (depth == 0)
            return missColor(ray);
          return attenuation * ambientLight;
        }
        
        const Arrow arrow = self.arrowBuffer[prd.instID];
        vec3f N = prd.gn;
        // Normal of the cylinder. No more on-the-fly calculation needed.        
        // printf("normal %f %f %f\n",N.x,N.y,N.z);

        //if (dot(N,(vec3f)ray.direction)  > 0.f)
        //  N = -N;
        
        N = normalize(N);

        // hardcoded albedo for now:
        const vec3f albedo = vec3f(.6f);
        // = randomColor(1+prd.instID);//link.matID);
        // hard-coded for the 'no path tracing' case:
        // if (pathDepth <= 1)
        //   return albedo * (.2f+.6f*fabsf(dot(N,(vec3f)ray.direction)));
          
        attenuation *= albedo;
        //attenuation *=  (.2f + .6f * fabsf(dot(N, (vec3f)ray.direction)));

        if (depth >= pathDepth) {
          // ambient term:
          return attenuation * ambientLight;
        }
        
        const vec3f scattered_origin    = ray.origin + prd.t * ray.direction;
        const vec3f scattered_direction = N + random_in_unit_sphere(rnd);
        ray = owl::Ray(/* origin   : */ scattered_origin,
                       /* direction: */ normalize(scattered_direction),
                       /* tmin     : */ 1e-3f,
                       /* tmax     : */ 1e+8f);
      }
    }

    OPTIX_MISS_PROGRAM(miss_program)()
    {
      /*! nothing to do - we initialize prd before trace */
    }

    /*! the actual ray generation program - note this has no formal
      function parameters, but gets its paramters throught the 'pixelID'
      and 'pixelBuffer' variables/buffers declared above */
    OPTIX_RAYGEN_PROGRAM(raygen_program)()
    {
      const RayGenData &self = owl::getProgramData<RayGenData>();
      const vec2i pixelID = owl::getLaunchIndex();
      const vec2i launchDim = owl::getLaunchDims();
  
      if (pixelID.x >= self.fbSize.x) return;
      if (pixelID.y >= self.fbSize.y) return;
      const int pixelIdx = pixelID.x+self.fbSize.x*pixelID.y;

      // for multi-gpu: only render every deviceCount'th column of 32 pixels:
      if (((pixelID.x/32) % self.deviceCount) != self.deviceIndex)
        return;
      
      uint64_t clock_begin = clock64();
      const FrameState *fs = &self.frameStateBuffer[0];
      int pixel_index = pixelID.y * launchDim.x + pixelID.x;
      vec4f col(0.f);
      Random rnd(pixel_index,
                 fs->accumID
                 );

      PerRayData prd;

      for (int s = 0; s < fs->samplesPerPixel; s++) {
        vec2f pixelSample = vec2f(pixelID) + vec2f(rnd(),rnd());
        float u = float(pixelID.x + rnd());
        float v = float(pixelID.y + rnd());
        owl::Ray ray = Camera::generateRay(*fs, pixelSample, rnd);
        col += vec4f(traceRay(self,ray,rnd,prd),1);
      }
      col = col / float(fs->samplesPerPixel);

      uint64_t clock_end = clock64();
      if (fs->heatMapEnabled) {
        float t = (clock_end-clock_begin)*fs->heatMapScale;
        if (t >= 256.f*256.f*256.f)
          col = vec4f(1,0,0,1);
        else {
          uint64_t ti = uint64_t(t);
          col.x = ((ti >> 16) & 255)/255.f;
          col.y = ((ti >> 8) & 255)/255.f;
          col.z = ((ti >> 0) & 255)/255.f;
        }
      }
    
      if (fs->accumID > 0)
        col = col + (vec4f)self.accumBufferPtr[pixelIdx];
      self.accumBufferPtr[pixelIdx] = col;

      uint32_t rgba = make_rgba8(col / (fs->accumID+1.f));
      self.colorBufferPtr[pixelIdx] = rgba;
    }
    
  }
}

