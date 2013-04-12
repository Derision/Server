#ifndef AZONE_H
#define AZONE_H

//pull in datatypes from zone's map.h
#include "../../zone/map.h"
#include "file_loader.hpp"

/*

	Father Nitwit's zone to map file converter.

*/

#include <stdio.h>
#include <vector>
#include <map>
using namespace std;


#define COUNT_MACTHES 1

//this is the version number to put in the map header
#undef MAP_VERSION  //override this from map.h with our version
#define MAP_VERSION 0x01000000

//quadtree stopping criteria, comment any to disable them
#define MAX_QUADRANT_FACES 1000	//if box has fewer than this, stop
//#define MIN_QUADRANT_SIZE 100.0f	//if box has a dimension smaller than this, stop
#define MIN_QUADRANT_GAIN 0.3f	//minimum split ratio before stopping
#define MAX_QUADRANT_MISSES 2	//maximum number of quads which can miss their gains
								//1 or 2 make sense, others are less useful

//attepmt to trim the data a little
#define MAX_Z	3000.0		//seems to be a lot of points above this
				//if all points on poly are, kill it.
/*
 This is used for the recursive node structure
 uint16s are adequate because, worst case
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
#define MAX_POLY_VTX 24		//arbitrary, im too lazy to figure it out
					//cut a triangle at most 6 times....

enum EQFileType { S3D, EQG, UNKNOWN };

struct POLYGON {
   VERTEX /*w[MAX_POLY_VTX], */c[MAX_POLY_VTX];
   int32 count;             // w is world points, c is for camera points
};

class GPoint {
public:
	GPoint();
	GPoint(VERTEX &v);
	GPoint(float x, float y, float z);
	
	inline void operator()(float nx, float ny, float nz) { x = nx; y = ny; z = nz; }
	
	GPoint cross(const GPoint &them) const;
	float dot3(const GPoint &them) const;
	
	float x;
	float y;
	float z;

};
GPoint operator-(const GPoint &v1, const GPoint &v2);

class GVector : public GPoint {
public:
	GVector();
	GVector(const GPoint &them);
	GVector(float x, float y, float z, float w = 1.0f);
	
	inline void operator()(float nx, float ny, float nz, float nw) { x = nx; y = ny; z = nz; W = nw; }
	float dot4(const GVector &them) const;
	float dot4(const GPoint &them) const;
	void normalize();
	float length();
	
	float W;
};

struct FaceRecord {
	FILEFACE *face;
	uint32 index;
};

#define Vmin3(o, a, b, c) ((a.o<b.o)? (a.o<c.o?a.o:c.o) : (b.o<c.o?b.o:c.o))

bool SortFaceRecordHighestZ(FaceRecord a, FaceRecord b)
{
	float amz = Vmin3(z, a.face->a, a.face->b, a.face->c);
	float bmz = Vmin3(z, b.face->a, b.face->b, b.face->c);
	return bmz < amz;
}

class QTNode;

class QTBuilder {
public:
	QTBuilder();
	~QTBuilder();
	
	bool build(const char *shortname);
	bool build_eqg(const char *shortname);
	void AddPlaceable(FileLoader *fileloader, char *ZoneFileName, bool ListPlaceable=false);
	void AddPlaceableV4(FileLoader *fileloader, char *ZoneFileName, bool ListPlaceable=false);
	void RotateVertex(VERTEX &v, float XRotation, float YRotation, float ZRotation);
	void ScaleVertex(VERTEX &v, float XScale, float YScale, float ZScale);
	void TranslateVertex(VERTEX &v, float XOffset, float YOffset, float ZOffset);
	bool writeMap(const char *file);
	
	bool FaceInNode(const QTNode *q, const FILEFACE *f);
protected:
	
	void AddFace(VERTEX &v1, VERTEX &v2, VERTEX &v3);
	
	int32 ClipPolygon(POLYGON *poly, GVector *plane);
	
	//dynamic during load
//	vector<VERTEX> _VertexList;
	vector<FILEFACE> _FaceList;
	
	//static once loaded
//	uint32 vertexCount;
	uint32 faceCount;
//	VERTEX * vertexBlock;
	FILEFACE * faceBlock;
	
	VERTEX tempvtx[MAX_POLY_VTX];
	
	QTNode *_root;
	
	static void NormalizeN(FILEFACE *p);

#ifdef COUNT_MACTHES
	uint32 gEasyMatches;
	uint32 gEasyExcludes;
	uint32 gHardMatches;
	uint32 gHardExcludes;
#endif

};

//quadtree node container
class QTNode {
public:
	QTNode(QTBuilder *builder, float Tminx, float Tmaxx, float Tminy, float Tmaxy);
	~QTNode();
	
	void clearNodes();
	
	void doSplit();
	void divideYourself(int32 depth);
	
	void buildVertexes();

//	bool writeFile(FILE *out);
	
	uint32 countNodes() const;
	uint32 countFacelists() const;
	
	void fillBlocks(nodeHeader *heads, uint32 *flist, uint32 &hindex, uint32 &findex);
	
	float minx;
	float miny;
	float maxx;
	float maxy;
	uint32 nfaces;
	vector<FaceRecord> faces;
	
	/*
	quadrent definitions:
		quad 1 (node1):
		x>=0, y>=0
		quad 2 (node2):
		x<0, y>=0
		quad 3 (node3):
		x<0, y<0
		quad 4 (node4):
		x>=0, y<0
	*/
	QTNode *node1;
	QTNode *node2;
	QTNode *node3;
	QTNode *node4;
	GPoint v[8];
	bool final;
	
protected:
	QTBuilder *builder;
};


#endif
