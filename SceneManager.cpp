///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Modified by Student to apply textures to the coffee mug
//  Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <iostream> // for debug printing

// declaration of global variables
namespace
{
    const char* g_ModelName = "model";
    const char* g_ColorValueName = "objectColor";
    const char* g_TextureValueName = "objectTexture";
    const char* g_UseTextureName = "bUseTexture";
    const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_basicMeshes = new ShapeMeshes();

    // Initialize texture count to zero
    m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
    // Free any GPU texture objects before shutting down
    DestroyGLTextures();

    m_pShaderManager = NULL;

    delete m_basicMeshes;
    m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
    int width = 0;
    int height = 0;
    int colorChannels = 0;
    GLuint textureID = 0;

    // indicate to always flip images vertically when loaded
    stbi_set_flip_vertically_on_load(true);

    // try to parse the image data from the specified image file
    unsigned char* image = stbi_load(
        filename,
        &width,
        &height,
        &colorChannels,
        0);

    // if the image was successfully read from the image file
    if (image)
    {
        std::cout << "[TextureLoader] Successfully loaded image: "
            << filename
            << ", width: " << width
            << ", height: " << height
            << ", channels: " << colorChannels
            << std::endl;

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // set the texture wrapping parameters (repeat for tiling)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters (linear + mipmaps)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // if the loaded image is in RGB format
        if (colorChannels == 3)
        {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGB8,
                width,
                height,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                image
            );
        }
        // if the loaded image is in RGBA format - it supports transparency
        else if (colorChannels == 4)
        {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA8,
                width,
                height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                image
            );
        }
        else
        {
            std::cout << "[TextureLoader] ERROR: Not implemented to handle image with "
                << colorChannels
                << " channels"
                << std::endl;
            stbi_image_free(image);
            glBindTexture(GL_TEXTURE_2D, 0);
            return false;
        }

        // generate the texture mipmaps for efficient minification
        glGenerateMipmap(GL_TEXTURE_2D);

        // free the image data from CPU memory
        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        // register the loaded texture and associate it with the given tag
        if (m_loadedTextures < MAX_TEXTURES)
        {
            m_textureIDs[m_loadedTextures].ID = textureID;
            m_textureIDs[m_loadedTextures].tag = tag;
            m_loadedTextures++;
        }
        else
        {
            std::cout << "[TextureLoader] WARNING: Exceeded MAX_TEXTURES; texture not stored."
                << std::endl;
        }

        return true;
    }

    std::cout << "[TextureLoader] Could not load image: " << filename << std::endl;
    return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots. There can be up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
    }
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glDeleteTextures(1, &m_textureIDs[i].ID);
    }
    m_loadedTextures = 0;
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed-in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
    int textureID = -1;
    int index = 0;
    bool bFound = false;

    while ((index < m_loadedTextures) && (bFound == false))
    {
        if (m_textureIDs[index].tag.compare(tag) == 0)
        {
            textureID = m_textureIDs[index].ID;
            bFound = true;
        }
        else
        {
            index++;
        }
    }

    return textureID;
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed-in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
    int textureSlot = -1;
    int index = 0;
    bool bFound = false;

    while ((index < m_loadedTextures) && (bFound == false))
    {
        if (m_textureIDs[index].tag.compare(tag) == 0)
        {
            textureSlot = index;  // slot = index into m_textureIDs
            bFound = true;
        }
        else
        {
            index++;
        }
    }

    return textureSlot;
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
    if (m_objectMaterials.size() == 0)
    {
        return false;
    }

    int index = 0;
    bool bFound = false;
    while ((index < m_objectMaterials.size()) && (bFound == false))
    {
        if (m_objectMaterials[index].tag.compare(tag) == 0)
        {
            bFound = true;
            material.diffuseColor = m_objectMaterials[index].diffuseColor;
            material.specularColor = m_objectMaterials[index].specularColor;
            material.shininess = m_objectMaterials[index].shininess;
        }
        else
        {
            index++;
        }
    }

    return true;
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
    glm::vec3 scaleXYZ,
    float      XrotationDegrees,
    float      YrotationDegrees,
    float      ZrotationDegrees,
    glm::vec3 positionXYZ)
{
    //variables for this method
    glm::mat4 scale = glm::scale(scaleXYZ);
    glm::mat4 rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 translation = glm::translate(positionXYZ);

    glm::mat4 modelView = translation * rotationZ * rotationY * rotationX * scale;

    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setMat4Value(g_ModelName, modelView);
    }
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed-in color
 *  into the shader for the next draw command (disables texture).
 ***********************************************************/
