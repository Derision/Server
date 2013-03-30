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
#ifndef RAYCASTMAP_H
#define RAYCASTMAP_H

#include <stdio.h>
#include "map_types.h"
#include "base_map.h"
#include "RaycastMesh.h"

class RayCastMap : public BaseMap
{
public:
	RayCastMap();
	~RayCastMap();
	
	 float FindBestZ(VERTEX start, VERTEX *result, FACE **on = NULL) const;
	 bool LineIntersectsZone(VERTEX start, VERTEX end, float step, VERTEX *result, FACE **on = NULL) const;
	 PFACE GetFace( int _idx) {return mFinalFaces + _idx;		}
	 bool LineIntersectsZoneNoZLeaps(VERTEX start, VERTEX end, float step_mag, VERTEX *result, FACE **on);
	 bool CheckLosFN(VERTEX myloc, VERTEX oloc);
	bool loadMap(FILE *fp);

private:

	uint32 m_Faces;
	PFACE mFinalFaces;
	
	float _minz, _maxz;
	float _minx, _miny, _maxx, _maxy;

	RaycastMesh *rm;
};

#endif

