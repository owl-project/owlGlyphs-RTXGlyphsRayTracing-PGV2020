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

struct Model {
  struct StreamLine {
    std::vector<vec3f> points;
  };
  std::vector<StreamLine> lines;
};

void load(Model &model, const std::string &fileName)
{
  if (fileName.substr(fileName.size()-4) == ".trk") {
    std::cout << "found trackvis file" << std::endl;
    TrackVisData::SP tvd = TrackVisData::read(fileName);
    model.lines.resize(tvd->tracks.size());
    for (int lineID=0;lineID<tvd->tracks.size();lineID++) {
      Model::StreamLine &line = model.lines[lineID];
      auto &track = tvd->tracks[lineID];
      for (int i=0;i<track.size;i++) {
        line.points.push_back(tvd->points[track.begin+i]);
      }
    }
    PRINT(model.lines.size());
    return;
  }

  int numTracks,numPoints;
  std::vector<std::pair<int,int>> tracks;
  std::ifstream in(fileName);
  in.read((char *)&numTracks,sizeof(numTracks));
  PRINT(numTracks);
  tracks.resize(numTracks);
  in.read((char *)tracks.data(),numTracks*sizeof(tracks[0]));
  model.lines.resize(numTracks);
  
  in.read((char *)&numPoints,sizeof(numPoints));
  PRINT(numPoints);
  for (int i=0;i<tracks.size();i++) {
    Model::StreamLine &line = model.lines[i];
    int num = tracks[i].second;
    for (int j=0;j<num;j++) {
      vec3f p;
      in.read((char*)&p,sizeof(p));
      line.points.push_back(p);
    }
  }
}

void save(const Model &model, const std::string &outFileName)
{
  std::ofstream out(outFileName);
  
  std::vector<vec3f>              points;
  std::vector<std::pair<int,int>> tracks;
  for (const auto &line : model.lines) {
    tracks.push_back({int(points.size()),line.points.size()});
    for (const auto &point : line.points)
      points.push_back(point);
  }
  int numTracks = tracks.size();
  out.write((const char *)&numTracks,sizeof(numTracks));
  out.write((const char *)tracks.data(),numTracks*sizeof(tracks[0]));
  
  int numPoints = points.size();
  out.write((const char *)&numPoints,sizeof(numPoints));
  out.write((const char *)points.data(),numPoints*sizeof(points[0]));
}
                 
int main(int ac, char **av)
{
  std::string inFileName;
  std::string outFileName;

  for (int i=1;i<ac;i++) {
    const std::string arg = av[i];
    if (arg == "-o") {
      outFileName = av[++i];
    } else {
      inFileName = arg;
    }
  }
  if (inFileName == "" || outFileName == "") {
    std::cout << "usage: ./trackVisToSLBin <inFile.trk> -o <outFileName>" << std::endl;
    exit(0);
  }
  Model model;
  load(model,inFileName);
  save(model,outFileName);
}