void SceneManager::SetShaderColor(
    float redColorValue,
    float greenColorValue,
    float blueColorValue,
    float alphaValue)
{
    glm::vec4 currentColor;
    currentColor.r = redColorValue;
    currentColor.g = greenColorValue;
    currentColor.b = blueColorValue;
    currentColor.a = alphaValue;

    if (NULL != m_pShaderManager)
    {
        // Turn off texturing in the shader
        m_pShaderManager->setIntValue(g_UseTextureName, 0);
        m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
    }
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed-in tag into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
    std::string textureTag)
{
    if (NULL != m_pShaderManager)
    {
        // Turn on texturing in the shader
        m_pShaderManager->setIntValue(g_UseTextureName, 1);

        int slot = FindTextureSlot(textureTag);
        if (slot >= 0)
        {
            // Tell the shader which texture unit to sample from
            m_pShaderManager->setSampler2DValue(g_TextureValueName, slot);
        }
        else
        {
            std::cerr << "[SceneManager] ERROR: Texture tag '"
                << textureTag
                << "' not found."
                << std::endl;
        }
    }
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader (for tiling).
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
    }
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(std::string materialTag)
{
    if (m_objectMaterials.size() > 0)
    {
        OBJECT_MATERIAL material;
        bool bReturn = false;

        bReturn = FindMaterial(materialTag, material);
        if (bReturn == true)
        {
            m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
            m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
            m_pShaderManager->setFloatValue("material.shininess", material.shininess);
        }
    }
}

/***********************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for***/
/*** preparing and rendering their own 3D replicated scenes***/
/*** Please refer to the code in the OpenGL sample project ***/
/*** for assistance.                                      ***/
/***********************************************************/

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the meshes and textures into memory so we can render.
 ***********************************************************/
 /***********************************************************
  *  PrepareScene()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the meshes and textures into memory so we can render.
  ***********************************************************/
