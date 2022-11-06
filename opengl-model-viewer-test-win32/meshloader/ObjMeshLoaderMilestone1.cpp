//
//  ObjMeshLoaderMilestone1.cpp
//  OpenGLOtherTest
//
//  Created by Abraham-mac on 7/8/16.
//  Copyright © 2016 Abraham-mac. All rights reserved.
//

#include "ObjMeshLoaderMilestone1.h"

const GLdouble ONEEIGHTY_OVER_PI = 57.2957795131;
const GLfloat DEGREE_MARGIN = 25.00;

bool ObjMeshLoaderMilestone1::isSplitVertex(const GLuint & index, const GLuint & normalIndex, std::map<GLuint, std::vector<GLuint>*> * normalIndexMap) {
	return false;
}

void ObjMeshLoaderMilestone1::loadFromFile(const GLchar * path) {
	std::string line;
	std::ifstream in(path);

	if (!in.is_open()) {
		std::cout << "Error opening file: " << path << std::endl;
		return;
	}

	groups = new std::vector<Group>();

	float x, y, z;
	size_t n = 0, t = 0; //a, b, c, d, e, n;
	GLuint * indices = NULL;
	GLuint * normalIndices = NULL;
	std::size_t currentIndicesSize = 0;
	std::size_t originalVerticesSize = 0;
	std::size_t originalNormalsSize = 0;
	glm::vec3 pos;
	glm::vec3 normal;
	Vertex tmp;
	GLuint indexOffset = 0;
	GLuint groupNr = 0;
	GLuint prevGroupIndexOffset = 0;
	Group * currGroup = NULL;
	std::map<GLuint, Face*> groupIndexFacesMap;
	GLuint isSmoothingGroup = false;

	while (std::getline(in, line)) {
		std::size_t pos;
		if ((pos = line.find("v ")) != std::string::npos) // vertices
		{
			sscanf(line.c_str(), "v %f %f %f", &x, &y, &z);
			vertices->push_back(glm::vec3(x, y, z));
			originalVerticesSize++;
		}
		else if ((pos = line.find("vn ")) != std::string::npos) {
			sscanf(line.c_str(), "vn %f %f %f", &x, &y, &z);
			normals->push_back(glm::vec3(x, y, z));
			originalNormalsSize++;
		}
		else if ((pos = line.find("s ")) != std::string::npos && pos == 0) {
			pos = line.find(" ", pos);
			if (line.find(" 0", pos) == std::string::npos && line.find("off", pos) == std::string::npos) {
				isSmoothingGroup = true;
			}
			else {
				isSmoothingGroup = false;
			}

		}
		else if ((pos = line.find("f ")) != std::string::npos) {
			if (currGroup == NULL)
				currGroup = new Group();

			size_t VERTEX_COUNT = std::count(line.begin(), line.end(), ' ');
			if (line.at(line.length() - 1) == ' ' || line.at(line.length() - 2) == ' ') {
				VERTEX_COUNT--;
			}

			if (currentIndicesSize < VERTEX_COUNT) {
				delete[] indices;
				delete[] normalIndices;

				// initialize new indices arrays...
				indices = new GLuint[VERTEX_COUNT];
				normalIndices = new GLuint[VERTEX_COUNT];

				currentIndicesSize = VERTEX_COUNT;
			}
			else {
				for (int i = 0; i < currentIndicesSize; i++)
					indices[i] = normalIndices[i] = -1;
			}

			Face * face = NULL;

			glm::vec3 smoothedNormal;
			pos = 0;
			for (int i = 0; i < VERTEX_COUNT; i++) {
				// get vertex index
				pos = line.find(" ", pos);
				std::size_t start = pos + 1;
				std::size_t end = line.find_first_of("/", start);

				// check if the indices are relative (negative values) or absolute
				if (line.at(start) == '-') {
					std::sscanf(line.substr(start + 1, end).c_str(), "%zd", &indices[i]);
					indices[i] = (GLuint)(originalVerticesSize - (indices[i] - 1));
				}
				else {
					std::sscanf(line.substr(start, end).c_str(), "%zd", &indices[i]);
				}

				indices[i] = indices[i] + prevGroupIndexOffset - 1;

				// skip over texture index for now...
				start = end + 1;
				end = line.find_first_of("/", start);

				// get normal index
				pos = end;
				start = line.find_first_not_of("/", end + 1);
				if (line.at(start) == '-') {
					std::sscanf(line.substr(start + 1).c_str(), "%zd", &n);
					n = originalNormalsSize - (n - 1);
				}
				else
					std::sscanf(line.substr(start).c_str(), "%zd", &n);

				n--;
				normalIndices[i] = (GLuint)n;

				if (!isSmoothingGroup) {
					bool newVerticesFound = false;
					bool isSmoothedNormal = false;
					std::map<GLuint, Face*>::iterator faceItr = groupIndexFacesMap.find(indices[i]);
					if (faceItr != groupIndexFacesMap.end()) {
						int j = 0;
						for (j = 0; j < 3; j++)
							if (faceItr->second->vertexIndices[j] == indices[i])
								break;

						if (faceItr->second->vertexNormalIndices[j] != n) {
							newVerticesFound = true;
							glm::vec3 tmp = vertices->at(indices[i]);
							glm::vec3 dup;
							dup.x = tmp.x; dup.y = tmp.y; dup.z = tmp.z;
							vertices->push_back(dup);
							indices[i] = (GLuint)vertices->size() - 1;
							indexOffset++;
						}
					}
				}

				currGroup->getIndices()->push_back(indices[i]);

				if (i > 1) {
					face = new Face();
					face->vertexIndices[0] = indices[0];
					face->vertexIndices[1] = indices[i - 1];
					face->vertexIndices[2] = indices[i];

					// adding face to map for fast searching times...
					groupIndexFacesMap[indices[0]] = face;
					groupIndexFacesMap[indices[i - 1]] = face;
					groupIndexFacesMap[indices[i]] = face;

					face->vertexNormalIndices[0] = normalIndices[0];
					face->vertexNormalIndices[1] = normalIndices[i - 1];
					face->vertexNormalIndices[2] = normalIndices[i];

					currGroup->getFaces()->push_back(*face);
				}
			}
		}
		else if (line.find("g ") != std::string::npos || line.find("o ") != std::string::npos) {
			if (currGroup) {
				groups->push_back(*currGroup);
				isSmoothingGroup = false;
			}

			currGroup = new Group();
			groupIndexFacesMap.clear();
			groupNr++;
			prevGroupIndexOffset = indexOffset;
		}
	}

	in.close();
	groupIndexFacesMap.clear();

	delete[] indices;
	delete[] normalIndices;

	if (currGroup) {
		groups->push_back(*currGroup);
	}
}