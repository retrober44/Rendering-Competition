#include "platform.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Program.h"
#include "core/Texture.h"
#include "core/Camera.h"

#include <iostream>
#include <list>
#include <vector>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <cmath>


struct ModelAsset {
    core::Program* shaders;
    core::Texture* texture;
    GLuint vbo;
    GLuint vao;
    GLenum drawType;
    GLint drawStart;
    GLint drawCount;
    GLfloat shininess;
    glm::vec3 specularColor;

    ModelAsset() :
        shaders(NULL),
        texture(NULL),
        vbo(0),
        vao(0),
        drawType(GL_TRIANGLES),
        drawStart(0),
        drawCount(0),
        shininess(0.0f),
        specularColor(1.0f, 1.0f, 1.0f)
    {}
};


struct ModelInstance {
    ModelAsset* asset;
    glm::mat4 transform;
    glm::vec3 transformInner;

    ModelInstance() :
        asset(NULL),
        transform()
    {}
};

struct Light {
    glm::vec4 position;
    glm::vec3 transformInner;
    glm::vec3 intensities; //a.k.a. the color of the light
    float attenuation;
    float ambientCoefficient;
    float coneAngle;
    glm::vec3 coneDirection;
};

const glm::vec2 SCREEN_SIZE(1920, 1080);
const enum BlockType { GRAS, BRICKS, GRANITE, STONE_BRICKS, TERRA_COTTA, OAK_LOG, OAK_PLANKS, STONE, COARSE_DIRT, COBBLE_STONE, BLUE_ICE, CLOUD, TIRE, BRAIN };

GLFWwindow* gWindow = NULL;
double gScrollY = 0.0;
core::Camera gCamera;
ModelAsset gExampleModelAsset;
std::vector<ModelAsset> blocks;
std::vector<core::Texture*> textures;

std::list<ModelInstance> gInstances;
std::list<ModelInstance> gCarInstances;
std::list<ModelInstance> gCarTireInstances;
GLfloat gDegreesRotated = 0.0f;
std::vector<Light> gLights;
std::vector<Light> gCarLights;

glm::vec3 carPosition = { 2, 2, 2 };
float carHorizontalAngle = 0;

static core::Program* LoadShaders(const char* vertFilename, const char* fragFilename) {
    std::vector<core::Shader> shaders;
    shaders.push_back(core::Shader::shaderFromFile(ResourcePath(vertFilename), GL_VERTEX_SHADER));
    shaders.push_back(core::Shader::shaderFromFile(ResourcePath(fragFilename), GL_FRAGMENT_SHADER));
    return new core::Program(shaders);
}

static core::Texture* LoadTexture(const char* filename) {
    core::Bitmap bmp = core::Bitmap::bitmapFromFile(ResourcePath(filename));
    bmp.flipVertically();
    return new core::Texture(bmp);
}

static void LoadExampleAssets() {

    gExampleModelAsset.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
    gExampleModelAsset.drawType = GL_TRIANGLES;
    gExampleModelAsset.drawStart = 0;
    gExampleModelAsset.drawCount = 6 * 2 * 3;
    gExampleModelAsset.texture = LoadTexture("gras.png");
    gExampleModelAsset.shininess = 80.0;
    gExampleModelAsset.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glGenBuffers(1, &gExampleModelAsset.vbo);
    glGenVertexArrays(1, &gExampleModelAsset.vao);

    // bind the VAO
    glBindVertexArray(gExampleModelAsset.vao);

    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, gExampleModelAsset.vbo);

    // Make a cube out of triangles (two triangles per side)
    GLfloat vertexData[] = {
        //  X     Y     Z       U     V          Normal
        // bottom
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
        1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   0.0f, -1.0f, 0.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,

        // top
        -1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
        1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,

        // front
        -1.0f,-1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
        1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
        1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,

        // back
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
        -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
        -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
        1.0f, 1.0f,-1.0f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,

        // left
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,

        // right
        1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
        1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
        1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
        1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

    // connect the xyz to the "vert" attribute of the vertex shader
    glEnableVertexAttribArray(gExampleModelAsset.shaders->attrib("vert"));
    glVertexAttribPointer(gExampleModelAsset.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);

    // connect the uv coords to the "vertTexCoord" attribute of the vertex shader
    glEnableVertexAttribArray(gExampleModelAsset.shaders->attrib("vertTexCoord"));
    glVertexAttribPointer(gExampleModelAsset.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

    // connect the normal to the "vertNormal" attribute of the vertex shader
    glEnableVertexAttribArray(gExampleModelAsset.shaders->attrib("vertNormal"));
    glVertexAttribPointer(gExampleModelAsset.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));

    // unbind the VAO
    glBindVertexArray(0);
}

