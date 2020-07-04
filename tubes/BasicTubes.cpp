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

#include "tubes/BasicTubes.h"

namespace tubes {
  
	using device::TubesGeom;

	extern "C" const char embedded_BasicTubes_programs[];

	BasicTubes::BasicTubes()
	{
		module = owlModuleCreate(context, embedded_BasicTubes_programs);
	}


	/*! the setup code for the 'all tubes are user-geom cylinders'
		variant. todo: add one with using instance transforms */
	void BasicTubes::buildModel(Tubes::SP model)
	{
		linkBuffer
			= owlDeviceBufferCreate(context,
				OWL_USER_TYPE(model->links[0]),
				model->links.size(),
				model->links.data());

		OWLVarDecl tubesVars[] = {
		  { "links",  OWL_BUFPTR, OWL_OFFSETOF(TubesGeom,links)},
		  { "radius", OWL_FLOAT , OWL_OFFSETOF(TubesGeom,radius)},
		  { /* sentinel to mark end of list */ }
		};

		OWLGeomType tubesType
			= owlGeomTypeCreate(context,
				OWL_GEOMETRY_USER,
				sizeof(TubesGeom),
				tubesVars, -1);
		owlGeomTypeSetBoundsProg(tubesType, module,
			"tubes_bounds");
		owlGeomTypeSetIntersectProg(tubesType, 0, module,
			"tubes_intersect");
		//owlGeomTypeSetBoundsProg(tubesType, module,
		//	"basicTubes_bounds");
		//owlGeomTypeSetIntersectProg(tubesType, 0, module,
		//	"basicTubes_intersect");

		owlGeomTypeSetClosestHit(tubesType, 0, module,
			"tubes_closest_hit");

		OWLGeom geom = owlGeomCreate(context, tubesType);
		owlGeomSetPrimCount(geom, model->links.size());

		owlGeomSetBuffer(geom, "links", linkBuffer);
		owlGeomSet1f(geom, "radius", model->radius);

		this->world = owlUserGeomGroupCreate(context, 1, &geom);
		owlBuildPrograms(context);
		owlGroupBuildAccel(this->world);
	}

}
