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
#include "map.h"
#include "zone.h"
#ifdef _WINDOWS
#define snprintf	_snprintf
#endif

extern Zone* zone;

//Do we believe the normals from the map file?
//you want this enabled if it dosent break things.
//#define TRUST_MAPFILE_NORMALS

//#define OPTIMIZE_QT_LOOKUPS
#define EPS 0.002f	//acceptable error

//#define DEBUG_SEEK 1
//#define DEBUG_BEST_Z 1

/*
 of note:
 it is possible to get a null node in a valid region if it
 does not have any triangles in it.
 this will dictate bahaviour on getting a null node
 TODO: listen to the above
 */

//quick functions to clean up vertex code.
#define Vmin3(o, a, b, c) ((a.o<b.o)? (a.o<c.o?a.o:c.o) : (b.o<c.o?b.o:c.o))
#define Vmax3(o, a, b, c) ((a.o>b.o)? (a.o>c.o?a.o:c.o) : (b.o>c.o?b.o:c.o))
#define ABS(x) ((x)<0?-(x):(x))

Map::Map() {
	_minz = FLT_MAX;
	_minx = FLT_MAX;
	_miny = FLT_MAX;
	_maxz = FLT_MIN;
	_maxx = FLT_MIN;
	_maxy = FLT_MIN;
	
	m_Faces = 0;
	m_Nodes = 0;
	m_FaceLists = 0;
	mFinalFaces = NULL;
	mNodes = NULL;
	mFaceLists = NULL;
}

