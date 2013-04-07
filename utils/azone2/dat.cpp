
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "file.hpp"
#include "dat.hpp"
#include "../../zone/map.h"
//#define DEBUGDAT

#define VARSTRUCT_DECODE_TYPE(Type, Buffer) *(Type *)Buffer; Buffer += sizeof(Type);
#define VARSTRUCT_DECODE_STRING(String, Buffer) strcpy(String, Buffer); Buffer += strlen(String)+1;
#define VARSTRUCT_ENCODE_STRING(Buffer, String) sprintf(Buffer, String); Buffer += strlen(String) + 1;
#define VARSTRUCT_ENCODE_INTSTRING(Buffer, Number) sprintf(Buffer, "%i", Number); Buffer += strlen(Buffer) + 1;
#define VARSTRUCT_ENCODE_TYPE(Type, Buffer, Value) *(Type *)Buffer = Value; Buffer += sizeof(Type);
#define VARSTRUCT_SKIP_TYPE(Type, Buffer) Buffer += sizeof(Type);


float HeightWithinQuad(VERTEX p1, VERTEX p2, VERTEX p3, VERTEX p4, float x, float y)
{
	// This function returns the height of the given x/y point with the quad defined by p1,p2,p3 and p4.
	//
	// It is assumed that the point does in fact lie somewhere within the quad. If for some reason it doesn't,
	// the program will abort, as thre is a logic flaw.
	//
	/*
	printf("Quad:	%8.3f, %8.3f, %8.3f\n", p1.x, p1.y, p1.z);
	printf("	%8.3f, %8.3f, %8.3f\n", p2.x, p2.y, p2.z);
	printf("	%8.3f, %8.3f, %8.3f\n", p3.x, p3.y, p3.z);
	printf("	%8.3f, %8.3f, %8.3f\n", p4.x, p4.y, p4.z);
	printf("\n\nPoint:	%8.3f, %8.3f\n", x, y);
	*/

	FILEFACE f;

	int32 inTriangle = 0;

	float fAB = (y-p1.y) * (p2.x-p1.x) - (x-p1.x) * (p2.y-p1.y);
	float fBC = (y-p2.y) * (p3.x-p2.x) - (x-p2.x) * (p3.y-p2.y);
	float fCA = (y-p3.y) * (p1.x-p3.x) - (x-p3.x) * (p1.y-p3.y);

	if((fAB * fBC >= 0) && (fBC * fCA >= 0))
	{
		inTriangle = 1;
		f.a = p1;
		f.b = p2;
		f.c = p3;
	}

	fAB = (y-p1.y) * (p3.x-p1.x) - (x-p1.x) * (p3.y-p1.y);
	fBC = (y-p3.y) * (p4.x-p3.x) - (x-p3.x) * (p4.y-p3.y);
	fCA = (y-p4.y) * (p1.x-p4.x) - (x-p4.x) * (p1.y-p4.y);


	if((fAB * fBC >= 0) && (fBC * fCA >= 0))
	{
		inTriangle = 2;
		f.a = p1;
		f.b = p3;
		f.c = p4;
	}

	if(inTriangle == 0)
	{
		printf("Something went wrong. Aborting in HeightWithinQuad\n");
		exit(0);
	}

	f.nx = (f.b.y - f.a.y)*(f.c.z - f.a.z) - (f.b.z - f.a.z)*(f.c.y - f.a.y);
	f.ny = (f.b.z - f.a.z)*(f.c.x - f.a.x) - (f.b.x - f.a.x)*(f.c.z - f.a.z);
	f.nz = (f.b.x - f.a.x)*(f.c.y - f.a.y) - (f.b.y - f.a.y)*(f.c.x - f.a.x);

	float len = sqrt(f.nx*f.nx + f.ny*f.ny + f.nz*f.nz);

	f.nx /= len;
	f.ny /= len;
	f.nz /= len;

	return (((f.nx) * (x - f.a.x) + (f.ny) * (y - f.a.y)) / -f.nz) + f.a.z;
}


string GetToken(uchar *&Buffer, int32 &Position)
{
	// This is used to return each Token, keyword, etc. from an EQG v4 .zon or .tog file
	//
	string Token;

	while((Buffer[Position] == 9) || (Buffer[Position] == 32))
		++Position;

	while((Buffer[Position] != 10) && (Buffer[Position] != 9) && (Buffer[Position] != 32))
	{
		if(Buffer[Position] > 32)
			Token = Token + (char)Buffer[Position];

		++Position;
	}
	++Position;
	//printf("Returning %s\n", Token.c_str());	
	return Token;
}

int32 AddModelName(Content_3D *C3D, string ModelName)
{
	vector<string>::iterator Iterator;
	
	for(uint32 i = 0; i < C3D->ModelNames.size(); ++i)
	{
#ifndef WIN32
		if(!strcasecmp(C3D->ModelNames[i].c_str(), ModelName.c_str()))
#else
	   	if(!_stricmp(C3D->ModelNames[i].c_str(), ModelName.c_str()))
#endif
			return i;
	}

	
	
	C3D->ModelNames.push_back(ModelName);

	return C3D->ModelNames.size() - 1;
}

DATLoader::DATLoader()
{
	this->buffer = NULL;
	this->buf_len = -1;
	this->archive = NULL;
	this->status = 0;
	GenerateQuads = false;
}

