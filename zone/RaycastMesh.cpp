#include "RaycastMesh.h"
#include <math.h>
#include <assert.h>
#include <vector>
#include <stdio.h>

// This code snippet allows you to create an axis aligned bounding volume tree for a triangle mesh so that you can do
// high-speed raycasting.
//
// There are much better implementations of this available on the internet.  In particular I recommend that you use 
// OPCODE written by Pierre Terdiman.
// @see: http://www.codercorner.com/Opcode.htm
//
// OPCODE does a whole lot more than just raycasting, and is a rather significant amount of source code.
//
// I am providing this code snippet for the use case where you *only* want to do quick and dirty optimized raycasting.
// I have not done performance testing between this version and OPCODE; so I don't know how much slower it is.  However,
// anytime you switch to using a spatial data structure for raycasting, you increase your performance by orders and orders 
// of magnitude; so this implementation should work fine for simple tools and utilities.
//
// It also serves as a nice sample for people who are trying to learn the algorithm of how to implement AABB trees.
// AABB = Axis Aligned Bounding Volume trees.
//
// http://www.cgal.org/Manual/3.5/doc_html/cgal_manual/AABB_tree/Chapter_main.html
//
//
// This code snippet was written by John W. Ratcliff on August 18, 2011 and released under the MIT. license.
//
// mailto:jratcliffscarab@gmail.com
//
// The official source can be found at:  http://code.google.com/p/raycastmesh/
//
// 

#pragma warning(disable:4100)

namespace RAYCAST_MESH
{

typedef std::vector< uint32 > TriVector;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	A method to compute a ray-AABB intersection.
*	Original code by Andrew Woo, from "Graphics Gems", Academic Press, 1990
*	Optimized code by Pierre Terdiman, 2000 (~20-30% faster on my Celeron 500)
*	Epsilon value added by Klaus Hartmann. (discarding it saves a few cycles only)
*
*	Hence this version is faster as well as more robust than the original one.
*
*	Should work provided:
*	1) the integer representation of 0.0f is 0x00000000
*	2) the sign bit of the float is the most significant one
*
*	Report bugs: p.terdiman@codercorner.com
*
*	\param		aabb		[in] the axis-aligned bounding box
*	\param		origin		[in] ray origin
*	\param		dir			[in] ray direction
*	\param		coord		[out] impact coordinates
*	\return		true if ray intersects AABB
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RAYAABB_EPSILON 0.00001f
//! Integer representation of a floating-point value.
#define IR(x)	((uint32&)x)

bool intersectRayAABB(VERTEX MinB, VERTEX MaxB, VERTEX origin, VERTEX dir, VERTEX &coord)
{
	bool Inside = true;
	VERTEX MaxT;
	MaxT.x = MaxT.y = MaxT.z = -1.0f;

	// Find candidate planes.

	for(uint32 i = 0; i < 3; i++)
	{
		if(origin.axis[i] < MinB.axis[i])
		{
			coord.axis[i]	= MinB.axis[i];
			Inside		= false;

			// Calculate T distances to candidate planes
			if(IR(dir.axis[i]))	MaxT.axis[i] = (MinB.axis[i] - origin.axis[i]) / dir.axis[i];
		}
		else if(origin.axis[i] > MaxB.axis[i])
		{
			coord.axis[i]	= MaxB.axis[i];
			Inside		= false;

			// Calculate T distances to candidate planes
			if(IR(dir.axis[i]))	MaxT.axis[i] = (MaxB.axis[i] - origin.axis[i]) / dir.axis[i];
		}
	}

	// Ray origin inside bounding box
	if(Inside)
	{
		coord.x = origin.x;
		coord.y = origin.y;
		coord.z = origin.z;
		return true;
	}

	// Get largest of the maxT's for final choice of intersection
	uint32 WhichPlane = 0;
	if(MaxT.axis[1] > MaxT.axis[WhichPlane])	WhichPlane = 1;
	if(MaxT.axis[2] > MaxT.axis[WhichPlane])	WhichPlane = 2;

	// Check final candidate actually inside box
	if(IR(MaxT.axis[WhichPlane])&0x80000000) return false;

	for(uint32 i=0;i<3;i++)
	{
		if(i!=WhichPlane)
		{
			coord.axis[i] = origin.axis[i] + MaxT.axis[WhichPlane] * dir.axis[i];
			#ifdef RAYAABB_EPSILON
			if(coord.axis[i] < MinB.axis[i] - RAYAABB_EPSILON || coord.axis[i] > MaxB.axis[i] + RAYAABB_EPSILON)	return false;
			#else
			if(coord.axis[i] < MinB.axis[i] || coord.axis[i] > MaxB.axis[i])	return false;
			#endif
		}
	}
	return true;	// ray hits box
}




