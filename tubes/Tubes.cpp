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

#include "Tubes.h"
#include <fstream>
#include <random>

namespace tubes {
  
#if _WIN32
  inline double drand48() { return double(rand()) / RAND_MAX; }
#endif

  std::string getExt(const std::string &fileName)
  {
    int pos = fileName.rfind('.');
    if (pos == fileName.npos)
      return "";
    return fileName.substr(pos);
  }

  Tubes::SP Tubes::procedural(int numArrows,
                              vec2f lengthInterval,
                              vec2f radiusInterval)
  {
    Tubes::SP model = std::make_shared<Tubes>();

    // Lengths distribution
    std::uniform_real_distribution<float> length_dist(1.f,5.f);
    // Thickness distribution
    std::uniform_real_distribution<float> thick_dist(.3f,.5f);
    // Pos x|y|z distribution
    std::uniform_real_distribution<float> pos_dist(-25.f,25.f);
    // Velocity vector distribution
    std::uniform_real_distribution<float> vel_dist(0.f,1.f);
    
    std::default_random_engine re;

    model->arrows.resize(numArrows);
    model->transforms.resize(numArrows);

    for (int i=0; i<numArrows; ++i) {
      // length and thickness
      model->arrows[i] = {length_dist(re), thick_dist(re)};
      
      // Particle data
      vec3f pos(pos_dist(re), pos_dist(re), pos_dist(re));
      model->transforms[i] = owl::affine3f::translate(pos);std::cout << model->transforms[i] << '\n';
    }

    return model;
  }

  /*! returns world space bounding box of the scene; to be used for
    the viewer to automatically set camera and motion speed */
  box3f Tubes::getBounds() const
  {
    box3f bounds;
    for (size_t i=0; i<arrows.size(); ++i) {
      const Arrow &arrow = arrows[i];
      box3f bbox = get_bounds(arrow);
      vec3f verts[8] = {
        { bbox.lower.x, bbox.lower.y, bbox.lower.z },
        { bbox.upper.x, bbox.lower.y, bbox.lower.z },
        { bbox.upper.x, bbox.upper.y, bbox.lower.z },
        { bbox.lower.x, bbox.upper.y, bbox.lower.z },
        { bbox.upper.x, bbox.upper.y, bbox.upper.z },
        { bbox.lower.x, bbox.upper.y, bbox.upper.z },
        { bbox.lower.x, bbox.lower.y, bbox.upper.z },
        { bbox.upper.x, bbox.lower.y, bbox.upper.z }
        };

      for (int j=0; j<8; ++j) {
        bounds.extend(owl::xfmPoint(transforms[i],verts[j]));
      }
    }
    return box3f(bounds.lower, bounds.upper);
  }
}
