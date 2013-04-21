/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	  You should have received a copy of the GNU General Public License
	  along with this program; if not, write to the Free Software
	  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "../common/debug.h"
#include "../common/MiscFunctions.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#ifndef WIN32
//comment this out if your worried about zone boot times and your not using valgrind
#define SLOW_AND_CRAPPY_MAKES_VALGRIND_HAPPY
#endif

#include "zone_profile.h"
#include "raycastmap.h"
#include "zone.h"
#ifdef _WINDOWS
#define snprintf	_snprintf
#endif

extern Zone* zone;

//quick functions to clean up vertex code.
#define Vmin3(o, a, b, c) ((a.o<b.o)? (a.o<c.o?a.o:c.o) : (b.o<c.o?b.o:c.o))
#define Vmax3(o, a, b, c) ((a.o>b.o)? (a.o>c.o?a.o:c.o) : (b.o>c.o?b.o:c.o))
#define ABS(x) ((x)<0?-(x):(x))

RayCastMap::RayCastMap()
{
	_minz = FLT_MAX;
	_minx = FLT_MAX;
	_miny = FLT_MAX;
	_maxz = FLT_MIN;
	_maxx = FLT_MIN;
	_maxy = FLT_MIN;
	
	m_Faces = 0;
	mFinalFaces = NULL;

	rm = NULL;
}

bool RayCastMap::loadMap(FILE *fp)
{

	mapHeader head;

	rewind(fp);

	if(fread(&head, sizeof(head), 1, fp) != 1)
		return(false);

	if(head.version != 2)
	{ 
		LogFile->write(EQEMuLog::Error, "Invalid map version 0x%8X", head.version);
		return(false);
	}
	
	LogFile->write(EQEMuLog::Status, "Map header: %u faces, %u nodes, %u facelists", head.face_count, head.node_count, head.facelist_count);

	m_Faces = head.face_count;
	
	mFinalFaces = new FACE [m_Faces];

	if(fread(mFinalFaces, sizeof(FACE), m_Faces, fp) != m_Faces)
	{
		LogFile->write(EQEMuLog::Error, "Unable to read %lu faces from map file.", m_Faces);
		return(false);
	}

	for(uint32 i = 0; i < m_Faces; i++)
	{
		if(mFinalFaces[i].vert[mFinalFaces[i].flags.maxxvert].x > _maxx)
			_maxx = mFinalFaces[i].vert[mFinalFaces[i].flags.maxxvert].x;

		if(mFinalFaces[i].vert[mFinalFaces[i].flags.minxvert].x < _minx)
			_minx = mFinalFaces[i].vert[mFinalFaces[i].flags.minxvert].x;

		if(mFinalFaces[i].vert[mFinalFaces[i].flags.maxyvert].y > _maxy)
			_maxy = mFinalFaces[i].vert[mFinalFaces[i].flags.maxyvert].y;

		if(mFinalFaces[i].vert[mFinalFaces[i].flags.minyvert].y < _miny)
			_miny = mFinalFaces[i].vert[mFinalFaces[i].flags.minyvert].y;

		if(mFinalFaces[i].vert[mFinalFaces[i].flags.maxzvert].z > _maxz)
			_maxz = mFinalFaces[i].vert[mFinalFaces[i].flags.maxzvert].z;

		if(mFinalFaces[i].vert[mFinalFaces[i].flags.minzvert].z < _minz)
			_minz = mFinalFaces[i].vert[mFinalFaces[i].flags.minzvert].z;
	}

	rm = loadRaycastMesh(fp, m_Faces,mFinalFaces);
#ifdef MAPBENCH	
	printf("Starting Benchmarks on loaded file\n");
	
	time_t StartTime = time(NULL);

	float x, y, z, Sum = 0;
	uint32 Tests = 0, Hits = 0;

	for(x = _minx; x < _maxx; x = x + 1.0f)
	{
		for(y = _miny; y < _maxy; y = y + 1.0f)
		{
			VERTEX start = {x, y, 10000};
			z = FindBestZ(start, NULL, NULL);
			
			++Tests;
			if(z != BEST_Z_INVALID)
			{
				++Hits;
				Sum += z;
			}
		}
	}
	time_t EndTime = time(NULL);

	printf("Elapsed Time: %i seconds, %i Tests, %i Hits, Sum: %f\n", EndTime - StartTime, Tests, Hits, Sum); fflush(stdout);
#endif // MAPBENCH

	LogFile->write(EQEMuLog::Status, "Loaded map: %u faces", m_Faces);
	LogFile->write(EQEMuLog::Status, "Map Bounds: (%.2f -> %.2f, %.2f -> %.2f, %.2f -> %.2f)", _minx, _maxx, _miny, _maxy, _minz, _maxz);
	
	return(true);
}

