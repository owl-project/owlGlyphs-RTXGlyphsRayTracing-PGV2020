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
#include "OptixGlyphs.h"
#include "ArrowGlyphs.h"
#include "MotionSpheres.h"
#include "SphereGlyphs.h"
#include "SuperGlyphs.h"
#include <math.h>
// std
#include <queue>
#include <map>
#include <set>
#include <string.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <condition_variable>

#include "samples/common/owlViewer/OWLViewer.h"
#define STB_IMAGE_IMPLEMENTATION 1
#include "samples/common/3rdParty/stb/stb_image.h"

namespace glyphs {

  typedef OWLGlyphs Renderer;

  struct {
    std::string method = "arrows"; //arrows,spheres,super,motionblur
    int spp = 4;
    int shadeMode = 0;
    int pathDepth = 0;
    bool lines = false;
    struct {
      vec3f vp = vec3f(0.f);
      vec3f vu = vec3f(0.f);
      vec3f vi = vec3f(0.f);
    } camera;
    vec2i windowSize = vec2i(800,800);
    bool measure = false;
    DisneyMaterial material;

    std::vector<std::string> objFileNames;

  } cmdline;
  
  std::string screenShotFileName
  = /* add the 'method' later in screenshot() - it's not set yet */
    "screenshot.png";

  void usage(const std::string &msg)
  {
    if (msg != "") std::cerr << "Error: " << msg << std::endl << std::endl;
    std::cout << "Usage: ./owlGlyphsViewer <inputfile>" << std::endl;
    exit(msg != "");
  }

  struct GlyphsViewer : public owl::viewer::OWLViewer
  {
    typedef owl::viewer::OWLViewer inherited;

    Renderer &renderer;
    OWLGlyphs *owl = nullptr;

    std::vector<std::string> args;

    std::vector<Glyphs::SP> glyphs;
    Triangles::SP triangles;
    
    GlyphsViewer(Renderer &renderer)
      : OWLViewer("owlGlyps"),
        renderer(renderer)
    {
      owl = static_cast<OWLGlyphs *>(&renderer);
    }

    std::string printCamera(std::ostream &os) const {

      const auto &fc = camera;
      os << "(C)urrent camera:" << std::endl;
      os << "- from :" << fc.position << std::endl;
      os << "- poi  :" << fc.getPOI() << std::endl;
      os << "- upVec:" << fc.upVector << std::endl;
      os << "- frame:" << fc.frame << std::endl;
      std::stringstream str;
      str.precision(10);
      str << "--camera "
          << fc.position.x << " "
          << fc.position.y << " "
          << fc.position.z << " "
          << fc.getPOI().x << " "
          << fc.getPOI().y << " "
          << fc.getPOI().z << " "
          << fc.upVector.x << " "
          << fc.upVector.y << " "
          << fc.upVector.z;
      std::cout << "cmdline: " << str.str() << std::endl;
      return str.str();
    }
    
    void screenShot()
    {
      std::string method = cmdline.method;
      const std::string fileName
        = cmdline.measure
        ? screenShotFileName
        : (method+"_"+screenShotFileName);

      std::fstream params(fileName+".params", std::ios::out);
      auto cam = printCamera(params);
      params << "cmdline: owlGlyphsViewer";
      for (const auto &a: args)
        params << " " << a;
      params << " " << cam;
      params << std::endl;


      std::cout << "parameters saved in '" << fileName << ".params" << "'" << std::endl;

      // std::vector<uint32_t> pixels;
      
      // const uint32_t *fb 
      //   = (const uint32_t *)fbPointer;
      
      // for (int y=0;y<fbSize.y;y++) {
      //   const uint32_t *line = fb + (fbSize.y-1-y)*fbSize.x;
      //   for (int x=0;x<fbSize.x;x++) {
      //     pixels.push_back(line[x] | (0xff << 24));
      //   }
      // }
      // stbi_write_png(fileName.c_str(),fbSize.x,fbSize.y,4,
      //                pixels.data(),fbSize.x*sizeof(uint32_t));
      inherited::screenShot(fileName);
      std::cout << "screenshot saved in '" << fileName << "'" << std::endl;
    }
    
    void updateFrameState()
    {
      owl->updateFrameState(frameState);
    }


    // /*! this function gets called whenever the viewer widget changes camera settings */
    virtual void cameraChanged() override 
    {
      inherited::cameraChanged();

      const owl::viewer::SimpleCamera camera = getSimplifiedCamera();

      const vec3f screen_du = camera.screen.horizontal / float(getWindowSize().x);
      const vec3f screen_dv = camera.screen.vertical   / float(getWindowSize().y);
      frameState.camera_screen_du = screen_du;
      frameState.camera_screen_dv = screen_dv;
      frameState.camera_screen_00 = camera.screen.lower_left;
      frameState.camera_lens_center = camera.lens.center;
      frameState.camera_lens_du = camera.lens.du;
      frameState.camera_lens_dv = camera.lens.dv;
      frameState.accumID = 0;
      updateFrameState();
    }
    