void SceneManager::PrepareScene()
{
    // Load basic meshes (plane, box, cylinder, torus)
    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadBoxMesh();
    m_basicMeshes->LoadCylinderMesh();
    m_basicMeshes->LoadTorusMesh();
    m_basicMeshes->LoadSphereMesh();

    // Load all textures
    bool loadedTent = CreateGLTexture("textures/tent.jpg", "tent");
    bool loadedGround = CreateGLTexture("textures/ground.jpg", "ground");
    bool loadedCampfire = CreateGLTexture("textures/campfire.jpg", "campfire");
    bool loadedSky = CreateGLTexture("textures/sky.jpg", "sky");
    bool loadedBark = CreateGLTexture("textures/bark.jpg", "bark");
    bool loadedMetal = CreateGLTexture("textures/metal.jpg", "metal");
    bool loadedRock = CreateGLTexture("textures/rock.jpg", "rock");

   
    // Report any missing textures
    if (!loadedTent)
        std::cerr << "[PrepareScene] ERROR: Failed to load textures/tent.jpg\n";
    if (!loadedGround)
        std::cerr << "[PrepareScene] ERROR: Failed to load textures/ground.jpg\n";
    if (!loadedCampfire)
        std::cerr << "[PrepareScene] ERROR: Failed to load textures/campfire.jpg\n";
    if (!loadedSky)
        std::cerr << "[PrepareScene] ERROR: Failed to load textures/sky.jpg\n";
    if (!loadedBark)
        std::cerr << "[PrepareScene] ERROR: Failed to load textures/bark.jpg\n";
    if (!loadedMetal)
        std::cerr << "[PrepareScene] ERROR: Failed to load textures/metal.jpg\n";

    // MATERIAL: Floor
    OBJECT_MATERIAL floorMat;
    floorMat.tag = "floor";
    floorMat.diffuseColor = glm::vec3(0.44f, 0.26f, 0.08f);
    floorMat.specularColor = glm::vec3(0.3f);
    floorMat.shininess = 32.0f;
    m_objectMaterials.push_back(floorMat);

    // MATERIAL: Tent
    OBJECT_MATERIAL tentMat;
    tentMat.tag = "tent";
    tentMat.diffuseColor = glm::vec3(0.2f, 0.4f, 0.1f);
    tentMat.specularColor = glm::vec3(0.2f);
    tentMat.shininess = 16.0f;
    m_objectMaterials.push_back(tentMat);

    // MATERIAL: Campfire
    OBJECT_MATERIAL fireMat;
    fireMat.tag = "campfire";
    fireMat.diffuseColor = glm::vec3(1.0f, 0.5f, 0.0f);
    fireMat.specularColor = glm::vec3(0.3f);
    fireMat.shininess = 8.0f;
    m_objectMaterials.push_back(fireMat);

    // MATERIAL: Logs (bark)
    OBJECT_MATERIAL barkMat;
    barkMat.tag = "bark";
    barkMat.diffuseColor = glm::vec3(0.35f, 0.2f, 0.1f);
    barkMat.specularColor = glm::vec3(0.1f);
    barkMat.shininess = 12.0f;
    m_objectMaterials.push_back(barkMat);

    // MATERIAL: Mug (metal)
    OBJECT_MATERIAL metalMat;
    metalMat.tag = "metal";
    metalMat.diffuseColor = glm::vec3(0.6f);
    metalMat.specularColor = glm::vec3(0.9f);
    metalMat.shininess = 64.0f;
    m_objectMaterials.push_back(metalMat);

    // MATERIAL: Sky
    OBJECT_MATERIAL skyMat;
    skyMat.tag = "sky";
    skyMat.diffuseColor = glm::vec3(1.0f);
    skyMat.specularColor = glm::vec3(0.0f);
    skyMat.shininess = 1.0f;
    m_objectMaterials.push_back(skyMat);
}

void SceneManager::RenderScene()
{
    BindGLTextures();
    SetupLighting();

    glm::vec3 scaleXYZ;
    glm::vec3 positionXYZ;
    float XrotationDegrees = -45.0f;

    /***** GROUND PLANE *****/
    scaleXYZ = glm::vec3(300.0f, 1.0f, 200.0f);
    positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
    SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
    SetShaderMaterial("floor");
    SetShaderTexture("ground");
    SetTextureUVScale(10.0f, 10.0f);
    m_basicMeshes->DrawPlaneMesh();

    // SKY BACKDROP FIX — ROTATE INTO VIEW
    m_pShaderManager->setIntValue("bUseLighting", 0);

    scaleXYZ = glm::vec3(1000.0f, 1.0f, 500.0f);  // Width, "thickness", height (Z = vertical span)
    positionXYZ = glm::vec3(0.0f, 300.0f, 0.0f); // Far back and raised

    SetTransformations(scaleXYZ, XrotationDegrees, 0.0f, 0.0f, positionXYZ);
    SetShaderMaterial("sky");
    SetShaderTexture("sky");
    SetTextureUVScale(1.0f, 1.0f);
    XrotationDegrees = glm::radians(360.0f);
    m_basicMeshes->DrawPlaneMesh();

    m_pShaderManager->setIntValue("bUseLighting", 1);



    /***** TENT BASE *****/
    scaleXYZ = glm::vec3(9.0f, 0.1f, -5.0f);
    positionXYZ = glm::vec3(-7.45f, .1f, -4.5f);
    SetTransformations(scaleXYZ, -1.0, 0, 0, positionXYZ);
    SetShaderMaterial("tent");
    SetShaderTexture("tent");
    m_basicMeshes->DrawBoxMesh();

    /***** TENT ROOF *****/
    scaleXYZ = glm::vec3(8.0f, 0.1f, -5.0f);
    positionXYZ = glm::vec3(-10.5f, 2.0f, -5.0f);
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 45.0f, positionXYZ);
    SetShaderMaterial("tent");
    SetShaderTexture("tent");
    m_basicMeshes->DrawBoxMesh();
    positionXYZ = glm::vec3(-5.f, 2.0f, -5.0f);
    SetTransformations(scaleXYZ, 0.0f, 0.0f, -45.0f, positionXYZ);
    m_basicMeshes->DrawBoxMesh();

    /***** CAMPFIRE RING *****/
    scaleXYZ = glm::vec3(2.0f, 0.1f, 2.5f);
    positionXYZ = glm::vec3(1.5f, 0.5f, -2.0f);
    SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
    SetShaderMaterial("campfire");
    SetShaderTexture("campfire");
    m_basicMeshes->DrawCylinderMesh();

    /***** FIRE LOGS - Simple 3-Log Teepee *****/

