// ======================================================================== //
// Copyright 2018-2019 Ingo Wald                                            //
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

#include "tubes/device/FrameState.h"
#include "Tubes.h"
#include "owl/owl.h"

namespace tubes {

  /*! the entire set of tubes, including all links - everything we
    wnat to render */
  struct OWLTubes {
    OWLTubes();
    /*! build owl-model (OWLGeoms, OWLGroup, etc) that we can ray
        trace against */
    virtual void buildModel(Tubes::SP tubes) = 0;

    /*! this should only ever once get called... it'll call the
        virtual buildModel(), and then set up the SBT, raygen, etc */
    void setModel(Tubes::SP tubes);

    void resizeFrameBuffer(const vec2i &newSize,
                           void *colorBufferPointer);
    void updateFrameState(device::FrameState &fs);

    uint32_t *mapColorBuffer();
    void unmapColorBuffer();

    void render();

    /*! size of current frame buffer */
    vec2i fbSize { -1,-1 };
    OWLContext context = 0;
    OWLModule module = 0;
    OWLBuffer frameStateBuffer = 0;
    OWLBuffer colorBuffer = 0;
    OWLBuffer accumBuffer = 0;
    OWLGroup  world = 0;
    OWLRayGen rayGen = 0;
    OWLBuffer arrowBuffer = 0;
  };
  
}
