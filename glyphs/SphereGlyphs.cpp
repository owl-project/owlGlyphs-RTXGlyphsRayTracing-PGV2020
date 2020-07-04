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

#include "glyphs/SphereGlyphs.h"
#include <random>
#include <omp.h>
#include <owl/common/parallel/parallel_for.h>

namespace glyphs {

  using device::GlyphsGeom;
  
  extern "C" const char embedded_SphereGlyphs_programs[];

  SphereGlyphs::SphereGlyphs()
  {
    module = owlModuleCreate(context, embedded_SphereGlyphs_programs);

    OWLVarDecl glyphsVars[] = {
      { "links",  OWL_BUFPTR, OWL_OFFSETOF(GlyphsGeom,links)},
      { "radius", OWL_FLOAT , OWL_OFFSETOF(GlyphsGeom,radius)},
      { "numLinks", OWL_INT , OWL_OFFSETOF(GlyphsGeom,numLinks)},
      { /* sentinel to mark end of list */ }
    };

    glyphsType
      = owlGeomTypeCreate(context,
                          OWL_GEOMETRY_USER,
                          sizeof(GlyphsGeom),
                          glyphsVars, -1);
    owlGeomTypeSetBoundsProg(glyphsType, module,
                             "SphereGlyphs");
    owlGeomTypeSetIntersectProg(glyphsType, 0, module,
                                "SphereGlyphs");
    owlGeomTypeSetClosestHit(glyphsType, 0, module,
                             "SphereGlyphs");

    buildModules();
  }

  /*! this takes a set of spheres, and builds one instance per sphere -
      the result is stored in the groups/transforms vectors */
  std::vector<std::pair<OWLGroup,affine3f>>
  SphereGlyphs::buildGlyphs(Glyphs::SP glyphs)
  {
    const int numLinks = glyphs->links.size();
    std::vector<std::pair<OWLGroup,affine3f>> result(glyphs->links.size());
    
    /* start with one group and transform per link */
    result.resize(numLinks);
  
    linkBuffer
      = owlDeviceBufferCreate(context,
                              OWL_USER_TYPE(glyphs->links[0]),
                              glyphs->links.size(),
                              glyphs->links.data());

    OWLGeom singleGlyphGeom
      = owlGeomCreate(context, glyphsType);
    owlGeomSetPrimCount(singleGlyphGeom, 1);
    owlGeomSetBuffer(singleGlyphGeom,"links",linkBuffer);
    owlGeomSet1f(singleGlyphGeom,"radius",glyphs->radius);
    owlGeomSet1i(singleGlyphGeom,"numLinks",(int)glyphs->links.size());
    
    OWLGroup singleGlyphGroup
      = owlUserGeomGroupCreate(context, 1, &singleGlyphGeom);
    
    // compile progs here because we need the bounds prog in accelbuild:
    owlBuildPrograms(context);
    owlGroupBuildAccel(singleGlyphGroup);
    
    owl::parallel_for(numLinks,[&](int linkID) {
        result[linkID].first = singleGlyphGroup;
        affine3f &xfm = result[linkID].second;
        const Link A = glyphs->links[linkID];
        xfm = Glyphs::getXform(glyphs,A);
      });
    return result;
  }
  
  void SphereGlyphs::build(Glyphs::SP glyphs, Triangles::SP triangles)
  {
    std::vector<std::pair<OWLGroup,affine3f>> rootGroups
      = buildGlyphs(glyphs);

    if (triangles)
      triangleGroup = buildTriangles(triangles);

    if (triangleGroup)
      rootGroups.push_back({triangleGroup,affine3f()});

    world
      = owlInstanceGroupCreate(context, rootGroups.size());
    for (int i=0; i<rootGroups.size(); i++) {
      owlInstanceGroupSetChild(world, i, rootGroups[i].first);
      owlInstanceGroupSetTransform(world, i, &(const owl4x3f&)rootGroups[i].second);
    }

    owlGroupBuildAccel(this->world);
  }
}