    /*! window notifies us that we got resized */
    virtual void resize(const vec2i &newSize) override
    {
      inherited::resize(newSize);
      owl->resizeFrameBuffer(fbPointer,newSize);
      
      // ... and finally: update the camera's aspect
      setAspect(newSize.x/float(newSize.y));
      
      // update camera as well, since resize changed both aspect and
      // u/v pixel delta vectors ...
      updateCamera();
      owlBuildSBT(owl->context);
    }
    
    /*! gets called whenever the viewer needs us to re-render out widget */
    virtual void render() override
    {
      static double t_last = -1;
      owl->render();
      
      double t_now = getCurrentTime();
      static double avg_t = 0.;
      if (t_last >= 0)
        avg_t = 0.8*avg_t + 0.2*(t_now-t_last);
      
      
      if (displayFPS) {
        char title[1000];
        sprintf(title,"owlGlyphs - %.2f FPS",(1.f/avg_t));
        this->setTitle(title);
      }
      t_last = t_now;
      
      if (cmdline.measure) {
        static double measure_begin = t_now;
        static int numFrames = 0;
        
        if (t_now - measure_begin > 10.f) {
          std::cout << "MEASURE_FPS " << (numFrames/(t_now-measure_begin)) << std::endl;
          screenShot();
          exit(0);
        }
        numFrames++;
      }
      
      frameState.accumID++;
      updateFrameState();
    }
    
   
    /*! this gets called when the user presses a key on the keyboard ... */
    virtual void key(char key, const vec2i &where) override
    {
      switch (key) {
      case '!':
        screenShot();
        break;
      case '<':
        frameState.heatMapScale *= 1.5f;
        frameState.accumID = 0;
        updateFrameState();
        break;
      case '>':
        frameState.heatMapScale /= 1.5f;
        frameState.accumID = 0;
        updateFrameState();
        break;

      case '^':
        frameState.dbgPixel = where;
        frameState.accumID = 0;
        updateFrameState();
        break;
        
      case 'h':
      case 'H':
        frameState.heatMapEnabled ^= 1;
        PRINT((int)frameState.heatMapEnabled);
        frameState.accumID = 0;
        updateFrameState();
        break;
      case 'V':
        displayFPS = !displayFPS;
        break;
      case 'C':
        printCamera(std::cout);
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        frameState.shadeMode = (key - '0');
        PRINT(frameState.shadeMode);
        frameState.accumID = 0;
        updateFrameState();
        break;
      case 'q':
#ifndef WIN32
        exit(0);
#endif         
        // Not sure if we need to std::exit here, it seems to return
        // from this call but quit out ok
        break;
      default:
        inherited::key(key,where);
      }
    }

    device::FrameState frameState;
    bool displayFPS = true;//false;
  };
  
