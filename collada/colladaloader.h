//
// Created by eric on 11/29/18.
//

#ifndef MAZE_COLLADALOADER_H
#define MAZE_COLLADALOADER_H

#include <string>
#include <vector>
#include <glad.h>
#include <map>
#include "../tinyxml/tinyxml2.h"
#include "../glm/glm.hpp"

using namespace tinyxml2;

struct SourceData {
    GLenum type;
    unsigned int size;
    unsigned int stride;
    unsigned int offset;
    void *data;
};

struct Geometry {
    std::string name;
    SourceData position;
    SourceData normals;
    SourceData texCoords;
    int primitiveCount;
    std::vector<float> *vertexData;
};

class ColladaLoader {

private:
    std::string filename;

    SourceData ReadElement(XMLElement *);

    XMLElement *GetSource(std::string sourceName, XMLElement *mesh);

    XMLElement *GetInput(std::string semantic, XMLElement *element);

    void BuildBuffer(unsigned short *indices, int indicesCount, Geometry *g);

public:
    explicit ColladaLoader(std::string filename);

    void ReadGeometry(Geometry *g);

    void FreeGeometry(Geometry *g);

};

#endif //MAZE_COLLADALOADER_H
