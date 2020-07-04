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

#include "glyphs/device/Super.h"
#include "glyphs/SuperGlyphs.h"


namespace glyphs {
  
  using device::GlyphsGeom;
  using device::SuperGeomData;

  extern "C" const char embedded_SuperGlyphs_programs[];

  static box3f tessellate(super::Quadric sq, float u1, float u2, float du, float v1, float v2, float dv,
                          std::vector<vec3f>& vertices, std::vector<vec3i>& indices, std::vector<vec3f>& colors,
                          const affine3f& xfm, float slack=.4f)
  {

      sq.A += slack;
      sq.B += slack;
      sq.C += slack;

      int U = (u2-u1)/du;
      int V = (v2-v1)/du;
  
      int f = (int)indices.size();
 
      box3f bounds;
      for (int u = 0; u <= U; ++u)
      {
          for (int v = 0; v <= V; ++v)
          {
              float umin = u1 + u * du;
              float umax = fminf(u1 + (u+1) * du, u2);
              float vmin = v1 + v * dv;
              float vmax = fminf(v1 + (v+1) * dv, v2);
  
              vec3f v1 = super::eval(sq,umax,vmin);
              vec3f v2 = super::eval(sq,umin,vmin);
              vec3f v3 = super::eval(sq,umin,vmax);
              vec3f v4 = super::eval(sq,umax,vmax);

              vertices.push_back(v1);
              vertices.push_back(v2);
              vertices.push_back(v3);
              vertices.push_back(v4);

              indices.push_back({f,f+1,f+2});
              indices.push_back({f,f+2,f+3});

              f+=4;

              colors.push_back({1,1,1});
              colors.push_back({1,1,1});
              colors.push_back({1,1,1});
              colors.push_back({1,1,1});
              
              bounds.extend(xfmPoint(xfm,v1));
              bounds.extend(xfmPoint(xfm,v2));
              bounds.extend(xfmPoint(xfm,v3));
              bounds.extend(xfmPoint(xfm,v4));
          }
      }

      return bounds;
  }

  static super::Quadric mapToSuperQuadric(const Link& link)
  {
      vec3f rst;
      vec3f ABC;
      rst.x = 1.f+drand48()*2.f;
      rst.y = 1.f+drand48()*2.f;
      rst.z = 1.f+drand48()*2.f;
      ABC.x = 1.f;
      ABC.y = 1.f;
      ABC.z = 1.f;
      return {rst.x,rst.y,rst.z, ABC.x,ABC.y,ABC.z};
  }

  SuperGlyphs::SuperGlyphs()
  {
    module = owlModuleCreate(context, embedded_SuperGlyphs_programs);

#if USER_GEOM_SUPER_GLYPHS

    OWLVarDecl trianglesGeomVars[] = {
      { "rst",    OWL_BUFPTR, OWL_OFFSETOF(SuperGeomData,rst)},
      { "ABC",    OWL_BUFPTR, OWL_OFFSETOF(SuperGeomData,ABC)},
      { nullptr /* sentinel to mark end of list */ }
    };

    glyphsType
      = owlGeomTypeCreate(context,
                          OWL_GEOMETRY_USER,
                          sizeof(device::SuperGeomData),
                          trianglesGeomVars, 2);

    owlGeomTypeSetBoundsProg(glyphsType, module,
                             "SuperGlyphs");
    owlGeomTypeSetIntersectProg(glyphsType, 0, module,
                                "SuperGlyphs");
    owlGeomTypeSetClosestHit(glyphsType, 0, module,
                             "SuperGlyphs"); // nothing happened here

#else

    OWLVarDecl trianglesGeomVars[] = {
      { "index",  OWL_BUFPTR, OWL_OFFSETOF(SuperGeomData,index)},
      { "vertex", OWL_BUFPTR, OWL_OFFSETOF(SuperGeomData,vertex)},
      { "color",  OWL_BUFPTR, OWL_OFFSETOF(SuperGeomData,color)},
      { "rst",    OWL_BUFPTR, OWL_OFFSETOF(SuperGeomData,rst)},
      { "ABC",    OWL_BUFPTR, OWL_OFFSETOF(SuperGeomData,ABC)},
      { nullptr /* sentinel to mark end of list */ }
    };
      
    glyphsType
      = owlGeomTypeCreate(context,
                          OWL_TRIANGLES,
                          sizeof(device::SuperGeomData),
                          trianglesGeomVars, 5);

#if TESSELLATE_SUPER_GLYPHS
    owlGeomTypeSetClosestHit(glyphsType, 0,
                             module, "SuperGlyphs");
#else
    owlGeomTypeSetAnyHit(glyphsType, 0,
                             module, "SuperGlyphs");
#endif

#endif
    buildModules();
  }

