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

// owl
#include "owl/common/math/vec.h"
#include "owl/common/math/box.h"
// std
#include <vector>

namespace tubes {
  using namespace owl::common;
  
  struct TrackVisData {
    typedef std::shared_ptr<TrackVisData> SP;
    
    static TrackVisData::SP read(const std::string &fileName);

    /*! scalars are once *per point* */
    struct ScalarField {
      std::string name;
      // one value per control point
      std::vector<float> perPoint;
    };
    /*! properties are once *per track */
    struct PropertyField {
      std::string name;
      std::vector<float> perTrack;
    };
    struct Track {
      /*! id of first point */
      int begin;
      /*! number of points */
      int size;
    };

    // one position per control point
    std::vector<vec3f>         points;
    std::vector<ScalarField>   scalarFields;
    std::vector<PropertyField> propertyFields;
    std::vector<Track>         tracks;

    box3f getBounds() const;
  private:
    void readFile(const std::string &fileName);
    void readTrack(std::ifstream &in, int numScalars, int numProps);
  };

}