void LoadTextures() {
    // ToDo: Push all this texture in the vector textures
    textures.push_back(LoadTexture("gras.png"));
    textures.push_back(LoadTexture("bricks.png"));
    textures.push_back(LoadTexture("granite.png"));
    textures.push_back(LoadTexture("stone_bricks.png"));
    textures.push_back(LoadTexture("terracotta.png"));
    textures.push_back(LoadTexture("oak_log.png"));
    textures.push_back(LoadTexture("oak_planks.png"));
    textures.push_back(LoadTexture("stone.png"));
    textures.push_back(LoadTexture("coarse_dirt.png"));
    textures.push_back(LoadTexture("cobblestone.png"));
    textures.push_back(LoadTexture("blue_ice.png"));
    textures.push_back(LoadTexture("water_overlay.png"));
    textures.push_back(LoadTexture("terracotta.png"));
    textures.push_back(LoadTexture("brain_coral_block.png"));
    textures.push_back(LoadTexture("gras.png"));
}

// initialises the gOtherCrate global
void LoadBlockByType(BlockType type) {

    ModelAsset gLocalAsset;

    // set all the elements of gOtherCrate
    gLocalAsset.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
    gLocalAsset.drawType = GL_TRIANGLES;
    gLocalAsset.drawStart = 0;
    gLocalAsset.drawCount = 6 * 2 * 3;

    // ToDo: Use here the vector textures !! Performance !!!

    // GRAS, BRICKS, GRANITE, STONE_BRICKS, TERRA_COTTA, OAK_LOG, OAK_PLANKS, STONE, COARSE_DIRT, COBBLE_STONE, BLUE_ICE, CLOUD, TIRE, BRAIN
    if (type == GRAS) {
        gLocalAsset.texture = textures.at(0);
    }
    else if (type == BRICKS) {
        gLocalAsset.texture = textures.at(1);
    }
    else if (type == GRANITE) {
        gLocalAsset.texture = textures.at(2);
    }
    else if (type == STONE_BRICKS) {
        gLocalAsset.texture = textures.at(3);
    }
    else if (type == TERRA_COTTA) {
        gLocalAsset.texture = textures.at(4);
    }
    else if (type == OAK_LOG) {
        gLocalAsset.texture = textures.at(5);
    }
    else if (type == OAK_PLANKS) {
        gLocalAsset.texture = textures.at(6);
    }
    else if (type == STONE) {
        gLocalAsset.texture = textures.at(7);
    }
    else if (type == COARSE_DIRT) {
        gLocalAsset.texture = textures.at(8);
    }
    else if (type == COBBLE_STONE) {
        gLocalAsset.texture = textures.at(9);
    }
    else if (type == BLUE_ICE) {
        gLocalAsset.texture = textures.at(10);
    }
    else if (type == CLOUD) {
        gLocalAsset.texture = textures.at(11);
    }
    else if (type == TIRE) {
        gLocalAsset.texture = textures.at(12);
    }
    else if (type == BRAIN) {
        gLocalAsset.texture = textures.at(13);
    }
    else {
        gLocalAsset.texture = textures.at(0);
    }
    
    gLocalAsset.shininess = 50.0;
    gLocalAsset.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glGenBuffers(1, &gLocalAsset.vbo);
    glGenVertexArrays(1, &gLocalAsset.vao);

    // bind the VAO
    glBindVertexArray(gLocalAsset.vao);

    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, gLocalAsset.vbo);

    // Make a cube out of triangles (two triangles per side)
    GLfloat vertexData[] = {
        //  X     Y     Z       U     V          Normal
        // bottom
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
        1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   0.0f, -1.0f, 0.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,

        // top
        -1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
        1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,

        // front
        -1.0f,-1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
        1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
        1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,

        // back
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
        -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
        -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
        1.0f, 1.0f,-1.0f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,

        // left
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,

        // right
        1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
        1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
        1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
        1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

    // connect the xyz to the "vert" attribute of the vertex shader
    glEnableVertexAttribArray(gLocalAsset.shaders->attrib("vert"));
    glVertexAttribPointer(gLocalAsset.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);

    // connect the uv coords to the "vertTexCoord" attribute of the vertex shader
    glEnableVertexAttribArray(gLocalAsset.shaders->attrib("vertTexCoord"));
    glVertexAttribPointer(gLocalAsset.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

    // connect the normal to the "vertNormal" attribute of the vertex shader
    glEnableVertexAttribArray(gLocalAsset.shaders->attrib("vertNormal"));
    glVertexAttribPointer(gLocalAsset.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));

    // unbind the VAO
    glBindVertexArray(0);

    blocks.push_back(gLocalAsset);
}

