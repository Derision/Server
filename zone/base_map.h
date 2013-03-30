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
#ifndef BASE_MAP_H
#define BASE_MAP_H

#include <stdio.h>
#include "map_types.h"


class BaseMap
{
public:
	virtual bool LineIntersectsFace( PFACE cface, VERTEX start, VERTEX end, VERTEX *result) const = 0;
	virtual float FindBestZ(VERTEX start, VERTEX *result, FACE **on = NULL) const = 0;
	virtual bool LineIntersectsZone(VERTEX start, VERTEX end, float step, VERTEX *result, FACE **on = NULL) const = 0;
	virtual PFACE GetFace( int _idx) = 0;
	virtual bool LineIntersectsZoneNoZLeaps(VERTEX start, VERTEX end, float step_mag, VERTEX *result, FACE **on) = 0;
	virtual bool CheckLosFN(VERTEX myloc, VERTEX oloc) = 0;
	virtual bool loadMap(FILE *fp) = 0;

private:
};

#endif