bool Map::loadMap(FILE *fp)
{
#ifndef INVERSEXY
#warning Map files do not work without inverted XY
	return(false);
#endif

	mapHeader head;
	
	rewind(fp);

	if(fread(&head, sizeof(head), 1, fp) != 1)
		return(false);
	
	if(head.version != 0x01000000)
	{
		LogFile->write(EQEMuLog::Error, "Invalid map version 0x%8X", head.version);
		return(false);
	}
	
	LogFile->write(EQEMuLog::Status, "Map header: %u faces, %u nodes, %u facelists", head.face_count, head.node_count, head.facelist_count);

	m_Faces = head.face_count;
	m_Nodes = head.node_count;
	m_FaceLists = head.facelist_count;
	
	mFinalFaces = new FACE	[m_Faces];
	mNodes = new NODE[m_Nodes];
	mFaceLists = new uint32[m_FaceLists];
	
	FILEFACE ff;
	uint32 r;
	for(r = 0; r < m_Faces; r++)
	{
		if(fread(&ff, sizeof(FILEFACE), 1, fp) != 1)
		{
			LogFile->write(EQEMuLog::Error, "Unable to read %u faces from map file, got %u.", m_Faces, r);
			return(false);
		}
		mFinalFaces[r].flags.type = 0;
		mFinalFaces[r].vert[0] = ff.a;
		mFinalFaces[r].vert[1] = ff.b;
		mFinalFaces[r].vert[2] = ff.c;
		mFinalFaces[r].nx = ff.nx;
		mFinalFaces[r].ny = ff.ny;
		mFinalFaces[r].nz = ff.nz;
		mFinalFaces[r].nd = ff.nd;

		mFinalFaces[r].flags.minxvert = 0;
		mFinalFaces[r].flags.maxxvert = 0;
		mFinalFaces[r].flags.minyvert = 0;
		mFinalFaces[r].flags.maxyvert = 0;
		mFinalFaces[r].flags.minzvert = 0;
		mFinalFaces[r].flags.maxzvert = 0;

		uint8 NumberOfFaces = (mFinalFaces[r].flags.type == 1) ? 4 : 3;

		for(int i = 1; i < NumberOfFaces; ++i)
		{
			if(mFinalFaces[r].vert[i].x < mFinalFaces[r].vert[mFinalFaces[r].flags.minxvert].x)
				mFinalFaces[r].flags.minxvert = i;

			if(mFinalFaces[r].vert[i].y < mFinalFaces[r].vert[mFinalFaces[r].flags.minyvert].y)
				mFinalFaces[r].flags.minyvert = i;

			if(mFinalFaces[r].vert[i].z < mFinalFaces[r].vert[mFinalFaces[r].flags.minzvert].z)
				mFinalFaces[r].flags.minzvert = i;

			if(mFinalFaces[r].vert[i].x > mFinalFaces[r].vert[mFinalFaces[r].flags.maxxvert].x)
				mFinalFaces[r].flags.maxxvert = i;

			if(mFinalFaces[r].vert[i].y > mFinalFaces[r].vert[mFinalFaces[r].flags.maxyvert].y)
				mFinalFaces[r].flags.maxyvert = i;

			if(mFinalFaces[r].vert[i].z > mFinalFaces[r].vert[mFinalFaces[r].flags.maxzvert].z)
				mFinalFaces[r].flags.maxzvert = i;
		}
	
	}
	
	for(r = 0; r < m_Nodes; r++)
	{
		if(fread(mNodes+r, sizeof(NODE), 1, fp) != 1)
		{
			LogFile->write(EQEMuLog::Error, "Unable to read %u nodes from map file, got %u.", m_Nodes, r);
			return(false);
		}
	}
	
	for(r = 0; r < m_FaceLists; r++)
	{
		if(fread(mFaceLists+r, sizeof(uint32), 1, fp) != 1)
		{
			LogFile->write(EQEMuLog::Error, "Unable to read %u face lists from map file, got %u.", m_FaceLists, r);
			return(false);
		}
	}
	
	uint32 i;
	float v;
	for(i = 0; i < m_Faces; i++) {
		v = Vmax3(x, mFinalFaces[i].vert[0], mFinalFaces[i].vert[1], mFinalFaces[i].vert[2]);
		if(v > _maxx)
			_maxx = v;
		v = Vmin3(x, mFinalFaces[i].vert[0], mFinalFaces[i].vert[1], mFinalFaces[i].vert[2]);
		if(v < _minx)
		{
			_minx = v;
		}
		v = Vmax3(y, mFinalFaces[i].vert[0], mFinalFaces[i].vert[1], mFinalFaces[i].vert[2]);
		if(v > _maxy)
			_maxy = v;
		v = Vmin3(y, mFinalFaces[i].vert[0], mFinalFaces[i].vert[1], mFinalFaces[i].vert[2]);
		if(v < _miny)
			_miny = v;
		v = Vmax3(z, mFinalFaces[i].vert[0], mFinalFaces[i].vert[1], mFinalFaces[i].vert[2]);
		if(v > _maxz)
			_maxz = v;
		v = Vmin3(z, mFinalFaces[i].vert[0], mFinalFaces[i].vert[1], mFinalFaces[i].vert[2]);
		if(v < _minz)
			_minz = v;
	}
	
	LogFile->write(EQEMuLog::Status, "Loaded map: %u vertices, %u faces", m_Faces*3, m_Faces);
	LogFile->write(EQEMuLog::Status, "Map BB: (%.2f -> %.2f, %.2f -> %.2f, %.2f -> %.2f)", _minx, _maxx, _miny, _maxy, _minz, _maxz);

	return(true);
}

Map::~Map()
{
	safe_delete_array(mFinalFaces);
	safe_delete_array(mNodes);
	safe_delete_array(mFaceLists);
}


