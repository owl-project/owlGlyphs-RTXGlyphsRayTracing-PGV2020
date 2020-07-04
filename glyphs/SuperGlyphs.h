// ======================================================================== //
// Copyright 2018-2020 The Contributors                                     //
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

#include <utility>
#include <vector>
#include "glyphs/OptixGlyphs.h"

namespace glyphs {
  namespace super {
    struct Quadric;
  }

  /*! Super quadric glyphs;
    This is a more involved glyph that uses a Newton/Rhaphson
    solver to compute ray/superquadric intersections.
    The glyph types implements a trick, as it doesn't use the bounding
    box or bounding sphere as an initial root estimate, but rather a
    coarse tessellation. The intersection with that tessellation is
    computed using hardware-accelerated ray/triangle intersections.
  */
  struct SuperGlyphs : public OWLGlyphs
  {
    SuperGlyphs();
    
    void build(Glyphs::SP glyphs,
               Triangles::SP triangles) override;

  private:
    void addUserGeom(std::vector<std::pair<OWLGroup,affine3f>>& groups,
                     const super::Quadric& sq,
                     const affine3f& xfm);

    void addTessellation(std::vector<std::pair<OWLGroup,affine3f>>& groups,
                         box3f& worldBounds,
                         size_t& numTris,
                         const super::Quadric& sq,
                         const affine3f& xfm,
                         float du,
                         float dv);

    std::vector<std::pair<OWLGroup,affine3f>> buildGlyphs(Glyphs::SP glyphs);
  };
  
}