// Log 1 - leaning back

    scaleXYZ = glm::vec3(0.4f, 3.0f, 0.5f);
    positionXYZ = glm::vec3(-0.25f, 0.5f, -1.25f);
    SetTransformations(scaleXYZ, XrotationDegrees, 0.0f, 0.0f, positionXYZ);
    SetShaderMaterial("bark");
    SetShaderTexture("bark");
    XrotationDegrees = 45.0; 
    m_basicMeshes->DrawCylinderMesh();


    // Log 2 - leaning left
    scaleXYZ = glm::vec3(0.4f, 3.0f, 0.5f);
    positionXYZ = glm::vec3(1.0f, 0.5f, .5f);
    SetTransformations(scaleXYZ, glm::radians(90.0f), glm::radians(60.0f), glm::radians(30.0f), positionXYZ);
    m_basicMeshes->DrawCylinderMesh();

    // Log 3 - leaning right
    scaleXYZ = glm::vec3(0.4f, 3.0f, 0.5f);
    positionXYZ = glm::vec3(1.5f, 0.5f, -4.0f);
    SetTransformations(scaleXYZ, glm::radians(90.0f), glm::radians(-60.0f), glm::radians(30.0f), positionXYZ);
    m_basicMeshes->DrawCylinderMesh();

    // Log 4 - leaning left
    scaleXYZ = glm::vec3(0.4f, 3.0f, 0.5f);
    positionXYZ = glm::vec3(3.0f, 0.5f, -1.0f);
    SetTransformations(scaleXYZ, glm::radians(90.0f), glm::radians(60.0f), glm::radians(30.0f), positionXYZ);
    m_basicMeshes->DrawCylinderMesh();

    // Log 5 - leaning left
    scaleXYZ = glm::vec3(0.4f, 3.0f, 0.5f);
    positionXYZ = glm::vec3(.1f, 0.5f, -.1f);
    SetTransformations(scaleXYZ, glm::radians(90.0f), glm::radians(60.0f), glm::radians(30.0f), positionXYZ);
    m_basicMeshes->DrawCylinderMesh();


    // Log 6 - leaning left
    scaleXYZ = glm::vec3(0.4f, 3.0f, 0.5f);
    positionXYZ = glm::vec3(2.00f, 0.5f, .5f);
    SetTransformations(scaleXYZ, glm::radians(90.0f), glm::radians(60.0f), glm::radians(30.0f), positionXYZ);
    m_basicMeshes->DrawCylinderMesh();


    // Log 7 - leaning left
    scaleXYZ = glm::vec3(0.4f, 3.0f, 0.5f);
    positionXYZ = glm::vec3(0.25f, 0.5f, -3.75f);
    SetTransformations(scaleXYZ, glm::radians(90.0f), glm::radians(60.0f), glm::radians(30.0f), positionXYZ);
    m_basicMeshes->DrawCylinderMesh();


    // Log 8 - leaning left
    scaleXYZ = glm::vec3(02.4f, 5.0f, 0.5f);
    positionXYZ = glm::vec3(2.5f, 0.5f, -3.75f);
    SetTransformations(scaleXYZ, glm::radians(90.0f), glm::radians(60.0f), glm::radians(30.0f), positionXYZ);
    m_basicMeshes->DrawCylinderMesh();

    /***** MUG BODY *****/
    scaleXYZ = glm::vec3(0.4f, 0.6f, 0.4f);
    positionXYZ = glm::vec3(6.0f, 0.3f, -1.5f);
    SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
    SetShaderMaterial("metal");
    SetShaderTexture("metal");
    m_basicMeshes->DrawCylinderMesh();

    /***** MUG HANDLE *****/
    scaleXYZ = glm::vec3(0.15f);
    positionXYZ = glm::vec3(5.5f, 0.6f, -1.5f);
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
    m_basicMeshes->DrawTorusMesh();

    /***** ROCKS GROUP *****/