NodeRef Map::SeekNode( NodeRef node_r, float x, float y ) const {
	if(node_r == NODE_NONE || node_r >= m_Nodes) {
		return(NODE_NONE);
	}
	PNODE _node = &mNodes[node_r];
	if( x>= _node->minx && x<= _node->maxx && y>= _node->miny && y<= _node->maxy ) {
		if( _node->flags & nodeFinal ) {
			return node_r;
		}
		//NOTE: could precalc these and store them in node headers

		NodeRef tmp = NODE_NONE;
#ifdef OPTIMIZE_QT_LOOKUPS
		float midx = _node->minx + (_node->maxx - _node->minx) * 0.5;
		float midy = _node->miny + (_node->maxy - _node->miny) * 0.5;
		//follow ordering rules from map.h...
		if(x < midx) {
			if(y < midy) { //quad 3
				if(_node->nodes[2] != NODE_NONE && _node->nodes[2] != node_r)
					tmp = SeekNode( _node->nodes[2], x, y );
			} else {	//quad 2
				if(_node->nodes[2] != NODE_NONE && _node->nodes[1] != node_r)
					tmp = SeekNode( _node->nodes[1], x, y );
			}
		} else {
			if(y < midy) {  //quad 4
				if(_node->nodes[2] != NODE_NONE && _node->nodes[3] != node_r)
					tmp = SeekNode( _node->nodes[3], x, y );
			} else {	//quad 1
				if(_node->nodes[2] != NODE_NONE && _node->nodes[0] != node_r)
					tmp = SeekNode( _node->nodes[0], x, y );
			}
		}
		if( tmp != NODE_NONE ) return tmp;
#else
		if(_node->nodes[0] == node_r) return(NODE_NONE); 	//prevent infinite recursion
		tmp = SeekNode( _node->nodes[0], x, y );
		if( tmp != NODE_NONE ) return tmp;
		if(_node->nodes[1] == node_r) return(NODE_NONE); 	//prevent infinite recursion
		tmp = SeekNode( _node->nodes[1], x, y );
		if( tmp != NODE_NONE ) return tmp;
		if(_node->nodes[2] == node_r) return(NODE_NONE); 	//prevent infinite recursion
		tmp = SeekNode( _node->nodes[2], x, y );
		if( tmp != NODE_NONE ) return tmp;
		if(_node->nodes[3] == node_r) return(NODE_NONE); 	//prevent infinite recursion
		tmp = SeekNode( _node->nodes[3], x, y );
		if( tmp != NODE_NONE ) return tmp;
#endif

	}
	return(NODE_NONE);
}

// maybe precalc edges.
int* Map::SeekFace(  NodeRef node_r, float x, float y ) {
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(NULL);
	}
	const PNODE _node = &mNodes[node_r];
	if(!(_node->flags & nodeFinal)) {
		return(NULL);   //not a final node... could find the proper node...
	}
	
	float	dx,dy;
	float	nx,ny;
	int		*face = mCandFaces;
	unsigned long i;
	for( i=0;i<_node->faces.count;i++ ) {
		const FACE &cf = mFinalFaces[ mFaceLists[_node->faces.offset + i] ];
		const VERTEX &v1 = cf.vert[0];
		const VERTEX &v2 = cf.vert[1];
		const VERTEX &v3 = cf.vert[2];
		
		dx = v2.x - v1.x; dy = v2.y - v1.y;
		nx =    x - v1.x; ny =    y - v1.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		dx = v3.x - v2.x; dy = v3.y - v2.y;
		nx =    x - v2.x; ny =    y - v2.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		dx = v1.x - v3.x; dy = v1.y - v3.y;
		nx =    x - v3.x; ny =    y - v3.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		*face++ = mFaceLists[_node->faces.offset + i];
	}
	*face = -1;
	return mCandFaces;
}

// can be op?
float Map::GetFaceHeight( int _idx, float x, float y ) const {
	const PFACE	pface = &mFinalFaces[ _idx ];
	return ( pface->nd - x * pface->nx - y * pface->ny ) / pface->nz;
}

//Algorithm stolen from internet
//p1=start of segment
//p2=end of segment

