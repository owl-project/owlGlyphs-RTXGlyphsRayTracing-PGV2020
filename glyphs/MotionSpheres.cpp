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

#include "glyphs/MotionSpheres.h"

namespace glyphs {
  
  using device::GlyphsGeom;

  extern "C" const char embedded_MotionSpheres_programs[];

  MotionSpheres::MotionSpheres()
  {
    module = owlModuleCreate(context, embedded_MotionSpheres_programs);

    OWLVarDecl glyphsVars[] = {
      { "links",  OWL_BUFPTR, OWL_OFFSETOF(GlyphsGeom,links)},
      { "radius", OWL_FLOAT , OWL_OFFSETOF(GlyphsGeom,radius)},
      { /* sentinel to mark end of list */ }
    };
    
    glyphsType
      = owlGeomTypeCreate(context,
                          OWL_GEOMETRY_USER,
                          sizeof(GlyphsGeom),
                          glyphsVars, -1);
    owlGeomTypeSetBoundsProg(glyphsType, module,
    	"MotionSpheres");
    owlGeomTypeSetIntersectProg(glyphsType, 0, module,
    	"MotionSpheres");

    owlGeomTypeSetClosestHit(glyphsType, 0, module,
    	"MotionSpheres");

    buildModules();
  }

  OWLGroup MotionSpheres::buildGlyphs(Glyphs::SP glyphs)
  {
    linkBuffer
      = owlDeviceBufferCreate(context,
                              OWL_USER_TYPE(glyphs->links[0]),
                              glyphs->links.size(),
                              glyphs->links.data());

    OWLGeom geom = owlGeomCreate(context, glyphsType);
    owlGeomSetPrimCount(geom, glyphs->links.size());

    owlGeomSetBuffer(geom, "links", linkBuffer);
    owlGeomSet1f(geom, "radius", glyphs->radius);
    owlBuildPrograms(context);
    
    OWLGroup glyphsGroup = owlUserGeomGroupCreate(context, 1, &geom);
    owlGroupBuildAccel(glyphsGroup);
    return glyphsGroup;
  }
  
  void MotionSpheres::build(Glyphs::SP glyphs,
                            Triangles::SP triangles)
  {
    assert(glyphs != nullptr);
    
    std::vector<OWLGroup> rootGroups;

    OWLGroup glyphsGroup = buildGlyphs(glyphs);
    if (glyphsGroup)
      rootGroups.push_back(glyphsGroup);

    if (triangles)
      triangleGroup = buildTriangles(triangles);

    if (triangleGroup)
      rootGroups.push_back(triangleGroup);

    this->world = owlInstanceGroupCreate(context, rootGroups.size());
    for (int i=0;i<rootGroups.size();i++)
      owlInstanceGroupSetChild(this->world, i, rootGroups[i]);
		
    owlGroupBuildAccel(this->world);
  }

}

