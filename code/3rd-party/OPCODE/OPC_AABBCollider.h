///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*	OPCODE - Optimized Collision Detection
*	Copyright (C) 2001 Pierre Terdiman
*	Homepage: http://www.codercorner.com/Opcode.htm
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Contains code for an AABB collider.
*	\file		OPC_AABBCollider.h
*	\author		Pierre Terdiman
*	\date		January, 1st, 2002
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
struct OPCODE_API AABBCache : VolumeCache
{
	AABBCache() : FatCoeff(1.1f)
	{
		FatBox.mCenter.Zero();
		FatBox.mExtents.Zero();
	}

	// Cached faces signature
	CollisionAABB	FatBox;		//!< Box used when performing the query resulting in cached faces
								// User settings
	float			FatCoeff;	//!< mRadius2 multiplier used to create a fat sphere
};

class OPCODE_API AABBCollider : public VolumeCollider
{
public:
	// Constructor / Destructor
	AABBCollider();
	virtual								~AABBCollider();
	// Generic collision query

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	*	Generic collision query for generic OPCODE models. After the call, access the results:
	*	- with GetContactStatus()
	*	- with GetNbTouchedFaces()
	*	- with GetTouchedFaces()
	*
	*	\param		cache			[in/out] a box cache
	*	\param		box				[in] collision AABB in world space
	*	\param		model			[in] Opcode model to collide with
	*	\return		true if success
	*	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
	*/
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool			Collide(AABBCache& cache, const CollisionAABB& box, OPCODE_Model* model);

	// Collision queries
	bool			Collide(AABBCache& cache, const CollisionAABB& box, const AABBCollisionTree* tree);
	bool			Collide(AABBCache& cache, const CollisionAABB& box, const AABBNoLeafTree* tree);
	bool			Collide(AABBCache& cache, const CollisionAABB& box, const AABBQuantizedTree* tree);
	bool			Collide(AABBCache& cache, const CollisionAABB& box, const AABBQuantizedNoLeafTree* tree);
	bool			Collide(AABBCache& cache, const CollisionAABB& box, const AABBTree* tree);
	// Settings

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	*	Validates current settings. You should call this method after all the settings and callbacks have been defined for a collider.
	*	\return		nullptr if everything is ok, else a string describing the problem
	*/
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual	const char*		ValidateSettings();

protected:
	CollisionAABB	mBox;			//!< Query box in (center, extents) form
	Point			mMin;			//!< Query box min point
	Point			mMax;			//!< Query box max point
									// Leaf description
	Point			mLeafVerts[3];	//!< Triangle vertices
									// Internal methods
	void			_Collide(const AABBCollisionNode* node);
	void			_Collide(const AABBNoLeafNode* node);
	void			_Collide(const AABBQuantizedNode* node);
	void			_Collide(const AABBQuantizedNoLeafNode* node);
	void			_Collide(const AABBTreeNode* node);
	// Overlap tests
	inline_				bool			AABBContainsBox(const Point& bc, const Point& be);
	inline_				bool			AABBAABBOverlap(const Point& b, const Point& Pb);
	inline_				bool			TriBoxOverlap();
	// Init methods
	bool			InitQuery(AABBCache& cache, const CollisionAABB& box);
};