bool Map::LineIntersectsZone(VERTEX start, VERTEX end, float step_mag, VERTEX *result, FACE **on) const
{
	_ZP(Map_LineIntersectsZone);
	// Cast a ray from start to end, checking for collisions in each node between the two points.
	//
	float stepx, stepy, stepz, curx, cury, curz;

	curx = start.x;
	cury = start.y;
	curz = start.z;

	VERTEX cur = start;
		
	stepx = end.x - start.x;
	stepy = end.y - start.y;
	stepz = end.z - start.z;

	if((stepx == 0) && (stepy == 0) && (stepz == 0))
		return false;

	float factor =  sqrt(stepx*stepx + stepy*stepy + stepz*stepz);

	stepx = (stepx/factor)*step_mag;
	stepy = (stepy/factor)*step_mag;
	stepz = (stepz/factor)*step_mag;

	NodeRef cnode, lnode, finalnode;
	lnode = NODE_NONE;      //last node visited

	cnode = SeekNode(GetRoot(), start.x, start.y);
	finalnode = SeekNode(GetRoot(), end.x, end.y);
	if(cnode == finalnode)
		return LineIntersectsNode(cnode, start, end, result, on);

	do {

		stepx = (float)end.x - curx;
		stepy = (float)end.y - cury;
		stepz = (float)end.z - curz;

		factor =  sqrt(stepx*stepx + stepy*stepy + stepz*stepz);

		stepx = (stepx/factor)*step_mag;
		stepy = (stepy/factor)*step_mag;
		stepz = (stepz/factor)*step_mag;
			
	    	cnode = SeekNode(GetRoot(), curx, cury);
		if(cnode != lnode)
		{
 		    	lnode = cnode;

			if(cnode == NODE_NONE)
				return false;

			if(LineIntersectsNode(cnode, start, end, result, on))
       				return(true);
       	       		
			if(cnode == finalnode)
				return false;
		}
		curx += stepx;
		cury += stepy;
		curz += stepz;

		cur.x = curx;
		cur.y = cury;
		cur.z = curz;

		if(ABS(curx - end.x) < step_mag) cur.x = end.x;
		if(ABS(cury - end.y) < step_mag) cur.y = end.y;
		if(ABS(curz - end.z) < step_mag) cur.z = end.z;

	} while(cur.x != end.x || cur.y != end.y || cur.z != end.z);

	return false;
}

bool Map::LocWithinNode( NodeRef node_r, float x, float y ) const {
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(false);
	}
	const PNODE _node = &mNodes[node_r];
	//this function exists so nobody outside of MAP needs to know
	//how the NODE sturcture works
	return( x>= _node->minx && x<= _node->maxx && y>= _node->miny && y<= _node->maxy );
}

bool Map::LineIntersectsNode( NodeRef node_r, VERTEX p1, VERTEX p2, VERTEX *result, FACE **on) const {
	_ZP(Map_LineIntersectsNode);
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(true);   //can see through empty nodes, just allow LOS on error...
	}
	const PNODE _node = &mNodes[node_r];
	if(!(_node->flags & nodeFinal)) {
		return(true);   //not a final node... not sure best action
	}
	
	unsigned long i;
	
	PFACE cur;
	const uint32 *cfl = mFaceLists + _node->faces.offset;
	
	for(i = 0; i < _node->faces.count; i++) {
		if(*cfl > m_Faces)
			continue;	//watch for invalid lists, they seem to happen
		cur = &mFinalFaces[ *cfl ];
		if(LineIntersectsFace(cur,p1, p2, result)) {
			if(on != NULL)
				*on = cur;
			return(true);
		}
		cfl++;
	}

	return(false);
}


