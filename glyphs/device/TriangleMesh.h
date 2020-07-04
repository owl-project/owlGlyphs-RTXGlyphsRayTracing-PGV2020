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

namespace glyphs {
  namespace device {

    OPTIX_CLOSEST_HIT_PROGRAM(TriangleMesh)()
    {
      PerRayData& prd = owl::getPRD<PerRayData>();
      const TrianglesGeomData& self = owl::getProgramData<TrianglesGeomData>();
    
      const int   primID = optixGetPrimitiveIndex();
      const vec3i index  = self.index[primID];
        
      // compute normal:
      const vec3f& A     = self.vertex[index.x];
      const vec3f& B     = self.vertex[index.y];
      const vec3f& C     = self.vertex[index.z];
      const vec3f Ng     = normalize(cross(B - A, C - A));

      // const vec3f rayDir = optixGetWorldRayDirection();
      // prd = (.2f + .8f * fabs(dot(rayDir, Ng))) * self.color;

      const float u = optixGetTriangleBarycentrics().x;
      const float v = optixGetTriangleBarycentrics().y;

      vec3f col = (self.color)
        ? ((1.f - u - v) * self.color[index.x]
           + u * self.color[index.y]
           + v * self.color[index.z])
        : vec3f(0.5);
      
      prd.primID = primID;
      // we currently have all triangles baked into a single mesh:
      prd.meshID = 0;
      prd.color = col;
      prd.t = optixGetRayTmax();
      prd.Ng = Ng;
    }

  }
}
