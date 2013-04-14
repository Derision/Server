/*

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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <algorithm>
#include <assert.h>
#include "types.h"
#include "azone3.h"
#include "wld.hpp"

#include "archive.hpp"
#include "pfs.hpp"

#include "file_loader.hpp"
#include "zon.hpp"
#include "ter.hpp"
#include "zonv4.hpp"

//#define DEBUG
#include <vector>
#include <map>
#include "../../zone/RaycastMesh.h"

using namespace std;

int main(int argc, char *argv[])
{

	printf("AZONE3: EQEmu .MAP file generator with placeable object support.\n");

	if(argc != 2) {
		printf("Usage: %s (zone short name)\n", argv[0]);
		return(1);
	}

	char bufm[250];

	sprintf(bufm, "%s.map", argv[1]);

	RCMBuilder RCM;

	if(!RCM.build(argv[1]))
		return(1);

	if(!RCM.writeMap(bufm))
		return(1);

	return(0);
}

RCMBuilder::RCMBuilder()
{
	faceCount = 0;
	faceBlock = NULL;
	rm = NULL;

}

RCMBuilder::~RCMBuilder()
{
	if(faceBlock != NULL)
		delete [] faceBlock;

	faceBlock = NULL;

	if(rm)
		rm->release();

	rm = NULL;
}

bool RCMBuilder::build(const char *shortname)
{
	char bufs[96];
  	Archive *archive;
  	FileLoader *fileloader;
  	Zone_Model *zm;
	FILE *fp;
	EQFileType FileType = UNKNOWN;

	bool V4Zone = false;

	sprintf(bufs, "%s.s3d", shortname);

	archive = new PFSLoader();
	fp = fopen(bufs, "rb");
	if(fp != NULL)
		FileType = S3D;
	else
	{
		sprintf(bufs, "%s.eqg", shortname);
		fp = fopen(bufs, "rb");
		if(fp != NULL)
			FileType = EQG;
	}

	if(FileType == UNKNOWN)
	{
		printf("Unable to locate %s.s3d or %s.eqg\n", shortname, shortname);
		return(false);
	}

  	if(archive->Open(fp) == 0)
	{
		printf("Unable to open container file '%s'\n", bufs);
		return(false);
	}

	switch(FileType)
	{
		case S3D:
  			fileloader = new WLDLoader();
  			if(fileloader->Open(NULL, (char *) shortname, archive) == 0)
			{
	  			printf("Error reading WLD from %s\n", bufs);
	  			return(false);
  			}
			break;
		case EQG:
			fileloader = new ZonLoader();
			if(fileloader->Open(NULL, (char *) shortname, archive) == 0)
			{
				delete fileloader;
				fileloader = new Zonv4Loader(true);
				if(fileloader->Open(NULL, (char *) shortname, archive) == 0)
				{
					printf("Error reading ZON/TER from %s\n", bufs);
					return(false);
				}
				V4Zone = true;
	        	}
			break;
		case UNKNOWN:
			// Just here to stop the compiler warning
			break;
	}

	zm = fileloader->model_data.zone_model;

	int32 i;
	VERTEX v1, v2, v3, v4;
	for(i = 0; i < zm->poly_count; ++i)
	{
		v1.y = zm->verts[zm->polys[i]->v1]->x;
		v1.x = zm->verts[zm->polys[i]->v1]->y;
		v1.z = zm->verts[zm->polys[i]->v1]->z;

		v2.y = zm->verts[zm->polys[i]->v2]->x;
		v2.x = zm->verts[zm->polys[i]->v2]->y;
		v2.z = zm->verts[zm->polys[i]->v2]->z;

		v3.y = zm->verts[zm->polys[i]->v3]->x;
		v3.x = zm->verts[zm->polys[i]->v3]->y;
		v3.z = zm->verts[zm->polys[i]->v3]->z;

		if(zm->polys[i]->type == 1)
		{
			v4.y = zm->verts[zm->polys[i]->v4]->x;
			v4.x = zm->verts[zm->polys[i]->v4]->y;
			v4.z = zm->verts[zm->polys[i]->v4]->z;
			AddFace(v1, v2, v3, v4);
		}
		else
			AddFace(v1, v2, v3);
	}

	printf("There are and %u faces.\n", _FaceList.size());

	if(fileloader->model_data.plac_count)
	{
		if(V4Zone)
		{
			vector<ObjectGroupEntry>::iterator Iterator;

			Iterator = fileloader->model_data.ObjectGroups.begin();
			AddPlaceableV4(fileloader, bufs, false);
		}
		else
			AddPlaceable(fileloader, bufs, false);
	}
	else
		printf("No placeable objects (or perhaps %s_obj.s3d not present).\n", shortname);

	printf("After processing placeable objects, there are %lu vertices and %lu faces.\n", _FaceList.size()*3, _FaceList.size());

	uint32 r;

	faceCount = _FaceList.size();

	faceBlock = new FACE[faceCount];

	for(r = 0; r < faceCount; r++)
	{
		faceBlock[r] = _FaceList[r];

		faceBlock[r].flags.minxvert = 0;
		faceBlock[r].flags.maxxvert = 0;
		faceBlock[r].flags.minyvert = 0;
		faceBlock[r].flags.maxyvert = 0;
		faceBlock[r].flags.minzvert = 0;
		faceBlock[r].flags.maxzvert = 0;

		uint8 NumberOfFaces = (faceBlock[r].flags.type == 1) ? 4 : 3;

		for(int i = 1; i < NumberOfFaces; ++i)
		{
			if(faceBlock[r].vert[i].x < faceBlock[r].vert[faceBlock[r].flags.minxvert].x)
				faceBlock[r].flags.minxvert = i;

			if(faceBlock[r].vert[i].y < faceBlock[r].vert[faceBlock[r].flags.minyvert].y)
				faceBlock[r].flags.minyvert = i;

			if(faceBlock[r].vert[i].z < faceBlock[r].vert[faceBlock[r].flags.minzvert].z)
				faceBlock[r].flags.minzvert = i;

			if(faceBlock[r].vert[i].x > faceBlock[r].vert[faceBlock[r].flags.maxxvert].x)
				faceBlock[r].flags.maxxvert = i;

			if(faceBlock[r].vert[i].y > faceBlock[r].vert[faceBlock[r].flags.maxyvert].y)
				faceBlock[r].flags.maxyvert = i;

			if(faceBlock[r].vert[i].z > faceBlock[r].vert[faceBlock[r].flags.maxzvert].z)
				faceBlock[r].flags.maxzvert = i;
		}
	}

	float minx, miny, minz, maxx, maxy, maxz;
	minx = 1e12;
	miny = 1e12;
	minz = 1e12;
	maxx = -1e12;
	maxy = -1e12;
	maxz = -1e12;

	//find our limits.
	for(uint32 i = 0; i < faceCount; ++i)
	{
		if(faceBlock[i].vert[faceBlock[i].flags.maxxvert].x > maxx)
			maxx = faceBlock[i].vert[faceBlock[i].flags.maxxvert].x;

		if(faceBlock[i].vert[faceBlock[i].flags.minxvert].x < minx)
			minx = faceBlock[i].vert[faceBlock[i].flags.minxvert].x;

		if(faceBlock[i].vert[faceBlock[i].flags.maxyvert].y > maxy)
			maxy = faceBlock[i].vert[faceBlock[i].flags.maxyvert].y;

		if(faceBlock[i].vert[faceBlock[i].flags.minyvert].y < miny)
			miny = faceBlock[i].vert[faceBlock[i].flags.minyvert].y;

		if(faceBlock[i].vert[faceBlock[i].flags.maxzvert].z > maxz)
			maxz = faceBlock[i].vert[faceBlock[i].flags.maxzvert].z;

		if(faceBlock[i].vert[faceBlock[i].flags.minzvert].z < minz)
			minz = faceBlock[i].vert[faceBlock[i].flags.minzvert].z;
	}

	printf("Bounding box: %.2f < x < %.2f, %.2f < y < %.2f, %.2f < z < %.2f\n", minx, maxx, miny, maxy, minz, maxz);

	printf("Building RayCastMesh.\n");

	rm = createRaycastMesh(faceCount, faceBlock);

	printf("Done building RCM...\n");

	fileloader->Close();

	delete fileloader;

	archive->Close();

	delete archive;

	return(true);
}



bool RCMBuilder::writeMap(const char *file)
{
	printf("Writing map file.\n");

	FILE *out = fopen(file, "wb");
	if(out == NULL)
	{
		printf("Unable to open output file '%s'.\n", file);
		return(1);
	}

	mapHeader head;
	head.version = MAP_VERSION;
	head.face_count = faceCount;
	head.node_count = 0;
	head.facelist_count = 0;

	if(fwrite(&head, sizeof(head), 1, out) != 1)
	{
		printf("Error writing map file header.\n");
		fclose(out);
		return(1);

	}

	printf("Map header: Version: 0x%08lX. %lu faces, %u nodes, %lu facelists\n", head.version, head.face_count, head.node_count, head.facelist_count);
	
	if(fwrite(faceBlock, sizeof(FACE), faceCount, out) != faceCount) {
		printf("Error writing map file faces.\n");
		fclose(out);
		return(1);
	}
	
	rm->Save(out);

	int32 MapFileSize = ftell(out);
	fclose(out);

	printf("Done writing map (%3.2fMB).\n", (float)MapFileSize/1048576);

	return(0);
}


void RCMBuilder::AddFace(VERTEX &v1, VERTEX &v2, VERTEX &v3)
{
	FACE f;
	f.flags.type = 0;
	f.flags.padding = 0;

	f.nx = (v2.y - v1.y)*(v3.z - v1.z) - (v2.z - v1.z)*(v3.y - v1.y);
	f.ny = (v2.z - v1.z)*(v3.x - v1.x) - (v2.x - v1.x)*(v3.z - v1.z);
	f.nz = (v2.x - v1.x)*(v3.y - v1.y) - (v2.y - v1.y)*(v3.x - v1.x);
	NormalizeN(&f);
	f.nd = - f.nx * v1.x - f.ny * v1.y - f.nz * v1.z;

	f.vert[0] = v1;
	f.vert[1] = v2;
	f.vert[2] = v3;

	_FaceList.push_back(f);
}


void RCMBuilder::AddFace(VERTEX &v1, VERTEX &v2, VERTEX &v3, VERTEX &v4)
{
	FACE f;
	f.flags.type = 1;
	f.flags.padding = 0;

	f.nx = (v2.y - v1.y)*(v3.z - v1.z) - (v2.z - v1.z)*(v3.y - v1.y);
	f.ny = (v2.z - v1.z)*(v3.x - v1.x) - (v2.x - v1.x)*(v3.z - v1.z);
	f.nz = (v2.x - v1.x)*(v3.y - v1.y) - (v2.y - v1.y)*(v3.x - v1.x);
	NormalizeN(&f);
	f.nd = - f.nx * v1.x - f.ny * v1.y - f.nz * v1.z;

	f.vert[0] = v1;
	f.vert[1] = v2;
	f.vert[2] = v3;
	f.vert[3] = v4;

	_FaceList.push_back(f);
}


void RCMBuilder::NormalizeN(FACE *p)
{
	float len = sqrt(p->nx*p->nx + p->ny*p->ny + p->nz*p->nz);
	p->nx /= len;
	p->ny /= len;
	p->nz /= len;
}

void RCMBuilder::AddPlaceable(FileLoader *fileloader, char *ZoneFileName, bool ListPlaceable)
{
	Polygon *poly;
	Vertex *verts[3];
	float XOffset, YOffset, ZOffset;
	float RotX, RotY, RotZ;
	float XScale, YScale, ZScale;
	VERTEX v1, v2, v3, tmpv;


	// Ghetto ini file parser to see which models to include
	// The format of each line in azone.ini is:
	//
	// shortname.[eqg|s3d],model number, model numner, ...
	// E.g.
	// tox.s3d,1,17,34
	// anguish.eqg,25,69,70
	//

	const int32 IniBufferSize = 255;
	enum ReadingState { ReadingZoneName, ReadingModelNumbers };
	ReadingState State = ReadingZoneName;
	bool INIEntryFound = false;
	int32 INIModelCount = 0;
	char IniBuffer[IniBufferSize], ch;
	vector<int32> ModelNumbers;
	int32 StrIndex = 0;
	int32 ModelNumber;

	FILE *IniFile = fopen("azone.ini", "rb");

	if(!IniFile) {
		printf("azone.ini not found in current directory. Not processing placeable models.\n");
		return;
	}
	printf("Processing azone.ini for placeable models.\n");

	while(!feof(IniFile)) {
		ch = fgetc(IniFile);
		if((ch=='#')&&(StrIndex==0)) { // Discard comment lines beginning with a hash
			while((ch!=EOF)&&(ch!='\n'))
	       	        	ch = fgetc(IniFile);

                	continue;
        	}
		if((ch=='\n') && (State==ReadingZoneName)) {
			StrIndex = 0;
			continue;
		}
    	   	if(ch=='\r') continue;
		if((ch==EOF)||(ch=='\n')) {
			IniBuffer[StrIndex] = '\0';
			if(State == ReadingModelNumbers) {
				ModelNumber = atoi(IniBuffer);
				if((ModelNumber >= 0) && (ModelNumber < fileloader->model_data.model_count))
				{
					fileloader->model_data.models[ModelNumber]->IncludeInMap = true;
					INIModelCount++;
				}
				else
					printf("ERROR: Specified model %s invalid, must be in range 0 to %i\n", IniBuffer, fileloader->model_data.model_count - 1);
			}
			break;
		}
		if(ch==',') {
			IniBuffer[StrIndex]='\0';
			StrIndex = 0;
			if(State == ReadingZoneName)  {
				if(strcmp(ZoneFileName, IniBuffer)) {
					StrIndex = 0;
					// Not our zone, skip to next line
					while((ch!=EOF)&&(ch!='\n'))
	       	        			ch = fgetc(IniFile);
					continue;
				}
				else {
					State = ReadingModelNumbers;
					INIEntryFound = true;
				}
			}
			else  {
				ModelNumber = atoi(IniBuffer);
				if((ModelNumber >= 0) && (ModelNumber < fileloader->model_data.model_count))
				{
					fileloader->model_data.models[ModelNumber]->IncludeInMap = true;
					INIModelCount++;
				}
				else
					printf("ERROR: Specified model %s invalid, must be in range 0 to %i\n", IniBuffer, fileloader->model_data.model_count - 1);
			}
			continue;
		}
		IniBuffer[StrIndex++] = tolower(ch);
	}
	fclose(IniFile);

	if(INIEntryFound) {
		printf("azone.ini entry found for this zone. ");
		if(INIModelCount > 0)
			printf("Including %d models.\n", INIModelCount);
		else
			printf("No valid model numbers specified.\n");
	}
	else {
		printf("No azone.ini entry found for zone %s\n", ZoneFileName);
	}



	for(int32 i = 0; i < fileloader->model_data.plac_count; ++i) {
		if(fileloader->model_data.placeable[i]->model==-1) continue;
		// The model pointer should only really be NULL for the zone model, as we process that separately.
		if(fileloader->model_data.models[fileloader->model_data.placeable[i]->model] == NULL) continue;
		if(ListPlaceable)
			printf("Placeable Object %4d @ (%9.2f, %9.2f, %9.2f uses model %4d %s\n",i,
		       	 	fileloader->model_data.placeable[i]->y,
		        	fileloader->model_data.placeable[i]->x,
		        	fileloader->model_data.placeable[i]->z,
				fileloader->model_data.placeable[i]->model,
				fileloader->model_data.models[fileloader->model_data.placeable[i]->model]->name);


		if(!fileloader->model_data.models[fileloader->model_data.placeable[i]->model]->IncludeInMap)
			continue;
		printf("Including Placeable Object %4d using model %4d (%s).\n", i,
			fileloader->model_data.placeable[i]->model,
			fileloader->model_data.models[fileloader->model_data.placeable[i]->model]->name);

		if(fileloader->model_data.placeable[i]->model>fileloader->model_data.model_count) continue;

		XOffset = fileloader->model_data.placeable[i]->x;
		YOffset = fileloader->model_data.placeable[i]->y;
		ZOffset = fileloader->model_data.placeable[i]->z;

		RotX = fileloader->model_data.placeable[i]->rx * 3.14159 / 180;  // Convert from degrees to radians
		RotY = fileloader->model_data.placeable[i]->ry * 3.14159 / 180;
		RotZ = fileloader->model_data.placeable[i]->rz * 3.14159 / 180;

		XScale = fileloader->model_data.placeable[i]->scale[0];
		YScale = fileloader->model_data.placeable[i]->scale[1];
		ZScale = fileloader->model_data.placeable[i]->scale[2];


		Model *model = fileloader->model_data.models[fileloader->model_data.placeable[i]->model];


		for(int32 j = 0; j < model->poly_count; ++j) {

			poly = model->polys[j];

			verts[0] = model->verts[poly->v1];
			verts[1] = model->verts[poly->v2];
			verts[2] = model->verts[poly->v3];

			v1.x = verts[0]->x; v1.y = verts[0]->y; v1.z = verts[0]->z;
			v2.x = verts[1]->x; v2.y = verts[1]->y; v2.z = verts[1]->z;
			v3.x = verts[2]->x; v3.y = verts[2]->y; v3.z = verts[2]->z;


			RotateVertex(v1, RotX, RotY, RotZ);
			RotateVertex(v2, RotX, RotY, RotZ);
			RotateVertex(v3, RotX, RotY, RotZ);

			ScaleVertex(v1, XScale, YScale, ZScale);
			ScaleVertex(v2, XScale, YScale, ZScale);
			ScaleVertex(v3, XScale, YScale, ZScale);

			TranslateVertex(v1, XOffset, YOffset, ZOffset);
			TranslateVertex(v2, XOffset, YOffset, ZOffset);
			TranslateVertex(v3, XOffset, YOffset, ZOffset);


			// Swap X & Y
			//
			tmpv = v1; v1.x = tmpv.y; v1.y = tmpv.x;
			tmpv = v2; v2.x = tmpv.y; v2.y = tmpv.x;
			tmpv = v3; v3.x = tmpv.y; v3.y = tmpv.x;

			AddFace(v1, v2, v3);
		}
	}
}


void RCMBuilder::AddPlaceableV4(FileLoader *fileloader, char *ZoneFileName, bool ListPlaceable) {
	Polygon *poly;
	Vertex *verts[3];
	float XOffset, YOffset, ZOffset;
	float RotX, RotY, RotZ;
	float XScale, YScale, ZScale;
	VERTEX v1, v2, v3, tmpv;

	//return;

	printf("EQG V4 Placeable Zone Support\n");
	printf("ObjectGroupCount = %lu\n", fileloader->model_data.ObjectGroups.size());

	vector<ObjectGroupEntry>::iterator Iterator;

	int32 OGNum = 0;

	bool BailOut = false;

	// Ghetto ini file parser to see which models to include
	// The format of each line in azone.ini is:
	//
	// shortname.[eqg|s3d],model number, model numner, ...
	// E.g.
	// tox.s3d,1,17,34
	// anguish.eqg,25,69,70
	//

	const int32 IniBufferSize = 255;
	enum ReadingState { ReadingZoneName, ReadingModelNumbers };
	ReadingState State = ReadingZoneName;
	bool INIEntryFound = false;
	int32 INIModelCount = 0;
	char IniBuffer[IniBufferSize], ch;
	vector<int32> ModelNumbers;
	int32 StrIndex = 0;
	int32 ModelNumber;

	FILE *IniFile = fopen("azone.ini", "rb");

	if(!IniFile)
		printf("azone.ini not found in current directory. Including default models.\n");
	else
	{
		printf("Processing azone.ini for placeable models.\n");

		while(!feof(IniFile)) {
			ch = fgetc(IniFile);
			if((ch=='#')&&(StrIndex==0)) { // Discard comment lines beginning with a hash
				while((ch!=EOF)&&(ch!='\n'))
		       	        	ch = fgetc(IniFile);

       		         	continue;
       		 	}
			if((ch=='\n') && (State==ReadingZoneName)) {
				StrIndex = 0;
				continue;
			}
    		   	if(ch=='\r') continue;
			if((ch==EOF)||(ch=='\n')) {
				IniBuffer[StrIndex] = '\0';
				if(State == ReadingModelNumbers) {
					bool Exclude = false;
					bool Group = false;
					if(IniBuffer[0]=='-')
					{
						Exclude = true;
						strcpy(IniBuffer, IniBuffer+1);
					}
					if(IniBuffer[0]=='g')
					{
						Group = true;
						strcpy(IniBuffer, IniBuffer+1);
					}
					ModelNumber = atoi(IniBuffer);
					if(!Group)
					{
						if((ModelNumber >= 0) && (ModelNumber < fileloader->model_data.model_count))
							fileloader->model_data.models[ModelNumber]->IncludeInMap = Exclude ? false : true;
						else
							printf("ERROR: Specified model %s invalid, must be in range 0 to %i\n", IniBuffer, fileloader->model_data.model_count - 1);
					}
					else
					{
						if((ModelNumber >= 0) && ((uint32 )ModelNumber < fileloader->model_data.ObjectGroups.size()))
							fileloader->model_data.ObjectGroups[ModelNumber].IncludeInMap = Exclude ? false : true;

					}
				}
				break;
			}
			if(ch==',') {
				IniBuffer[StrIndex]='\0';
				StrIndex = 0;
				if(State == ReadingZoneName)  {
					if(strcmp(ZoneFileName, IniBuffer)) {
						StrIndex = 0;
						// Not our zone, skip to next line
						while((ch!=EOF)&&(ch!='\n'))
		       	        			ch = fgetc(IniFile);
						continue;
					}
					else {
						State = ReadingModelNumbers;
						INIEntryFound = true;
					}
				}
				else  {
					bool Exclude = false;
					bool Group = false;
					if(IniBuffer[0]=='-')
					{
						Exclude = true;
						strcpy(IniBuffer, IniBuffer+1);
					}
					if(IniBuffer[0]=='g')
					{
						Group = true;
						strcpy(IniBuffer, IniBuffer+1);
					}
					ModelNumber = atoi(IniBuffer);
					if(!Group)
					{
						if((ModelNumber >= 0) && (ModelNumber < fileloader->model_data.model_count))
							fileloader->model_data.models[ModelNumber]->IncludeInMap = Exclude ? false : true;
						else
							printf("ERROR: Specified model %s invalid, must be in range 0 to %i\n", IniBuffer, fileloader->model_data.model_count - 1);
					}
					else
					{
						if((ModelNumber >= 0) && ((uint32 )ModelNumber < fileloader->model_data.ObjectGroups.size()))
							fileloader->model_data.ObjectGroups[ModelNumber].IncludeInMap = Exclude ? false : true;

					}
				}
				continue;
			}
			IniBuffer[StrIndex++] = tolower(ch);
		}
		fclose(IniFile);

		if(INIEntryFound)
			printf("azone.ini entry found for this zone. ");
		else
			printf("No azone.ini entry found for zone %s\n", ZoneFileName);
	}

	Iterator = fileloader->model_data.ObjectGroups.begin();

	while(Iterator != fileloader->model_data.ObjectGroups.end())
	{
		if(!(*Iterator).IncludeInMap)
		{
			++OGNum;
			++Iterator;
			continue;
		}


#ifdef DEBUG
		printf("ObjectGroup Number: %i\n", OGNum++);

		printf("ObjectGroup Coordinates: %8.3f, %8.3f, %8.3f\n",
			(*Iterator).x, (*Iterator).y, (*Iterator).z);

		printf("Tile: %8.3f, %8.3f, %8.3f\n",
			(*Iterator).TileX, (*Iterator).TileY, (*Iterator).TileZ);

		printf("ObjectGroup Rotations  : %8.3f, %8.3f, %8.3f\n",
			(*Iterator).RotX, (*Iterator).RotY, (*Iterator).RotZ);

		printf("ObjectGroup Scale      : %8.3f, %8.3f, %8.3f\n",
			(*Iterator).ScaleX, (*Iterator).ScaleY, (*Iterator).ScaleZ);
#endif

		list<int32>::iterator ModelIterator;

		ModelIterator = (*Iterator).SubObjects.begin();

		while(ModelIterator != (*Iterator).SubObjects.end())
		{
			int32 SubModel = (*ModelIterator);

#ifdef DEBUG
			printf("  SubModel: %i\n", (*ModelIterator));
#endif

			XOffset = fileloader->model_data.placeable[SubModel]->x;
			YOffset = fileloader->model_data.placeable[SubModel]->y;
			ZOffset = fileloader->model_data.placeable[SubModel]->z;
#ifdef DEBUG
			printf("   Translations: %8.3f, %8.3f, %8.3f\n", XOffset, YOffset, ZOffset);
			printf("   Rotations   : %8.3f, %8.3f, %8.3f\n", fileloader->model_data.placeable[SubModel]->rx,
				fileloader->model_data.placeable[SubModel]->ry, fileloader->model_data.placeable[SubModel]->rz);
			printf("   Scale       : %8.3f, %8.3f, %8.3f\n", fileloader->model_data.placeable[SubModel]->scale[0],
				fileloader->model_data.placeable[SubModel]->scale[1], fileloader->model_data.placeable[SubModel]->scale[2]);
#endif

			RotX = fileloader->model_data.placeable[SubModel]->rx * 3.14159 / 180;  // Convert from degrees to radians
			RotY = fileloader->model_data.placeable[SubModel]->ry * 3.14159 / 180;
			RotZ = fileloader->model_data.placeable[SubModel]->rz * 3.14159 / 180;

			XScale = fileloader->model_data.placeable[SubModel]->scale[0];
			YScale = fileloader->model_data.placeable[SubModel]->scale[1];
			ZScale = fileloader->model_data.placeable[SubModel]->scale[2];

			Model *model = fileloader->model_data.models[fileloader->model_data.placeable[SubModel]->model];

#ifdef DEBUG
			printf("Model %i, name is %s\n", fileloader->model_data.placeable[SubModel]->model, model->name);
#endif
			if(!model->IncludeInMap)
			{
				++ModelIterator;
#ifdef DEBUG
				printf("Not including in .map\n");
#endif
				continue;
			}
			for(int32 j = 0; j < model->poly_count; ++j) {

				poly = model->polys[j];

				verts[0] = model->verts[poly->v1];
				verts[1] = model->verts[poly->v2];
				verts[2] = model->verts[poly->v3];

				v1.x = verts[0]->x; v1.y = verts[0]->y; v1.z = verts[0]->z;
				v2.x = verts[1]->x; v2.y = verts[1]->y; v2.z = verts[1]->z;
				v3.x = verts[2]->x; v3.y = verts[2]->y; v3.z = verts[2]->z;

				// Scale by the values specified for the individual object.
				//
				ScaleVertex(v1, XScale, YScale, ZScale);
				ScaleVertex(v2, XScale, YScale, ZScale);
				ScaleVertex(v3, XScale, YScale, ZScale);
				// Translate by the values specified for the individual object
				//
				TranslateVertex(v1, XOffset, YOffset, ZOffset);
				TranslateVertex(v2, XOffset, YOffset, ZOffset);
				TranslateVertex(v3, XOffset, YOffset, ZOffset);
				// Rotate by the values specified for the group
				// TODO: The X/Y rotations can be combined
				//
				RotateVertex(v1, (*Iterator).RotX * 3.14159 / 180.0f, 0, 0);
				RotateVertex(v2, (*Iterator).RotX * 3.14159 / 180.0f, 0, 0);
				RotateVertex(v3, (*Iterator).RotX * 3.14159 / 180.0f, 0, 0);

				RotateVertex(v1, 0, (*Iterator).RotY * 3.14159 / 180.0f, 0);
				RotateVertex(v2, 0, (*Iterator).RotY * 3.14159 / 180.0f, 0);
				RotateVertex(v3, 0, (*Iterator).RotY * 3.14159 / 180.0f, 0);

				// To make things align properly, we need to translate the object back to the origin
				// before applying the model Z rotation. This is what the Correction VERTEX is for.
				//
				VERTEX Correction(XOffset, YOffset, ZOffset);

				RotateVertex(Correction, (*Iterator).RotX * 3.14159 / 180.0f, 0, 0);

				TranslateVertex(v1, -Correction.x, -Correction.y, -Correction.z);
				TranslateVertex(v2, -Correction.x, -Correction.y, -Correction.z);
				TranslateVertex(v3, -Correction.x, -Correction.y, -Correction.z);
				// Rotate by model Z rotation
				//
				//
				RotateVertex(v1, RotX, 0, 0);
				RotateVertex(v2, RotX, 0, 0);
				RotateVertex(v3, RotX, 0, 0);

				// Don't know why the Y rotation needs to be negative
				//
				RotateVertex(v1, 0, -RotY, 0);
				RotateVertex(v2, 0, -RotY, 0);
				RotateVertex(v3, 0, -RotY, 0);

				RotateVertex(v1, 0, 0, RotZ);
				RotateVertex(v2, 0, 0, RotZ);
				RotateVertex(v3, 0, 0, RotZ);
				// Now we have applied the individual model rotations, translate back to where we were.
				//
				TranslateVertex(v1, Correction.x, Correction.y, Correction.z);
				TranslateVertex(v2, Correction.x, Correction.y, Correction.z);
				TranslateVertex(v3, Correction.x, Correction.y, Correction.z);
				// Rotate by the Z rotation value specified for the object group
				//
				RotateVertex(v1, 0, 0, (*Iterator).RotZ * 3.14159 / 180.0f);
				RotateVertex(v2, 0, 0, (*Iterator).RotZ * 3.14159 / 180.0f);
				RotateVertex(v3, 0, 0, (*Iterator).RotZ * 3.14159 / 180.0f);
				// Scale by the object group values
				//
				ScaleVertex(v1, (*Iterator).ScaleX, (*Iterator).ScaleY, (*Iterator).ScaleZ);
				ScaleVertex(v2, (*Iterator).ScaleX, (*Iterator).ScaleY, (*Iterator).ScaleZ);
				ScaleVertex(v3, (*Iterator).ScaleX, (*Iterator).ScaleY, (*Iterator).ScaleZ);
				// Translate to the relevant tile starting co-ordinates
				//
				TranslateVertex(v1, (*Iterator).TileX, (*Iterator).TileY, (*Iterator).TileZ); // Y+14, Z + 68
				TranslateVertex(v2, (*Iterator).TileX, (*Iterator).TileY, (*Iterator).TileZ);
				TranslateVertex(v3, (*Iterator).TileX, (*Iterator).TileY, (*Iterator).TileZ);
				// Translate to the position within the tile for this object group
				//
				TranslateVertex(v1, (*Iterator).x, (*Iterator).y, (*Iterator).z);
				TranslateVertex(v2, (*Iterator).x, (*Iterator).y, (*Iterator).z);
				TranslateVertex(v3, (*Iterator).x, (*Iterator).y, (*Iterator).z);
				// Swap X & Y
				//
				tmpv = v1; v1.x = tmpv.y; v1.y = tmpv.x;
				tmpv = v2; v2.x = tmpv.y; v2.y = tmpv.x;
				tmpv = v3; v3.x = tmpv.y; v3.y = tmpv.x;

				AddFace(v1, v2, v3);

			}
			++ModelIterator;
		}

		++Iterator;
	}


}

void RCMBuilder::RotateVertex(VERTEX &v, float XRotation, float YRotation, float ZRotation) {

	VERTEX nv;

	nv = v;

	// Rotate about the X axis
	//
	nv.y = (cos(XRotation) * v.y) - (sin(XRotation) * v.z);
	nv.z = (sin(XRotation) * v.y) + (cos(XRotation) * v.z);

	v = nv;


	// Rotate about the Y axis
	//
	nv.x = (cos(YRotation) * v.x) + (sin(YRotation) * v.z) ;
	nv.z = -(sin(YRotation) * v.x) + (cos(YRotation) * v.z);

	v = nv;

	// Rotate about the Z axis
	//
	nv.x = (cos(ZRotation) * v.x) - (sin(ZRotation) * v.y) ;
	nv.y = (sin(ZRotation) * v.x) + (cos(ZRotation) * v.y) ;

	v = nv;
}

void RCMBuilder::ScaleVertex(VERTEX &v, float XScale, float YScale, float ZScale) {

	v.x = v.x * XScale;
	v.y = v.y * YScale;
	v.z = v.z * ZScale;
}


void RCMBuilder::TranslateVertex(VERTEX &v, float XOffset, float YOffset, float ZOffset) {

	v.x = v.x + XOffset;
	v.y = v.y + YOffset;
	v.z = v.z + ZOffset;
}






