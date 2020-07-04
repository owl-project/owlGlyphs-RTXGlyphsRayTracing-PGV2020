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
#include "glyphs/device/disney_bsdf.h"
#include "glyphs/device/TriangleMesh.h"

namespace glyphs {
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
        p = 2.0f*vec3f(rnd(),rnd(),rnd()) - vec3f(1.f, 1.f, 1.f);
      } while (dot(p,p) >= 1.0f);
      return p;
    }
    
    // ------------------------------------------------------------------
    // A simple path tracer; if pathDepth <= 1 falls back to local
    // shading.
    // Enable FAST_SHADING via cmake option to get Lambertian
    // instead of Disney BRDF
    // ------------------------------------------------------------------

    inline __device__
    vec3f missColor(const Ray &ray)
    {
      const vec2i pixelID = owl::getLaunchIndex();
      const float t = pixelID.y / (float)optixGetLaunchDimensions().y;
      const vec3f c = (1.0f - t)*vec3f(1.0f, 1.0f, 1.0f) + t * vec3f(0.5f, 0.7f, 1.0f);
      return c;
    }

    inline __device__
    vec3f pathTrace(const RayGenData &self,
                    owl::Ray &ray,
                    Random &rnd,
                    PerRayData &prd)
    {
      vec3f attenuation = 1.f;
      vec3f ambientLight(.8f);

      const FrameState *fs = &self.frameStateBuffer[0];
      int pathDepth = fs->pathDepth;
      
      if (pathDepth <= 1) {
        prd.primID = -1;
        owl::traceRay(/*accel to trace against*/self.world,
                      /*the ray to trace*/ ray,
                      /*prd*/prd/*,
                              OPTIX_RAY_FLAG_DISABLE_ANYHIT*/);

        if (prd.primID < 0)
          return missColor(ray);
        
        vec3f N = prd.Ng;
        if (dot(N,(vec3f)ray.direction)  > 0.f)
          N = -N;
        N = normalize(N);
        
        vec3f albedo;
        
        // Random colors for glyphs, grey for triangles
        if (prd.meshID == 0) {
          albedo = vec3f(.8f);
        } else {
          unsigned rgba = self.linkBuffer[prd.primID].col; // ignore alpha for now
          albedo = vec3f((rgba & 0xff) / 255.f,
                        ((rgba >> 8) & 0xff) / 255.f,
                        ((rgba >> 16) & 0xff) / 255.f);
        }
        vec3f color = albedo * (.2f+.6f*fabsf(dot(N,(vec3f)ray.direction)));
        return color;
      }

      // could actually swtich material based on meshID ...
      DisneyMaterial material = fs->material;
      /* iterative version of recursion, up to depth 50 */
      for (int depth=0;true;depth++) {
        prd.primID = -1;
        owl::traceRay(/*accel to trace against*/self.world,
                      /*the ray to trace*/ ray,
                      /*prd*/prd/*,
                      OPTIX_RAY_FLAG_DISABLE_ANYHIT*/);
        
        if (prd.primID == -1) {
          // miss...
          if (depth == 0)
            return missColor(ray);

#if FAST_SHADING
          return attenuation * ambientLight;
#else
          float phi = atan2(ray.direction.y, ray.direction.x);
          float theta = acos(ray.direction.z / length(ray.direction));
          const float half_width = 0.1f;

          if (theta > (0.55f - half_width) * M_PIF && theta < (0.55f + half_width) * M_PIF
              && phi > (0.75f - half_width) * M_PIF && phi < (0.75f + half_width) * M_PIF) {
            return attenuation * owl::vec3f(8.f);
          } else {
            return attenuation * owl::vec3f(ambientLight / 2.f);
          }
#endif
        }

        vec3f N = normalize(prd.Ng);
        const vec3f w_o = -ray.direction;
        if (dot(N, w_o) < 0.f) {
          N = -N;
        }
        
        // Random colors for glyphs, grey for triangles
        if (prd.meshID >= 0) { 
          material.base_color = vec3f(.8f);
        } else {
          unsigned rgba = self.linkBuffer[prd.primID].col; // ignore alpha for now
          material.base_color = vec3f((rgba & 0xff) / 255.f,
                                      ((rgba >> 8) & 0xff) / 255.f,
                                      ((rgba >> 16) & 0xff) / 255.f);
        }

        owl::vec3f v_x, v_y;
        ortho_basis(v_x, v_y, N);
        // pdf and dir are set by sampling the BRDF
        float pdf;
        vec3f scattered_direction;
        vec3f albedo = sample_disney_brdf(material, N, w_o, v_x, v_y, rnd,
                                          scattered_direction, pdf);
        
        const vec3f scattered_origin    = ray.origin + prd.t * ray.direction;
        ray = owl::Ray(/* origin   : */ scattered_origin,
                       /* direction: */ scattered_direction,
                       /* tmin     : */ 1e-3f,
                       /* tmax     : */ 1e+8f);

        if (depth >= pathDepth || pdf == 0.f || albedo == owl::vec3f(0.f)) {
          // ambient term:
          return owl::vec3f(0.f);//attenuation * ambientLight;
        }

        attenuation *= albedo * fabs(dot(scattered_direction, N)) / pdf;
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
      prd.rnd = &rnd;

      for (int s = 0; s < fs->samplesPerPixel; s++) {
        vec2f pixelSample = vec2f(pixelID) + vec2f(rnd(),rnd());
        float u = float(pixelID.x + rnd());
        float v = float(pixelID.y + rnd());
        owl::Ray ray = Camera::generateRay(*fs, pixelSample, rnd);
        col += vec4f(pathTrace(self,ray,rnd,prd),1);
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

