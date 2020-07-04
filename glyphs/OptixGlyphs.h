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

#pragma once

#include "Triangles.h"
#include "glyphs/device/FrameState.h"
#include "Glyphs.h"
#include "owl/owl.h"

namespace glyphs {

  /*! the entire set of glyphs, including all links - everything we
    wnat to render */
  struct OWLGlyphs {
    OWLGlyphs();
    /*! build owl-model (OWLGeoms, OWLGroup, etc) that we can ray
        trace against */
    virtual void build(/*! the glyphs to build over, using either
                         one group of user prims, or one inst
                         per glyph */
                       Glyphs::SP glyphs,
                       /*! the triangle mesh(es) */
                       Triangles::SP triangles) = 0;
    
    /*! this should only ever once get called... it'll call the
        virtual buildModel(), and then set up the SBT, raygen, etc */
    void setModel(Glyphs::SP glyphs, Triangles::SP triModel);

    void resizeFrameBuffer(void *fbPointer, const vec2i &newSize);
    void updateFrameState(device::FrameState &fs);

    uint32_t *mapColorBuffer();
    void unmapColorBuffer();

    void render();

    // helper function that turns a triangle model into a owl geometry
    OWLGroup buildTriangles(Triangles::SP triModel);

    
    /*! size of current frame buffer */
    vec2i fbSize { -1,-1 };
    OWLContext context = 0;
    OWLModule module = 0;
    OWLBuffer frameStateBuffer = 0;
    OWLBuffer colorBuffer = 0;
    OWLBuffer accumBuffer = 0;
    OWLGroup  world = 0;
    OWLRayGen rayGen = 0;
    OWLBuffer linkBuffer = 0;
    OWLGeomType glyphsType = 0;
    OWLGeomType trianglesGeomType = 0;
    OWLGroup triangleGroup = 0;

  protected:
    /*! *must* be called by derived classes when all modules are
        set up appropriately; will build optix programs and build
        the owl pipeline */
    void buildModules();

  };
  
}
