/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <cmath>
#include "mapgen_math.h"
#include "voxel.h"
#include "mapblock.h"
#include "mapnode.h"
#include "map.h"
#include "nodedef.h"
#include "voxelalgorithms.h"
#include "profiler.h"
#include "settings.h" // For g_settings
#include "main.h" // For g_profiler
#include "emerge.h"
#include "biome.h"

// can use ported lib from http://mandelbulber.googlecode.com/svn/trunk/src
//#include "mandelbulber/fractal.h"
//#include "mandelbulber/fractal.cpp"

double mandelbox(double x, double y, double z, double d, int nn = 10) {
	int s = 7;
	x *= s;
	y *= s;
	z *= s;
	d *= s;

	double posX = x;
	double posY = y;
	double posZ = z;

	double dr = 1.0;
	double r = 0.0;

	double scale = 2;

	double minRadius2 = 0.25;
	double fixedRadius2 = 1;

	for (int n = 0; n < nn; n++) {
		// Reflect
		if (x > 1.0)
			x = 2.0 - x;
		else if (x < -1.0)
			x = -2.0 - x;
		if (y > 1.0)
			y = 2.0 - y;
		else if (y < -1.0)
			y = -2.0 - y;
		if (z > 1.0)
			z = 2.0 - z;
		else if (z < -1.0)
			z = -2.0 - z;

		// Sphere Inversion
		double r2 = x * x + y * y + z * z;

		if (r2 < minRadius2) {
			x = x * fixedRadius2 / minRadius2;
			y = y * fixedRadius2 / minRadius2;
			z = z * fixedRadius2 / minRadius2;
			dr = dr * fixedRadius2 / minRadius2;
		} else if (r2 < fixedRadius2) {
			x = x * fixedRadius2 / r2;
			y = y * fixedRadius2 / r2;
			z = z * fixedRadius2 / r2;
			fixedRadius2 *= fixedRadius2 / r2;
		}

		x = x * scale + posX;
		y = y * scale + posY;
		z = z * scale + posZ;
		dr *= scale;
	}
	r = sqrt(x * x + y * y + z * z);
	return ((r / fabs(dr)) < d);

}

double mengersponge(double x, double y, double z, double d, int MI = 10) {

	double r = x * x + y * y + z * z;
	double scale = 3;
	int i = 0;


	for (i = 0; i < MI && r < 9; i++) {


		x = fabs(x);
		y = fabs(y);
		z = fabs(z);


		if (x - y < 0) {
			double x1 = y;
			y = x;
			x = x1;
		}
		if (x - z < 0) {
			double x1 = z;
			z = x;
			x = x1;
		}
		if (y - z < 0) {
			double y1 = z;
			z = y;
			y = y1;
		}


		x = scale * x - 1 * (scale - 1);
		y = scale * y - 1 * (scale - 1);
		z = scale * z;


		if (z > 0.5 * 1 * (scale - 1))
			z -= 1 * (scale - 1);


		r = x * x + y * y + z * z;
	}
	return ((sqrt(r)) * pow(scale, (-i)) < d);
}

double sphere(double x, double y, double z, double d, int ITR = 1) {
	return v3f(x, y, z).getLength() < d;
}


//////////////////////// Mapgen Math parameter read/write

void MapgenMathParams::readParams(Settings *settings) {
	//params = settings->getJson("mg_math");
	// can be counfigured from here.
	std::string value = "{}";
	Json::Reader reader;
	if (!reader.parse( value, params ) ) {
		errorstream  << "Failed to parse json conf var ='" << value << "' : " << reader.getFormattedErrorMessages();
	}

	if (params["generator"].empty()) params["generator"] = settings->get("mgmath_generator");
}


void MapgenMathParams::writeParams(Settings *settings) {
	//settings->setJson("mg_math", params);
	settings->set("mgmath_generator", params["generator"].asString());
}

///////////////////////////////////////////////////////////////////////////////