bool intersectLineSegmentAABB(VERTEX bmin, VERTEX bmax, VERTEX p1, VERTEX dir,float &dist,VERTEX &intersect)
{
	bool ret = false;

	if ( dist > RAYAABB_EPSILON )
	{
		ret = intersectRayAABB(bmin,bmax,p1,dir,intersect);
		if ( ret )
		{
			float dx = p1.x-intersect.x;
			float dy = p1.y-intersect.y;
			float dz = p1.z-intersect.z;
			float d = dx*dx+dy*dy+dz*dz;
			if ( d < dist*dist )
			{
				dist = sqrtf(d);
			}
			else
			{
				ret = false;
			}
		}
	}
	return ret;
}

/* a = b - c */
#define vector(a,b,c) \
	(a).x = (b).x - (c).x;	\
	(a).y = (b).y - (c).y;	\
	(a).z = (b).z - (c).z;

#define innerProduct(v,q) \
	((v).x * (q).x + \
	(v).y * (q).y + \
	(v).z * (q).z)

#define crossProduct(a,b,c) \
	(a).x = (b).y * (c).z - (c).y * (b).z; \
	(a).y = (b).z * (c).x - (c).z * (b).x; \
	(a).z = (b).x * (c).y - (c).x * (b).y;


static inline bool rayIntersectsTriangle(VERTEX p,VERTEX d, VERTEX v0, VERTEX v1, VERTEX v2,float &t)
{
	VERTEX e1,e2,h,s,q;
	float a,f,u,v;

	vector(e1,v1,v0);
	vector(e2,v2,v0);
	crossProduct(h,d,e2);
	a = innerProduct(e1,h);

	if (a > -0.00001 && a < 0.00001)
		return(false);

	f = 1/a;
	vector(s,p,v0);
	u = f * (innerProduct(s,h));

	if (u < 0.0 || u > 1.0)
		return(false);

	crossProduct(q,s,e1);
	v = f * innerProduct(d,q);
	if (v < 0.0 || u + v > 1.0)
		return(false);
	// at this stage we can compute t to find out where
	// the intersection point is on the line
	t = f * innerProduct(e2,q);
	if (t > 0) // ray intersection
		return(true);
	else // this means that there is a line intersection
		// but not a ray intersection
		return (false);
}

static float computePlane(VERTEX A,VERTEX B, VERTEX C, VERTEX *n) // returns D
{
	float vx = (B.x - C.x);
	float vy = (B.y - C.y);
	float vz = (B.z - C.z);

	float wx = (A.x - B.x);
	float wy = (A.y - B.y);
	float wz = (A.z - B.z);

	float vw_x = vy * wz - vz * wy;
	float vw_y = vz * wx - vx * wz;
	float vw_z = vx * wy - vy * wx;

	float mag = sqrt((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z));

	if ( mag < 0.000001f )
	{
		mag = 0;
	}
	else
	{
		mag = 1.0f/mag;
	}

	float x = vw_x * mag;
	float y = vw_y * mag;
	float z = vw_z * mag;


	float D = 0.0f - ((x*A.x)+(y*A.y)+(z*A.z));

	n->x = x;
	n->y = y;
	n->z = z;

	return D;
}


#define TRI_EOF 0xFFFFFFFF

enum AxisAABB
{
	AABB_XAXIS,
	AABB_YAXIS,
	AABB_ZAXIS
};

enum ClipCode
{
	CC_MINX  =       (1<<0),
	CC_MAXX  =       (1<<1),
	CC_MINY  =       (1<<2),
	CC_MAXY	 =       (1<<3),
	CC_MINZ  =       (1<<4),
	CC_MAXZ  =       (1<<5),
};