float Map::FindBestZ( NodeRef node_r, VERTEX p1, VERTEX *result, FACE **on) const {
	_ZP(Map_FindBestZ);

	if(on)
		*on = NULL;

	VERTEX tmp_result;	//dummy placeholder if they do not ask for a result.
	if(result == NULL)
		result = &tmp_result;

	p1.z += RuleI(Map, FindBestZHeightAdjust);

	if(RuleB(Map, UseClosestZ))
		return FindClosestZ(p1);

	if(node_r == GetRoot()) {
		node_r = SeekNode(node_r, p1.x, p1.y);
	}
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(BEST_Z_INVALID);
	}
	const PNODE _node = &mNodes[node_r];
	if(!(_node->flags & nodeFinal)) {
		return(BEST_Z_INVALID);   //not a final node... could find the proper node...
	}
	
	
	VERTEX p2(p1);
	p2.z = BEST_Z_INVALID;
	
	float best_z = BEST_Z_INVALID;
	int zAttempt;

	unsigned long i;

	PFACE cur;

	// If we don't find a bestZ on the first attempt, we try again from a position CurrentZ + 10 higher
	// This is in case the pathing between waypoints temporarily sends the NPC below ground level.
	// 
	for(zAttempt=1; zAttempt<=2; zAttempt++) {

		const uint32 *cfl = mFaceLists + _node->faces.offset;

		for(i = 0; i < _node->faces.count; i++) {
			if(*cfl > m_Faces)
		               continue;       //watch for invalid lists, they seem to happen, e.g. in eastwastes.map

			cur = &mFinalFaces[ *cfl ];

			if(cur->vert[cur->flags.minzvert].z > p1.z)
			{	
				cfl++;
				continue;
			}

			if(cur->vert[cur->flags.maxxvert].x < p1.x)
			{
				cfl++;
				continue;
			}

			if(cur->vert[cur->flags.minxvert].x > p1.x)
			{
				cfl++;
				continue;
			}

			if(cur->vert[cur->flags.maxyvert].y < p1.y)
			{
				cfl++;
				continue;
			}

			if(cur->vert[cur->flags.minyvert].y > p1.y)
			{
				cfl++;
				continue;
			}
			if(LineIntersectsFace(cur, p1, p2, result)) {
				if (result->z > best_z) {
					if(on != NULL)
						*on = cur;
					best_z = result->z;
				}
			}
			cfl++;
		}

		if(best_z != BEST_Z_INVALID) return best_z;

		 p1.z = p1.z + 10 ;   // If we can't find a best Z, the NPC is probably just under the world. Try again from 10 units higher up.
	}

	return best_z;
}