MapgenMath::MapgenMath(int mapgenid, MapgenParams *params_, EmergeManager *emerge) : MapgenV7(mapgenid, params_, emerge) {
	mg_params = (MapgenMathParams *)params_;
	this->flags &= ~MG_LIGHT;

	Json::Value & params = mg_params->params;
	invert = params["invert"].empty() ? 1 : params["invert"].asBool(); //params["invert"].empty()?1:params["invert"].asBool();
	size = params["size"].empty() ? 0 : params["size"].asDouble(); // = max_r
	scale = params["scale"].empty() ? 0 : params["scale"].asDouble(); //(double)1 / size;
	if(!params["center"].empty()) center = v3f(params["center"]["x"].asFloat(), params["center"]["y"].asFloat(), params["center"]["z"].asFloat()); //v3f(5, -size - 5, 5);
	iterations = params["iterations"].empty() ? 0 : params["iterations"].asInt(); //10;
	distance = params["distance"].empty() ? 0 : params["distance"].asDouble(); // = 1/size;

	func = &sphere;

	if (params["generator"].empty()) params["generator"] = "mandelbox";
	if (params["generator"].asString() == "mengersponge") {
		if (!size) size = (MAP_GENERATION_LIMIT - 1000) / 2;
		if (!iterations) iterations = 10;
		if (!distance) distance = 0.0003;
		//if (!scale) scale = (double)0.1 / size;
		//if (!distance) distance = 0.01; //10/size;//sqrt3 * bd4;
		//if (!scale) scale = 0.01; //10/size;//sqrt3 * bd4;
		//center=v3f(-size/3,-size/3+(-2*-invert),2);
		center = v3f(-size, -size, -size);
		func = &mengersponge;
	} else if (params["generator"].asString() == "mandelbox") {
		/*
			size = MAP_GENERATION_LIMIT - 1000;
			//size = 1000;
			distance = 0.01; //100/size; //0.01;
			iterations = 10;
			center = v3f(1, 1, 1); // *size/6;
		*/

		//mandelbox
		if (!size) size = 1000;
		if (!distance) distance = 0.01;
		if(params["invert"].empty()) invert = 0;
		//center=v3f(2,-size/4,2);
		//size = 10000;
		//center=v3f(size/2,-size*0.9,size/2);
		if(params["center"].empty())center = v3f(size * 0.3, -size * 0.6, size * 0.5);
		func = &mandelbox;
	} else if (params["generator"].asString() == "sphere") {
		if(params["invert"].empty()) invert = 0;
		if (!size) size = 100;
		if (!distance) distance = size;
		func = &sphere;
		if (!scale) scale = 1;
		//sphere
		//size = 1000;scale = 1;center = v3f(2,-size-2,2);
	}

	if (!iterations) iterations = 10;
	if (!size) size = 1000;
	if (!scale) scale = (double)1 / size;
	if (!distance)  distance = scale;
	if (params["center"].empty() && !center.getLength()) center = v3f(3, -size + (-5 - (-invert * 10)), 3);
	//size ||= params["size"].empty()?1000:params["size"].asDouble(); // = max_r

}


MapgenMath::~MapgenMath() {
}

//////////////////////// Map generator

int MapgenMath::generateTerrain() {

	MapNode n_air(CONTENT_AIR, LIGHT_SUN), n_water_source(c_water_source, LIGHT_SUN);
	MapNode n_stone(c_stone, LIGHT_SUN);
	u32 index = 0;
	v3s16 em = vm->m_area.getExtent();

#if 1

	/* debug
	v3f vec0 = (v3f(node_min.X, node_min.Y, node_min.Z) - center) * scale ;
	errorstream << " X=" << node_min.X << " Y=" << node_min.Y << " Z=" << node_min.Z
	            //<< " N="<< mengersponge(vec0.X, vec0.Y, vec0.Z, distance, iterations)
	            << " N=" << (*func)(vec0.X, vec0.Y, vec0.Z, distance, iterations)
	            << " Sc=" << scale << " gen=" << params["generator"].asString() << " J=" << Json::FastWriter().write(params) << std::endl;
	*/
	for (s16 z = node_min.Z; z <= node_max.Z; z++) {
		for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
			//Biome *biome = bmgr->biomes[biomemap[index]];
			u32 i = vm->m_area.index(x, node_min.Y, z);
			for (s16 y = node_min.Y; y <= node_max.Y; y++) {
				v3f vec = (v3f(x, y, z) - center) * scale ;
				double d = (*func)(vec.X, vec.Y, vec.Z, distance, iterations);
				if ((!invert && d > 0) || (invert && d == 0)  ) {
					if (vm->m_data[i].getContent() == CONTENT_IGNORE)
	//					vm->m_data[i] = (y > water_level + biome->filler) ?
		//				                MapNode(biome->c_filler) : n_stone;
						vm->m_data[i] = n_stone;
				} else if (y <= water_level) {
					vm->m_data[i] = n_water_source;
				} else {
					vm->m_data[i] = n_air;
				}
				vm->m_area.add_y(em, i, 1);
			}
		}
	}
#endif


