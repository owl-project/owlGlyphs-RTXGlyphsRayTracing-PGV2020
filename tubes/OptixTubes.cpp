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



#include "tubes/OptixTubes.h"
#include "tubes/device/RayGenData.h"
#include "tubes/device/TubesGeom.h"

namespace tubes {

  using device::RayGenData;
  using device::ArrowsGeom;
  
  extern "C" const char embedded_common_programs[];
  
  OWLTubes::OWLTubes()
  {
    context = owlContextCreate();//optix::ContextObj::create();
    module  = owlModuleCreate(context, embedded_common_programs);
    frameStateBuffer = owlDeviceBufferCreate(context,
                                             OWL_USER_TYPE(device::FrameState),
                                             1,nullptr);

    // -------------------------------------------------------
    // set up miss prog 
    // -------------------------------------------------------
    OWLVarDecl missProgVars[] = {
      { /* sentinel to mark end of list */ }
    };
    // ........... create object  ............................
    OWLMissProg missProg
      = owlMissProgCreate(context,module,"miss_program",0,//sizeof(MissProgData),
                          missProgVars,-1);

    // -------------------------------------------------------
    // set up ray gen program
    // -------------------------------------------------------
    OWLVarDecl rayGenVars[] = {
      { "deviceIndex",     OWL_DEVICE, OWL_OFFSETOF(RayGenData,deviceIndex)},
      { "deviceCount",     OWL_INT,    OWL_OFFSETOF(RayGenData,deviceCount)},
      { "colorBuffer",     OWL_RAW_POINTER, OWL_OFFSETOF(RayGenData,colorBufferPtr)},
      { "accumBuffer",     OWL_BUFPTR, OWL_OFFSETOF(RayGenData,accumBufferPtr)},
      { "arrowBuffer",     OWL_BUFPTR, OWL_OFFSETOF(RayGenData,arrowBuffer)},
      { "frameStateBuffer",OWL_BUFPTR, OWL_OFFSETOF(RayGenData,frameStateBuffer)},
      { "fbSize",          OWL_INT2,   OWL_OFFSETOF(RayGenData,fbSize)},
      { "world",           OWL_GROUP,  OWL_OFFSETOF(RayGenData,world)},
      { /* sentinel to mark end of list */ }
    };

    // ........... create object  ............................
    this->rayGen
      = owlRayGenCreate(context,module,"raygen_program",
                        sizeof(RayGenData),
                        rayGenVars,-1);

    owlRayGenSetBuffer(rayGen,"frameStateBuffer",frameStateBuffer);    
  }


  void OWLTubes::setModel(Tubes::SP model)
  {
    buildModel(model);
    owlRayGenSetGroup(rayGen,"world",world);
    owlRayGenSetBuffer(rayGen,"arrowBuffer",arrowBuffer);
    
    std::cout << "building programs" << std::endl;
    owlBuildPrograms(context);
    std::cout << "building pipeline" << std::endl;
    owlBuildPipeline(context);
    owlBuildSBT(context);
  }

  // Move thisto the child class.

  /*! the setup code for the 'all tubes are user-geom cylinders'
      variant. todo: add one with using instance transforms */
  //void OWLTubes::buildModel(Tubes::SP model)
  //{
  //  linkBuffer
  //    = owlDeviceBufferCreate(context,
  //    OWL_USER_TYPE(model->links[0]),
  //    model->links.size(),
  //    model->links.data());


  //  OWLVarDecl tubesVars[] = {
  //    { "links",  OWL_BUFPTR, OWL_OFFSETOF(TubesGeom,links)},
  //    { "radius", OWL_FLOAT , OWL_OFFSETOF(TubesGeom,radius)},
  //    { /* sentinel to mark end of list */ }
  //  };

  //  OWLGeomType tubesType
  //    = owlGeomTypeCreate(context,
  //                        OWL_GEOMETRY_USER,
  //                        sizeof(TubesGeom),
  //                        tubesVars,-1);
  //  owlGeomTypeSetBoundsProg(tubesType,module,
  //                           "tubes_bounds");
  //  owlGeomTypeSetIntersectProg(tubesType,0,module,
  //                              "tubes_intersect");
  //  owlGeomTypeSetClosestHit(tubesType,0,module,
  //                           "tubes_closest_hit");

  //  OWLGeom geom = owlGeomCreate(context,tubesType);
  //  owlGeomSetPrimCount(geom,model->links.size());
  //  
  //  owlGeomSetBuffer(geom,"links",linkBuffer);
  //  owlGeomSet1f(geom,"radius",model->radius);
  //  
  //  this->world = owlUserGeomGroupCreate(context,1,&geom);
  //  owlBuildPrograms(context);
  //  owlGroupBuildAccel(this->world);
  //}
   
  void OWLTubes::render()
  {
    owlRayGenLaunch2D(rayGen,fbSize.x,fbSize.y);
  }

  void OWLTubes::resizeFrameBuffer(const vec2i &newSize,
                                   void *colorBufferPointer)
  {
    fbSize = newSize;
    if (!accumBuffer)
      accumBuffer = owlDeviceBufferCreate(context,OWL_FLOAT4,fbSize.x*fbSize.y,nullptr);
    owlBufferResize(accumBuffer,fbSize.x*fbSize.y);
    owlRayGenSetBuffer(rayGen,"accumBuffer",accumBuffer);
    owlRayGenSet1i(rayGen,"deviceCount",owlGetDeviceCount(context));
      
    owlRayGenSetPointer(rayGen,"colorBuffer",colorBufferPointer);
    // owlRayGenSetBuffer(rayGen,"colorBuffer",colorBuffer);
    owlRayGenSet2i(rayGen,"fbSize",fbSize.x,fbSize.y);
  }
  
  void OWLTubes::updateFrameState(device::FrameState &fs)
  {
    owlBufferUpload(frameStateBuffer,&fs);
  }

}