// Big main rock
    m_pShaderManager->setIntValue("bUseLighting", 0);  // Turn off lighting for backdrop-style boulders

    scaleXYZ = glm::vec3(10.3f, 5.2f, 8.7f);
    positionXYZ = glm::vec3(15.0f, 0.5f, -15.0f);
    SetTransformations(scaleXYZ, glm::radians(15.0f), glm::radians(20.0f), glm::radians(5.0f), positionXYZ);
    SetShaderMaterial("rock");
    SetShaderTexture("rock");
    m_basicMeshes->DrawSphereMesh();

    m_pShaderManager->setIntValue("bUseLighting", 0);  // Re-enable lighting

    // Smaller rock
    scaleXYZ = glm::vec3(1.2f, 0.6f, 1.0f);
    positionXYZ = glm::vec3(12.0f, 0.3f, -4.5f);
    SetTransformations(scaleXYZ, glm::radians(12.0f), glm::radians(8.0f), glm::radians(3.0f), positionXYZ);
    SetShaderMaterial("rock");
    SetShaderTexture("rock");
    m_basicMeshes->DrawSphereMesh();

    // Another boulder
    scaleXYZ = glm::vec3(1.8f, 1.2f, 1.4f);
    positionXYZ = glm::vec3(7.0f, 0.4f, -7.0f);
    SetTransformations(scaleXYZ, glm::radians(25.0f), glm::radians(18.0f), glm::radians(12.0f), positionXYZ);
    SetShaderMaterial("rock");
    SetShaderTexture("rock");
    m_basicMeshes->DrawSphereMesh();


  






}

void SceneManager::SetupLighting()
{
    m_pShaderManager->setIntValue("bUseLighting", 1);
    m_pShaderManager->setVec3Value("viewPosition", glm::vec3(0.0f, 5.0f, 15.0f));

    // Directional light
    m_pShaderManager->setVec3Value("dirLight.direction", glm::vec3(-0.5f, -0.5f, -1.0f));
    m_pShaderManager->setVec3Value("dirLight.diffuse", glm::vec3(0.9f, 0.9f, 0.9f));
    m_pShaderManager->setVec3Value("dirLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));
    m_pShaderManager->setBoolValue("dirLight.bActive", true);

    // Point light (white overhead)
    m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(14.0f, 6.0f, -14.0f));
    m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.4f, 0.4f, 0.4f));
    m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
    m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(1.0f, 1.0f, 1.0f));
    m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
    m_pShaderManager->setFloatValue("pointLights[0].linear", 0.09f);
    m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.032f);
    m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

    // Point light (warm campfire glow)
    m_pShaderManager->setVec3Value("pointLights[1].position", glm::vec3(1.5f, 1.0f, -2.0f));
    m_pShaderManager->setVec3Value("pointLights[1].ambient", glm::vec3(0.6f, 0.3f, 0.1f));
    m_pShaderManager->setVec3Value("pointLights[1].diffuse", glm::vec3(0.9f, 0.4f, 0.1f));
    m_pShaderManager->setVec3Value("pointLights[1].specular", glm::vec3(0.8f, 0.3f, 0.2f));
    m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
    m_pShaderManager->setFloatValue("pointLights[1].linear", 0.14f);
    m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.07f);
    m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
}