// initialises the gOtherCrate global
void LoadAllBlockTypes() {
    // GRAS, BRICKS, GRANITE, STONE_BRICKS, TERRA_COTTA, OAK_LOG, OAK_PLANKS, STONE, COARSE_DIRT, COBBLE_STONE, BLUE_ICE, CLOUD, TIRE, BRAIN
    LoadBlockByType(GRAS);          // 0
    LoadBlockByType(BRICKS);        // 1
    LoadBlockByType(GRANITE);       // 2
    LoadBlockByType(STONE_BRICKS);  // 3
    LoadBlockByType(TERRA_COTTA);   // 4
    LoadBlockByType(OAK_LOG);       // 5
    LoadBlockByType(OAK_PLANKS);    // 6
    LoadBlockByType(STONE);         // 7
    LoadBlockByType(COARSE_DIRT);   // 8
    LoadBlockByType(COBBLE_STONE);  // 9
    LoadBlockByType(BLUE_ICE);      // 10
    LoadBlockByType(CLOUD);         // 11
    LoadBlockByType(TIRE);         // 12
    LoadBlockByType(BRAIN);         // 13
}

// convenience function that returns a translation matrix
glm::mat4 translate(GLfloat x, GLfloat y, GLfloat z) {
    return glm::translate(glm::mat4(), glm::vec3(x, y, z));
}

// convenience function that returns a scaling matrix
glm::mat4 scale(GLfloat x, GLfloat y, GLfloat z) {
    return glm::scale(glm::mat4(), glm::vec3(x, y, z));
}

//create all the `instance` structs for the 3D scene, and add them to `gInstances`
static void CreateCar() {

    GLfloat offset = 2.0f;
    GLfloat baseHeight = 1.0f;


    for (int transX = 0; transX < 2; transX++) {
        for (int transY = 0; transY < 4; transY++) {
            ModelInstance ass1;
            ass1.asset = &blocks.at(13);
            ass1.transform = glm::mat4() * glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
            ass1.transformInner = glm::vec3(transX * offset, baseHeight, transY * offset);
            gCarInstances.push_back(ass1);
        }
    }

    baseHeight = 3.0f;
    for (int transX = 0; transX < 2; transX++) {
        for (int transY = 0; transY < 2; transY++) {
            ModelInstance ass1;
            ass1.asset = &blocks.at(13);
            ass1.transform = glm::mat4() * glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
            ass1.transformInner = glm::vec3(transX * offset, baseHeight, transY * offset);
            gCarInstances.push_back(ass1);
        }
    }

    baseHeight = 3.0f;
    for (int transX = 0; transX < 2; transX++) {
        for (int transY = 2; transY < 3; transY++) {
            ModelInstance ass1;
            ass1.asset = &blocks.at(11);
            ass1.transform = glm::mat4() * glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
            ass1.transformInner = glm::vec3(transX * offset, baseHeight, transY * offset);
            gCarInstances.push_back(ass1);
        }
    }

    ModelInstance tire1;
    tire1.asset = &blocks.at(12);
    tire1.transform = glm::mat4() * glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
    tire1.transformInner = glm::vec3(1.5f * offset, 0.0f, 3.0f * offset);
    gCarTireInstances.push_back(tire1);

    ModelInstance tire2;
    tire2.asset = &blocks.at(12);
    tire2.transform = glm::mat4() * glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
    tire2.transformInner = glm::vec3(-0.5f * offset, 0.0f, 3.0f * offset);
    gCarTireInstances.push_back(tire2);

    ModelInstance tire3;
    tire3.asset = &blocks.at(12);
    tire3.transform = glm::mat4() * glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
    tire3.transformInner = glm::vec3(1.5f * offset, 0.0f, -0.0f * offset);
    gCarTireInstances.push_back(tire3);

    ModelInstance tire4;
    tire4.asset = &blocks.at(12);
    tire4.transform = glm::mat4() * glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
    tire4.transformInner = glm::vec3(-0.5f * offset, 0.0f, -0.0f * offset);
    gCarTireInstances.push_back(tire4);
}

