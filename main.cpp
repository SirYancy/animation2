#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include "glad/glad.h"
#include "collada/colladaloader.h"

#if defined(__APPLE__) || defined(__linux__)

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#endif

using namespace std;
using std::ifstream;

bool fullscreen = false;
int screenWidth = 800;
int screenHeight = 600;

GLuint shaderProgram;
GLuint vao, vbo;

std::vector<float> dataBuffer;

const char *model = "../models/spaceman.dae";

const char *vertfn = "../shaders/vert.glsl";
const char *fragfn = "../shaders/frag.glsl";

const char *texfn = "../textures/spaceman.bmp";

int LoadShader(const char *filename, GLuint shaderID);

void CreateShaderProgram(GLuint &vertShader, GLuint &fragShader);

void LoadTexture(GLenum texture, GLuint *id, const char *filename, const char *samplerID, const int index);

void CreateBuffers(Geometry *g);

#define GLM_FORCE_RADIANS

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

    //Print the version of SDL we are using
    SDL_version comp;
    SDL_version linked;
    SDL_VERSION(&comp);
    SDL_GetVersion(&linked);
    printf("\nCompiled against SDL version %d.%d.%d\n", comp.major, comp.minor, comp.patch);
    printf("Linked SDL version %d.%d.%d.\n", linked.major, linked.minor, linked.patch);

    //Ask SDL to get a recent version of OpenGL (3.2 or greater)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    //Create a window (offsetx, offsety, width, height, flags)
    SDL_Window *window = SDL_CreateWindow("ogl", 100, 100,
                                          screenWidth, screenHeight, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("Could not create window: %s\n", SDL_GetError());
        return EXIT_FAILURE; //Exit as SDL failed
    }

    float aspect = screenWidth / (float) screenHeight; //aspect ratio needs update on resize

    SDL_GLContext context = SDL_GL_CreateContext(window); //Bind OpenGL to the window

    if (gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        printf("OpenGL loaded\n");
        printf("Vendor:   %s\n", glGetString(GL_VENDOR));
        printf("Renderer: %s\n", glGetString(GL_RENDERER));
        printf("Version:  %s\n", glGetString(GL_VERSION));
    } else {
        printf("ERROR: Failed to initialize OpenGL context.\n");
        return -1;
    }
    GLuint vertShader;
    GLuint fragShader;
    CreateShaderProgram(vertShader, fragShader);

    ColladaLoader *loader = new ColladaLoader(model);

    auto *geometry = new Geometry();
    loader->ReadGeometry(geometry);

    CreateBuffers(geometry);

    GLuint brickTex;
    LoadTexture(GL_TEXTURE0, &brickTex, texfn, "brick", 0);

    glEnable(GL_DEPTH_TEST);

    SDL_Event windowEvent;
    bool quit = false;

    while (!quit) {
        while (SDL_PollEvent(&windowEvent)) {
            if (windowEvent.type == SDL_QUIT) quit = true;;
            if (windowEvent.type == SDL_KEYUP) {
                switch (windowEvent.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        quit = true;
                        break;
                    default:
                        break;
                }
            }
        }

        glClearColor(0.2f, 0.4f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        GLint unicolor = glGetUniformLocation(shaderProgram, "inColor");
        glm::vec3 colVec(0.f, 0.7f, 0.f);
        glUniform3fv(unicolor, 1, glm::value_ptr(colVec));

        glm::mat4 model;
        GLint uniModel = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

        glm::mat4 view = glm::lookAt(
                glm::vec3(0.0f, -20.f, 1.0f),
                glm::vec3(0.0f, 0.0f, 1.0f),
                glm::vec3(0.0f, 0.0f, 1.0f));
        GLint uniView = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 proj = glm::perspective(3.14f / 4, aspect, 0.001f, 1000.0f);
        GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
        glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

        glBindVertexArray(vao);

        glDrawArrays(GL_TRIANGLES, 0, geometry->primitiveCount * 3);

        SDL_GL_SwapWindow(window);
    }


    glDeleteProgram(shaderProgram);
    glDeleteShader(fragShader);
    glDeleteShader(vertShader);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    SDL_GL_DeleteContext(context);
    SDL_Quit();

    return 0;
}

void CreateBuffers(Geometry *g) {
    glUseProgram(shaderProgram);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    float *data = g->vertexData->data();

//    for (int i = 0; i < g->vertexData->size(); i++) {
//        if (i % 8 == 0) printf("Vertex\n");
//        printf("%f\n", data[i]);
//    }

    int size = g->vertexData->size();

    glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), data, GL_STATIC_DRAW);

    //Bind Position Data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glEnableVertexAttribArray(posAttrib);

    //Bind Normal Data
    GLint normAttrib = glGetAttribLocation(shaderProgram, "inNormal");
    glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(normAttrib);

    //Bind Texture Data
    GLint texAttrib = glGetAttribLocation(shaderProgram, "inTexCoord");
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (6 * sizeof(float)));
    glEnableVertexAttribArray(texAttrib);

    printf("Attributes: %d %d %d\n", posAttrib, normAttrib, texAttrib);

}

void LoadTexture(GLenum texture, GLuint *id, const char *filename, const char *samplerID, const int index) {
    glUseProgram(shaderProgram);

    SDL_Surface *surface = SDL_LoadBMP(filename);
    if (surface == nullptr) {
        printf("Error: \"%s\"\n", SDL_GetError());
    }

    glGenTextures(1, id);

    glActiveTexture(texture);
    glBindTexture(GL_TEXTURE_2D, *id);

    glUniform1i(glGetUniformLocation(shaderProgram, samplerID), index);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_FreeSurface(surface);
}

int LoadShader(const char *filename, GLuint shaderID) {
    ifstream file;
    file.open(filename, std::ios::in);
    if (!file) return -1;

    file.tellg();
    file.seekg(0, std::ios::end);
    int len = static_cast<int>(file.tellg());
    file.seekg(std::ios::beg);

    if (len == 0) return -1; //empty file

    GLchar *shaderSource = new GLchar[len + 1];
    shaderSource[len] = 0;

    int i = 0;
    while (file.good()) {
        shaderSource[i] = static_cast<GLchar>(file.get());
        if (!file.eof())
            i++;
    }

    shaderSource[i] = 0;
    file.close();

    glShaderSource(shaderID, 1, &shaderSource, NULL);
    glCompileShader(shaderID);

    GLint status;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &status);
    if (!status) {
        char buffer[512];
        glGetShaderInfoLog(shaderID, 512, NULL, buffer);
        printf("Shader Compile Failed. Info:\n\n%s\n", buffer);
        return -1;
    }
    return 0;
}


void CreateShaderProgram(GLuint &vertShader, GLuint &fragShader) {
    vertShader = glCreateShader(GL_VERTEX_SHADER);
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    LoadShader(vertfn, vertShader);
    LoadShader(fragfn, fragShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertShader);
    glAttachShader(shaderProgram, fragShader);
//     glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);
}