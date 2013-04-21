#ifndef AZONE_H
#define AZONE_H

#include "../../zone/map2.h"
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
#define MAP_VERSION 0x00000002

#define MAX_POLY_VTX 24		//arbitrary, im too lazy to figure it out
					//cut a triangle at most 6 times....

enum EQFileType { S3D, EQG, UNKNOWN };

struct POLYGON {
   VERTEX /*w[MAX_POLY_VTX], */c[MAX_POLY_VTX];
   int32 count;             // w is world points, c is for camera points
};

struct FaceRecord {
	FACE *face;
	uint32 index;
};


#define Vmin3(o, a, b, c) ((a.o<b.o)? (a.o<c.o?a.o:c.o) : (b.o<c.o?b.o:c.o))
#define Vmax3(o, a, b, c) ((a.o>b.o)? (a.o>c.o?a.o:c.o) : (b.o>c.o?b.o:c.o))

class RCMBuilder {
public:
	RCMBuilder();
	~RCMBuilder();
	
	bool build(const char *shortname);
	bool build_eqg(const char *shortname);
	void AddPlaceable(FileLoader *fileloader, char *ZoneFileName, bool ListPlaceable=false);
	void AddPlaceableV4(FileLoader *fileloader, char *ZoneFileName, bool ListPlaceable=false);
	void RotateVertex(VERTEX &v, float XRotation, float YRotation, float ZRotation);
	void ScaleVertex(VERTEX &v, float XScale, float YScale, float ZScale);
	void TranslateVertex(VERTEX &v, float XOffset, float YOffset, float ZOffset);
	bool writeMap(const char *file);
	
protected:
	static void NormalizeN(FACE *p);	
	void AddFace(VERTEX &v1, VERTEX &v2, VERTEX &v3);
	void AddFace(VERTEX &v1, VERTEX &v2, VERTEX &v3, VERTEX &v4);
	
	vector<FACE> _FaceList;
	
	//static once loaded
	uint32 faceCount;
	FACE * faceBlock;
	
	VERTEX tempvtx[MAX_POLY_VTX];

	RaycastMesh *rm;
	
};

#endif

