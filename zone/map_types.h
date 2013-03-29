#ifndef MAP_TYPES_H
#define MAP_TYPES_H

#pragma pack(1)

typedef struct _vertex{
//	unsigned long order;
	
	union
	{
		struct
		{
			float x;
			float y;
			float z;
		};
		float axis[3];
	};

	_vertex()
	{
		x  = y = z = 0.0f;
	};

	_vertex(float ix, float iy, float iz)
	{
		x = ix;
		y = iy;
		z = iz;
	}
	bool  operator==(const _vertex &v1) const
	{
		return((v1.x == x) && (v1.y == y) && (v1.z ==z));
	}

}VERTEX, *PVERTEX;

struct FILEFACE
{
	VERTEX a;
	VERTEX b;
	VERTEX c;
	float nx, ny, nz, nd;
};

typedef struct _face
{
	VERTEX a;
	VERTEX b;
	VERTEX c;
	float nx, ny, nz, nd;
	float minx, maxx, miny, maxy, minz, maxz;
}FACE, *PFACE;

typedef struct _mapHeader {
	uint32 version;
	uint32 face_count;
	uint16 node_count;
	uint32 facelist_count;
} mapHeader;

/*
 This is used for the recursive node structure
 unsigned shorts are adequate because, worst case
 even in a zone that is 6000x6000 with a small node
 size of 30x30, there are only 40000 nodes.
 
 quadrent definitions:
 quad 1 (nodes[0]):
 x>=0, y>=0
 quad 2 (nodes[1]):
 x<0, y>=0
 quad 3 (nodes[2]):
 x<0, y<0
 quad 4 (nodes[3]):
 x>=0, y<0
 
 */
enum {  //node flags
	nodeFinal = 0x01
	//7 more bits if theres something to use them for...
};
typedef struct _nodeHeader {
	//bounding box of this node
	//there is no reason that these could not be unsigned
	//shorts other than we have to compare them to floats
	//all over the place, so they stay floats for now.
	//changing it could save 8 bytes per node record (~320k for huge maps)
	float minx;
	float miny;
	float maxx;
	float maxy;
	
	uint8 flags;
	union {
		uint16 nodes[4];	//index 0 means NULL, not root
		struct {
			uint32 count;
			uint32 offset;
		} faces;
	};
} nodeHeader, NODE, *PNODE;

#pragma pack()
#endif