  void SuperGlyphs::addUserGeom(std::vector<std::pair<OWLGroup,affine3f>>& groups,
                                const super::Quadric& sq,
                                const affine3f& xfm)
  {
    vec3f rst(sq.r,sq.s,sq.t);
    vec3f ABC(sq.A,sq.B,sq.C);

    OWLGeom geom = owlGeomCreate(context, glyphsType);
    owlGeomSetPrimCount(geom, 1);
    OWLBuffer rstBuffer
      = owlDeviceBufferCreate(context, OWL_FLOAT3, 1, &rst);
    OWLBuffer ABCBuffer
      = owlDeviceBufferCreate(context, OWL_FLOAT3, 1, &ABC);
    owlGeomSetBuffer(geom, "rst", rstBuffer);
    owlGeomSetBuffer(geom, "ABC", ABCBuffer);

    OWLGroup grp = owlUserGeomGroupCreate(context, 1, &geom);
    owlGroupBuildAccel(grp);
    groups.push_back({grp,xfm});
  }

  void SuperGlyphs::addTessellation(std::vector<std::pair<OWLGroup,affine3f>>& groups,
                                    box3f& worldBounds,
                                    size_t& numTris,
                                    const super::Quadric& sq,
                                    const affine3f& xfm,
                                    float du,
                                    float dv)
  {
    std::vector<vec3f> vertices;
    std::vector<vec3f> colors;
    std::vector<vec3i> indices;

    box3f bounds = tessellate(sq,-M_PI,M_PI,du,-M_PI/2.f,M_PI/2.f,dv,
                              vertices,indices,colors,xfm);
    worldBounds.extend(bounds);

    vec3f rst(sq.r,sq.s,sq.t);
    vec3f ABC(sq.A,sq.B,sq.C);

    OWLGeom trianglesGeom = owlGeomCreate(context, glyphsType);

    OWLBuffer vertexBuffer
      = owlDeviceBufferCreate(context, OWL_FLOAT3, vertices.size(), vertices.data());
    OWLBuffer colorBuffer
      = owlDeviceBufferCreate(context, OWL_FLOAT3, colors.size(), colors.data());
    OWLBuffer indexBuffer
      = owlDeviceBufferCreate(context, OWL_INT3, indices.size(), indices.data());
    OWLBuffer rstBuffer
      = owlDeviceBufferCreate(context, OWL_FLOAT3, 1, &rst);
    OWLBuffer ABCBuffer
      = owlDeviceBufferCreate(context, OWL_FLOAT3, 1, &ABC);
    numTris += vertices.size()/3;

    owlTrianglesSetVertices(trianglesGeom, vertexBuffer,
                            vertices.size(), sizeof(vec3f), 0);
    owlTrianglesSetIndices(trianglesGeom, indexBuffer,
                           indices.size(), sizeof(vec3i), 0);

    owlGeomSetBuffer(trianglesGeom, "vertex", vertexBuffer);
    owlGeomSetBuffer(trianglesGeom, "color", colorBuffer);
    owlGeomSetBuffer(trianglesGeom, "index", indexBuffer);
    owlGeomSetBuffer(trianglesGeom, "rst", rstBuffer);
    owlGeomSetBuffer(trianglesGeom, "ABC", ABCBuffer);

    OWLGroup grp = owlTrianglesGeomGroupCreate(context, 1, &trianglesGeom);
    owlGroupBuildAccel(grp);
    groups.push_back({grp,xfm});
  }

  std::vector<std::pair<OWLGroup,affine3f>> SuperGlyphs::buildGlyphs(Glyphs::SP glyphs)
  {
#if USER_GEOM_SUPER_GLYPHS
    // compile progs here because we need the bounds prog in accelbuild:
    owlBuildPrograms(context);
#endif

    std::vector<std::pair<OWLGroup,affine3f>> groups;
    box3f worldBounds;
    size_t numTris = 0;
    for (size_t i=0; i<glyphs->links.size(); i+=2) {
      const Link& l = glyphs->links[i];
      super::Quadric sq = mapToSuperQuadric(l);
      affine3f xfm = Glyphs::getXform(glyphs,l);
      if (std::isfinite(xfm.l.vx.x)) // rofl
#if USER_GEOM_SUPER_GLYPHS
        addUserGeom(groups,sq,xfm);
#else
        addTessellation(groups,worldBounds,numTris,sq,xfm,.8f,.8f);
#endif
    }
    std::cout << "numTris: " << numTris << ", numGlyphs: " << groups.size()
              << ", avg: " << numTris/(double)groups.size() << '\n';
    //std::cout << worldBounds << '\n';
    linkBuffer
      = owlDeviceBufferCreate(context,
                              OWL_USER_TYPE(glyphs->links[0]),
                              glyphs->links.size(),
                              glyphs->links.data());

    return groups;
  }
  
  void SuperGlyphs::build(Glyphs::SP glyphs,
                         Triangles::SP triangles)
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
