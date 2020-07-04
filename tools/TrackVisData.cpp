// ======================================================================== //
// Copyright 2019 Ingo Wald                                                 //
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

// owl
#include "TrackVisData.h"
// std
#include <fstream>

namespace tubes {
  
  // const int SUB_SAMPLE = 4;

  /*
    Content of the header
    Name	Data type	Bytes 	Comment
    id_string[6]	char	6 	ID string for track file. The first 5 characters must be "TRACK".
    dim[3]	short int	6 	Dimension of the image volume.
    voxel_size[3]	float	12 	Voxel size of the image volume.
    origin[3]	float	12 	Origin of the image volume. This field is not yet being used by TrackVis. That means the origin is always (0, 0, 0).
    n_scalars	short int	2 	Number of scalars saved at each track point (besides x, y and z coordinates).
    scalar_name[10][20]	char	200 	Name of each scalar. Can not be longer than 20 characters each. Can only store up to 10 names.
    n_properties	short int	2 	Number of properties saved at each track.
    property_name[10][20]	char	200 	Name of each property. Can not be longer than 20 characters each. Can only store up to 10 names.
    vox_to_ras[4][4]	float	64 	4x4 matrix for voxel to RAS (crs to xyz) transformation. If vox_to_ras[3][3] is 0, it means the matrix is not recorded. This field is added from version 2.
    reserved[444]	char	444 	Reserved space for future version.
    voxel_order[4]	char	4 	Storing order of the original image data. Explained here.
    pad2[4]	char	4 	Paddings.
    image_orientation_patient[6]	float	24 	Image orientation of the original image. As defined in the DICOM header.
    pad1[2]	char	2 	Paddings.
    invert_x	unsigned char	1 	Inversion/rotation flags used to generate this track file. For internal use only.
    invert_y	unsigned char	1 	As above.
    invert_x	unsigned char	1 	As above.
    swap_xy	unsigned char	1 	As above.
    swap_yz	unsigned char	1 	As above.
    swap_zx	unsigned char	1 	As above.
    n_count	int	4 	Number of tracks stored in this track file. 0 means the number was NOT stored.
    version	int	4 	Version number. Current version is 2.
    hdr_size	int	4 	Size of the header. Used to determine byte swap. Should be 1000.*/
  struct TrackVisHeader {
    char id_string[6];
    short int dim[3];
    float voxel_size[3];
    float origin[3];
    short int n_scalars;
    char scalar_name[10][20];
    short int n_properties;
    char property_name[10][20];
    float vox_to_ras[4][4];
    char reserved[444];
    char voxel_order[4];
    char pad2[4];
    float image_orientation_patient[6];
    char pad1[2];
    unsigned char invert_x;
    unsigned char invert_y;
    unsigned char invert_z;
    unsigned char swap_xy;
    unsigned char swap_yz;
    unsigned char swap_zx;
    int n_count;
    int version;
    int hdr_size;
  };

  void TrackVisData::readTrack(std::ifstream &in, int numScalars, int numProps)
  {
    int numSubPoints = -1;
    in.read((char*)&numSubPoints,sizeof(numSubPoints));
    int begin = points.size();
    for (int subPointID=0;subPointID<numSubPoints;subPointID++) {
#if SUB_SAMPLE
      bool skip
        = (subPointID % SUB_SAMPLE > 0)
        && !(subPointID == (numSubPoints-1));
#else
      bool skip = false;
#endif
      vec3f pos;
      in.read((char *)&pos,sizeof(pos));
      if (!skip)points.push_back(pos);
      for (int scalarID=0;scalarID<numScalars;scalarID++) {
        float scalar;
        in.read((char *)&scalar,sizeof(scalar));
        if (!skip)
          scalarFields[scalarID].perPoint.push_back(scalar);
      }
    }
    tracks.push_back({begin,int(points.size()-begin)});
    for (int propID=0;propID<numProps;propID++) {
      float prop;
      in.read((char *)&prop,sizeof(prop));
      propertyFields[propID].perTrack.push_back(prop);
    }
  }

  TrackVisData::SP TrackVisData::read(const std::string &fileName)
  {
    TrackVisData::SP data = std::make_shared<TrackVisData>();
    data->readFile(fileName);
    return data;
  }

  void TrackVisData::readFile(const std::string &fileName)
  {
    size_t maxTracks = -1;
    const char *maxTracksFromEnv
      = getenv("TV_MAX_TRACKS");
    if (maxTracksFromEnv)
      maxTracks = std::atol(maxTracksFromEnv);
    
    std::ifstream in(fileName,std::ios::binary);
    TrackVisHeader header;
    in.read((char *)&header,sizeof(header));
    scalarFields.resize(header.n_scalars);
    for (int i=0;i<header.n_scalars;i++) {
      scalarFields[i].name = header.scalar_name[i];
      std::cout << "scalar #" << i
                << " is '" << scalarFields[i].name << "'"
                << std::endl;
    }
    propertyFields.resize(header.n_properties);
    for (int i=0;i<header.n_properties;i++) {
      propertyFields[i].name = header.property_name[i];
      std::cout << "property #" << i
                << " is '" << propertyFields[i].name << "'"
                << std::endl;
    }

    PRINT(header.n_count);
    for (int i=0;i<std::min(maxTracks,size_t(header.n_count));i++) {
      readTrack(in,header.n_scalars,header.n_properties);
      if (!in.good())
        throw std::runtime_error("read partial file!?");
    }

    std::cout << "done reading TrackVis file; found "
              << prettyNumber(tracks.size()) << " tracks with "
              << prettyNumber(points.size()) << " points" << std::endl;
#if 0
    const std::string outFileName = "/space/out.slbin";
    std::ofstream out(outFileName);
    int numTracks = tracks.size();
    out.write((const char *)&numTracks,sizeof(numTracks));
    out.write((const char *)tracks.data(),numTracks*sizeof(tracks[0]));
    
    int numPoints = points.size();
    out.write((const char *)&numPoints,sizeof(numPoints));
    out.write((const char *)points.data(),numPoints*sizeof(points[0]));
    std::cout << "written in slbin format to " << outFileName << std::endl;
    // std::cout << "SUBSAMPLE WAS " << SUB_SAMPLE << std::endl;
    
    exit(0);
#endif
  }

  box3f TrackVisData::getBounds() const
  {
    box3f bounds;
    for (auto point : points)
      bounds.extend(point);
    return bounds;
  }

}




