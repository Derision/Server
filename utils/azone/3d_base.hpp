#ifndef __OPENEQ_THREED_BASE__
#define __OPENEQ_THREED_BASE__


#pragma pack(1)


#include "archive.hpp"

#ifdef GLMODELVIEWER
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#endif

struct Light {
  float x, y, z;
  float r, g, b;
  float rad;
} typedef Light;

struct Vertex {
	float x, y, z;
	float i, j, k;
	float u, v;

//	int32 bone;

	Vertex()
	{
		x = y = z = i = j = k = u = v = 0.0f;
	};

	Vertex(float ix, float iy, float iz)
	{
		x = ix;
		y = iy;
		z = iz;
	}
} typedef Vertex;


struct Polygon
{
  uint8 type;	// 0 = triangle, 1 = quad
  int32 flags;
  int32 v1, v2, v3, v4;
  int32 tex;

  Polygon() { type = 0; }
} typedef Polygon;


struct Texture {
  Archive *archive;
  char frame_count;
  char current_frame;
#ifdef GLMODELVIEWER
  GLuint *tex;
#else
  int32 *tex;
#endif
  char **filenames;

  char flags;
} typedef Texture;

class Model {
public:
  Model() { IncludeInMap = false; verts = nullptr; vert_count = 0; polys = nullptr; poly_count = 0; tex = nullptr; tex_count = 0; name = nullptr; }
  ~Model() {}

  Vertex **verts;
  Polygon **polys;
  Texture **tex;

  int32 vert_count, poly_count, tex_count;

  char *name;
  bool IncludeInMap; // Include in EQEmu .map file
};


class Placeable {
public:
	Placeable() {}

	Placeable(int32 iModel, float ix, float iy, float iz, float irx, float iry, float irz, float iScale0, float iScale1, float iScale2)
	{
		model = iModel;
		x = ix;
		y = iy;
		z = iz;
		rx = irx;
		ry = iry;
		rz = irz;
		scale[0] = iScale0;
		scale[1] = iScale1;
		scale[2] = iScale2;
	}

	float x, y, z;
	float rx, ry, rz;
	float scale[3];
	char *model_name;
	int32 model;
};

#pragma pack()

#endif
