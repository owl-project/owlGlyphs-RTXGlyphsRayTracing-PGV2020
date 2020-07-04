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

#include "Tubes.h"
#include "OptixTubes.h"
#include "InstanceTubes.h"
#include <math.h>
#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>
// eventually to go into 'apps/'
// #define STB_IMAGE_WRITE_IMPLEMENTATION 1
// #include "samples/common/3rdParty/stb/stb_image_write.h"
// #define STB_IMAGE_IMPLEMENTATION 1
// #include "samples/common/3rdParty/stb/stb_image.h"

#include <math.h>
#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>
#include "samples/common/owlViewer/OWLViewer.h"
// std
#include <queue>
#include <map>
#include <set>
#include <string.h>
#include <thread>
#include <condition_variable>

using namespace owl::common;

namespace tubes {
  
  struct {
    std::string method = "instance"; //instance,basic
    int spp = 4;
    int shadeMode = 0;
    float radius = 0.f;
    struct {
      vec3f vp = vec3f(0.f);
      vec3f vu = vec3f(0.f);
      vec3f vi = vec3f(0.f);
    } camera;
    vec2i windowSize = vec2i(800,800);
    bool measure = false;
  } cmdline;
  
  std::string screenShotFileName
  = /* add the 'method' later in screenshot() - it's not set yet */
    "screenshot.png";

  void usage(const std::string &msg)
  {
    if (msg != "") std::cerr << "Error: " << msg << std::endl << std::endl;
    std::cout << "Usage: ./owlTubesViewer <inputfile>" << std::endl;
    exit(msg != "");
  }

  struct TubesViewer : public owl::viewer::OWLViewer
  {
    typedef owl::viewer::OWLViewer inherited;

    TubesViewer(OWLTubes &renderer)
      : owl::viewer::OWLViewer("owlTubesViewer",cmdline.windowSize),
        renderer(renderer)
    {}
    
    void screenShot()
    {
      inherited::screenShot(screenShotFileName);
      std::cout << "screenshot saved in '" << screenShotFileName << "'" << std::endl;
    }
    
    // /*! this function gets called whenever the viewer widget changes camera settings */
    virtual void cameraChanged() override 
    {
      inherited::cameraChanged();
      const viewer::SimpleCamera &camera = inherited::getCamera();
      
      const vec3f screen_du = camera.screen.horizontal / float(getWindowSize().x);
      const vec3f screen_dv = camera.screen.vertical   / float(getWindowSize().y);
      frameState.camera_screen_du = screen_du;
      frameState.camera_screen_dv = screen_dv;
      frameState.camera_screen_00 = camera.screen.lower_left;
      frameState.camera_lens_center = camera.lens.center;
      frameState.camera_lens_du = camera.lens.du;
      frameState.camera_lens_dv = camera.lens.dv;
      frameState.accumID = 0;
      renderer.updateFrameState(frameState);
    }
    
    /*! window notifies us that we got resized */
    virtual void resize(const vec2i &newSize) override
    {
      this->fbSize = newSize;
      // ... tell parent to resize (also resizes the pbo in the window)
      inherited::resize(newSize);
      
      renderer.resizeFrameBuffer(newSize,fbPointer);
      
      
      // ... and finally: update the camera's aspect
      setAspect(newSize.x/float(newSize.y));
      
      // update camera as well, since resize changed both aspect and
      // u/v pixel delta vectors ...
      updateCamera();
      owlBuildSBT(renderer.context);
    }
    
    /*! gets called whenever the viewer needs us to re-render out widget */
    virtual void render() override
    {
      if (fbSize.x < 0) return;
      
      static double t_last = -1;
      renderer.render();
      
      double t_now = getCurrentTime();
      static double avg_t = 0.;
      if (t_last >= 0)
        avg_t = 0.8*avg_t + 0.2*(t_now-t_last);

      if (displayFPS) {
        //        std::cout << "fps : " << (1.f/avg_t) << std::endl;
        char title[1000];
        sprintf(title,"owlTubes - %.2f FPS",(1.f/avg_t));
        inherited::setTitle(title);
      }
      t_last = t_now;
      
      frameState.accumID++;
      renderer.updateFrameState(frameState);
    }
    
