#include <float.h>
#include "tubes/device/TubesGeom.h"
#include "tubes/device/PerRayData.h"
#include "tubes/device/RayGenData.h"
#include "tubes/device/Camera.h"

namespace tubes {
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

    // ------------------------------------------------------------------
    // actual arrows stuff
    // ------------------------------------------------------------------

    /*! ray-cylinder intersector : ref code from shadertoy.com/view/4lcSRn */
    inline __device__ bool intersectCylinder(const owl::Ray ray, const Cylinder &cylinder, float &hit_t, vec3f &isec_normal)
    {
      const float ra = cylinder.radius;
      const vec3f  pa = cylinder.p1;
      const vec3f  pb = cylinder.p2;

      const vec3f  ba = pb - pa;
      const vec3f  oc = ray.origin - pa;

      float baba = dot(ba, ba);
      float bard = dot(ba, ray.direction);
      float baoc = dot(ba, oc);

      float k2 = baba - bard * bard;
      float k1 = baba * dot(oc, ray.direction) - baoc * bard;
      float k0 = baba * dot(oc, oc) - baoc * baoc - ra * ra * baba;

      float h = k1 * k1 - k2 * k0;

      if (h < 0.f) return false;

      h = sqrtf(h);
      float t = (-k1 - h) / k2;

      // body
      float y = baoc + t * bard;
      if (y > 0.0 && y < baba) {
        hit_t = t;
        isec_normal = (oc + t * ray.direction - ba * y / baba) / ra;
        return true;
      }
      // caps notice we only need one side 
      t = (((y < 0.0) ? 0.0 : baba) - baoc) / bard;
      if (abs(k1 + k2 * t) < h)
        {
          hit_t = t;
          isec_normal = ba * sign(y) / baba;
          return true;
        }
      return false;
    }

    inline __device__ bool intersectCone(const owl::Ray ray, const Cone &cone, float &hit_t, vec3f &n)
    {
      const vec3f dir(ray.direction);

      vec3f co(ray.origin.x - cone.pos.x,
               ray.origin.z - cone.pos.z,
               cone.height - ray.origin.y + cone.pos.y);
      float A = (cone.radius / cone.height) * (cone.radius / cone.height);

      float a = dir.x*dir.x + dir.z*dir.z - A*dir.y*dir.y;
      float b = 2.f*co.x*dir.x + 2.f*co.y*dir.z + 2.f*A*co.z*dir.y;
      float c = co.x*co.x + co.y*co.y - A*co.z*co.z;

      float det = b*b - 4.f*a*c;
      if (det < 0.f) return false;

      det = sqrtf(det);
      float t1 = (-b - det) / (2.f * a);
      float t2 = (-b + det) / (2.f * a);

      float t = (t1<=t2)?t1:t2;
      float h = ray.origin.y+t*dir.y;

      if (h>cone.pos.y && h<cone.pos.y+cone.height) {
        hit_t = t;
        vec3f p = ray.origin+ray.direction*t;
        float r = sqrtf((p.x-cone.pos.x)*(p.x-cone.pos.x)+(p.z-cone.pos.z)*(p.z-cone.pos.z));
        n = vec3f(p.x-cone.pos.x, r*(cone.radius/cone.height), p.z-cone.pos.z);
        return true;
      } else {
        return false;
      }
    }

    inline __device__ bool intersectArrow(const owl::Ray ray, const Arrow &arrow, float &hit_t, vec3f &n)
    {
      bool result = false;

      ArrowParts parts = disassemble(arrow);

      // Test against cylinder
      float cyl_t = FLT_MAX;
      vec3f cyl_n;
      bool hit_cyl = intersectCylinder(ray, parts.cyl, cyl_t, cyl_n);
      if (hit_cyl && cyl_t<hit_t) {
        hit_t = cyl_t;
        n = cyl_n;
        result = true;
      }

      // Test against cone
      float cone_t = FLT_MAX;
      vec3f cone_n;
      bool hit_cone = intersectCone(ray, parts.cone, cone_t, cone_n);
      if (hit_cone && cone_t<hit_t) {
        hit_t = cone_t;
        n = cone_n;
        result = true;
      }

      return result;
    }

    //Correct the intersection program.
    OPTIX_INTERSECT_PROGRAM(Arrows_intersect)()
    {
      const int instID = optixGetPrimitiveIndex();
      const auto& self
        = owl::getProgramData<ArrowsGeom>();
      //owl::Ray ray(optixGetWorldRayOrigin(),
      //             optixGetWorldRayDirection(),
      //             optixGetRayTmin(),
      //             optixGetRayTmax());
      owl::Ray ray(optixGetObjectRayOrigin(),
          optixGetObjectRayDirection(),
          optixGetRayTmin(),
          optixGetRayTmax());

      const Arrow arrow = self.arrows[instID];
      float t = ray.tmax;
      vec3f normal;

      if (intersectArrow(ray, arrow, t, normal)) {
        if(optixReportIntersection(t,instID)) {
          PerRayData &prd = owl::getPRD<PerRayData>();
          prd.instID = instID;
          prd.t = t;
          prd.gn = normal;
        }
      }
    }

    // Round Cone boundingBox
    OPTIX_BOUNDS_PROGRAM(Arrows_bounds)(const void* geomData,
                                        box3f& primBounds,
                                        const int    instID)
    {
      const ArrowsGeom& self = *(const ArrowsGeom*)geomData;
      const Arrow& arrow = self.arrows[instID];
      primBounds = get_bounds(arrow);
    }

    OPTIX_CLOSEST_HIT_PROGRAM(Arrows_closest_hit)()
    {  }
  }
}