bool Map::LineIntersectsFace( PFACE cface, VERTEX p1, VERTEX p2, VERTEX *result) const {
	if( cface == NULL ) {
		return(false);  //cant intersect a face we dont have... i guess
	}
	
	const VERTEX &pa = cface->vert[0];
	const VERTEX &pb = cface->vert[1];
	const VERTEX &pc = cface->vert[2];
	
	//quick bounding box checks
	//float tbb;
	
	if(p1.x < cface->vert[cface->flags.minxvert].x && p2.x < cface->vert[cface->flags.minxvert].x)
		return(false);
	if(p1.y < cface->vert[cface->flags.minyvert].y && p2.y < cface->vert[cface->flags.minyvert].y)
		return(false);
	if(p1.z < cface->vert[cface->flags.minzvert].z && p2.z < cface->vert[cface->flags.minzvert].z)
		return(false);
	
	if(p1.x > cface->vert[cface->flags.maxxvert].x && p2.x > cface->vert[cface->flags.maxxvert].x)
		return(false);
	if(p1.y > cface->vert[cface->flags.maxyvert].y && p2.y > cface->vert[cface->flags.maxyvert].y)
		return(false);
	if(p1.z > cface->vert[cface->flags.maxzvert].z && p2.z > cface->vert[cface->flags.maxzvert].z)
		return(false);
	
	float d;
	float denom,mu;
	VERTEX n, intersect;
	
	VERTEX *p = &intersect;
	if(result != NULL)
		p = result;

	// Calculate the parameters for the plane
	//recalculate from points
#ifndef TRUST_MAPFILE_NORMALS
	n.x = (pb.y - pa.y)*(pc.z - pa.z) - (pb.z - pa.z)*(pc.y - pa.y);
	n.y = (pb.z - pa.z)*(pc.x - pa.x) - (pb.x - pa.x)*(pc.z - pa.z);
	n.z = (pb.x - pa.x)*(pc.y - pa.y) - (pb.y - pa.y)*(pc.x - pa.x);
	Normalize(&n);
	d = - n.x * pa.x - n.y * pa.y - n.z * pa.z;
#else
	//use precaled data from .map file
	n.x = cface->nx;
	n.y = cface->ny;
	n.z = cface->nz;
	d = cface->nd;
#endif
	
	//try inverting the normals...
	n.x = -n.x;
	n.y = -n.y;
	n.z = -n.z;
	d = - n.x * pa.x - n.y * pa.y - n.z * pa.z;	//recalc
	
	
	// Calculate the position on the line that intersects the plane
	denom = n.x * (p2.x - p1.x) + n.y * (p2.y - p1.y) + n.z * (p2.z - p1.z);
	if (ABS(denom) < EPS)         // Line and plane don't intersect
	  return(false);
	mu = - (d + n.x * p1.x + n.y * p1.y + n.z * p1.z) / denom;
	if (mu < 0 || mu > 1)   // Intersection not along line segment
	  return(false);
	p->x = p1.x + mu * (p2.x - p1.x);
	p->y = p1.y + mu * (p2.y - p1.y);
	p->z = p1.z + mu * (p2.z - p1.z);
	
	n.x = -n.x;
	n.y = -n.y;
	n.z = -n.z;
	VERTEX pa1,pa2,pa3, tmp;
	float t;
	
	//pa1 = pa - p
	pa1.x = pa.x - p->x;
	pa1.y = pa.y - p->y;
	pa1.z = pa.z - p->z;
	
	//pa2 = pb - p
	pa2.x = pb.x - p->x;
	pa2.y = pb.y - p->y;
	pa2.z = pb.z - p->z;
	
	//tmp = pa1 cross pa2
	tmp.x = pa1.y * pa2.z - pa1.z * pa2.y;
	tmp.y = pa1.z * pa2.x - pa1.x * pa2.z;
	tmp.z = pa1.x * pa2.y - pa1.y * pa2.x;
	
	//t = tmp dot n
	t = tmp.x * n.x + tmp.y * n.y + tmp.z * n.z;
	if(t < 0)
		return(false);
	
	//pa3 = pb - p
	pa3.x = pc.x - p->x;
	pa3.y = pc.y - p->y;
	pa3.z = pc.z - p->z;
	
	//tmp = pa2 cross pa3
	tmp.x = pa2.y * pa3.z - pa2.z * pa3.y;
	tmp.y = pa2.z * pa3.x - pa2.x * pa3.z;
	tmp.z = pa2.x * pa3.y - pa2.y * pa3.x;
	
	//t = tmp dot n
	t = tmp.x * n.x + tmp.y * n.y + tmp.z * n.z;
	if(t < 0)
		return(false);
	
	//tmp = pa3 cross pa1
	tmp.x = pa3.y * pa1.z - pa3.z * pa1.y;
	tmp.y = pa3.z * pa1.x - pa3.x * pa1.z;
	tmp.z = pa3.x * pa1.y - pa3.y * pa1.x;
	
	//t = tmp dot n
	t = tmp.x * n.x + tmp.y * n.y + tmp.z * n.z;
	if(t < 0)
		return(false);
	
	return(true);
}

void Map::Normalize(VERTEX *p) {
	float len = sqrtf(p->x*p->x + p->y*p->y + p->z*p->z);
	p->x /= len;
	p->y /= len;
	p->z /= len;
}

