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

#include "tubes/InstanceTubes.h"
#include <random>
#include <omp.h>
#include <owl/common/parallel/parallel_for.h>

namespace tubes {

  using device::ArrowsGeom;
	

  extern "C" const char embedded_InstanceTubes_programs[];

  InstanceTubes::InstanceTubes()
  {
    module = owlModuleCreate(context, embedded_InstanceTubes_programs);
  }

  inline void owlGeomSet3f(OWLGeom rayGen, const char* varName, vec3f v)
  {
    OWLVariable var = owlGeomGetVariable(rayGen, varName);
    owlVariableSet3f(var, v.x,v.y,v.z);
    owlVariableRelease(var);
  }

  box3f getBounds(const vec3f &pa, const float &ra,const vec3f &pb,const float &rb) 
  {
    box3f bounds;
    bounds.extend(pa - ra).extend(pa + ra).extend(pb - rb).extend(pb + rb);
    //PRINT(bounds);
    return bounds;
  }
  /*! the setup code for the 'all tubes are user-geom cylinders'
    variant. todo: add one with using instance transforms */
  void InstanceTubes::buildModel(Tubes::SP model)
  {
    arrowBuffer
      = owlDeviceBufferCreate(context,
                              OWL_USER_TYPE(model->arrows[0]),
                              model->arrows.size(),
                              model->arrows.data());

    OWLVarDecl tubesVars[] = {
      { "arrows", OWL_BUFPTR, OWL_OFFSETOF(ArrowsGeom,arrows)},
      { /* sentinel to mark end of list */ }
    };

    OWLGeomType tubesType
      = owlGeomTypeCreate(context,
                          OWL_GEOMETRY_USER,
                          sizeof(ArrowsGeom),
                          tubesVars, -1);
    owlGeomTypeSetBoundsProg(tubesType, module,
                             "Arrows_bounds");
    owlGeomTypeSetIntersectProg(tubesType, 0, module,
                                "Arrows_intersect");
    owlGeomTypeSetClosestHit(tubesType, 0, module,
                             "Arrows_closest_hit"); // nothing happened here

    OWLGeom arrowsGeom
      = owlGeomCreate(context, tubesType);
    owlGeomSetPrimCount(arrowsGeom, 1);
    owlGeomSetBuffer(arrowsGeom,"arrows",arrowBuffer);
    OWLGroup singleTubeGroup
      = owlUserGeomGroupCreate(context, 1, &arrowsGeom);

    // compile progs here because we need the bounds prog in accelbuild:
    owlBuildPrograms(context);
    owlGroupBuildAccel(singleTubeGroup);
    
    int numArrows = model->arrows.size();		
    std::vector<affine3f> transforms(numArrows);
    
    this->world
      = owlInstanceGroupCreate(context, model->transforms.size());
    for (int i = 0; i < transforms.size(); i++) {	
      owlInstanceGroupSetChild(this->world, i, singleTubeGroup);			
      owlInstanceGroupSetTransform(this->world, i, &(const owl4x3f&)model->transforms[i]);
    }
    owlGroupBuildAccel(this->world);
  }
}