class BoundsAABB
{
public:


	void setMin(VERTEX v)
	{
		mMin.x = v.x;
		mMin.y = v.y;
		mMin.z = v.z;
	}

	void setMax(VERTEX v)
	{
		mMax.x = v.x;
		mMax.y = v.y;
		mMax.z = v.z;
	}

	void setMin(float x,float y,float z)
	{
		mMin.x = x;
		mMin.y = y;
		mMin.z = z;
	}

	void setMax(float x,float y,float z)
	{
		mMax.x = x;
		mMax.y = y;
		mMax.z = z;
	}

	void include(VERTEX v)
	{
		if ( v.x < mMin.x ) mMin.x = v.x;
		if ( v.y < mMin.y ) mMin.y = v.y;
		if ( v.z < mMin.z ) mMin.z = v.z;

		if ( v.x > mMax.x ) mMax.x = v.x;
		if ( v.y > mMax.y ) mMax.y = v.y;
		if ( v.z > mMax.z ) mMax.z = v.z;
	}

	void getCenter(VERTEX *center) const
	{
		center->x = (mMin.x + mMax.x)*0.5f;
		center->y = (mMin.y + mMax.y)*0.5f;
		center->z = (mMin.z + mMax.z)*0.5f;
	}

	bool intersects(const BoundsAABB &b) const
	{
		if ((mMin.x > b.mMax.x) || (b.mMin.x > mMax.x)) return false;
		if ((mMin.y > b.mMax.y) || (b.mMin.y > mMax.y)) return false;
		if ((mMin.z > b.mMax.z) || (b.mMin.z > mMax.z)) return false;
		return true;
	}

	bool containsTriangle(FACE f) const
	{
		BoundsAABB b;
		b.setMin(f.a);
		b.setMax(f.a);
		b.include(f.b);
		b.include(f.c);
		return intersects(b);
	}

	bool containsTriangleExact(FACE f,uint32 &orCode) const
	{
		bool ret = false;

		uint32 andCode;
		orCode = getClipCode(f, andCode);
		if ( andCode == 0 )
		{
			ret = true;
		}

		return ret;
	}

	inline uint32 getClipCode(FACE f, uint32 &andCode) const
	{
		andCode = 0xFFFFFFFF;
		uint32 c1 = getClipCode(f.a);
		uint32 c2 = getClipCode(f.b);
		uint32 c3 = getClipCode(f.c);
		andCode&=c1;
		andCode&=c2;
		andCode&=c3;
		return c1|c2|c3;
	}

	inline uint32 getClipCode(VERTEX p) const
	{
		uint32 ret = 0;

		if ( p.x < mMin.x ) 
		{
			ret|=CC_MINX;
		}
		else if ( p.x > mMax.x )
		{
			ret|=CC_MAXX;
		}

		if ( p.y < mMin.y ) 
		{
			ret|=CC_MINY;
		}
		else if ( p.y > mMax.y )
		{
			ret|=CC_MAXY;
		}

		if ( p.z < mMin.z ) 
		{
			ret|=CC_MINZ;
		}
		else if ( p.z > mMax.z )
		{
			ret|=CC_MAXZ;
		}

		return ret;
	}

	inline void clamp(const BoundsAABB &aabb)
	{
		if ( mMin.x < aabb.mMin.x ) mMin.x = aabb.mMin.x;
		if ( mMin.y < aabb.mMin.y ) mMin.y = aabb.mMin.y;
		if ( mMin.z < aabb.mMin.z ) mMin.z = aabb.mMin.z;
		if ( mMax.x > aabb.mMax.x ) mMax.x = aabb.mMax.x;
		if ( mMax.y > aabb.mMax.y ) mMax.y = aabb.mMax.y;
		if ( mMax.z > aabb.mMax.z ) mMax.z = aabb.mMax.z;
	}

	VERTEX mMin;
	VERTEX mMax;
};


class NodeAABB;

