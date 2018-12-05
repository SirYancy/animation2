//
// Created by eric on 11/29/18.
//

#include <iostream>
#include "colladaloader.h"

ColladaLoader::ColladaLoader(std::string filename) : filename(filename) {}

void ColladaLoader::ReadGeometry(Geometry *data) {
    XMLDocument doc;
    doc.LoadFile(filename.c_str());
    XMLElement *root = doc.RootElement();

    XMLElement *mesh, *vertices, *triangles, *input, *source;
    std::string sourceName;
    int primitiveCount, indicesCount;

    XMLElement *geometry = root->FirstChildElement("library_geometries")->FirstChildElement("geometry");

    data->name = geometry->Attribute("id");
    // Get Mesh Element
    mesh = geometry->FirstChildElement("mesh");

    // Get Triangles and Vertices Elements
    triangles = mesh->FirstChildElement("triangles");
    vertices = mesh->FirstChildElement("vertices");

    if (triangles == nullptr) {
        std::cerr << "We do not support the primitives in " << filename << std::endl;
        exit(1);
    }

    // First process Position Data
    input = GetInput("POSITION", vertices);

    sourceName = std::string(input->Attribute("source"));
    sourceName = sourceName.erase(0, 1);

    std::cout << input->Attribute("semantic") << " " << sourceName << std::endl;

    source = GetSource(sourceName, mesh);
    data->position = ReadElement(source);
    unsigned int offset;
    input->QueryAttribute("offset", &offset);
    data->position.offset = offset;

    // Process Normal Data
    input = GetInput("NORMAL", triangles);
    sourceName = std::string(input->Attribute("source"));
    sourceName = sourceName.erase(0, 1);

    std::cout << input->Attribute("semantic") << " " << sourceName << std::endl;

    source = GetSource(sourceName, mesh);
    data->normals = ReadElement(source);
    input->QueryAttribute("offset", &data->normals.offset);

    // Process Texture Data
    input = GetInput("TEXCOORD", triangles);
    sourceName = std::string(input->Attribute("source"));
    sourceName = sourceName.erase(0, 1);

    std::cout << input->Attribute("semantic") << " " << sourceName << std::endl;

    source = GetSource(sourceName, mesh);
    data->texCoords = ReadElement(source);
    input->QueryAttribute("offset", &data->texCoords.offset);


    triangles->QueryAttribute("count", &primitiveCount);

    data->primitiveCount = primitiveCount;

    indicesCount = primitiveCount * 3 * 3;

    auto *indices = (unsigned short *) malloc(indicesCount * sizeof(unsigned short));

    std::cout << "Primitives: " << primitiveCount << std::endl <<
              "Indices: " << indicesCount << std::endl;

    char *text = (char *) (triangles->FirstChildElement("p")->GetText());

    indices[0] = (unsigned short) atoi(strtok(text, " "));
    for (int index = 1; index < indicesCount; index++) {
        indices[index] = (unsigned short) atoi(strtok(NULL, " "));
    }

    BuildBuffer(indices, indicesCount, data);

    printf("Geometry Loaded\n");
    free(indices);
}

SourceData ColladaLoader::ReadElement(XMLElement *element) {
    SourceData sourceData;
    XMLElement *array;
    char *text;
    unsigned int numVals, stride;
    int check;

    array = element->FirstChildElement("float_array");
    if (array != nullptr) {
        array->QueryUnsignedAttribute("count", &numVals);
        std::cout << "Values: " << numVals << std::endl;

        check = element->FirstChildElement("technique_common")->FirstChildElement("accessor")
                ->QueryUnsignedAttribute("stride", &stride);
        if (check != XML_NO_ATTRIBUTE)
            sourceData.stride = stride;
        else
            sourceData.stride = 1;

        text = (char *) (array->GetText());

        sourceData.type = GL_FLOAT;
        sourceData.size = numVals * sizeof(float);
        sourceData.data = malloc(numVals * sizeof(float));

        ((float *) sourceData.data)[0] = atof(strtok(text, " "));
        for (unsigned int index = 1; index < numVals; index++) {
            ((float *) sourceData.data)[index] = atof(strtok(NULL, " "));
        }
    }
    return sourceData;
}

void ColladaLoader::FreeGeometry(Geometry *g) {
    free(g->normals.data);
    free(g->position.data);
    free(g->texCoords.data);
    free(g->vertexData);
    free(g);
}

XMLElement *ColladaLoader::GetSource(std::string sourceName, XMLElement *mesh) {
    XMLElement *source;
    source = mesh->FirstChildElement("source");
    while (source != nullptr) {
        if (std::string(source->Attribute("id")) == sourceName) {
            return source;
        }
        source = source->NextSiblingElement("source");
    }
    return nullptr;
}

XMLElement *ColladaLoader::GetInput(std::string semantic, XMLElement *element) {
    XMLElement *input;
    input = element->FirstChildElement("input");
    while (input != nullptr) {
        if (std::string(input->Attribute("semantic")) == semantic) {
            return input;
        }
        input = input->NextSiblingElement("input");
    }
    return nullptr;
}

void ColladaLoader::BuildBuffer(unsigned short *indices, int indicesCount, Geometry *g) {
    g->vertexData = new std::vector<float>();

    for (int i = 0; i < indicesCount; i++) {

        if (i % 3 == 0) { // Do Position
            g->vertexData->push_back(((float *) (g->position.data))[indices[i] * g->position.stride]);
            g->vertexData->push_back(((float *) (g->position.data))[indices[i] * g->position.stride + 1]);
            g->vertexData->push_back(((float *) (g->position.data))[indices[i] * g->position.stride + 2]);
        } else if (i % 3 == 1) { // Do normals
            g->vertexData->push_back(((float *) (g->normals.data))[indices[i] * g->normals.stride]);
            g->vertexData->push_back(((float *) (g->normals.data))[indices[i] * g->normals.stride + 1]);
            g->vertexData->push_back(((float *) (g->normals.data))[indices[i] * g->normals.stride + 2]);
        } else { //i%3 == 2  Do Texcoords
            g->vertexData->push_back(((float *) (g->texCoords.data))[indices[i] * g->texCoords.stride]);
            g->vertexData->push_back(((float *) (g->texCoords.data))[indices[i] * g->texCoords.stride + 1]);
        }
    }

//    for(int i = 0; i < g->vertexData->size(); i++){
//        if(i%8 == 0){
//            printf("VERTEX\n");
//        }
//        printf("%f\n", (*g->vertexData)[i]);
//    }
}