#if 0
// mandelbulber, unfinished but works
	sFractal par;
	par.doubles.N = 10;

	par.doubles.power = 9.0;
	par.doubles.foldingSphericalFixed =  1.0;
	par.doubles.foldingSphericalMin = 0.5;
	//no par.formula = smoothMandelbox; par.doubles.N = 40; invert = 0;//no
	par.mandelbox.doubles.sharpness = 3.0;
	par.mandelbox.doubles.scale = 1;
	par.mandelbox.doubles.sharpness = 2;
	par.mandelbox.doubles.foldingLimit = 1.0;
	par.mandelbox.doubles.foldingValue = 2;

//ok	par.formula = mandelboxVaryScale4D; par.doubles.N = 50; scale = 5; invert = 1; //ok
	par.mandelbox.doubles.vary4D.scaleVary =  0.1;
	par.mandelbox.doubles.vary4D.fold = 1;
	par.mandelbox.doubles.vary4D.rPower = 1;
	par.mandelbox.doubles.vary4D.minR = 0.5;
	par.mandelbox.doubles.vary4D.wadd = 0;
	par.doubles.constantFactor = 1.0;

	par.formula = menger_sponge; par.doubles.N = 15; invert = 0; size = 30000; center = v3f(-size / 2, -size + (-2 * -invert), 2);  scale = (double)1 / size; //ok

	//double tresh = 1.5;
	//par.formula = mandelbulb2; par.doubles.N = 10; scale = (double)1/size; invert=1; center = v3f(5,-size-5,0); //ok
	//par.formula = hypercomplex; par.doubles.N = 20; scale = 0.0001; invert=1; center = v3f(0,-10001,0); //(double)50 / max_r;

	//no par.formula = trig_DE; par.doubles.N = 5;  scale = (double)10; invert=1;

	//no par.formula = trig_optim; scale = (double)10;  par.doubles.N = 4;

	//par.formula = mandelbulb2; scale = (double)1/10000; par.doubles.N = 10; invert = 1; center = v3f(1,-4201,1); //ok
	// no par.formula = tglad;

	//par.formula = xenodreambuie;  par.juliaMode = 1; par.doubles.julia.x = -1; par.doubles.power = 2.0; center=v3f(-size/2,-size/2-5,5); //ok

	par.mandelbox.doubles.vary4D.scaleVary = 0.1;
	par.mandelbox.doubles.vary4D.fold = 1;
	par.mandelbox.doubles.vary4D.minR = 0.5;
	par.mandelbox.doubles.vary4D.rPower = 1;
	par.mandelbox.doubles.vary4D.wadd = 0;
	//no par.formula = mandelboxVaryScale4D;
	par.doubles.cadd = -1.3;
	//par.formula = aexion; // ok but center
	//par.formula = benesi; par.doubles.N = 10; center = v3f(0,0,0); invert = 0; //ok

	// par.formula = bristorbrot; //ok

	v3f vec0(node_min.X, node_min.Y, node_min.Z);
	vec0 = (vec0 - center) * scale ;
	errorstream << " X=" << node_min.X << " Y=" << node_min.Y << " Z=" << node_min.Z
	            << " N=" << Compute<normal>(CVector3(vec0.X, vec0.Y, vec0.Z), par)
	            //<<" F="<< Compute<fake_AO>(CVector3(node_min.X,node_min.Y,node_min.Z), par)
	            //<<" L="<<node_min.getLength()<< " -="<<node_min.getLength() - Compute<normal>(CVector3(node_min.X,node_min.Y,node_min.Z), par)
	            << " Sc=" << scale
	            << std::endl;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
		for (s16 y = node_min.Y; y <= node_max.Y; y++) {
			u32 i = vm->m_area.index(node_min.X, y, z);
			for (s16 x = node_min.X; x <= node_max.X; x++) {
				v3f vec(x, y, z);
				vec = (vec - center) * scale ;
				//double d = Compute<fake_AO>(CVector3(x,y,z), par);
				double d = Compute<normal>(CVector3(vec.X, vec.Y, vec.Z), par);
				//if (d>0)
				// errorstream << " d=" << d  <<" v="<< vec.getLength()<< " -="<< vec.getLength() - d <<" yad="
				//<< Compute<normal>(CVector3(x,y,z), par)
				//<< std::endl;
				if ((!invert && d > 0) || (invert && d == 0)/*&& vec.getLength() - d > tresh*/ ) {
					if (vm->m_data[i].getContent() == CONTENT_IGNORE)
						vm->m_data[i] = n_stone;
				} else {
					vm->m_data[i] = n_air;
				}
				i++;
			}
		}


#endif
	return 0;
}

int MapgenMath::getGroundLevelAtPoint(v2s16 p) {
	return 0;
}