class NodeInterface
{
public:
	virtual NodeAABB * getNode(void) = 0;
	virtual void getFaceNormal(uint32 tri, VERTEX *faceNormal) = 0;
};

	class NodeAABB
	{
	public:
		NodeAABB(void)
		{
			mLeft = NULL;
			mRight = NULL;
			mLeafTriangleIndex= TRI_EOF;
		}

		NodeAABB(uint32 FaceCount,PFACE Faces,
			uint32 maxDepth,	// Maximum recursion depth for the triangle mesh.
			uint32 minLeafSize,	// minimum triangles to treat as a 'leaf' node.
			float	minAxisSize,
			NodeInterface *callback,
			TriVector &leafTriangles)	// once a particular axis is less than this size, stop sub-dividing.

		{
			mLeft = NULL;
			mRight = NULL;
			mLeafTriangleIndex = TRI_EOF;
			TriVector triangles;
			triangles.reserve(FaceCount);
			for (uint32 i=0; i<FaceCount; i++)
			{
				triangles.push_back(i);
			}
			mBounds.setMin( Faces[0].a );
			mBounds.setMax( Faces[0].a );

			for(uint32 i = 0; i < FaceCount; ++i)
			{
				mBounds.include(Faces[i].a);
				mBounds.include(Faces[i].b);
				mBounds.include(Faces[i].c);
			}
			split(triangles, FaceCount, Faces, 0, maxDepth, minLeafSize, minAxisSize, callback, leafTriangles);
		}

		NodeAABB(const BoundsAABB &aabb)
		{
			mBounds = aabb;
			mLeft = NULL;
			mRight = NULL;
			mLeafTriangleIndex = TRI_EOF;
		}

		~NodeAABB(void)
		{
		}

		// here is where we split the mesh..
		void split(const TriVector &triangles, uint32 FaceCount, PFACE Faces,
			uint32 depth,
			uint32 maxDepth,	// Maximum recursion depth for the triangle mesh.
			uint32 minLeafSize,	// minimum triangles to treat as a 'leaf' node.
			float	minAxisSize,
			NodeInterface *callback,
			TriVector &leafTriangles)	// once a particular axis is less than this size, stop sub-dividing.

		{
			// Find the longest axis of the bounding volume of this node
			float dx = mBounds.mMax.x - mBounds.mMin.x;
			float dy = mBounds.mMax.y - mBounds.mMin.y;
			float dz = mBounds.mMax.z - mBounds.mMin.z;

			AxisAABB axis = AABB_XAXIS;
			float laxis = dx;

			if ( dy > dx )
			{
				axis = AABB_YAXIS;
				laxis = dy;
			}

			if ( dz > dx && dz > dy )
			{
				axis = AABB_ZAXIS;
				laxis = dz;
			}

			uint32 count = triangles.size();

			// if the number of triangles is less than the minimum allowed for a leaf node or...
			// we have reached the maximum recursion depth or..
			// the width of the longest axis is less than the minimum axis size then...
			// we create the leaf node and copy the triangles into the leaf node triangle array.
			if ( count < minLeafSize || depth >= maxDepth || laxis < minAxisSize )
			{ 
				// Copy the triangle indices into the leaf triangles array
				mLeafTriangleIndex = leafTriangles.size(); // assign the array start location for these leaf triangles.
				leafTriangles.push_back(count);
				for (TriVector::const_iterator i=triangles.begin(); i!=triangles.end(); ++i)
				{
					uint32 tri = *i;
					leafTriangles.push_back(tri);
				}
			}
			else
			{
				VERTEX center;
				mBounds.getCenter(&center);
				BoundsAABB b1,b2;
				splitRect(axis,mBounds,b1,b2,center);

				// Compute two bounding boxes based upon the split of the longest axis

				BoundsAABB leftBounds,rightBounds;

				TriVector leftTriangles;
				TriVector rightTriangles;


				// Create two arrays; one of all triangles which intersect the 'left' half of the bounding volume node
				// and another array that includes all triangles which intersect the 'right' half of the bounding volume node.
				for (TriVector::const_iterator i=triangles.begin(); i!=triangles.end(); ++i)
				{

					uint32 tri = (*i); 

					{
						uint32 addCount = 0;
						uint32 orCode=0xFFFFFFFF;
						if ( b1.containsTriangleExact(Faces[tri],orCode))
						{
							addCount++;
							if ( leftTriangles.empty() )
							{
								leftBounds.setMin(Faces[tri].a);
								leftBounds.setMax(Faces[tri].a);
							}
							leftBounds.include(Faces[tri].a);
							leftBounds.include(Faces[tri].b);
							leftBounds.include(Faces[tri].c);
							leftTriangles.push_back(tri); // Add this triangle to the 'left triangles' array and revise the left triangles bounding volume
						}
						// if the orCode is zero; meaning the triangle was fully self-contiained int he left bounding box; then we don't need to test against the right
						if ( orCode && b2.containsTriangleExact(Faces[tri],orCode))
						{
							addCount++;
							if ( rightTriangles.empty() )
							{
								rightBounds.setMin(Faces[tri].a);
								rightBounds.setMax(Faces[tri].a);
							}
							rightBounds.include(Faces[tri].a);
							rightBounds.include(Faces[tri].b);
							rightBounds.include(Faces[tri].c);
							rightTriangles.push_back(tri); // Add this triangle to the 'right triangles' array and revise the right triangles bounding volume.
						}
						assert( addCount );
					}
				}

				if ( !leftTriangles.empty() ) // If there are triangles in the left half then...
				{
					leftBounds.clamp(b1); // we have to clamp the bounding volume so it stays inside the parent volume.
					mLeft = callback->getNode();	// get a new AABB node
					new ( mLeft ) NodeAABB(leftBounds);		// initialize it to default constructor values.  
					// Then recursively split this node.
					mLeft->split(leftTriangles, FaceCount, Faces, depth+1, maxDepth, minLeafSize, minAxisSize, callback, leafTriangles);
				}

				if ( !rightTriangles.empty() ) // If there are triangles in the right half then..
				{
					rightBounds.clamp(b2);	// clamps the bounding volume so it stays restricted to the size of the parent volume.
					mRight = callback->getNode(); // allocate and default initialize a new node
					new ( mRight ) NodeAABB(rightBounds);
					// Recursively split this node.
					mRight->split(rightTriangles, FaceCount, Faces, depth+1, maxDepth, minLeafSize, minAxisSize, callback, leafTriangles);
				}

			}
		}

		void splitRect(AxisAABB axis,const BoundsAABB &source,BoundsAABB &b1,BoundsAABB &b2,VERTEX midpoint)
		{
			switch ( axis )
			{
				case AABB_XAXIS:
					{
						b1.setMin( source.mMin );
						b1.setMax( midpoint.x, source.mMax.y, source.mMax.z );

						b2.setMin( midpoint.x, source.mMin.y, source.mMin.z );
						b2.setMax(source.mMax);
					}
					break;
				case AABB_YAXIS:
					{
						b1.setMin(source.mMin);
						b1.setMax(source.mMax.x, midpoint.y, source.mMax.z);

						b2.setMin(source.mMin.x, midpoint.y, source.mMin.z);
						b2.setMax(source.mMax);
					}
					break;
				case AABB_ZAXIS:
					{
						b1.setMin(source.mMin);
						b1.setMax(source.mMax.x, source.mMax.y, midpoint.z);

						b2.setMin(source.mMin.x, source.mMin.y, midpoint.z);
						b2.setMax(source.mMax);
					}
					break;
			}
		}


		virtual void raycast(bool &hit,
							VERTEX from,
							VERTEX to,
							VERTEX dir,
							VERTEX *hitLocation,
							VERTEX *hitNormal,
							float *hitDistance,
							PFACE Faces,
							float &nearestDistance,
							NodeInterface *callback,
							uint32 *raycastTriangles,
							uint32 raycastFrame,
							const TriVector &leafTriangles,
							uint32 &nearestTriIndex)
		{
			VERTEX sect;
			float nd = nearestDistance;
			if ( !intersectLineSegmentAABB(mBounds.mMin,mBounds.mMax,from,dir,nd,sect) )
			{
				return;	
			}
			if ( mLeafTriangleIndex != TRI_EOF )
			{
				const uint32 *scan = &leafTriangles[mLeafTriangleIndex];
				uint32 count = *scan++;
				for (uint32 i=0; i<count; i++)
				{
					uint32 tri = *scan++;
					// This is raycastFrame check is checking if we checked this triangle during this call to raycast
					//
					if ( raycastTriangles[tri] != raycastFrame )
					{
						raycastTriangles[tri] = raycastFrame;

						float t;
						if ( rayIntersectsTriangle(from, dir, Faces[tri].a, Faces[tri].b, Faces[tri].c, t))
						{
							bool accept = false;
							if ( t == nearestDistance && tri < nearestTriIndex )
							{
								accept = true;
							}
							if ( t < nearestDistance || accept )
							{
								nearestDistance = t;
								if ( hitLocation )
								{
									hitLocation->x = from.x + dir.x * t;
									hitLocation->y = from.y + dir.y * t;
									hitLocation->z = from.z + dir.z * t;
								}
								if ( hitNormal )
								{
									callback->getFaceNormal(tri,hitNormal);
								}
								if ( hitDistance )
								{
									*hitDistance = t;
								}
								nearestTriIndex = tri;
								hit = true;
							}
						}
					}
				}
			}
			else
			{
				if ( mLeft )
				{
					mLeft->raycast(hit,from,to,dir,hitLocation,hitNormal,hitDistance, Faces, nearestDistance,callback,raycastTriangles,raycastFrame,leafTriangles,nearestTriIndex);
				}
				if ( mRight )
				{
					mRight->raycast(hit,from,to,dir,hitLocation,hitNormal,hitDistance, Faces, nearestDistance,callback,raycastTriangles,raycastFrame,leafTriangles,nearestTriIndex);
				}
			}
		}

		void Dump(uint32 Depth)
		{
			//if(Depth > 3)
			//	return;

			printf("Dump at depth %i, mLeafTriangleIndex is %i\n", Depth, mLeafTriangleIndex);
			if((mLeafTriangleIndex != 0) && (mLeafTriangleIndex != 0xFFFFFFFF))
				abort();
			if(mLeft)
				mLeft->Dump(Depth + 1);

			if(mRight)
				mLeft->Dump(Depth + 1);
		}

		NodeAABB		*mLeft;			// left node
		NodeAABB		*mRight;		// right node
		BoundsAABB		mBounds;		// bounding volume of node
		uint32		mLeafTriangleIndex;	// if it is a leaf node; then these are the triangle indices.
	};