    /*! this gets called when the user presses a key on the keyboard ... */
    virtual void key(char key, const vec2i &where)
    {
      switch (key) {
      case '!':
        screenShot();
        break;
      case '<':
        frameState.heatMapScale *= 1.5f;
        frameState.accumID = 0;
        renderer.updateFrameState(frameState);
        break;
      case '>':
        frameState.heatMapScale /= 1.5f;
        frameState.accumID = 0;
        renderer.updateFrameState(frameState);
        break;

      case '^':
        frameState.dbgPixel = where;
        frameState.accumID = 0;
        renderer.updateFrameState(frameState);
        break;
        
      case 'h':
      case 'H':
        frameState.heatMapEnabled ^= 1;
        PRINT((int)frameState.heatMapEnabled);
        frameState.accumID = 0;
        renderer.updateFrameState(frameState);
        break;
      case 'V':
        displayFPS = !displayFPS;
        break;
      case 'C': {
        auto &fc = getCamera();//fullCamera;
        std::cout << "(C)urrent camera:" << std::endl;
        std::cout << "- from :" << fc.position << std::endl;
        std::cout << "- poi  :" << fc.getPOI() << std::endl;
        std::cout << "- upVec:" << fc.upVector << std::endl; 
        std::cout << "- frame:" << fc.frame << std::endl;
        std::cout.precision(10);
        std::cout << "cmdline: --camera "
                  << fc.position.x << " "
                  << fc.position.y << " "
                  << fc.position.z << " "
                  << fc.getPOI().x << " "
                  << fc.getPOI().y << " "
                  << fc.getPOI().z << " "
                  << fc.upVector.x << " "
                  << fc.upVector.y << " "
                  << fc.upVector.z << std::endl;
      } break;
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
        renderer.updateFrameState(frameState);
        break;
      default:
        inherited::key(key,where);
      }
    }

    vec2i fbSize { -1,-1 };
    device::FrameState frameState;
    bool displayFPS = true;//false;

    /*! the actual rnederer code (ie, everything that's got nothing to
        do with windows or UI */
    OWLTubes &renderer;
  };
  
  extern "C" int main(int argc, char **argv)
  {
    std::string sceneFileName = "hair.obj";
    //std::string sceneFileName = "procedural://5000:4.0,12.0:0.2,0.0";
    
    for (int i=1;i<argc;i++) {
      const std::string arg = argv[i];
      if (arg[0] != '-') {
        sceneFileName = arg;
      } 
      else if (arg == "--radius" || arg == "-r") {
        cmdline.radius = std::stof(argv[++i]);
      }
      else if (arg == "--camera") {
        cmdline.camera.vp.x = std::atof(argv[++i]);
        cmdline.camera.vp.y = std::atof(argv[++i]);
        cmdline.camera.vp.z = std::atof(argv[++i]);
        cmdline.camera.vi.x = std::atof(argv[++i]);
        cmdline.camera.vi.y = std::atof(argv[++i]);
        cmdline.camera.vi.z = std::atof(argv[++i]);
        cmdline.camera.vu.x = std::atof(argv[++i]);
        cmdline.camera.vu.y = std::atof(argv[++i]);
        cmdline.camera.vu.z = std::atof(argv[++i]);
      }
      else if (arg == "-win" || arg == "--size") {
        cmdline.windowSize.x = std::atoi(argv[++i]);
        cmdline.windowSize.y = std::atoi(argv[++i]);
      }
      else if (arg == "-o") {
        screenShotFileName = argv[++i];
      }
      else if (arg == "-spp") {
        cmdline.spp = std::stoi(argv[++i]);
      }
      else if (arg == "--basic" || arg == "-user") {
        cmdline.method = "basic";
      }
      else if (arg == "--instance" || arg == "-inst") {
        cmdline.method = "instance";
      }
      else if (arg == "--rec-depth" || arg == "-sm" || arg == "--shade-mode") {
        cmdline.shadeMode = std::atoi(argv[++i]);
      }
      else
        usage("unknown cmdline arg '"+arg+"'");
    }    
    //Tubes::SP tubes = Tubes::load(sceneFileName,cmdline.radius);
    Tubes::SP tubes = Tubes::procedural();

    std::cout << "#tubes.viewer: creating back-end ..." << std::endl;
    OWLTubes *owlTubes = nullptr;
    
    if (cmdline.method == "instance") {
      owlTubes = new InstanceTubes;
    }
    else
      throw std::runtime_error("unknown tubes method '"+cmdline.method+"'");
    
    owlTubes->setModel(tubes);
    
    TubesViewer widget(*owlTubes);
    widget.frameState.samplesPerPixel = cmdline.spp;
    widget.frameState.shadeMode = cmdline.shadeMode;
    box3f sceneBounds = tubes->getBounds();
    widget.enableInspectMode(/* valid range of poi*/sceneBounds,
                             /* min distance      */1e-3f,
                             /* max distance      */1e6f);
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
} // ::tubes
