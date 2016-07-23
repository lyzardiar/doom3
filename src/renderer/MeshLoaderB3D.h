/*
http://www.blitzbasic.com/sdkspecs/sdkspecs/b3dfile_specs.txt
*/

#ifndef __MESHLOADB3D_H__
#define __MESHLOADB3D_H__


class MeshLoaderB3D
{
public:
	MeshLoaderB3D();
	~MeshLoaderB3D();

	struct SB3dChunkHeader
	{
		char name[4];
		int size;
	};

	struct SB3dTexture
	{
		idStr TextureName;
		int Flags;
		int Blend;
		float Xpos;
		float Ypos;
		float Xscale;
		float Yscale;
		float Angle;
	};

	struct SB3dMaterial
	{
		SB3dMaterial() : red(1.0f), green(1.0f),
			blue(1.0f), alpha(1.0f), shininess(0.0f), blend(1),
			fx(0)
		{
		//	for (unsigned int i=0; i<3; ++i)
				//Textures[i]=0;
		}
		//video::SMaterial Material;
		float red, green, blue, alpha;
		float shininess;
		int blend,fx;
		//SB3dTexture *Textures[video::MATERIAL_MAX_TEXTURES];
	};

	bool Load(const char* file) ;

	void ReadTexs();
	bool ReadVrts();
	void ReadNode();
	void ReadBrus();
	void ReadBone();
	void ReadMesh();
	void ReadTris();
	void ReadAnim();
	void ReadKey();

	idStr ReadChunk();
	bool CheckSize();
	void ExitChunk();

	void PrintTree(const char *psz, ...);

	long _curpos;
	int _stackIndex;
	idList<unsigned int> _stack;
	idList<SB3dTexture> _textures;
	idList<unsigned short> _indices;
	idList<modelSurface_t> _surfaces;

	idFile*  _file;
};

#endif
