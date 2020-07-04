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

#include "owl/common/math/vec.h"
#include "TrackVisData.h"
#include <vector>
#include <queue>
#include <fstream>

using namespace owl::common;

using tubes::TrackVisData;

/*! the thresholds - in fraction of original model size - at which
    we'll dump reduced model sizes; ie, the value ".4" in this list
    means well dump a version as soon as reduction has led to a model
    that's 0.4x (or 40% of) the input model */
std::vector<float> targets = {
  1.0, .6, .4, .3, .2, .15, .1, .05, .02
};



struct Model {
  struct StreamLine {
    
    void findBestEliminationPos();

    /*! eliminate control point with least error */
    void eliminateOne();

    //! the points in this stream line/track
    std::vector<vec3f> points;
    
    /*! 'best' control point to eliminate in this tack (in sense of
      'the one withleast error) */
    int   bestEliminationPos = -1;

    /*! the errof of the 'bestEliminationPos' control point, if we were to
        eliminate it */
    float errorWhenEliminating = std::numeric_limits<float>::infinity();
  };
  std::vector<StreamLine> lines;
};


void Model::StreamLine::findBestEliminationPos()
{
  bestEliminationPos = -1;
  errorWhenEliminating = std::numeric_limits<float>::infinity();
  for (int i=1;i<(points.size()-1);i++) {
    vec3f lst = points[i-1];
    vec3f nxt = points[i+1];
    vec3f cur = points[i];
    if (cur==lst || cur==nxt) {
      bestEliminationPos = i;
      errorWhenEliminating = 0.f;
      return;
    }

    // const vec3f A = normalize(cur-lst);
    // const vec3f B = normalize(nxt-lst);
    const float cosAngle = dot(normalize(nxt-cur),
                               normalize(cur-lst));
    if (cosAngle < 0.f)
      continue;
        
    float errorThisNode
      = 1.f - cosAngle;
      // = length(cross(nxt-cur,cur-lst))
      // / (cosAngle+1e-5f)
      ;

    if (errorThisNode < errorWhenEliminating) {
      errorWhenEliminating = errorThisNode;
      bestEliminationPos  = i;
    }
  }
}
    

void Model::StreamLine::eliminateOne()
{
  assert(bestEliminationPos >= 0);
  assert(bestEliminationPos < points.size());
  assert(points.size() > 2);
  points.erase(points.begin()+bestEliminationPos);
  findBestEliminationPos();
}



void load(Model &model, const std::string &fileName)
{
  std::ifstream in(fileName);
  
  int numTracks;
  in.read((char *)&numTracks,sizeof(numTracks));
  std::vector<std::pair<int,int>> tracks(numTracks);
  in.read((char *)tracks.data(),numTracks*sizeof(tracks[0]));
  
  int numPoints;
  in.read((char *)&numPoints,sizeof(numPoints));
  std::vector<vec3f> points(numPoints);
  in.read((char *)points.data(),numPoints*sizeof(points[0]));

  for (int i=0;i<tracks.size();i++) {
    Model::StreamLine line;
    int begin = tracks[i].first;
    int count = tracks[i].second;
    
    for (int j=0;j<count;j++)
      // kill replicated points here:
      if (j == 0 || points[begin+j] != points[begin+j-1])
        line.points.push_back(points[begin+j]);
    
    if (line.points.size() < 2)
      // not even a *line*, let's kill it
      continue;
    
    model.lines.push_back(line);
  }
  PRINT(model.lines.size());
}

void saveCurrent(const Model &model, const std::string &outFileBase, float target)
{
  std::cout << "saving target " << (target*100) << "%" << std::endl;
  char fileName[100];
  sprintf(fileName,"%s%03i.slbin",outFileBase.c_str(),(int)(target*100));
  std::ofstream out(fileName);
  
  std::vector<vec3f>              points;
  std::vector<std::pair<int,int>> tracks;
  for (const auto &line : model.lines) {
    tracks.push_back({int(points.size()),line.points.size()});
    for (const auto &point : line.points)
      points.push_back(point);
  }
  int numTracks = tracks.size();
  PRINT(numTracks);
  out.write((const char *)&numTracks,sizeof(numTracks));
  out.write((const char *)tracks.data(),numTracks*sizeof(tracks[0]));
  
  int numPoints = points.size();
  PRINT(numPoints);
  out.write((const char *)&numPoints,sizeof(numPoints));
  out.write((const char *)points.data(),numPoints*sizeof(points[0]));
}
                 
int main(int ac, char **av)
{
  std::string inFileName;
  std::string outBasePath;

  for (int i=1;i<ac;i++) {
    const std::string arg = av[i];
    if (arg == "-o") {
      outBasePath = av[++i];
    } else {
      inFileName = arg;
    }
  }
  if (inFileName == "" || outBasePath == "") {
    std::cout << "usage: ./simplifyStreamLines <inFile.slbin> -o <outBasePath>" << std::endl;
    exit(0);
  }
  Model model;
  load(model,inFileName);

  // initial fill poriority queue
  std::priority_queue<std::pair<float,int>> pq;
  size_t originalSize = 0;
  for (int i=0;i<model.lines.size();i++) {
    Model::StreamLine &line = model.lines[i];
    originalSize += line.points.size();
    line.findBestEliminationPos();
    if (line.bestEliminationPos >= 0)
      pq.push({-line.errorWhenEliminating,i});
  }

  PING; PRINT(pq.size());
  // queue filled, pop...
  size_t currentSize = originalSize;
  PRINT(currentSize);
  while (!pq.empty()) {
    if (currentSize <= originalSize*targets[0]) {
      saveCurrent(model,outBasePath,targets[0]);
      targets.erase(targets.begin());
      if (targets.empty()) {
        std::cout << "DONE!" << std::endl;
        exit(0);
      }
    }

    auto best = pq.top();
    int bestLineID = best.second;
    pq.pop();
    Model::StreamLine &line = model.lines[bestLineID];
    // PRINT(line.errorWhenEliminating);
    // PRINT(line.bestEliminationPos);
    // PRINT(line.points.size());
    assert(line.bestEliminationPos >= 0);
    line.eliminateOne();
    currentSize--;
    if (line.bestEliminationPos >= 0) {
      pq.push({-line.errorWhenEliminating,bestLineID});
    }
  }
  std::cout << "PQ is empty!!!" << std::endl;
}
