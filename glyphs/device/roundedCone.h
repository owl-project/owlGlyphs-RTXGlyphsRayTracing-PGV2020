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

namespace glyphs {
  namespace device {

    inline __device__
    float sign(float& val)
    {
      return val < 0.0f ? -1.0f : 1.0;
    }

    inline __device__
    int32_t make_8bit(const float f)
    {
      return min(255, max(0, int(f * 256.f)));
    }

    inline __device__
    int32_t make_rgba8(const vec4f color)
    {
      return
        (make_8bit(color.x) << 0) +
        (make_8bit(color.y) << 8) +
        (make_8bit(color.z) << 16);
    }

    inline __device__
    vec3f uIntToVec3f(int32_t irgba)
    {
      vec3f rgb;
      rgb.z = uint_as_float(irgba >> 16 & 0xFF) / 255;
      rgb.y = uint_as_float(irgba >> 8 & 0xFF) / 255;
      rgb.x = uint_as_float(irgba & 0xFF) / 255;
      return rgb;
    }

    inline __device__
    int32_t vec3fToUInt(vec3f t)
    {
      t.x = min(0.f, max(t.x, 1.0f));   // clamp to [0.0, 1.0]
      t.y = min(0.f, max(t.y, 1.0f));
      t.z = min(0.f, max(t.z, 1.0f));
      return (int32_t(t.z * 255) << 16) | (int32_t(t.y * 255) << 8) | int32_t(t.x * 255);
    }

    inline __device__ vec3f random_in_unit_sphere(Random& rnd) {
      vec3f p;
      do {
        p = 2.0f * vec3f(rnd(), rnd(), rnd()) - vec3f(1, 1, 1);
      } while (dot(p, p) >= 1.0f);
      return p;
    }

    inline __device__
    bool intersectSphere2(const vec3f   pa,
                          const float   ra,
                          owl::Ray ray,
                          float& hit_t,
                          vec3f& isec_normal)
    {
      float minDist = max(0.f,length(pa-ray.origin)-ra);
      ray.origin = ray.origin + minDist * vec3f(ray.direction);

      const vec3f  oc = ray.origin - pa;
      const float  a = dot((vec3f)ray.direction, (vec3f)ray.direction);
      const float  b = dot(oc, (vec3f)ray.direction);
      const float  c = dot(oc, oc) - ra * ra;
      const float  discriminant = b * b - a * c;

      if (discriminant < 0.f) return false;

      {
        float temp = (-b - sqrtf(discriminant)) / a + minDist;
        if (temp < hit_t && temp > ray.tmin) {
          hit_t = temp;
          isec_normal = ray.origin + hit_t * ray.direction - pa;
          return true;
        }
      }

      {
        float temp = (-b + sqrtf(discriminant)) / a + minDist;
        if (temp < hit_t && temp > ray.tmin) {
          hit_t = temp;
          isec_normal = ray.origin + hit_t * ray.direction - pa;
          return true;
        }
      }
      return false;
    }

    // Sphere intersection test from:
    // Haines, Gunther (2019): Precision Improvements for Ray/Sphere Intersection
    // in: Ray Tracing Gems
    // https://link.springer.com/content/pdf/10.1007%2F978-1-4842-4427-2_7.pdf
    inline __device__
    bool intersectInstanceSphereRTGem(vec3f pa, float ra, owl::Ray ray, float& hit_t, vec3f& isec_normal)
    {
      vec3f d = ray.direction;
      vec3f f = ray.origin; // sphere origin (0,0,0)
      float a = dot(d,d);
      float b = dot(-f,d);
      float discr = ra*ra - dot((f+b/a*d),(f+b/a*d));

      if (discr < 0.f)
        return false;

      float q = b + copysignf(sqrtf(a*discr),b);
      float c = dot(f,f) - ra*ra;

      float t0 = c/q;
      if (t0 < hit_t && t0 > ray.tmin) {
        hit_t = t0;
        isec_normal = ray.origin + hit_t * ray.direction;
        return true;
      }

      float t1 = q/a;
      if (t1 < hit_t && t1 > ray.tmin) {
        hit_t = t1;
        isec_normal = ray.origin + hit_t * ray.direction;
        return true;
      }
      return false;
    }


    /*! ray-cylinder intersector from shadertoy.com/view/4lcSRn */
    /*! author: Inigo Quilez (2016), license is MIT */
    inline __device__ bool intersectCylinder(const vec3f   pa,
                                             const vec3f   pb,
                                             const float   ra,
                                             const owl::Ray ray,
                                             float &hit_t,
                                             vec3f &isec_normal)
    {
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
      if( y>0.0 && y<baba && t > ray.tmin){
        hit_t = t;
        isec_normal = oc+t*ray.direction - ba*y/baba;
        return true;
      }

      // caps
      t = ( ((y<0.0) ? 0.0 : baba) - baoc)/bard;
      if( abs(k1+k2*t)<h && t > ray.tmin)
        {
          hit_t = t;
          isec_normal =  ba*sign(y);
          return true;
        }
      return false;
    }



    /* ray - rounded cone intersection from https://www.shadertoy.com/view/MlKfzm */
    /*! author: Inigo Quilez (2018), license is MIT */
    inline __device__
    bool intersectRoundedCone(
                              const vec3f  pa, const vec3f  pb,
                              const float  ra, const float  rb,
                              const owl::Ray ray,
                              float& hit_t,
                              vec3f& isec_normal)
    {
      const vec3f &ro = ray.origin;
      const vec3f &rd = ray.direction;

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
      if (h >= 0.f) {
        float t = (-sqrtf(h) - k1) / k2;

      float y = m1 - ra * rr + t * m2;
      if (y > 0.0f && y < d2 && t > ray.tmin)
        {
          hit_t = t;
          isec_normal = (d2 * (oa + t * rd) - ba * y);
          return true;
        }
      }

      // Caps. 
      float h1 = m3 * m3 - m5 + ra * ra;
      if (h1 > 0.0f) {
        float t = -m3 - sqrtf(h1);
        if (t > ray.tmin)
        {
          hit_t = t;
          isec_normal = oa + t * rd;
          return true;
        }
      }
      float h2 = m6 * m6 - m7 + rb * rb;
      if( h2>0.0f )
        {
          float t = -m6 - sqrtf( h2 );
          if (t > ray.tmin) {
            hit_t = t;
            isec_normal = oa + t * rd;
            return true;
          }
        }
      return false;
    }
    
  }
}
