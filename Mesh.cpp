#include <fstream>
#include <algorithm> 

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
// Needed on MsWindows
#include <windows.h>
#endif // Win32 platform

#include <GL/gl.h>
#include <GL/glu.h>
// Download glut from: http://www.opengl.org/resources/libraries/glut/
#include <GL/glut.h>

#include "mesh.h"


using namespace std;

Mesh::Mesh(const char *filename)
{
	fstream file(filename); 
	if(!file.is_open())       
	{
		return;
	}

	char buffer[256];
	while(!file.eof())
	{
		file.getline(buffer,256);
		rows.push_back(new string(buffer));
	}

	submeshFaces.push_back(std::vector<Face*>());
	std::vector<Face*>* faces = &submeshFaces.at(submeshFaces.size()-1);

	for(int i = 0; i < rows.size(); i++)
	{
		if(rows[i]->empty() || (*rows[i])[0] == '#') 
			continue;      
		else if((*rows[i])[0] == 'v' && (*rows[i])[1] == ' ')
		{
			float tmpx,tmpy,tmpz;
			sscanf_s(rows[i]->c_str(), "v %f %f %f" ,&tmpx,&tmpy,&tmpz);      
			positions.push_back(new float3(tmpx,tmpy,tmpz));  
		}
		else if((*rows[i])[0] == 'v' && (*rows[i])[1] == 'n')    
		{
			float tmpx,tmpy,tmpz;   
			sscanf_s(rows[i]->c_str(), "vn %f %f %f" ,&tmpx,&tmpy,&tmpz);
			normals.push_back(new float3(tmpx,tmpy,tmpz));     
		}
		else if((*rows[i])[0] == 'v' && (*rows[i])[1] == 't')
		{
			float tmpx,tmpy;
			sscanf_s(rows[i]->c_str(), "vt %f %f" ,&tmpx,&tmpy);
			texcoords.push_back(new float2(tmpx,tmpy));     
		}
		else if((*rows[i])[0] == 'f')  
		{
			if(count(rows[i]->begin(),rows[i]->end(), ' ') == 3)
			{
				Face* f = new Face();
				f->isQuad = false;
				sscanf_s(rows[i]->c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d",
					&f->positionIndices[0], &f->texcoordIndices[0], &f->normalIndices[0],
					&f->positionIndices[1], &f->texcoordIndices[1], &f->normalIndices[1],
					&f->positionIndices[2], &f->texcoordIndices[2], &f->normalIndices[2]);
				faces->push_back(f);
			}
			else
			{
				Face* f = new Face();
				f->isQuad = true;
				sscanf_s(rows[i]->c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", 
					&f->positionIndices[0], &f->texcoordIndices[0], &f->normalIndices[0],
					&f->positionIndices[1], &f->texcoordIndices[1], &f->normalIndices[1],
					&f->positionIndices[2], &f->texcoordIndices[2], &f->normalIndices[2],
					&f->positionIndices[3], &f->texcoordIndices[3], &f->normalIndices[3]);
				faces->push_back(f);   
			}
		}
		else if((*rows[i])[0] == 'g')
		{
			if(faces->size() > 0)
			{
				submeshFaces.push_back(std::vector<Face*>());
				faces = &submeshFaces.at(submeshFaces.size()-1);
			}
		}
	}

	modelid = glGenLists(submeshFaces.size());

	for(int iSubmesh=0; iSubmesh<submeshFaces.size(); iSubmesh++)
	{
		std::vector<Face*>& faces = submeshFaces.at(iSubmesh);

		glNewList(modelid + iSubmesh,GL_COMPILE);     

		glBegin(GL_TRIANGLES);
		for(int i=0;i<faces.size();i++)
		{
			//printf("%i\n", normals.size());
			//printf("%i\n", faces.size());
			//printf("%i\n", faces[i]->normalIndices[1]);
			if(faces[i]->isQuad)
			{
				glNormal3f(normals[faces[i]->normalIndices[0]-1]->x,normals[faces[i]->normalIndices[0]-1]->y,normals[faces[i]->normalIndices[0]-1]->z);
				glTexCoord2f(texcoords[faces[i]->texcoordIndices[0]-1]->x,1-texcoords[faces[i]->texcoordIndices[0]-1]->y);
				glVertex3f(positions[faces[i]->positionIndices[0]-1]->x,positions[faces[i]->positionIndices[0]-1]->y,positions[faces[i]->positionIndices[0]-1]->z);
				glNormal3f(normals[faces[i]->normalIndices[1]-1]->x,normals[faces[i]->normalIndices[1]-1]->y,normals[faces[i]->normalIndices[1]-1]->z);
				glTexCoord2f(texcoords[faces[i]->texcoordIndices[1]-1]->x,1-texcoords[faces[i]->texcoordIndices[1]-1]->y);
				glVertex3f(positions[faces[i]->positionIndices[1]-1]->x,positions[faces[i]->positionIndices[1]-1]->y,positions[faces[i]->positionIndices[1]-1]->z);
				glNormal3f(normals[faces[i]->normalIndices[2]-1]->x,normals[faces[i]->normalIndices[2]-1]->y,normals[faces[i]->normalIndices[2]-1]->z);
				glTexCoord2f(texcoords[faces[i]->texcoordIndices[2]-1]->x,1-texcoords[faces[i]->texcoordIndices[2]-1]->y);
				glVertex3f(positions[faces[i]->positionIndices[2]-1]->x,positions[faces[i]->positionIndices[2]-1]->y,positions[faces[i]->positionIndices[2]-1]->z);

				glNormal3f(normals[faces[i]->normalIndices[1]-1]->x,normals[faces[i]->normalIndices[1]-1]->y,normals[faces[i]->normalIndices[1]-1]->z);
				glTexCoord2f(texcoords[faces[i]->texcoordIndices[1]-1]->x,1-texcoords[faces[i]->texcoordIndices[1]-1]->y);
				glVertex3f(positions[faces[i]->positionIndices[1]-1]->x,positions[faces[i]->positionIndices[1]-1]->y,positions[faces[i]->positionIndices[1]-1]->z);
				glNormal3f(normals[faces[i]->normalIndices[2]-1]->x,normals[faces[i]->normalIndices[2]-1]->y,normals[faces[i]->normalIndices[2]-1]->z);
				glTexCoord2f(texcoords[faces[i]->texcoordIndices[2]-1]->x,1-texcoords[faces[i]->texcoordIndices[2]-1]->y);
				glVertex3f(positions[faces[i]->positionIndices[2]-1]->x,positions[faces[i]->positionIndices[2]-1]->y,positions[faces[i]->positionIndices[2]-1]->z);
				glNormal3f(normals[faces[i]->normalIndices[3]-1]->x,normals[faces[i]->normalIndices[3]-1]->y,normals[faces[i]->normalIndices[3]-1]->z);
				glTexCoord2f(texcoords[faces[i]->texcoordIndices[3]-1]->x,1-texcoords[faces[i]->texcoordIndices[3]-1]->y);
				glVertex3f(positions[faces[i]->positionIndices[3]-1]->x,positions[faces[i]->positionIndices[3]-1]->y,positions[faces[i]->positionIndices[3]-1]->z);

			}
			else
			{
				glNormal3f(normals[faces[i]->normalIndices[0]-1]->x,normals[faces[i]->normalIndices[0]-1]->y,normals[faces[i]->normalIndices[0]-1]->z);
				glTexCoord2f(texcoords[faces[i]->texcoordIndices[0]-1]->x,1-texcoords[faces[i]->texcoordIndices[0]-1]->y);
				glVertex3f(positions[faces[i]->positionIndices[0]-1]->x,positions[faces[i]->positionIndices[0]-1]->y,positions[faces[i]->positionIndices[0]-1]->z);
				glNormal3f(normals[faces[i]->normalIndices[1]-1]->x,normals[faces[i]->normalIndices[1]-1]->y,normals[faces[i]->normalIndices[1]-1]->z);
				glTexCoord2f(texcoords[faces[i]->texcoordIndices[1]-1]->x,1-texcoords[faces[i]->texcoordIndices[1]-1]->y);
				glVertex3f(positions[faces[i]->positionIndices[1]-1]->x,positions[faces[i]->positionIndices[1]-1]->y,positions[faces[i]->positionIndices[1]-1]->z);
				glNormal3f(normals[faces[i]->normalIndices[2]-1]->x,normals[faces[i]->normalIndices[2]-1]->y,normals[faces[i]->normalIndices[2]-1]->z);
				glTexCoord2f(texcoords[faces[i]->texcoordIndices[2]-1]->x,1-texcoords[faces[i]->texcoordIndices[2]-1]->y);
				glVertex3f(positions[faces[i]->positionIndices[2]-1]->x,positions[faces[i]->positionIndices[2]-1]->y,positions[faces[i]->positionIndices[2]-1]->z);
			}
		}
		glEnd();

		glEndList();
	}
}

void Mesh::draw()
{
	for(int iSubmesh=0; iSubmesh<submeshFaces.size(); iSubmesh++)
		glCallList(modelid + iSubmesh);
}

void Mesh::drawSubmesh(unsigned int iSubmesh)
{
	glCallList(modelid + iSubmesh);
}

Mesh::~Mesh()
{
	for(unsigned int i = 0; i < rows.size(); i++)
		delete rows[i];   
	for(unsigned int i = 0; i < positions.size(); i++)
		delete positions[i];
	for(unsigned int i = 0; i < submeshFaces.size(); i++)
		for(unsigned int j = 0; j < submeshFaces.at(i).size(); j++)
			delete submeshFaces.at(i).at(j); 
	for(unsigned int i = 0; i < normals.size(); i++)
		delete normals[i];
	for(unsigned int i = 0; i < texcoords.size(); i++)
		delete texcoords[i];
}
