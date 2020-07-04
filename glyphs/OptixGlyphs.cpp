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

#include "glyphs/OptixGlyphs.h"
#include "glyphs/device/GlyphsGeom.h"
#include "glyphs/device/RayGenData.h"

namespace glyphs {

  using device::RayGenData;
  using device::GlyphsGeom;

  using device::TrianglesGeomData;
  
  extern "C" const char embedded_common_programs[];
  
  OWLGlyphs::OWLGlyphs()
  {
    context = owlContextCreate();//optix::ContextObj::create();
    module  = owlModuleCreate(context, embedded_common_programs);
    frameStateBuffer = owlDeviceBufferCreate(context,
                                             OWL_USER_TYPE(device::FrameState),
                                             1,nullptr);

    // -------------------------------------------------------
    // triangle mesh
    // -------------------------------------------------------
    OWLVarDecl trianglesGeomVars[] = {
      { "index",  OWL_BUFPTR, OWL_OFFSETOF(TrianglesGeomData,index)},
      { "vertex", OWL_BUFPTR, OWL_OFFSETOF(TrianglesGeomData,vertex)},
      { "color",  OWL_BUFPTR, OWL_OFFSETOF(TrianglesGeomData,color)},
      { nullptr /* sentinel to mark end of list */ }
    };
      
    trianglesGeomType
      = owlGeomTypeCreate(context,
                          OWL_TRIANGLES,
                          sizeof(TrianglesGeomData),
                          trianglesGeomVars, 3);

    owlGeomTypeSetClosestHit(trianglesGeomType, 0,
                             module, "TriangleMesh");


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
      { "linkBuffer",  OWL_BUFPTR, OWL_OFFSETOF(RayGenData,linkBuffer)},
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

  void OWLGlyphs::setModel(Glyphs::SP glyphs, Triangles::SP triangles)
  {
    build(glyphs,triangles);
    
    owlRayGenSetGroup(rayGen,"world",world);
    owlRayGenSetBuffer(rayGen,"linkBuffer",linkBuffer);
    
    owlBuildSBT(context);
  }

  void OWLGlyphs::render()
  {
    owlRayGenLaunch2D(rayGen,fbSize.x,fbSize.y);
  }

  OWLGroup OWLGlyphs::buildTriangles(Triangles::SP triangles)
  {
    if (triangles == 0) // ...
      return 0;

    std::vector<vec3f> vertices;
    std::vector<vec3f> colors;
    std::vector<vec3i> indices;
      
    for (auto m : triangles->meshes) {
      int size = vertices.size();
          
      vertices.reserve(vertices.size() + m->vertex.size());
      colors.reserve(colors.size() + m->color.size());
      vertices.insert(vertices.end(), m->vertex.begin(), m->vertex.end());
      colors.insert(colors.end(), m->color.begin(), m->color.end());

      for (auto id : m->index)           
        indices.push_back(vec3i(id) +size );
    }

    int numVertices = vertices.size();
    PRINT(numVertices);
    
    // ------------------------------------------------------------------
    // triangle mesh
    // ------------------------------------------------------------------
    OWLBuffer vertexBuffer
      = owlDeviceBufferCreate(context, OWL_FLOAT3, vertices.size(), vertices.data());
    OWLBuffer colorBuffer
      = owlDeviceBufferCreate(context, OWL_FLOAT3, colors.size(), colors.data());
    OWLBuffer indexBuffer
      = owlDeviceBufferCreate(context, OWL_INT3, indices.size(), indices.data());

    OWLGeom trianglesGeom
      = owlGeomCreate(context, trianglesGeomType);

    owlTrianglesSetVertices(trianglesGeom, vertexBuffer,
                            vertices.size(), sizeof(vec3f), 0);
    owlTrianglesSetIndices(trianglesGeom, indexBuffer,
                           indices.size(), sizeof(vec3i), 0);
    PRINT(indices.size());
    
    owlGeomSetBuffer(trianglesGeom, "vertex", vertexBuffer);
    owlGeomSetBuffer(trianglesGeom, "color", colorBuffer);
    owlGeomSetBuffer(trianglesGeom, "index", indexBuffer);
 
    // ------------------------------------------------------------------
    // the group/accel for that mesh
    // ------------------------------------------------------------------
    OWLGroup triGroup
      = owlTrianglesGeomGroupCreate(context, 1, &trianglesGeom);
    owlGroupBuildAccel(triGroup);
    PRINT(triGroup);
    return triGroup;
  }

  void OWLGlyphs::resizeFrameBuffer(void *fbPointer, const vec2i &newSize)
  {
    fbSize = newSize;
    if (!accumBuffer)
      accumBuffer = owlDeviceBufferCreate(context,OWL_FLOAT4,fbSize.x*fbSize.y,nullptr);
    owlBufferResize(accumBuffer,fbSize.x*fbSize.y);
    owlRayGenSetBuffer(rayGen,"accumBuffer",accumBuffer);
    owlRayGenSet1i(rayGen,"deviceCount",owlGetDeviceCount(context));
      
    owlRayGenSet1ul(rayGen,"colorBuffer",(uint64_t)fbPointer);

    owlRayGenSet2i(rayGen,"fbSize",fbSize.x,fbSize.y);
  }
  
  void OWLGlyphs::updateFrameState(device::FrameState &fs)
  {
    owlBufferUpload(frameStateBuffer,&fs);
  }

  uint32_t *OWLGlyphs::mapColorBuffer()
  {
    if (!colorBuffer) return nullptr;
    return (uint32_t*)owlBufferGetPointer(colorBuffer,0);
  }

  void OWLGlyphs::unmapColorBuffer()
  {
    assert(colorBuffer);
  }

  void OWLGlyphs::buildModules()
  {
    std::cout << "building programs" << std::endl;
    owlBuildPrograms(context);

    std::cout << "building pipeline" << std::endl;
    owlBuildPipeline(context);
  }

}