DATLoader::~DATLoader()
{
	this->Close();
}

void DATLoader::ReturnQuads()
{
	GenerateQuads = true;
}

int32 DATLoader::Open(char *base_path, char *zone_name, Archive *archive) {

#ifdef DEBUGDAT
	printf("DATLoader::Open %s, [%s]\n", base_path, zone_name);
	fflush(stdout);
#endif
	uint32 i, j, mat_count = 0;

	Zone_Model *zm;

	char *buffer, *buf_start;
	int32 buf_len, bone_count;

	char *filename;

	if(zone_name[strlen(zone_name) - 4] == '.')
		filename = zone_name;
	else
	{
		filename = new char[strlen(zone_name) + 5];
		sprintf(filename, "%s.ter", zone_name);
	}

	if(!GetFile((uchar **)&buffer, &buf_len, base_path, filename, archive))
	{
		if(filename != zone_name)
	    		delete[] filename;
		return 0;
	}
#ifdef DEBUGDAT
	printf("Filename = %s, zonename = %s\n", filename, zone_name);
#endif
	if(filename != zone_name)
		delete[] filename;

  
	buf_start = buffer;

	//TODO: Find out what these three unknowns are
	int32 Unknown1 = VARSTRUCT_DECODE_TYPE(uint32, buffer);
	int32 Unknown2 = VARSTRUCT_DECODE_TYPE(uint32, buffer);
	int32 Unknown3 = VARSTRUCT_DECODE_TYPE(uint32, buffer);

#ifdef DEBUGDAT
	printf("Unknowns: %i, %i, %i\n", Unknown1, Unknown2, Unknown3);
#endif

	char s[255];

	VARSTRUCT_DECODE_STRING(s, buffer)

#ifdef DEBUGDAT
	printf("('%s',)\n", s);
#endif

	int32 TileCount = VARSTRUCT_DECODE_TYPE(uint32, buffer);

#ifdef DEBUGDAT
	printf("TileCount is  %i\n", TileCount);
#endif

	// Most of these default values will be overwritten when we read the .zon file
	//
	int32 QuadsPerTile = 32;

	float MinLAT = -24;
	float MaxLAT = -1;
	float MinLNG = 0;
	float MaxLNG = 62;

	float UnitsPerVert = 5.0;

	float ZoneMinX = 0;
	float ZoneMaxX = 0;

	float ZoneMinY = 0;
	float ZoneMaxY = 0;

	float MinExtentX = -999999, MinExtentY = -999999, MinExtentZ = -999999;
	float MaxExtentX = 999999, MaxExtentY = 999999, MaxExtentZ = 999999;

	// Parse .zon file.
	//
	uchar *ZonBuffer;

	int32 ZonBufferLength, ZonPosition;

	if(!GetFile((uchar **)&ZonBuffer, &ZonBufferLength, base_path, "*.zon", archive))
	{
		printf("Unable to open .zon\n");
		exit(0);
	}
	else
	{
		ZonPosition = 0;

		while(ZonPosition < ZonBufferLength)
		{
			string Token = GetToken(ZonBuffer, ZonPosition);

			if(Token == "*MINLNG")
			{
				Token = GetToken(ZonBuffer, ZonPosition);
				MinLNG = atof(Token.c_str());
#ifdef DEBUGDAT
				printf("Set MinLNG to %8.3f\n", MinLNG);
#endif
			}
			if(Token == "*MAXLNG")
			{
				Token = GetToken(ZonBuffer, ZonPosition);
				MaxLNG = atof(Token.c_str());
#ifdef DEBUGDAT
				printf("Set MaxLNG to %8.3f\n", MaxLNG);
#endif
			}
			if(Token == "*MINLAT")
			{
				Token = GetToken(ZonBuffer, ZonPosition);
				MinLAT = atof(Token.c_str());
#ifdef DEBUGDAT
				printf("Set MinLAT to %8.3f\n", MinLAT);
#endif
			}
			if(Token == "*MAXLAT")
			{
				Token = GetToken(ZonBuffer, ZonPosition);
				MaxLAT = atof(Token.c_str());
#ifdef DEBUGDAT
				printf("Set MaxLAT to %8.3f\n", MaxLAT);
#endif
			}
			if(Token == "*UNITSPERVERT")
			{
				Token = GetToken(ZonBuffer, ZonPosition);
				UnitsPerVert = atof(Token.c_str());
#ifdef DEBUGDAT
				printf("Set UnitsPerVert to %8.3f\n", UnitsPerVert);
#endif
			}
			if(Token == "*QUADSPERTILE")
			{
				Token = GetToken(ZonBuffer, ZonPosition);
				QuadsPerTile = atoi(Token.c_str());
#ifdef DEBUGDAT
				printf("Set QuadsPerTile to %i\n", QuadsPerTile);
#endif
			}
			if(Token == "*MIN_EXTENTS")
			{
				Token = GetToken(ZonBuffer, ZonPosition);
				MinExtentX = atof(Token.c_str());
				Token = GetToken(ZonBuffer, ZonPosition);
				MinExtentY = atof(Token.c_str());
				Token = GetToken(ZonBuffer, ZonPosition);
				MinExtentZ = atof(Token.c_str());
#ifdef DEBUGDAT
				printf("Set MinExtents to %8.3f, %8.3f, %8.3f\n", MinExtentX, MinExtentY, MinExtentZ);
#endif
			}
			if(Token == "*MAX_EXTENTS")
			{
				Token = GetToken(ZonBuffer, ZonPosition);
				MaxExtentX = atof(Token.c_str());
				Token = GetToken(ZonBuffer, ZonPosition);
				MaxExtentY = atof(Token.c_str());
				Token = GetToken(ZonBuffer, ZonPosition);
				MaxExtentZ = atof(Token.c_str());
#ifdef DEBUGDAT
				printf("Set MaxExtents to %8.3f, %8.3f, %8.3f\n", MaxExtentX, MaxExtentY, MaxExtentZ);
#endif
			}
		}
	}

	float UnitsPerVertX = UnitsPerVert;
	float UnitsPerVertY = UnitsPerVert;

	float LATRange = (MaxLAT - MinLAT);
	float LNGRange = (MaxLNG - MinLNG);

	ZoneMinX = MinLAT * QuadsPerTile * UnitsPerVert;
	ZoneMaxX = (MaxLAT + 1) * QuadsPerTile * UnitsPerVert;

	ZoneMinY = MinLNG * QuadsPerTile * UnitsPerVert;
	ZoneMaxY = (MaxLNG + 1) * QuadsPerTile * UnitsPerVert;

	int32 QuadCount = (QuadsPerTile * QuadsPerTile);
	int32 VertCount = ((QuadsPerTile + 1) * (QuadsPerTile + 1));
#ifdef DEBUGDAT
	printf("X Range: %8.3f to %8.3f\n", ZoneMinX, ZoneMaxX);
	printf("Y Range: %8.3f to %8.3f\n", ZoneMinY, ZoneMaxY);


	printf("UnitsPerVertX = %8.3f, UnitsPerVertY = %8.3f\n", UnitsPerVertX, UnitsPerVertY);

	printf("LATRange is from %8.3f to %8.3f (%8.3f), x = %8.3f to %8.3f (%8.3f)\n",
		MinLAT, MaxLAT, LATRange, ZoneMinX, ZoneMaxX, ZoneMaxX - ZoneMinX);

#endif
	int32 VertexNumber = -1;
	int32 PolyNumber = -1;

	this->model_data.zone_model = new Zone_Model;
	zm = this->model_data.zone_model;
	
	zm->vert_count = QuadCount * TileCount * 4;
	zm->poly_count = QuadCount * 2 * TileCount;
	
	zm->verts = new Vertex *[zm->vert_count];
	zm->polys = new Polygon *[zm->poly_count];

	this->model_data.plac_count = 0;
	this->model_data.model_count = 0;
	this->model_data.ObjectGroupCount = 0;


	float *Floats = new float[VertCount];
	int32 *Color1 = new int[VertCount];
	int32 *Color2 = new int[VertCount];
	uint8 *Bytes = new uint8[QuadCount];

	for(int32 TileNumber = 0; TileNumber < TileCount; ++TileNumber)
	{
		
		int32 TileLNG = VARSTRUCT_DECODE_TYPE(uint32, buffer);
		int32 TileLAT = VARSTRUCT_DECODE_TYPE(uint32, buffer);
		int32 TileUNK = VARSTRUCT_DECODE_TYPE(uint32, buffer);

		float TileYStart = ZoneMinY + (TileLNG - 100000 - MinLNG) * UnitsPerVertY * QuadsPerTile;
		float TileXStart = ZoneMinX + (TileLAT - 100000 - MinLAT) * UnitsPerVertX * QuadsPerTile;

#ifdef DEBUGDAT
		printf("TileXStart = %8.3f + %f * %f * %i\n", ZoneMinX, (TileLAT - 100000 - MinLAT) , UnitsPerVertX, QuadsPerTile);


		printf("****** Tile # %5d header %6d, %6d, %6d (LNG: %6d, LAT: %6d)\n", TileNumber, TileLNG, TileLAT, TileUNK,
			TileLNG - 100000, TileLAT - 100000);

		printf("TileXStart = %8.3f, TileYStart = %8.3f\n", TileXStart, TileYStart);
		printf("TileXEnd = %8.3f, TileYEnd = %8.3f\n", TileXStart + QuadsPerTile * UnitsPerVertX,
			TileYStart + QuadsPerTile * UnitsPerVertY);
#endif

		// Height Values
		//
		bool AllFloatsTheSame = true;

		for(int32 i = 0; i < VertCount; ++i)
		{
			Floats[i] = VARSTRUCT_DECODE_TYPE(float, buffer);
			if((i > 0) && (Floats[i] != Floats[0]))
				AllFloatsTheSame = false;
		}


		// It is an assumption that these next two groups are Color info, but they look like it.
		//
		for(int32 i = 0; i < VertCount; ++i)
		{
			Color1[i] = VARSTRUCT_DECODE_TYPE(uint32, buffer);
		}

		for(int32 i = 0; i < VertCount; ++i)
		{
			Color2[i] = VARSTRUCT_DECODE_TYPE(uint32, buffer);
		}

		// The low order bit of this group of bytes indicates whether the corresponding quad should
		// be rendered or not.
		for(int32 i = 0; i < QuadCount; ++i)
		{
			Bytes[i] = VARSTRUCT_DECODE_TYPE(uint8, buffer);
		}


		float UnkFloat = VARSTRUCT_DECODE_TYPE(float, buffer);

		int32 UnkUnk = VARSTRUCT_DECODE_TYPE(uint32, buffer);

		buffer -= 4;

		float UnkUnk2 = VARSTRUCT_DECODE_TYPE(float, buffer);
#ifdef DEBUGDAT
		printf("Unknowns  %.1f %i (%8.3f)\n", UnkFloat, UnkUnk, UnkUnk2);
#endif

		if(UnkUnk > 0)
		{
			int8 UnkByte = VARSTRUCT_DECODE_TYPE(uint8, buffer);
#ifdef DEBUGDAT
			printf("Byte is  (%i,)\n", UnkByte);
#endif
			if(UnkByte > 0)
			{
				float f1 = VARSTRUCT_DECODE_TYPE(float, buffer);
				float f2 = VARSTRUCT_DECODE_TYPE(float, buffer);
				float f3 = VARSTRUCT_DECODE_TYPE(float, buffer);
				float f4 = VARSTRUCT_DECODE_TYPE(float, buffer);
#ifdef DEBUGDAT
				printf("Floats:  %.1f %.1f %1.f %.1f\n", f1, f2, f3, f4);
#endif
			}

			float f1 = VARSTRUCT_DECODE_TYPE(float, buffer);
#ifdef DEBUGDAT
			printf("Float:  (%.1f,)\n", f1);
#endif
		}

		int32 LayerCount = VARSTRUCT_DECODE_TYPE(uint32, buffer);

		VARSTRUCT_DECODE_STRING(s, buffer);
		
		int32 OverlayCount = 0;

		for(int32 Layer = 1; Layer < LayerCount; ++Layer)
		{
			VARSTRUCT_DECODE_STRING(s, buffer);

			int32 Size = VARSTRUCT_DECODE_TYPE(uint32, buffer);

			for(int32 b = 0; b < (Size * Size); ++b)
			{
				uint8 Byte = VARSTRUCT_DECODE_TYPE(uint8, buffer);
			}
				
			++OverlayCount;

		}	

		int32 Count1 = VARSTRUCT_DECODE_TYPE(uint32, buffer);
#ifdef DEBUGDAT
		printf("Count 1 is  (%iL,)\n", Count1);
#endif

		// The Count1 section is a list of single placeable models
		//
		for(int32 j = 0; j < Count1; ++j)
		{
			char ModelName[255];

			VARSTRUCT_DECODE_STRING(ModelName, buffer);
#ifdef DEBUGDAT
			printf("%s\n", ModelName);
#endif
			int32 ModelNumber = AddModelName(&model_data, ModelName);

			VARSTRUCT_DECODE_STRING(s, buffer);
#ifdef DEBUGDAT
			printf("%s\n", s);
#endif

			// Although the LNG/LAT is included for each model within this tile, the values always appear to be
			// the same as for the parent tile.
			//
			int32 Longitude = VARSTRUCT_DECODE_TYPE(uint32, buffer);

			int32 Latitude = VARSTRUCT_DECODE_TYPE(uint32, buffer);

			float x = VARSTRUCT_DECODE_TYPE(float, buffer);
			float y = VARSTRUCT_DECODE_TYPE(float, buffer);
			float z = VARSTRUCT_DECODE_TYPE(float, buffer);

			float RotX = VARSTRUCT_DECODE_TYPE(float, buffer);
			float RotY = VARSTRUCT_DECODE_TYPE(float, buffer);
			float RotZ = VARSTRUCT_DECODE_TYPE(float, buffer);

			float ScaleX = VARSTRUCT_DECODE_TYPE(float, buffer);
			float ScaleY = VARSTRUCT_DECODE_TYPE(float, buffer);
			float ScaleZ = VARSTRUCT_DECODE_TYPE(float, buffer);

			VARSTRUCT_DECODE_TYPE(uint8, buffer);

			Placeable p(ModelNumber, 0, 0, 0, RotX, RotY, RotZ, ScaleX, ScaleY, ScaleZ);

			model_data.PlaceableList.push_back(p);
#ifdef DEBUGDAT
			printf("Model: %s LOC: %8.3f, %8.3f, %8.3f, ROT: %8.3f, %8.3f, %8.3f\n", ModelName, x, y, z, RotX, RotY, RotZ);
#endif
			ObjectGroupEntry NewObjectGroup;
			
			NewObjectGroup.FromTOG = false;

			NewObjectGroup.y = y;
			NewObjectGroup.x = x;
			NewObjectGroup.z = z;

			NewObjectGroup.TileX = TileYStart;
			NewObjectGroup.TileY = TileXStart;

			// The objects in the Count1 section have a Z offset relative to the ground level, so we calculate the
			// ground level within this quad for the given x/y offsets. However ...
			//
			// Some objects within a tile have an X/Y offset specified such that they actually sit within the bounds
			// of another tile, which means based on the data for the tile we are processing, we should be unable to
			// calculate the Z value for that object.
			//
			// By hand editing an .EQG, it was discovered that what the client appears to do is calculate the Z value 
			// for these objects by adjusting the x/y offsets by +/- (QuadsPerTile * UnitsPerVert) until the offsets
			// are within this tiles boundaries and using the corresponding height values.
			//
			// For example, if a model has an offset of -157.5, -157.5 and QPT * UPV = 160, then the height value used 
			// would be the height at +2.5, +2.5 in this tile.
			//
			// This doesn't make a great deal of sense, but it is how the EQ graphics engine appears to do things.
			//
			// These 'Adjusted' x/y offsets are used only for determing the ground Z for the object. The actual placement
			// of the object relative to the terrain uses the unadjusted offsets.
			// 
			// For the record, as of SoF, this applies to the following zones and objects:
			//
			//	arcstone: Model translations adjusted from  -14.000,  219.000 to  146.000,   59.000
			//	arcstone: Model translations adjusted from  159.692,  256.589 to  159.692,   96.589
			//	arcstone: Model translations adjusted from  122.314,  170.098 to  122.314,   10.098
			//	elddar: Model translations adjusted from  144.980,  396.406 to  144.980,   76.406
			//	elddar: Model translations adjusted from  167.534, -193.584 to    7.534,  126.416
			//	kithicor: Model translations adjusted from   18.217,  166.512 to   18.217,    6.512
			//	hro.out: Model translations adjusted from   38.249,   -4.658 to   38.249,  155.342
			//	steppes: Model translations adjusted from  101.380,  380.000 to  101.380,  124.000
			//	steppes: Model translations adjusted from   32.770, -210.000 to   32.770,   46.000
			//	steppes: Model translations adjusted from  560.000,  150.000 to   48.000,   22.000
			//	steppes: Model translations adjusted from  420.000,  445.000 to   36.000,   61.000
			//
			float TerrainHeight = 0;

			float AdjustedX = x;

			float AdjustedY = y;

			if(AdjustedX < 0)
				AdjustedX = AdjustedX + (-(int32)(AdjustedX / (UnitsPerVertX * QuadsPerTile)) + 1) * (UnitsPerVertX * QuadsPerTile);
			else
				AdjustedX = fmod(AdjustedX, UnitsPerVertX * QuadsPerTile);

			if(AdjustedY < 0)
				AdjustedY = AdjustedY + (-(int32)(AdjustedY / (UnitsPerVertX * QuadsPerTile)) + 1) * (UnitsPerVertX * QuadsPerTile);
			else
				AdjustedY = fmod(AdjustedY, UnitsPerVertY * QuadsPerTile);

#ifdef DEBUGDAT
			if((AdjustedX != x) || (AdjustedY != y))
				printf(" Model translations adjusted from %8.3f, %8.3f to %8.3f, %8.3f\n", x, y, AdjustedX, AdjustedY);
#endif

			//	Calculated which quad the adjusted x/y coordinated of the object lie in.
			//
			int32 RowNumber = (int32)(AdjustedY / UnitsPerVertY);

			int32 Column = (int32)(AdjustedX / UnitsPerVertX);

			int32 Quad = RowNumber * QuadsPerTile + Column;

			float QuadVertex1Z = Floats[Quad + RowNumber];
			float QuadVertex2Z = Floats[Quad + RowNumber + QuadsPerTile + 1];
			float QuadVertex3Z = Floats[Quad + RowNumber + QuadsPerTile + 2];
			float QuadVertex4Z = Floats[Quad + RowNumber + 1];

			VERTEX P1(RowNumber * UnitsPerVertX, (Quad % QuadsPerTile) * UnitsPerVertY, QuadVertex1Z);
			VERTEX P2(P1.x + UnitsPerVertX, P1.y, QuadVertex2Z);
			VERTEX P3(P1.x + UnitsPerVertX, P1.y + UnitsPerVertY, QuadVertex3Z);
			VERTEX P4(P1.x, P1.y + UnitsPerVertY, QuadVertex4Z);

			TerrainHeight = HeightWithinQuad(P1, P2, P3, P4, AdjustedY, AdjustedX);

#ifdef DEBUGDAT
			printf("XY: %8.3f, %8.3f Row: %i, Columm: %i, Quad: %i\n", x, y, RowNumber, Column, Quad);
			printf("Zs: %8.3f, %8.3f, %8.3f, %8.3f\n", QuadVertex1Z, QuadVertex2Z, QuadVertex3Z, QuadVertex4Z);
			printf("Height is %8.3f\n", TerrainHeight);
#endif
			NewObjectGroup.TileZ = TerrainHeight;

			NewObjectGroup.RotX = 0;
			NewObjectGroup.RotY = 0;
			NewObjectGroup.RotZ = 0;

			NewObjectGroup.ScaleX = 1;
			NewObjectGroup.ScaleY = 1;
			NewObjectGroup.ScaleZ = 1;

			NewObjectGroup.SubObjects.push_back(model_data.PlaceableList.size()-1);

			model_data.ObjectGroups.push_back(NewObjectGroup);
#ifdef DEBUGDAT
			printf("ObjectGroup %i created.\n", model_data.ObjectGroups.size()-1);
#endif

		}

		// We don't currently do anything with the Count2 objects.
		// Things like water, teleports, zonelines seem to be in here, judging from the strings.
		//
		int32 Count2 = VARSTRUCT_DECODE_TYPE(uint32, buffer);
#ifdef DEBUGDAT
		printf("Count 2 is  (%iL,)\n", Count2);
#endif
		for(int32 j = 0; j < Count2; ++j)
		{
			VARSTRUCT_DECODE_STRING(s, buffer);
#ifdef DEBUGDAT
			printf("%s\n", s);
#endif

			VARSTRUCT_DECODE_TYPE(uint32, buffer);

			VARSTRUCT_DECODE_STRING(s, buffer);
#ifdef DEBUGDAT
			printf("%s\n", s);
#endif

			int32 Longitude = VARSTRUCT_DECODE_TYPE(uint32, buffer);
			int32 Latitude = VARSTRUCT_DECODE_TYPE(uint32, buffer);

			float x = VARSTRUCT_DECODE_TYPE(float, buffer);
			float y = VARSTRUCT_DECODE_TYPE(float, buffer);
			float z = VARSTRUCT_DECODE_TYPE(float, buffer);

			float RotX = VARSTRUCT_DECODE_TYPE(float, buffer);
			float RotY = VARSTRUCT_DECODE_TYPE(float, buffer);
			float RotZ = VARSTRUCT_DECODE_TYPE(float, buffer);

			float ScaleX = VARSTRUCT_DECODE_TYPE(float, buffer);
			float ScaleY = VARSTRUCT_DECODE_TYPE(float, buffer);
			float ScaleZ = VARSTRUCT_DECODE_TYPE(float, buffer);

			float SizeX = VARSTRUCT_DECODE_TYPE(float, buffer);
			float SizeY = VARSTRUCT_DECODE_TYPE(float, buffer);
			float SizeZ = VARSTRUCT_DECODE_TYPE(float, buffer);
		}

		int32 Count3 = VARSTRUCT_DECODE_TYPE(uint32, buffer);

		// We don't currently do anything with the Count 3 data. Mostly seems to be camp fires and suchlike.
		//
#ifdef DEBUGDAT
		printf("Count 3 is  (%iL,)\n", Count3);
#endif
		for(int32 j = 0; j < Count3; ++j)
		{
			VARSTRUCT_DECODE_STRING(s, buffer);
#ifdef DEBUGDAT
			printf("%s\n", s);
#endif
			VARSTRUCT_DECODE_STRING(s, buffer);
#ifdef DEBUGDAT
			printf("%s\n", s);
#endif

			VARSTRUCT_DECODE_TYPE(uint8, buffer);

			int32 Longitude = VARSTRUCT_DECODE_TYPE(uint32, buffer);
			int32 Latitude = VARSTRUCT_DECODE_TYPE(uint32, buffer);

			float x = VARSTRUCT_DECODE_TYPE(float, buffer);
			float y = VARSTRUCT_DECODE_TYPE(float, buffer);
			float z = VARSTRUCT_DECODE_TYPE(float, buffer);

			float RotX = VARSTRUCT_DECODE_TYPE(float, buffer);
			float RotY = VARSTRUCT_DECODE_TYPE(float, buffer);
			float RotZ = VARSTRUCT_DECODE_TYPE(float, buffer);

			float ScaleX = VARSTRUCT_DECODE_TYPE(float, buffer);
			float ScaleY = VARSTRUCT_DECODE_TYPE(float, buffer);
			float ScaleZ = VARSTRUCT_DECODE_TYPE(float, buffer);

			VARSTRUCT_DECODE_TYPE(float, buffer);
		}

		int32 Count4 = VARSTRUCT_DECODE_TYPE(uint32, buffer);

		// The data in this section mostly references .tog (Object Group) files.
		//
#ifdef DEBUGDAT
		printf("Count 4 is  (%iL,)\n", Count4);
#endif
		for(int32 j = 0; j < Count4; ++j)
		{
			char TogName[255];

			VARSTRUCT_DECODE_STRING(TogName, buffer);
#ifdef DEBUGDAT
			printf("%s\n", TogName);
#endif

			int32 Longitude = VARSTRUCT_DECODE_TYPE(uint32, buffer);
			int32 Latitude = VARSTRUCT_DECODE_TYPE(uint32, buffer);

			float x = VARSTRUCT_DECODE_TYPE(float, buffer);
			float y = VARSTRUCT_DECODE_TYPE(float, buffer);
			float z = VARSTRUCT_DECODE_TYPE(float, buffer);

			float RotX = VARSTRUCT_DECODE_TYPE(float, buffer);
			float RotY = VARSTRUCT_DECODE_TYPE(float, buffer);
			float RotZ = VARSTRUCT_DECODE_TYPE(float, buffer);

			float ScaleX = VARSTRUCT_DECODE_TYPE(float, buffer);
			float ScaleY = VARSTRUCT_DECODE_TYPE(float, buffer);
			float ScaleZ = VARSTRUCT_DECODE_TYPE(float, buffer);

			float ZAdjust = VARSTRUCT_DECODE_TYPE(float, buffer);
#ifdef DEBUGDAT
			printf("ZAdjust %8.3f\n", ZAdjust);	
#endif
			char TogFileName[255];

			sprintf(TogFileName, "%s.tog", TogName);

			uchar *TogBuffer;
			int32 TogBufferLength, TogPosition;

			if(!GetFile((uchar **)&TogBuffer, &TogBufferLength, base_path, TogFileName, archive))
			{
				printf("Unable to open %s\n", TogFileName);
			}
			else
			{
				ObjectGroupEntry NewObjectGroup;

				NewObjectGroup.FromTOG = true;

				NewObjectGroup.y = y;
				NewObjectGroup.x = x;
				NewObjectGroup.z = z;

				NewObjectGroup.TileX = TileYStart;
				NewObjectGroup.TileY = TileXStart;

				NewObjectGroup.TileZ = 0;
	
				NewObjectGroup.RotX = RotX;
				NewObjectGroup.RotY = RotY;
				NewObjectGroup.RotZ = RotZ;

				NewObjectGroup.ScaleX = ScaleX;
				NewObjectGroup.ScaleY = ScaleY;
				NewObjectGroup.ScaleZ = ScaleZ;

				NewObjectGroup.z = NewObjectGroup.z + (NewObjectGroup.ScaleZ * ZAdjust);

				Placeable p;

				TogPosition = 0;

				string ModelName;

				while(TogPosition < TogBufferLength)
				{
					string Token = GetToken(TogBuffer, TogPosition);

					if(Token == "*NAME")
					{
						Token = GetToken(TogBuffer, TogPosition);
						ModelName = Token;
						p.model = AddModelName(&model_data, Token.c_str());
					}
					if(Token == "*POSITION")
					{
						p.x = atof(GetToken(TogBuffer, TogPosition).c_str());
						p.y = atof(GetToken(TogBuffer, TogPosition).c_str());
						p.z = atof(GetToken(TogBuffer, TogPosition).c_str());
					}
					if(Token == "*ROTATION")
					{
						p.rx = atof(GetToken(TogBuffer, TogPosition).c_str());
						p.ry = atof(GetToken(TogBuffer, TogPosition).c_str());
						p.rz = atof(GetToken(TogBuffer, TogPosition).c_str());
					}
					if(Token == "*SCALE")
					{
						p.scale[0] = atof(GetToken(TogBuffer, TogPosition).c_str());
						p.scale[1] = p.scale[2] = p.scale[0];
					}
					if(Token == "*END_OBJECT")
					{
#ifdef DEBUGDAT
						printf("Pushing back object with model: %3i %s\n", p.model, ModelName.c_str());
						printf(" Position  : %8.3f, %8.3f, %8.3f\n", p.x, p.y, p.z);
						printf(" Rotations : %8.3f, %8.3f, %8.3f\n", p.rx, p.ry, p.rz);
						printf(" Scale     : %8.3f, %8.3f, %8.3f\n", p.scale[0], p.scale[1], p.scale[2]);
#endif
						model_data.PlaceableList.push_back(p);

						NewObjectGroup.SubObjects.push_back(model_data.PlaceableList.size()-1);
					}
					if(Token == "*END_OBJECTGROUP")
					{	
#ifdef DEBUGDAT
						printf("Pushing back new ObjectGroup\n");
						printf(" Position  : %8.3f, %8.3f, %8.3f\n", NewObjectGroup.x, NewObjectGroup.y, NewObjectGroup.z);
						printf(" Rotations : %8.3f, %8.3f, %8.3f\n", NewObjectGroup.RotX, NewObjectGroup.RotY, NewObjectGroup.RotZ);
						printf(" Scale     : %8.3f, %8.3f, %8.3f\n", NewObjectGroup.ScaleX, NewObjectGroup.ScaleY, NewObjectGroup.ScaleZ);
						printf("Adjusting NewObjectGroup.z by %8.3f\n", NewObjectGroup.ScaleZ * ZAdjust);
#endif
						model_data.ObjectGroups.push_back(NewObjectGroup);
#ifdef DEBUGDAT
						printf("ObjectGroup %i created.\n", model_data.ObjectGroups.size()-1);
						// There can be several object groups defined in the same .tog
						NewObjectGroup.SubObjects.clear();
#endif
					}
					if(Token == "*BEGIN_AREA")
					{
						while(Token != "*END_AREA")
						{
#ifdef DEBUGDAT
							printf("Skipping %s\n", Token.c_str());
#endif
							Token = GetToken(TogBuffer, TogPosition);
						}
					}


				}
			}

		}

		// If all the height values in this tile are the same, then we can just make one big quad of two triangles.
		//
		// This cuts over 200MB off the size of the oceanoftears .map, however not very much for other zones.
		//
		if(AllFloatsTheSame)
		{

			float QuadVertex1X = TileXStart;
			float QuadVertex1Y = TileYStart;
			float QuadVertex1Z = Floats[0];

			float QuadVertex2X = QuadVertex1X + (QuadsPerTile * UnitsPerVertX);
			float QuadVertex2Y = QuadVertex1Y;
			float QuadVertex2Z = QuadVertex1Z;
			
			float QuadVertex3X = QuadVertex2X;
			float QuadVertex3Y = QuadVertex1Y + (QuadsPerTile * UnitsPerVertY);
			float QuadVertex3Z = QuadVertex1Z;

			float QuadVertex4X = QuadVertex1X;
			float QuadVertex4Y = QuadVertex3Y;
			float QuadVertex4Z = QuadVertex1Z;
			

			++VertexNumber;

			zm->verts[VertexNumber++] = new Vertex(QuadVertex1Y, QuadVertex1X, QuadVertex1Z);

			zm->verts[VertexNumber++] = new Vertex(QuadVertex2Y, QuadVertex2X, QuadVertex2Z);

			zm->verts[VertexNumber++] = new Vertex(QuadVertex3Y, QuadVertex3X, QuadVertex3Z);

			zm->verts[VertexNumber] = new Vertex(QuadVertex4Y, QuadVertex4X, QuadVertex4Z);

			PolyNumber++;

			zm->polys[PolyNumber] = new Polygon;

			zm->polys[PolyNumber]->v1 = VertexNumber;
			zm->polys[PolyNumber]->v2 = VertexNumber - 2;
			zm->polys[PolyNumber]->v3 = VertexNumber - 1;
			zm->polys[PolyNumber]->tex = -1;

			PolyNumber++;

			zm->polys[PolyNumber] = new Polygon;

			zm->polys[PolyNumber]->v1 = VertexNumber;
			zm->polys[PolyNumber]->v2 = VertexNumber - 3;
			zm->polys[PolyNumber]->v3 = VertexNumber - 2;
			zm->polys[PolyNumber]->tex = -1; 
		}
		else
		{
			int32 RowNumber = -1;

			// This is the code that generates triangles for this tile of the terrain based on the height values we decode above.
			//
			for(int32 Quad = 0; Quad < QuadCount; ++Quad)
			{
				if((Quad % QuadsPerTile) == 0)
					++RowNumber;
	
				// Other common values for this Byte are 0x80, 0x82. Setting them all to zero has no obvious visual
				// effect.
				if(Bytes[Quad] & 0x01) // Indicates Quad should not be included because an object will overlay it.
					continue;

				float QuadVertex1X = TileXStart + (RowNumber * UnitsPerVertX);
				float QuadVertex1Y = TileYStart + (Quad % QuadsPerTile) * UnitsPerVertY;
				float QuadVertex1Z = Floats[Quad + RowNumber];
	
				float QuadVertex2X = QuadVertex1X + UnitsPerVertX;
				float QuadVertex2Y = QuadVertex1Y;
				float QuadVertex2Z = Floats[Quad + RowNumber + QuadsPerTile + 1];
				
				float QuadVertex3X = QuadVertex1X + UnitsPerVertX;
				float QuadVertex3Y = QuadVertex1Y + UnitsPerVertY;
				float QuadVertex3Z = Floats[Quad + RowNumber + QuadsPerTile + 2];
	
				float QuadVertex4X = QuadVertex1X;
				float QuadVertex4Y = QuadVertex1Y + UnitsPerVertY;
				float QuadVertex4Z = Floats[Quad + RowNumber + 1];
				
				++VertexNumber;
	
				zm->verts[VertexNumber++] = new Vertex(QuadVertex1Y, QuadVertex1X, QuadVertex1Z);
	
				zm->verts[VertexNumber++] = new Vertex(QuadVertex2Y, QuadVertex2X, QuadVertex2Z);
	
				zm->verts[VertexNumber++] = new Vertex(QuadVertex3Y, QuadVertex3X, QuadVertex3Z);
	
				zm->verts[VertexNumber] = new Vertex(QuadVertex4Y, QuadVertex4X, QuadVertex4Z);
	
				PolyNumber++;
	
				zm->polys[PolyNumber] = new Polygon;

				if(!GenerateQuads)
				{	
					zm->polys[PolyNumber]->v1 = VertexNumber;
					zm->polys[PolyNumber]->v2 = VertexNumber - 2;
					zm->polys[PolyNumber]->v3 = VertexNumber - 1;
					zm->polys[PolyNumber]->tex = -1;
	
					PolyNumber++;
	
					zm->polys[PolyNumber] = new Polygon;
	
					zm->polys[PolyNumber]->v1 = VertexNumber;
					zm->polys[PolyNumber]->v2 = VertexNumber - 3;
					zm->polys[PolyNumber]->v3 = VertexNumber - 2;
					zm->polys[PolyNumber]->tex = -1; 
				}
				else
				{
					zm->polys[PolyNumber]->type = 1;
					zm->polys[PolyNumber]->v1 = VertexNumber;
					zm->polys[PolyNumber]->v2 = VertexNumber - 3;
					zm->polys[PolyNumber]->v3 = VertexNumber - 2;
					zm->polys[PolyNumber]->v4 = VertexNumber - 1;
					zm->polys[PolyNumber]->tex = -1; 
				}	
			}
		}
		zm->vert_count = VertexNumber + 1;
		zm->poly_count = PolyNumber + 1;

	}

	delete [] Floats;
	delete [] Color1;
	delete [] Color2;
	delete [] Bytes;

	this->status = 1;
	return 1;
}

int32 DATLoader::Close() {

	if(!status)
		return 1;

	Zone_Model *zm = this->model_data.zone_model;

	int32 i;

	for(i = 0; i < zm->vert_count; ++i)
		delete zm->verts[i];

	for(i = 0; i < zm->poly_count; ++i)
		delete zm->polys[i];
	
	delete[] zm->verts;

	delete[] zm->polys;

	delete this->model_data.zone_model;

	status = 0;

	return 1;
}
