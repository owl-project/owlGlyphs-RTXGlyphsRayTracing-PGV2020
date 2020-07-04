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

#include "device/common.h"
#include "device/TubesGeom.h"
// std
#include <vector>
#include <memory>
// owl
#include <owl/owl.h>
#include "owl/common/math/box.h"
#include "owl/common/math/AffineSpace.h"

namespace tubes {

  /*! the entire set of tubes, including all links - everything we
    wnat to render */
  struct Tubes {
    typedef device::Arrow Arrow;
    typedef std::shared_ptr<Tubes> SP;
    
    /*! load a file - supports various file formats of procedural
      methods of generating, see below:

      filename= procedural://num:a,b:c,d: returns
      Tubes::prodedural(num,{a,b},{c,d}
    */
    static Tubes::SP load(const std::string &fileName,
                          float radius);

    /*! procedurally generate some linked tubes,so we have "something"
      to test.  numTubes --- maximum supported size on 1080 card:
      29000000.  lengthInterval radiusInterval
    */
    static Tubes::SP procedural(int numTubes = 1000,
                                vec2f lengthInterval = vec2f(0.5f,3.5f),
                                vec2f radiusInterval = vec2f(0.2f, 0.02f));
    
    /*! returns world space bounding box of the scene; to be used for
      the viewer to automatically set camera and motion speed */
    box3f getBounds() const;

    // Arrow data, but w/o transforms
    std::vector<Arrow> arrows;

    // Arrow instance transforms
    std::vector<owl::affine3f> transforms;
  };
}