class MyRaycastMesh : public RaycastMesh, public NodeInterface
{
public:

	MyRaycastMesh(uint32 FaceCount, PFACE Faces, uint32 maxDepth,uint32 minLeafSize,float minAxisSize)
	{
		mRaycastFrame = 0;
		if ( maxDepth < 2 )
		{
			maxDepth = 2;
		}
		if ( maxDepth > 15 )
		{
			maxDepth = 15;
		}
		uint32 pow2Table[16] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 65536 };
		mMaxNodeCount = 0;
		for (uint32 i=0; i<=maxDepth; i++)
		{
			mMaxNodeCount+=pow2Table[i];
		}
		mNodes = new NodeAABB[mMaxNodeCount];
		mNodeCount = 0;
		mFaceCount = FaceCount;
		mFaces = Faces;
		mRaycastTriangles = (uint32 *)::malloc(mFaceCount*sizeof(uint32));
		memset(mRaycastTriangles,0,mFaceCount*sizeof(uint32));
		mRoot = getNode();
		mFaceNormals = NULL;
		new ( mRoot ) NodeAABB(mFaceCount,mFaces, maxDepth,minLeafSize,minAxisSize,this,mLeafTriangles);
	}

	~MyRaycastMesh(void)
	{
		delete []mNodes;
		::free(mFaceNormals);
		::free(mRaycastTriangles);
	}
	
	void Dump(uint32 Depth)
	{
		mRoot->Dump(Depth);
	}

	virtual bool raycast(VERTEX from, VERTEX to, VERTEX *hitLocation, VERTEX *hitNormal, float *hitDistance, FACE **hitFace)
	{
		bool ret = false;

		VERTEX dir(to.x - from .x, to.y - from.y, to.z - from.z);
		
		float distance = sqrtf( dir.x*dir.x + dir.y*dir.y+dir.z*dir.z );
		if ( distance < 0.0000000001f ) return false;
		float recipDistance = 1.0f / distance;
		dir.x*=recipDistance;
		dir.y*=recipDistance;
		dir.z*=recipDistance;
		mRaycastFrame++;
		uint32 nearestTriIndex=TRI_EOF;
		mRoot->raycast(ret,from,to,dir,hitLocation,hitNormal,hitDistance,mFaces,distance,this,mRaycastTriangles,mRaycastFrame,mLeafTriangles,nearestTriIndex);

		if((nearestTriIndex != TRI_EOF) && hitFace)
			*hitFace = &mFaces[nearestTriIndex];

		return ret;
	}

	virtual void release(void)
	{
		delete this;
	}

	virtual const VERTEX getBoundMin(void) const // return the minimum bounding box
	{
		return mRoot->mBounds.mMin;
	}
	virtual const VERTEX getBoundMax(void) const // return the maximum bounding box.
	{
		return mRoot->mBounds.mMax;
	}

	virtual NodeAABB * getNode(void) 
	{
		assert( mNodeCount < mMaxNodeCount );
		NodeAABB *ret = &mNodes[mNodeCount];
		mNodeCount++;
		return ret;
	}

	virtual void getFaceNormal(uint32 tri, VERTEX *faceNormal) 
	{
		if ( mFaceNormals == NULL )
		{
			mFaceNormals = (VERTEX *)::malloc(sizeof(VERTEX) * mFaceCount);
			
			for (uint32 i = 0; i < mFaceCount; ++i)
			{
				VERTEX *dest = &mFaceNormals[i];
				computePlane(mFaces[i].c, mFaces[i].b, mFaces[i].a, dest);
			}
		}
		VERTEX *src = &mFaceNormals[tri];
		faceNormal = src;
	}

	virtual bool bruteForceRaycast(VERTEX from, VERTEX to,VERTEX *hitLocation,VERTEX *hitNormal,float *hitDistance)
	{
		bool ret = false;

		VERTEX dir;

		dir.x = to.x - from.x;
		dir.y = to.y - from.y;
		dir.z = to.z - from.z;

		float distance = sqrtf( dir.x*dir.x + dir.y*dir.y+dir.z*dir.z );
		if ( distance < 0.0000000001f ) return false;
		float recipDistance = 1.0f / distance;
		dir.x*=recipDistance;
		dir.y*=recipDistance;
		dir.z*=recipDistance;
		float nearestDistance = distance;

		for (uint32 tri=0; tri<mFaceCount; ++tri)
		{
			float t;
			if ( rayIntersectsTriangle(from,dir,mFaces[tri].a, mFaces[tri].b, mFaces[tri].c, t))
			{
				if ( t < nearestDistance )
				{
					nearestDistance = t;
					if ( hitLocation )
					{
						hitLocation->x = from.x+dir.x*t;
						hitLocation->y = from.y+dir.y*t;
						hitLocation->z = from.z+dir.z*t;
					}

					if ( hitNormal )
					{
						getFaceNormal(tri,hitNormal);
					}

					if ( hitDistance )
					{
						*hitDistance = t;
					}
					ret = true;
				}
			}
		}
		return ret;
	}

	uint32		mRaycastFrame;
	uint32		*mRaycastTriangles;
	PFACE		mFaces;
	uint32		mFaceCount;
	VERTEX		*mFaceNormals;
	NodeAABB	*mRoot;
	uint32		mNodeCount;
	uint32		mMaxNodeCount;
	NodeAABB	*mNodes;
	TriVector	mLeafTriangles;
};

};



using namespace RAYCAST_MESH;


RaycastMesh * createRaycastMesh(uint32 FaceCount,
				PFACE Faces,
				uint32 maxDepth,	// Maximum recursion depth for the triangle mesh.
				uint32 minLeafSize,	// minimum triangles to treat as a 'leaf' node.
				float	minAxisSize	// once a particular axis is less than this size, stop sub-dividing.
				)
{
	MyRaycastMesh *m = new MyRaycastMesh(FaceCount,Faces,maxDepth,minLeafSize,minAxisSize);
	return static_cast< RaycastMesh * >(m);
}