bool Map::LineIntersectsZoneNoZLeaps(VERTEX start, VERTEX end, float step_mag, VERTEX *result, FACE **on) {
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
	
	NodeRef cnode, lnode;
	lnode = 0;
	//while we are not past end
	//always do this once, even if start == end.
	while(cur.x != end.x || cur.y != end.y || cur.z != end.z)
	{
		steps++;
		cnode = SeekNode(GetRoot(), cur.x, cur.y);
		if (cnode == NODE_NONE)
		{
			return(true);
		}		
		VERTEX me;
		me.x = cur.x;
		me.y = cur.y;
		me.z = cur.z;
		VERTEX hit;
		FACE *onhit;
		float best_z = FindBestZ(cnode, me, &hit, &onhit);
		float diff = ABS(best_z-z);
//		diff *= sign(diff);
		if (z == -999999 || best_z == -999999 || diff < 12.0)
			z = best_z;
		else
			return(true);
		//look at current location
		if(cnode != NODE_NONE && cnode != lnode) {
			if(LineIntersectsNode(cnode, start, end, result, on))
			{
				return(true);
			}
			lnode = cnode;
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

float Map::FindClosestZ(VERTEX p ) const
{
	// Unlike FindBestZ, this method finds the closest Z value above or below the specified point.
	//
	std::list<float> ZSet;

	NodeRef NodeR = SeekNode(MAP_ROOT_NODE, p.x, p.y);
	
	if( NodeR == NODE_NONE || NodeR >= m_Nodes)
		return 0;
		
	PNODE CurrentNode = &mNodes[NodeR];

	if(!(CurrentNode->flags & nodeFinal))
		return 0; 
		   
	VERTEX p1(p), p2(p), Result;

	p1.z = 999999;
	
	p2.z = BEST_Z_INVALID;

	const uint32 *CurrentFaceList = mFaceLists + CurrentNode->faces.offset;

	for(unsigned long i = 0; i < CurrentNode->faces.count; ++i)
	{
		if(*CurrentFaceList > m_Faces)
			continue;

		PFACE CurrentFace = &mFinalFaces[ *CurrentFaceList ];

		if(CurrentFace->nz > 0 && LineIntersectsFace(CurrentFace, p1, p2, &Result))
			ZSet.push_back(Result.z);
		
		CurrentFaceList++;

	}
	if(ZSet.size() == 0)
		return 0;

	if(ZSet.size() == 1)
		return ZSet.front();

	float ClosestZ = -999999;

	for(list<float>::iterator Iterator = ZSet.begin(); Iterator != ZSet.end(); ++Iterator)
	{
		if(ABS(p.z - (*Iterator)) < ABS(p.z - ClosestZ))
				ClosestZ = (*Iterator);
	}

	return ClosestZ;
}

bool Map::CheckLoS(VERTEX myloc, VERTEX oloc)
{
	FACE *onhit;
	NodeRef mynode;
	NodeRef onode;
	
	VERTEX hit;
	//see if anything in our node is in the way
	mynode = SeekNode(GetRoot(), myloc.x, myloc.y);
	if(mynode != NODE_NONE) {
		if(LineIntersectsNode(mynode, myloc, oloc, &hit, &onhit)) {
#if LOSDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target position, cannot see.", GetName());
			LogFile->write(EQEMuLog::Debug, "\tPoly: (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
				onhit->a.x, onhit->a.y, onhit->a.z,
				onhit->b.x, onhit->b.y, onhit->b.z, 
				onhit->c.x, onhit->c.y, onhit->c.z);
#endif
			return(false);
		}
	}
#if LOSDEBUG>=5
	 else {
		LogFile->write(EQEMuLog::Debug, "WTF, I have no node, what am I standing on??? (%.2f, %.2f).", myloc.x, myloc.y);
	}
#endif
	
	//see if they are in a different node.
	//if so, see if anything in their node is blocking me.
	if(!LocWithinNode(mynode, oloc.x, oloc.y)) {
		onode = SeekNode(GetRoot(), oloc.x, oloc.y);
		if(onode != NODE_NONE && onode != mynode) {
			if(LineIntersectsNode(onode, myloc, oloc, &hit, &onhit)) {
#if LOSDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target position, cannot see (2).", GetName());
			LogFile->write(EQEMuLog::Debug, "\tPoly: (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
				onhit->a.x, onhit->a.y, onhit->a.z,
				onhit->b.x, onhit->b.y, onhit->b.z, 
				onhit->c.x, onhit->c.y, onhit->c.z);
#endif
				return(false);
			}
		}
#if LOSDEBUG>=5
		 else if(onode == NODE_NONE) {
			LogFile->write(EQEMuLog::Debug, "WTF, They have no node, what are they standing on??? (%.2f, %.2f).", myloc.x, myloc.y);
		}
#endif
	}
	
#if LOSDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target position, CAN SEE.", GetName());
#endif
	
	return(true);
}
