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
#ifndef MAP_H
#define MAP_H

#include <stdio.h>
#include "map_types.h"
#include "RaycastMesh.h"

//this is the current version number to expect from the map header
#define MAP_VERSION 0x01000000

#define BEST_Z_INVALID -999999

//special value returned as 'not found'
#define NODE_NONE 65534
#define MAP_ROOT_NODE 0

typedef uint16 NodeRef;

/*typedef struct _node {
	nodeHeader head;
	unsigned int *	pfaces;
	char						mask;
	struct  _node	*	node1, *node2, *node3, *node4;
}NODE, *PNODE;*/

class Map {
public:
	static Map* LoadMapfile(const char* in_zonename, const char *directory = NULL);
	
	Map();
	~Map();
	
	bool loadMap(FILE *fp);
	
	//the result is always final, except special NODE_NONE
	NodeRef SeekNode( NodeRef _node, float x, float y ) const;
	
	//these are untested since rewrite:
	int *SeekFace( NodeRef _node, float x, float y );
	float GetFaceHeight( int _idx, float x, float y ) const;
	
	bool LocWithinNode( NodeRef _node, float x, float y ) const;
	
	//nodes to these functions must be final
	bool LineIntersectsNode( NodeRef _node, VERTEX start, VERTEX end, VERTEX *result, FACE **on = NULL) const;
	bool LineIntersectsFace( PFACE cface, VERTEX start, VERTEX end, VERTEX *result) const;
	inline float FindBestZ(VERTEX start, VERTEX *result, FACE **on = NULL) const { return FindBestZ(GetRoot(), start, result, on); }
	bool LineIntersectsZone(VERTEX start, VERTEX end, float step, VERTEX *result, FACE **on = NULL) const;
		
//	inline unsigned int		GetVertexNumber( ) {return m_Vertex; }
	inline uint32		GetFacesNumber( ) const { return m_Faces; }
//	inline PVERTEX	GetVertex( int _idx ) {return mFinalVertex + _idx;	}
	inline PFACE		GetFace( int _idx) {return mFinalFaces + _idx;		}
	inline PFACE		GetFaceFromlist( int _idx) {return &mFinalFaces[ mFaceLists[_idx] ];	}
	inline NodeRef		GetRoot( ) const { return MAP_ROOT_NODE; }
	inline PNODE		GetNode( NodeRef r ) { return( mNodes + r ); }
	
	inline float GetMinX() const { return(_minx); }
	inline float GetMaxX() const { return(_maxx); }
	inline float GetMinY() const { return(_miny); }
	inline float GetMaxY() const { return(_maxy); }
	inline float GetMinZ() const { return(_minz); }
	inline float GetMaxZ() const { return(_maxz); }
	bool LineIntersectsZoneNoZLeaps(VERTEX start, VERTEX end, float step_mag, VERTEX *result, FACE **on);
	float FindClosestZ(VERTEX p ) const;
	bool CheckLosFN(VERTEX myloc, VERTEX oloc);

private:
	float FindBestZ( NodeRef _node, VERTEX start, VERTEX *result, FACE **on = NULL) const;
//	unsigned long m_Vertex;
	uint32 m_Faces;
	uint32 m_Nodes;
	uint32 m_FaceLists;
//	PVERTEX mFinalVertex;
	PFACE mFinalFaces;
	PNODE mNodes;
	uint32 *mFaceLists;
	
	
	int mCandFaces[100];
	
	float _minz, _maxz;
	float _minx, _miny, _maxx, _maxy;
	
	static void Normalize(VERTEX *p);
	RaycastMesh *rm;

//	void	RecLoadNode( PNODE	_node, FILE *l_f );
//	void	RecFreeNode( PNODE	_node );
};

#endif