RayCastMap::~RayCastMap()
{
	if(rm)
	{
		rm->release();
		rm = 0;
	}
	safe_delete_array(mFinalFaces);
}


bool RayCastMap::LineIntersectsZone(VERTEX start, VERTEX end, float step_mag, VERTEX *result, FACE **on) const
{
	return rm->raycast(start, end, result, NULL, on);
}

float RayCastMap::FindBestZ(VERTEX p1, VERTEX *result, FACE **on) const
{
	if(on)
		*on = NULL;

	VERTEX tmp_result;	//dummy placeholder if they do not ask for a result.
	if(result == NULL)
		result = &tmp_result;

	p1.z += RuleI(Map, FindBestZHeightAdjust);

	VERTEX from = {p1.x, p1.y, p1.z};
	VERTEX to = {p1.x, p1.y, BEST_Z_INVALID};
	VERTEX normal;

	float hitDistance;

	bool hit = false;

	for(int zAttempt = 0; zAttempt < 2; ++zAttempt)
	{
		hit = rm->raycast(from, to, result, &hitDistance, on);

		if(hit)
			return result->z;

		from.z = from.z + 10;

	}

	return BEST_Z_INVALID;
}

bool RayCastMap::LineIntersectsZoneNoZLeaps(VERTEX start, VERTEX end, float step_mag, VERTEX *result, FACE **on) {
	float z = -999999;
	VERTEX step;
	VERTEX cur;
	cur.x = start.x;
	cur.y = start.y;
	cur.z = start.z;
	
	step.x = end.x - start.x;
	step.y = end.y - start.y;
	step.z = end.z - start.z;
	float factor = step_mag / sqrt(step.x*step.x + step.y*step.y + step.z*step.z);

	step.x *= factor;
	step.y *= factor;
	step.z *= factor;

	int steps = 0;

	if (step.x > 0 && step.x < 0.001f)
		step.x = 0.001f;
	if (step.y > 0 && step.y < 0.001f)
		step.y = 0.001f;
	if (step.z > 0 && step.z < 0.001f)
		step.z = 0.001f;
	if (step.x < 0 && step.x > -0.001f)
		step.x = -0.001f;
	if (step.y < 0 && step.y > -0.001f)
		step.y = -0.001f;
	if (step.z < 0 && step.z > -0.001f)
		step.z = -0.001f;
	
	//while we are not past end
	//always do this once, even if start == end.
	while(cur.x != end.x || cur.y != end.y || cur.z != end.z)
	{
		steps++;
		VERTEX me;
		me.x = cur.x;
		me.y = cur.y;
		me.z = cur.z;
		VERTEX hit;
		FACE *onhit;
		float best_z = FindBestZ(me, &hit, &onhit);
		float diff = ABS(best_z-z);
//		diff *= sign(diff);
		if (z == -999999 || best_z == -999999 || diff < 12.0)
			z = best_z;
		else
			return(true);
		//look at current location
		if(LineIntersectsZone(start, end, step_mag, result, on))
		{
			return(true);
		}
		
		//move 1 step
		if (cur.x != end.x)
			cur.x += step.x;
		if (cur.y != end.y)
			cur.y += step.y;
		if (cur.z != end.z)
			cur.z += step.z;
		
		//watch for end conditions
		if ( (cur.x > end.x && end.x >= start.x) || (cur.x < end.x && end.x <= start.x) || (step.x == 0) ) {
			cur.x = end.x;
		}
		if ( (cur.y > end.y && end.y >= start.y) || (cur.y < end.y && end.y <= start.y) || (step.y == 0) ) {
			cur.y = end.y;
		}
		if ( (cur.z > end.z && end.z >= start.z) || (cur.z < end.z && end.z < start.z) || (step.z == 0) ) {
			cur.z = end.z;
		}
	}
	
	//walked entire line and didnt run into anything...
	return(false);
}

bool RayCastMap::CheckLoS(VERTEX myloc, VERTEX oloc)
{
	return !rm->raycast(myloc, oloc, NULL, NULL, NULL);
}
