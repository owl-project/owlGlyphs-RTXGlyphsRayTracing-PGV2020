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

#include "glyphs/OptixGlyphs.h"

namespace glyphs {

  /*! Motion blur glyph type;
    This is a sphere glyph, but the geometry of the glyph is
    non-affine, and instead it is spread out and blurred over the
    extent defined by its instance bounds according to the
    link velocity and link acceleration
  */
  struct MotionSpheres : public OWLGlyphs
  {
    MotionSpheres();
    
    void build(Glyphs::SP glyphs,
               Triangles::SP triangles) override;

  private:
    OWLGroup buildGlyphs(Glyphs::SP glyphs);
  };
  
}



