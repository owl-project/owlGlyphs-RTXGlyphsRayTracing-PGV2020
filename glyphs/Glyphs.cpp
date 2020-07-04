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

#include "Glyphs.h"
#include <cstddef>
#include <fstream>
#include <cstring>
#include <future>

namespace glyphs {
  using namespace owl;
  using namespace owl::common;

#if _WIN32
  inline double drand48() { return double(rand()) / RAND_MAX; }
#endif

  std::string getExt(const std::string &fileName)
  {
    int pos = fileName.rfind('.');
    if (pos == fileName.npos)
      return "";
    return fileName.substr(pos);
  }


  /*! See testdata.glyphs for a more verbose description of
    this simple ascii file format */
  Glyphs::SP tryGlyphs(const std::string& fileName)
  {
    const std::string ext = getExt(fileName);
    if (ext != ".glyphs")
      return nullptr;

    Glyphs::SP glyphs = std::make_shared<Glyphs>();
    std::ifstream in(fileName);

    std::string line;
    while (std::getline(in, line))
    {
      if (line.empty() || line[0] == '#')
        continue;

      vec3f pos1, pos2, col;
      vec3f accel(0.f);
      int res;
      res = sscanf(line.c_str(), "(%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)",
                   &pos1.x,&pos1.y,&pos1.z,
                   &pos2.x,&pos2.y,&pos2.z,
                   &col.x,&col.y,&col.z,
                   &accel.x,&accel.y,&accel.z);

      if (res < 6 || res > 12) {
        std::cerr << "Invalid line: " << line << '\n';
        return nullptr;
      }

      unsigned colRGBA8(-1);
      if (res > 6) {
        unsigned r=(unsigned)clamp(col.x*255.f,0.f,255.f);
        unsigned g=(unsigned)clamp(col.y*255.f,0.f,255.f);
        unsigned b=(unsigned)clamp(col.z*255.f,0.f,255.f);
        colRGBA8 = r | (g<<8) | (b<<16) | (255<<24);
      }

      Link l1;
      l1.pos = pos1;
      l1.rad = glyphs->radius;
      l1.col = colRGBA8;
      l1.accel = accel;
      l1.prev = -1;

      Link l2;
      l2.pos = pos2;
      l2.rad = glyphs->radius;
      l2.col = colRGBA8;
      l2.accel = accel;
      l2.prev = (int)glyphs->links.size();

      glyphs->links.push_back(l1);
      glyphs->links.push_back(l2);
    }

    return glyphs;
  }
          
  Glyphs::SP Glyphs::load(const std::string& fileName)
  {
    if (Glyphs::SP glyphs = tryGlyphs(fileName))
      return glyphs;

    throw std::runtime_error("could not load/create input '"+fileName+"'");
  }

  /*! load several files */
  Glyphs::SP Glyphs::load(const std::vector<std::string>& fileNames)
  {
    std::vector<std::future<Glyphs::SP>> files;
    for (std::size_t i=0; i<fileNames.size(); ++i) {
      const std::string file = fileNames[i];
      files.emplace_back(std::async(std::launch::async, [file](){
        return load(file);
      }));
    }

    Glyphs::SP result = nullptr;
    for (std::size_t i=0; i<fileNames.size(); ++i) {
      Glyphs::SP glyphs = files[i].get();
      if (glyphs == nullptr) {
        throw std::runtime_error("could not load/create input from file no. '"+std::to_string(i)+"'");
      }
      Glyphs::SP step = result;
      while (glyphs != nullptr) {
        if (step != nullptr) {
            for (std::size_t j=0; j<glyphs->links.size(); ++j) {
              // Yeah, I'm sure that's not the most elegant way but should
              // ensure that the links are correctly set up even for file
              // types that I can't test..
              if (glyphs->links[j].prev >= 0)
                glyphs->links[j].prev += (int)step->links.size();
            }
            step->links.insert(step->links.end(),glyphs->links.begin(),glyphs->links.end());
            glyphs = glyphs->nextTimestep;
            step = step->nextTimestep;
        } else {
          step = glyphs;
          glyphs = nullptr;
          result = step;
        }
      }
    }
    return result;
  }

  affine3f Glyphs::getXform(const Glyphs::SP& glyphs, const Link& l)
  {
    affine3f xfm;
    const Link A = l;
    if (A.prev < 0) {
      xfm
        = affine3f::translate(A.pos);
      xfm.l.vx *= glyphs->radius;
      xfm.l.vy *= glyphs->radius;
      xfm.l.vz *= glyphs->radius;
    } else {
      const Link B = glyphs->links[A.prev];
      const float linkLength = length(A.pos - B.pos)+glyphs->radius;
      if (linkLength <= 1e-6f) {
        // too short, ignore this,it'll only get numerically weird...
        xfm
          = affine3f::translate(A.pos);
        xfm.l.vx *= glyphs->radius;
        xfm.l.vy *= glyphs->radius;
        xfm.l.vz *= glyphs->radius;
      } else {
        xfm
          = affine3f::translate(B.pos)
          * affine3f(frame(normalize(A.pos-B.pos)));
        xfm.l.vx *= glyphs->radius;
        xfm.l.vy *= glyphs->radius;
        xfm.l.vz *= linkLength;
      }
    }
    return xfm;
  }

  /*! returns world space bounding box of the scene; to be used for
    the viewer to automatically set camera and motion speed */
  box3f Glyphs::getBounds() const
  {
    box3f bounds;
    for (auto comp : links) 
      bounds.extend(comp.pos);
    PRINT(bounds);
    return box3f(bounds.lower - radius, bounds.upper + radius);
  }

}