static void CreateTerrain() {

    // A block got a height and width of 2 !
    const int size = 30;
    const int offset = 2;
    float base_height = 0;
    float hori_offset = -size / 2;

    // Base ground

    /*  BLOCK TYPE BY ID !!!!
     *  0 = GRAS
     *  1 = BRICKS
     *  2 = GRANITE
     *  3 = STONE_BRICKS
     *  4 = TERRA_COTTA
     *  5 = OAK_LOG
     *  6 = OAK_PLANKS
     *  7 = STONE
     *  8 = COARSE_DIRT
     *  9 = COBBLE_STONE
     * 10 = BLUE_ICE
     * 11 = CLOUD
     * 12 = TIRE
     * 13 = BRAIN
     */

    int maze[size][size] = {
            {-1,-1, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,-1,-1 },
            {-1, 0, 0, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 0, 0,-1 },
            { 0, 0, 1, 1, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 1, 1, 0, 0 },
            { 0, 1, 1, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 1, 1, 0 },
            { 0, 1, 7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 7, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 7, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 1, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0, 0, 0, 0, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  0, 0, 0, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0, 0, 0,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1, 0, 0, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0, 0,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1, 0, 0, 1, 7, 7, 7, 1, 0 },

            { 0, 1, 7, 7, 7, 1, 0,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1, 0, 1, 7, 7, 7, 1, 0 },

            { 0, 1, 7, 7, 7, 1, 0,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1,-1, 0, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0, 0,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, -1, 0, 0, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 0, 0, 0,-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  0, 0, 0, 0, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 1, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 1, 1, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 7, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 7, 7, 7, 7, 1, 0 },
            { 0, 1, 7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 7, 1, 0 },
            { 0, 1, 1, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 1, 1, 0 },
            { 0, 0, 1, 1, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 1, 1, 0, 0 },
            {-1, 0, 0, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 0, 0,-1 },
            {-1,-1, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,-1,-1 }
    };

    int maze_cloud[size][size] = {
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11, 0, 0, 0, 0 },
            { 0, 0, 0,11,11,11,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11, 0,11, 0, 0, 0 },
            { 0, 0, 0, 0,11,11,11, 0, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0, 0, 0,11,11,11, 0, 0,11, 0, 0, 0 },
            { 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0, 0,11,11,11,11, 0, 0, 0, 0, 0, 0,11,11,11,11, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11,11,11,11, 0, 0, 0, 0, 0, 0,11,11, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11,11,11,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0 },
            { 0, 0, 0,11,11, 0, 0, 0, 0,11, 0, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0, 0,11,11,11, 0, 0, 0, 0 },
            { 0, 0,11,11,11,11,11, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11, 0, 0, 0, 0,11,11,11, 0,11, 0, 0, 0 },
            { 0, 0, 0, 0,11,11,11, 0, 0, 0, 0, 0, 0,11, 0,11,11, 0, 0, 0,11,11,11,11, 0, 0,11, 0, 0, 0 },
            { 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0, 0,11,11,11,11, 0, 0, 0, 0, 0, 0,11,11,11,11, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11,11,11,11, 0, 0, 0, 0, 0, 0,11,11,11, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11,11,11,11, 0, 0, 0, 0, 0, 0, 0,11,11,11, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0,11, 0,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11, 0, 0, 0, 0 },
            { 0, 0, 0,11,11,11,11,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11, 0,11, 0, 0, 0 },
            { 0, 0, 0, 0,11,11,11, 0, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0, 0, 0,11,11,11, 0, 0,11, 0, 0, 0 },
            { 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0, 0,11,11,11,11, 0, 0, 0, 0, 0, 0,11,11,11,11, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11,11,11,11, 0, 0, 0, 0, 0, 0,11,11, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11,11,11,11,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    int maze_inner_0[size / 3][size / 3] = {
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 9, 9, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 9, 9, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    };

    int maze_inner_1[size / 3][size / 3] = {
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 9, 9, 0, 0, 0, 0 },
            { 0, 0, 0, 9, 0, 0, 9, 0, 0, 0 },
            { 0, 0, 0, 9, 0, 0, 9, 0, 0, 0 },
            { 0, 0, 0, 0, 9, 9, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    };

    int maze_inner_2[size / 3][size / 3] = {
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 9, 9, 0, 0, 0, 0 },
            { 0, 0, 0, 9, 0, 0, 9, 0, 0, 0 },
            { 0, 0, 9, 0, 0, 0, 0, 9, 0, 0 },
            { 0, 0, 9, 0, 0, 0, 0, 9, 0, 0 },
            { 0, 0, 0, 9, 0, 0, 9, 0, 0, 0 },
            { 0, 0, 0, 0, 9, 9, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    };

    int maze_inner_3[size / 3][size / 3] = {
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 9, 9, 9, 9, 0, 0, 0 },
            { 0, 0, 9, 0, 0, 0, 9, 9, 0, 0 },
            { 0, 9, 0, 0, 0, 0, 0, 0, 9, 0 },
            { 0, 9, 0, 0, 0, 0, 0, 0, 9, 0 },
            { 0, 9, 0, 0, 0, 0, 0, 0, 9, 0 },
            { 0, 9, 0, 0, 0, 0, 0, 0, 9, 0 },
            { 0, 0, 9, 0, 0, 0, 0, 9, 0, 0 },
            { 0, 0, 0, 9, 9, 9, 9, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    };

    int maze_inner_4[size / 3][size / 3] = {
            { 0, 0, 0, 9, 9, 9, 9, 0, 0, 0 },
            { 0, 0, 9, 0, 0, 0, 0, 9, 0, 0 },
            { 0, 9, 0, 0, 0, 0, 0, 0, 9, 0 },
            { 9, 0, 0, 0, 0, 0, 0, 0, 0, 9 },
            { 0, 9, 0, 0, 0, 0, 0, 0, 9, 0 },
            { 0, 9, 0, 0, 0, 0, 0, 0, 9, 0 },
            { 9, 0, 0, 0, 0, 0, 0, 0, 0, 9 },
            { 0, 9, 0, 0, 0, 0, 0, 0, 9, 0 },
            { 0, 0, 9, 0, 0, 0, 0, 9, 0, 0 },
            { 0, 0, 0, 9, 9, 9, 9, 0, 0, 0 },
    };

    int start_height = -12;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 4; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_0[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(9);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 4;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 3; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_1[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(9);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 2;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 2; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_2[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(4);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 2;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 2; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_3[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(4);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 2;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 2; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_4[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(8);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 2;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 2; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_3[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(8);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 2;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 2; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_4[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(8);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 2;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 2; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_3[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(0);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 2;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 2; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_4[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(0);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 2;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 2; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_3[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(0);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 2;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 2; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_2[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(10);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 2;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 3; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_1[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(10);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    start_height += 3;

    for (int transX = 0; transX < size / 3; transX++) {
        for (int transY = 0; transY < size / 3; transY++) {
            for (int transZ = 0; transZ < 4; transZ++) {
                // creedy impl... now if bigger null custom texture...
                if (maze_inner_0[transX][transY] > 0) {  // negative, so no cube
                    ModelInstance block;
                    block.asset = &blocks.at(10);
                    block.transform = translate(20 + transX * offset, transZ * offset + (start_height * offset), 20 + transY * offset) * scale(1, 1, 1);
                    gInstances.push_back(block);
                }
            }
        }
    }

    // Race Track
    for (int transX = 0; transX < size; transX++) {
        for (int transY = 0; transY < size; transY++) {
            if (maze[transX][transY] >= 0) {  // negative, so no cube
                ModelInstance block;
                block.asset = &blocks.at(maze[transX][transY]);
                block.transform = translate(transX * offset, base_height, transY * offset) * scale(1, 1, 1);
                gInstances.push_back(block);
            }
        }
    }
}

template <typename T>
void SetLightUniform(core::Program* shaders, const char* propertyName, size_t lightIndex, const T& value) {
    std::ostringstream ss;
    ss << "allLights[" << lightIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shaders->setUniform(uniformName.c_str(), value);
}

// Setup all lights
void CreateAllLights() {

    Light spotlight1;
    spotlight1.position = glm::vec4(8.0, 12.0, 8.0, 1);
    spotlight1.intensities = glm::vec3(2, 2, 2); //strong white light
    spotlight1.attenuation = 0.1f;
    spotlight1.ambientCoefficient = 0.0f; //no ambient light
    spotlight1.coneAngle = 15.0f;
    spotlight1.coneDirection = glm::vec3(0, -1, 0);

    Light spotlight2;
    spotlight2.position = glm::vec4(5.0, 12.0, 20.0, 1);
    spotlight2.intensities = glm::vec3(2, 2, 2); //strong white light
    spotlight2.attenuation = 0.1f;
    spotlight2.ambientCoefficient = 0.0f; //no ambient light
    spotlight2.coneAngle = 15.0f;
    spotlight2.coneDirection = glm::vec3(0, -1, 0);

    Light spotlight3;
    spotlight3.position = glm::vec4(5.0, 12.0, 40.0, 1);
    spotlight3.intensities = glm::vec3(2, 2, 2); //strong white light
    spotlight3.attenuation = 0.1f;
    spotlight3.ambientCoefficient = 0.0f; //no ambient light
    spotlight3.coneAngle = 15.0f;
    spotlight3.coneDirection = glm::vec3(0, -1, 0);

    Light spotlight4;
    spotlight4.position = glm::vec4(8.0, 12.0, 52.0, 1);
    spotlight4.intensities = glm::vec3(2, 2, 2); //strong white light
    spotlight4.attenuation = 0.1f;
    spotlight4.ambientCoefficient = 0.0f; //no ambient light
    spotlight4.coneAngle = 15.0f;
    spotlight4.coneDirection = glm::vec3(0, -1, 0);

    Light directionalLight;
    directionalLight.position = glm::vec4(6.0, 6.0, 6.0, 0); //w == 0 indications a directional light
    directionalLight.intensities = glm::vec3(1.0, 1.0, 1.0); // white light
    directionalLight.ambientCoefficient = 0.06f;

    gLights.push_back(spotlight1);
    gLights.push_back(spotlight2);
    gLights.push_back(spotlight3);
    gLights.push_back(spotlight4);
    gLights.push_back(directionalLight);
}

//renders a single `ModelInstance`
static void RenderInstance(const ModelInstance& inst) {
    ModelAsset* asset = inst.asset;
    core::Program* shaders = asset->shaders;

    //bind the shaders
    shaders->use();

    //set the shader uniforms
    shaders->setUniform("camera", gCamera.matrix());
    shaders->setUniform("model", inst.transform);
    shaders->setUniform("materialTex", 0); //set to 0 because the texture will be bound to GL_TEXTURE0
    shaders->setUniform("materialShininess", asset->shininess);
    shaders->setUniform("materialSpecularColor", asset->specularColor);
    shaders->setUniform("cameraPosition", gCamera.position());
    shaders->setUniform("numLights", (int)gLights.size());

    for (size_t i = 0; i < gLights.size(); ++i) {
        SetLightUniform(shaders, "position", i, gLights[i].position);
        SetLightUniform(shaders, "intensities", i, gLights[i].intensities);
        SetLightUniform(shaders, "attenuation", i, gLights[i].attenuation);
        SetLightUniform(shaders, "ambientCoefficient", i, gLights[i].ambientCoefficient);
        SetLightUniform(shaders, "coneAngle", i, gLights[i].coneAngle);
        SetLightUniform(shaders, "coneDirection", i, gLights[i].coneDirection);
    }


    //bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, asset->texture->object());

    //bind VAO and draw
    glBindVertexArray(asset->vao);
    glDrawArrays(asset->drawType, asset->drawStart, asset->drawCount);

    //unbind everything
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    shaders->stopUsing();
}

// draws a single frame
static void Render() {
    // clear everything
    glClearColor(0.6, 0.8, 1.0, 1.0); // white -> we want white clouds :)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render all the instances
    std::list<ModelInstance>::const_iterator it;
    for (it = gInstances.begin(); it != gInstances.end(); ++it) {
        RenderInstance(*it);
    }

    std::list<ModelInstance>::const_iterator cit;
    for (cit = gCarInstances.begin(); cit != gCarInstances.end(); ++cit) {
        RenderInstance(*cit);
    }

    std::list<ModelInstance>::const_iterator rit;
    for (rit = gCarTireInstances.begin(); rit != gCarTireInstances.end(); ++rit) {
        RenderInstance(*rit);
    }

    // swap the display buffers (displays what was just drawn)
    glfwSwapBuffers(gWindow);
}

// update the scene based on the time elapsed since last update
static void Update(float secondsElapsed) {
    // https://glm.g-truc.net/0.9.9/api/a00668.html#ga1a4ecc4ad82652b8fb14dcb087879284
    // FOR EXAMPLE !!!! ***********************************************************
    // // rotate the first Block in `gInstances`
    const GLfloat degreesPerSecond = 20.0f;
    gDegreesRotated -= secondsElapsed * degreesPerSecond;
    while (gDegreesRotated > 360.0f) gDegreesRotated -= 360.0f;

    std::list<ModelInstance>::iterator carElement;
    for (carElement = gCarInstances.begin(); carElement != gCarInstances.end(); ++carElement) {
        // Set the element to the mid of the map
        carElement->transform = glm::translate(glm::mat4(), glm::vec3(30.0f, 2.0f, 30.0f));
        // Rotate the element
        carElement->transform *= glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0));
        // Set the element to the correct group position
        carElement->transform *= glm::translate(glm::mat4(), carElement->transformInner);
        // Set the element in the grup position to the final position in the map
        carElement->transform *= glm::translate(glm::mat4(), glm::vec3(25.0f, 0.0f, 0.0f));
        // *glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0));
        // cit->transform *= glm::translate(glm::mat4(), glm::vec3(27.0f, 0.0f, 0.0f));
    }

    // Rotate and translate the tires
    std::list<ModelInstance>::iterator carTireElement;
    for (carTireElement = gCarTireInstances.begin(); carTireElement != gCarTireInstances.end(); ++carTireElement) {
        // Set the element to the mid of the map
        carTireElement->transform = glm::translate(glm::mat4(), glm::vec3(30.0f, 2.0f, 30.0f));
        // Rotate the element
        carTireElement->transform *= glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0));
        // Set the element to the correct group position
        carTireElement->transform *= glm::translate(glm::mat4(), carTireElement->transformInner);
        carTireElement->transform *= glm::mat4() * glm::rotate(glm::mat4(), glm::radians(-gDegreesRotated*8), glm::vec3(1, 0, 0));
        // Set the element in the grup position to the final position in the map
        carTireElement->transform *= glm::translate(glm::mat4(), glm::vec3(25.0f, 0.0f, 0.0f));
        // *glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0));
        // cit->transform *= glm::translate(glm::mat4(), glm::vec3(27.0f, 0.0f, 0.0f));
    }


    // gInstances.front().transform = glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0));
    // gCarInstances.front().transform = glm::translate(glm::mat4(), glm::vec3(30.0f, 2.0f, 30.0f)) * glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0));
    // gCarInstances.front().transform *= glm::translate(glm::mat4(), glm::vec3(27.0f, 0.0f, 0.0f));

    //move position of camera based on WASD keys, and XZ keys for up and down
    const float moveSpeed = 4.0; //units per second
    if (glfwGetKey(gWindow, 'S')) {
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.forward());
    }
    else if (glfwGetKey(gWindow, 'W')) {
        gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.forward());
    }
    if (glfwGetKey(gWindow, 'A')) {
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.right());
    }
    else if (glfwGetKey(gWindow, 'D')) {
        gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.right());
    }
    if (glfwGetKey(gWindow, 'Z')) {
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -glm::vec3(0, 1, 0));
    }
    else if (glfwGetKey(gWindow, 'X')) {
        gCamera.offsetPosition(secondsElapsed * moveSpeed * glm::vec3(0, 1, 0));
    }

    //move light
    if (glfwGetKey(gWindow, '1')) {
        gLights[0].position = glm::vec4(gCamera.position(), 1.0);
        gLights[0].coneDirection = gCamera.forward();
    }

    // change light color
    if (glfwGetKey(gWindow, '2'))
        gLights[0].intensities = glm::vec3(2, 0, 0); //red
    else if (glfwGetKey(gWindow, '3'))
        gLights[0].intensities = glm::vec3(0, 2, 0); //green
    else if (glfwGetKey(gWindow, '4'))
        gLights[0].intensities = glm::vec3(2, 2, 2); //white


    //rotate camera based on mouse movement
    const float mouseSensitivity = 0.1f;
    double mouseX, mouseY;
    glfwGetCursorPos(gWindow, &mouseX, &mouseY);
    gCamera.offsetOrientation(mouseSensitivity * (float)mouseY, mouseSensitivity * (float)mouseX);
    glfwSetCursorPos(gWindow, 0, 0); //reset the mouse, so it doesn't go out of the window

    //increase or decrease field of view based on mouse wheel
    const float zoomSensitivity = -0.2f;
    float fieldOfView = gCamera.fieldOfView() + zoomSensitivity * (float)gScrollY;
    if (fieldOfView < 5.0f) fieldOfView = 5.0f;
    if (fieldOfView > 130.0f) fieldOfView = 130.0f;
    gCamera.setFieldOfView(fieldOfView);
    gScrollY = 0;
}

// records how far the y axis has been scrolled
void OnScroll(GLFWwindow* window, double deltaX, double deltaY) {
    gScrollY += deltaY;
}

void OnError(int errorCode, const char* msg) {
    throw std::runtime_error(msg);
}

void SetupCamera() {
    // setup gCamera
    float trans_x = 0.0;
    float trans_y = 0.0;
    float trans_z = -3.0;
    gCamera.setPosition(glm::vec3(trans_x, trans_y, trans_z));
    gCamera.setViewportAspectRatio(SCREEN_SIZE.x / SCREEN_SIZE.y);
    gCamera.setNearAndFarPlanes(0.5f, 100.0f);
    // Rotate the camera so that we look to the positive z achse
    gCamera.offsetOrientation(0.0, 180.0);
}

// Here is the Main, the program starts here and initialize everything...
void AppMain() {
    // initialise GLFW
    glfwSetErrorCallback(OnError);
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed");

    // open a window with GLFW
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    gWindow = glfwCreateWindow((int)SCREEN_SIZE.x, (int)SCREEN_SIZE.y, "OpenGL Tutorial", NULL, NULL);
    if (!gWindow)
        throw std::runtime_error("glfwCreateWindow failed. Can your hardware handle OpenGL 3.2?");

    // GLFW settings
    glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(gWindow, 0, 0);
    glfwSetScrollCallback(gWindow, OnScroll);
    glfwMakeContextCurrent(gWindow);

    glClearColor(0.41, 0.41, 0.41, 1.0);
    // initialise GLEW
    glewExperimental = GL_TRUE; //stops glew crashing on OSX :-/
    if (glewInit() != GLEW_OK)
        throw std::runtime_error("glewInit failed");

    // GLEW throws some errors, so discard all the errors so far
    while (glGetError() != GL_NO_ERROR) {}

    // print out some info about the graphics drivers
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // make sure OpenGL version 3.2 API is available
    if (!GLEW_VERSION_3_2)
        throw std::runtime_error("OpenGL 3.2 API is not available.");

    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Load all textures once !
    LoadTextures();

    // initialise the gExampleModelAsset asset
    LoadExampleAssets();

    // initialise all different block types and push them into the vector "blocks"
    LoadAllBlockTypes();

    // create all the instances in the 3D scene based on the gExampleModelAsset asset
    CreateCar();
    CreateTerrain();

    // Creates the Camera
    SetupCamera();

    CreateAllLights();

    // run while the window is open
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(gWindow)) {
        // process pending events
        glfwPollEvents();

        // update the scene based on the time elapsed since last update
        double thisTime = glfwGetTime();
        Update((float)(thisTime - lastTime));
        lastTime = thisTime;

        // draw one frame
        Render();

        // check for errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
            std::cerr << "OpenGL Error " << error << std::endl;

        //exit program if escape key is pressed
        if (glfwGetKey(gWindow, GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(gWindow, GL_TRUE);
    }

    // clean up and exit
    glfwTerminate();
}


int main(int argc, char* argv[]) {
    try {
        AppMain();
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}