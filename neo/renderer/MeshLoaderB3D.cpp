#include "../idlib/precompiled.h"
#pragma hdrstop

#include "MeshLoaderB3D.h"
#include "tr_local.h"


MeshLoaderB3D::MeshLoaderB3D() : _stackIndex(0) {
}

MeshLoaderB3D::~MeshLoaderB3D() {
}

bool MeshLoaderB3D::Load(const char* filename) {
	_file = fileSystem->OpenFileRead( filename );
	if ( !_file ) {
		return false;

	}

	idStr head = ReadChunk();
	int nB3DVersion;
	_file->ReadInt(nB3DVersion);

	Sys_Printf("load b3d file %s, version %s %d\n", filename, head.c_str(), nB3DVersion);

	while( CheckSize() ) {
		idStr t = ReadChunk();
		if (t == "TEXS")
			ReadTexs();
		else if (t == "BRUS")
			ReadBrus();
		else if (t == "NODE")
			ReadNode();
			//_mesh->SetJoint(ReadNode());

		ExitChunk();
	}

	fileSystem->CloseFile( _file );
	_file = NULL;

	// The MESH chunk describes a mesh. 
	// A mesh only has one VRTS chunk, but potentially many TRIS chunks.
	//srfTriangles_t* tri = _mesh->GetGeometries(0);
	//tri->numIndices = _indices.size();
	//_indices.set_free_when_destroyed(false);
	//tri->indices = _indices.pointer();

	return true;
}


bool MeshLoaderB3D::ReadVrts() {
	const int max_tex_coords = 3;
	int flags, tex_coord_sets, tex_coord_set_size;

	_file->ReadInt(flags);
	_file->ReadInt(tex_coord_sets);
	_file->ReadInt(tex_coord_set_size);

	if (tex_coord_sets >= max_tex_coords || tex_coord_set_size >= 4) // Something is wrong 
	{
		Sys_Printf("tex_coord_sets or tex_coord_set_size too big");
		return false;
	}

	//------ Allocate Memory, for speed -----------//

	int sizeOfVertex = 3;
	bool hasNormal = false;
	bool hasVertexColors = false;
	if (flags & 1) {
		hasNormal = true;
		sizeOfVertex += 3;
	}
	if (flags & 2) {
		sizeOfVertex += 4;
		hasVertexColors=true;
	}

	sizeOfVertex += tex_coord_sets*tex_coord_set_size;
	unsigned int size = _stack[_stackIndex-1] - _file->Tell();
	//106407 16800
	unsigned int numVertex = size / sizeof(float) ;
	numVertex /= sizeOfVertex;

	srfTriangles_t* tri = R_AllocStaticTriSurf();
	R_AllocStaticTriSurfVerts(tri, numVertex);
	tri->numVerts = numVertex;

	int idx = 0;
	while( CheckSize()) {
		float color[4]={1.0f, 1.0f, 1.0f, 1.0f};
		_file->ReadVec3(tri->verts[idx].xyz);
		tri->verts[idx].xyz *= 10.f;

		if (flags & 1) {
			_file->ReadVec3(tri->verts[idx].normal);
		}
		if (flags & 2) {
			_file->ReadFloat(color[0]);
			_file->ReadFloat(color[1]);
			_file->ReadFloat(color[2]);
			_file->ReadFloat(color[3]);
		}
		float u, v;
		for (int i = 0; i < tex_coord_sets; ++i) {
			//for (int j = 0; j < tex_coord_set_size; ++j)
			{
				_file->ReadFloat(u);
				_file->ReadFloat(v);
				//v = 1 - v;
			}
		}

		tri->verts[idx].st = idVec2(u, v);
		idx++;
	}

	modelSurface_t modelSurf;
	modelSurf.id = _surfaces.Size();
	modelSurf.shader = declManager->FindMaterial("textures/ninja/ninja");
	modelSurf.geometry = tri;
	_surfaces.Append(modelSurf);

	return true;
}

idStr MeshLoaderB3D::ReadChunk()
{
	idStr s;
	for (int i = 0; i < 4;++i) {
		char c;
		_file->ReadChar(c);
		s += c;
	}
	
	int size;
	_file->ReadInt(size);
	unsigned int pos = _file->Tell();
	_stack.Append(pos + size);
	_stackIndex++;
	return s;
}

bool MeshLoaderB3D::CheckSize()
{
	unsigned int pos = _file->Tell();
	unsigned int size = _stack[_stackIndex-1];
	return size > pos;
}

void MeshLoaderB3D::ExitChunk()
{
	_curpos = _stack[_stackIndex-1];
	_stack.RemoveIndex(_stackIndex-1);
	--_stackIndex;
}

static void readString(idFile* file, idStr& str)
{
	char c;
	file->ReadChar(c);

	while (c)
	{
		str += c;
		if (c=='/0')
			break;
		file->ReadChar(c);
	}
}