  extern "C" int main(int argc, char **argv)
  {
    std::vector<std::string> fileNames;
    std::vector<std::string> args;
    
    for (int i=1;i<argc;i++) {
      std::string arg(argv[i]);
      args.emplace_back(argv[i]);

      if (arg[0] != '-') {
        fileNames.push_back(arg);
      } 
      else if (arg == "--camera") {
        cmdline.camera.vp.x = std::atof(argv[++i]);
        args.emplace_back(argv[i]);
        cmdline.camera.vp.y = std::atof(argv[++i]);
        args.emplace_back(argv[i]);
        cmdline.camera.vp.z = std::atof(argv[++i]);
        args.emplace_back(argv[i]);
        cmdline.camera.vi.x = std::atof(argv[++i]);
        args.emplace_back(argv[i]);
        cmdline.camera.vi.y = std::atof(argv[++i]);
        args.emplace_back(argv[i]);
        cmdline.camera.vi.z = std::atof(argv[++i]);
        args.emplace_back(argv[i]);
        cmdline.camera.vu.x = std::atof(argv[++i]);
        args.emplace_back(argv[i]);
        cmdline.camera.vu.y = std::atof(argv[++i]);
        args.emplace_back(argv[i]);
        cmdline.camera.vu.z = std::atof(argv[++i]);
        args.emplace_back(argv[i]);
      }
      else if (arg == "-win" || arg == "--size") {
        cmdline.windowSize.x = std::atoi(argv[++i]);
        args.emplace_back(argv[i]);
        cmdline.windowSize.y = std::atoi(argv[++i]);
        args.emplace_back(argv[i]);
      }
      else if (arg == "-o") {
        screenShotFileName = argv[++i];
        args.emplace_back(argv[i]);
      }
      else if (arg == "-spp") {
        cmdline.spp = std::stoi(argv[++i]);
        args.emplace_back(argv[i]);
      }
      else if (arg == "--arrows" || arg == "-arr") {
        cmdline.method = "arrows";
      }
      else if (arg == "--spheres" || arg == "-sph") {
        cmdline.method = "spheres";
      }
      else if (arg == "--super" || arg == "-spr") {
        cmdline.method = "super";
      }
      else if (arg == "--motionblur" || arg == "-mb") {
        cmdline.method = "motionblur";
      }
      else if (arg == "--lines") {
        cmdline.lines = true;
      }
      else if (arg == "-sm" || arg == "--shade-mode") {
        cmdline.shadeMode = std::atoi(argv[++i]);
        args.emplace_back(argv[i]);
      }
      else if (arg == "--rec-depth" || arg == "-rd") {
        cmdline.pathDepth = std::atoi(argv[++i]);
        args.emplace_back(argv[i]);
      }
      else if (arg == "-measure" || arg == "--measure") {
        cmdline.measure = true;
      }
      else if (arg == "-triobj" ||
               arg == "-obj" ||
               arg == "-quadobj"
               ) {
        cmdline.objFileNames.push_back(argv[++i]);
        args.emplace_back(argv[i]);
      }
      else
        usage("unknown cmdline arg '"+arg+"'");
    }

    if (fileNames.empty())
      usage("No glyph file name provided. See testdata.glyphs in project root directory.");

    // ------------------------------------------------------------------
    // load input data
    // ------------------------------------------------------------------
    std::vector<Glyphs::SP> glyphs;
    for (Glyphs::SP curGlyphs = Glyphs::load(fileNames);
         curGlyphs;
         curGlyphs = curGlyphs->nextTimestep) {

      glyphs.push_back(curGlyphs);
    }
    if (glyphs.empty()) {
      std::cout << "did not load any glyphs" << std::endl;
    }

    Triangles::SP triangles = nullptr;
    if (cmdline.objFileNames.size() > 0) {
      triangles = Triangles::load(cmdline.objFileNames,vec3f(.8f));
    }

    // ------------------------------------------------------------------
    // creating backend, and ask backend to build its data
    // ------------------------------------------------------------------
    std::cout << "#glyphs.viewer: creating back-end ..." << std::endl;
    OWLGlyphs *owlGlyphs = nullptr;
    Renderer *rend = nullptr;
   
    if (cmdline.method == "arrows") {
      owlGlyphs = new ArrowGlyphs;
    }
    else if (cmdline.method == "spheres") {
      owlGlyphs = new SphereGlyphs;
    }
    else if (cmdline.method == "super") {
      owlGlyphs = new SuperGlyphs;
    }
    else if (cmdline.method == "motionblur") {
      owlGlyphs = new MotionSpheres;
    }
    else
      throw std::runtime_error("unknown glyphs method '"+cmdline.method+"'");
    owlGlyphs->setModel(glyphs[0],triangles);
    rend = owlGlyphs;
           
    // ------------------------------------------------------------------
    // create viewer
    // ------------------------------------------------------------------
    GlyphsViewer widget(*rend);
    widget.glyphs = glyphs;
    widget.args = args;
    widget.triangles = triangles;
    widget.frameState.samplesPerPixel = cmdline.spp;
    widget.frameState.shadeMode = cmdline.shadeMode;
    widget.frameState.pathDepth = cmdline.pathDepth;
    widget.frameState.material = cmdline.material;
    box3f sceneBounds = glyphs[0]->getBounds();
    if (triangles)
      sceneBounds.extend(triangles->bounds);

    widget.enableInspectMode(owl::viewer::OWLViewer::Arcball,
                             /* valid range of poi*/sceneBounds,
                             /* min distance      */1e-4f,
                             /* max distance      */1e5f);
    widget.enableFlyMode();
    
    if (cmdline.camera.vu != vec3f(0.f)) {
      widget.setCameraOrientation(/*origin   */cmdline.camera.vp,
                                  /*lookat   */cmdline.camera.vi,
                                  /*up-vector*/cmdline.camera.vu,
                                  /*fovy(deg)*/70.f);
    } else {
      widget.setCameraOrientation(/*origin   */
                                  sceneBounds.center()
                                  + vec3f(-.3f,.7f,+1.f)*sceneBounds.span(),
                                  /*lookat   */sceneBounds.center(),
                                  /*up-vector*/vec3f(0.f, 1.f, 0.f),
                                  /*fovy(deg)*/70);
    }
    widget.setWorldScale(length(sceneBounds.span()));
    widget.showAndRun();
  }
} // ::glyphs
