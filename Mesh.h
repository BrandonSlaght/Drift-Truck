#pragma once
#include "float2.h"
#include "float3.h"
#include <vector>

class   Mesh
{
	struct  Face
	{
		int       positionIndices[4];
		int       normalIndices[4];
		int       texcoordIndices[4];
		bool      isQuad;
	};

	std::vector<std::string*>	rows;
	std::vector<float3*>		positions;
	std::vector<std::vector<Face*> >          submeshFaces;
	std::vector<float3*>		normals;
	std::vector<float2*>		texcoords;

	int            modelid;

public:
	Mesh(const char *filename);
	~Mesh();

	void        draw();
	void        drawSubmesh(unsigned int iSubmesh);
};