void MeshLoaderB3D::ReadTexs()
{
	while (CheckSize()) {
		PrintTree("read texs \n");
		SB3dTexture tex;
		readString(_file, tex.TextureName);
		_file->ReadInt(tex.Flags);
		 _file->ReadInt(tex.Blend);
		_file->ReadFloat(tex.Xpos);
		_file->ReadFloat(tex.Ypos);
		_file->ReadFloat(tex.Xscale);
		_file->ReadFloat(tex.Yscale);
		_file->ReadFloat(tex.Angle);
		_textures.Insert(tex);
	}
}

void MeshLoaderB3D::ReadNode()
{
	//Joint* joint = new Joint;

	idStr str;
	readString(_file, str);
	PrintTree(str.c_str());

	idVec3 t;
	idVec3 s;
	_file->ReadVec3(t);
	_file->ReadVec3(s);
	idQuat r; 
	_file->ReadFloat(r.x);
	_file->ReadFloat(r.y);
	_file->ReadFloat(r.z);
	_file->ReadFloat(r.w);

	//joint->name = str;
	//joint->position = t;
	//joint->scale = s;
	//joint->rotation = r;

	while( CheckSize() ){
		idStr t = ReadChunk();
		if( t=="MESH" ){
			ReadMesh();
		}else if( t=="BONE" ){
			ReadBone();
		}else if( t=="ANIM" ){
			ReadAnim();
		}else if( t=="KEYS" ){
			ReadKey();
		}else if( t=="NODE" ){
			ReadNode();
			//Joint* child = ReadNode();
			//Sys_Printf("parent %s children %s\n", joint->name.c_str(), child->name.c_str());
			//joint->children.push_back(child);
			//child->parent = joint;
		}
		ExitChunk();
	}
}

void MeshLoaderB3D::ReadBrus()
{
	int n_texs;
	_file->ReadInt(n_texs);
	if( n_texs<0 || n_texs>8 ){
		printf( "Bad texture count" );
	}
	while( CheckSize() ){
		idStr name;
		readString(_file, name);
		PrintTree(name.c_str());
		idVec3 color;
		_file->ReadVec3(color);
		float alpha;
		_file->ReadFloat(alpha);
		float shiny;
		_file->ReadFloat(shiny);
		int blend;
		/*int blend=**/_file->ReadInt(blend);
		int fx;
		_file->ReadInt(fx);

		//Textures
		for( int i=0;i<n_texs;++i ){
			int texid;
			_file->ReadInt(texid);
		}
	}
}

void MeshLoaderB3D::ReadBone( )
{
	int i = 0;
	while( CheckSize() ){
		int vertex;
		_file->ReadInt(vertex);
		float weight;
		_file->ReadFloat(weight);
		//joint->vertexIndices.push_back(vertex);
		//joint->vertexWeights.push_back(weight);
		i++;
	}
	PrintTree("vertex count: %d", i);
}

void MeshLoaderB3D::ReadMesh() {
	int matId;
	/*int matid=*/_file->ReadInt(matId);

	//printTree("mesh");
	while( CheckSize() ){
		idStr t = ReadChunk();
		if( t=="VRTS" ){
			ReadVrts();
		}else if( t=="TRIS" ){
			ReadTris();
		}
		ExitChunk();
	}
}

void MeshLoaderB3D::PrintTree(const char *psz, ...) {
	char sBuf[128];
	va_list ap;
	va_start(ap, psz);
	vsnprintf_s(sBuf, 128, 128, psz, ap);
	va_end(ap);

	for (int i = 0; i < _stack.Num();i++)
		common->Printf("-");

	common->Printf(sBuf);
}

void MeshLoaderB3D::ReadTris(){
	int matid;
	_file->ReadInt(matid);
	if( matid==-1 ){
		matid=0;
	}
	int size = _stack[_stackIndex-1] - _file->Tell();
	int n_tris=size/12;

	for( int i=0;i<n_tris;++i ){
		unsigned int i0, i1, i2;
		_file->ReadUnsignedInt(i0);
		_file->ReadUnsignedInt(i1);
		_file->ReadUnsignedInt(i2);

		_indices.Insert(i0);
		_indices.Insert(i1);
		_indices.Insert(i2);
	}
}



void MeshLoaderB3D::ReadAnim(){
	int flag, frames;
	float fps;
	/*int flags=*/_file->ReadInt(flag);
	_file->ReadInt(frames);
	/*float fps = */_file->ReadFloat(fps);
	//_mesh->SetNumFrames(frames);
}

void MeshLoaderB3D::ReadKey()
{
	int flags;
	_file->ReadInt(flags);
	while( CheckSize() ){
		int frame;
		_file->ReadInt(frame);
		if (flags & 1){
			idVec3 scale;
			 _file->ReadVec3(scale);
		}
		if( flags & 2 ){
			idVec3 pos;
			_file->ReadVec3(pos);
		}
		if( flags & 4 ){
			idVec4 quat;
			_file->ReadVec4(quat);
		}
	}
}