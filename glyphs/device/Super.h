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

#include "glyphs/device/common.h"

namespace glyphs {

  // https://en.wikipedia.org/wiki/Superquadrics
  namespace super {
    struct Quadric {
      float r, s, t;
      float A, B, C; // scaling
    };

    __both__
    inline float sgn(float x)
    {
        return x<0.f?-1.f:x>0.f?+1.f:0.f;
    }

    __both__
    inline float f(float w, float m)
    {
        float sinw = sinf(w);
        return sgn(sinw) * powf(fabsf(sinw), m);
    }

    __both__
    inline float g(float w, float m)
    {
        float cosw = cosf(w);
        return sgn(cosw) * powf(fabsf(cosw), m);
    }

    /*! evaluate superquadric in uv parameter space
        where -PI <= u <= PI and -PI/2 <= v <= P */
    __both__
    inline vec3f eval(const Quadric& q, float u, float v)
    {
        return {
            q.A * g(v, 2.f/q.r) * g(u, 2.f/q.r),
            q.B * g(v, 2.f/q.s) * f(u, 2.f/q.s),
            q.C * f(v, 2.f/q.t)
            };
    }

    /*! evaluate distance to superquadric */
    __both__
    inline float f(const Quadric& q, const vec3f& pos)
    {
        return powf(fabsf(pos.x/q.A),q.r)
             + powf(fabsf(pos.y/q.B),q.s)
             + powf(fabsf(pos.z/q.C),q.t)
             - 1.f;
    }

    /*! Partial derivatives */
    __both__
    inline vec3f df(const Quadric& q, const vec3f& pos)
    {
        return {
            pos.x > 0.f ? powf(pos.x/q.A,q.r-1.f) : -powf(-pos.x/q.A,q.r-1.f),
            pos.y > 0.f ? powf(pos.y/q.B,q.r-1.f) : -powf(-pos.y/q.B,q.r-1.f),
            pos.z > 0.f ? powf(pos.z/q.C,q.r-1.f) : -powf(-pos.z/q.C,q.r-1.f)
            };
    }

    /*! Normal (partials w/o scale) */
    __both__
    inline vec3f normal(const Quadric& q, const vec3f& pos)
    {
        vec3f n{
            pos.x > 0.f ? powf(pos.x,q.r-1.f) : -powf(-pos.x,q.r-1.f),
            pos.y > 0.f ? powf(pos.y,q.r-1.f) : -powf(-pos.y,q.r-1.f),
            pos.z > 0.f ? powf(pos.z,q.r-1.f) : -powf(-pos.z,q.r-1.f)
            };
        return normalize(n);
    }

  } // ::super
} // ::glyps
