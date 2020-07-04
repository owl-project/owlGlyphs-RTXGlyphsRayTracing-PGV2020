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
#include "device/GlyphsGeom.h"
// std
#include <vector>
#include <memory>
#include <string>

namespace glyphs {

  using device::Link;
  
  /*! the entire set of glyphs, including all links - everything we
    wnat to render */
  struct Glyphs {
    typedef device::Link Link;
    typedef std::shared_ptr<Glyphs> SP;

    /*! return number of links in this model */
    inline size_t size() const { return links.size(); }
    
    /*! load a file */
    static Glyphs::SP load(const std::string &fileName);

    /*! load several files

      @see load(const std::string&,float) for more info
     */
    static Glyphs::SP load(const std::vector<std::string> &fileNames);

    static affine3f getXform(const Glyphs::SP& glyphs, const Link& l);

    /*! returns world space bounding box of the scene; to be used for
      the viewer to automatically set camera and motion speed */
    box3f getBounds() const;
  
    std::vector<Link> links;
    float             radius { 0.2f };

    SP nextTimestep { nullptr };
  };
}
