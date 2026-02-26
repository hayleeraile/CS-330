///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// added to initialize the textures 
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
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
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
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
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
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
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
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
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
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
		return(false);
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

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
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
void SceneManager::SetShaderMaterial(
	std::string materialTag)
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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{

	//used the materials from this weeks resources for examples. 
	//https://learn.snhu.edu/content/enforced/2138202-CS-330-10504.202611-1/course_documents/CS%20330%20Applying%20Lighting%20to%20a%203D%20Scene.pdf?isCourseFile=true&ou=2138202
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	goldMaterial.ambientStrength = 0.4f;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	goldMaterial.shininess = 32.0;
	goldMaterial.tag = "gold";
	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	tileMaterial.ambientStrength = 0.3f;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 15.0;
	clayMaterial.tag = "clay";
	m_objectMaterials.push_back(clayMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *  This method is called to enable the phong lighting model
 ***********************************************************/

void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	//added a neutral point light to match my overhead office light
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[0].position", -3.0f, 8.0f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.95f, 0.92f, 0.85f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.6f, 0.6f, 0.6f);
	
	//added this as a secondary fill light to help with shadows
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[1].position", 0.0f, 6.0f, -8.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.1f, 0.12f, 0.15f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.25f, 0.30f, 0.35f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.10f, 0.10f, 0.10f);
	
	m_pShaderManager->setBoolValue("spotLight.bActive", false);
}


 /***********************************************************
  *  LoadSceneTextures()
  *
  *  Prepares the scene for textures
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;
/**Images too small
	bReturn = CreateGLTexture(
		"textures/spiralTexture.jpeg",
		"spiral"); //https://www.needpix.com/photo/1348787/metal-silver-stainlesssteel-brushedmetal-texture-wallpaper-background-art-abstract

		bReturn = CreateGLTexture(
	"textures/calcBody.jpeg",
	"calcBody"); //https://www.freepik.com/free-photo/black-fabric_928747.htm#fromView=keyword&page=1&position=0&uuid=050f0281-b3ba-4a14-9b39-d138ef1f3a65&query=Black+plastic+texture+seamless+loop
		**/

	//added a desk texture
	bReturn = CreateGLTexture(
		"textures/deskTexture.jpg",
		"desk"); //https://www.deviantart.com/black-light-studio/art/SDP-Texture-Seamless-453691498
	
	//added a lined paper texture
	bReturn = CreateGLTexture(
		"textures/linedPaperTexture.jpg",
		"lines"); //created myself in powerpoint because i couldn't find an image that was the size i needed

	
	// binds the images to the texture slots
	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	DefineObjectMaterials();//loaded the shader materials
	LoadSceneTextures();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadConeMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	SetupSceneLights();
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.13f, 0.17f, 0.22f, 1.0f);
	SetShaderTexture("desk"); //added texture to the image
	SetTextureUVScale(4.0f, 4.0f);
	SetShaderMaterial("tile"); //added shader material

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/***********************************
	**            notebook            **
	* *********************************/

	//box mesh for main pad
	//set scale for x y and z
	scaleXYZ = glm::vec3(4.0f, 0.15f, 6.0f);

	//set x y and z rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;

	//set x y and z position
	positionXYZ = glm::vec3(-2.0f, 0.08f, 1.5f);

	//set transformations to memory
	SetTransformations(scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set shader color based off picture
	SetShaderColor(0.84, 0.83, 0.8, 1.0);
	//adds in the notebook texture picture
	SetShaderTexture("lines");
	SetShaderMaterial("wood"); //added shader material


	//draw the box mesh
	m_basicMeshes->DrawBoxMesh();


	//torus loop for the spiral on the notebook

	//set x y and z scale
	scaleXYZ = glm::vec3(0.08f, 0.025f, 0.08f);
	
	//set x y and z rotations
	XrotationDegrees = 90.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = -180.0f;

	//set shader color based on picture
	SetShaderColor(0.69f, 0.69f, 0.68f, 1.0f);
	
	SetShaderMaterial("gold"); //added shader material

	// based on the yaw we want the spiral to follow the angle of the notebook
	float yawRad = glm::radians(YrotationDegrees);
	float stepX = sin(yawRad) * 0.45f;
	float stepZ = cos(yawRad) * 0.45f;

	//loop to create all spirals
	for (int i = 0; i < 13; i++)
	{
		float xPos = 0.5f + i * stepX;
		float yPos = 0.2f;            
		float zPos = -0.35f + i * stepZ;

		positionXYZ = glm::vec3(xPos, yPos, zPos);

		SetTransformations(scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		m_basicMeshes->DrawTorusMesh();
	}

	/***********************************
    **          calculator            **
    * *********************************/
	//Calculator body
	scaleXYZ = glm::vec3(2.2f, 0.25f, 3.4f);
	//set x y and z rotations
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;

	//set x y and z position
	positionXYZ = glm::vec3(-6.0f, 0.14f, 2.0f);

	//set transformations to memory
	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ);

	SetShaderColor(0.2f, 0.24f, 0.25f, 1.0f); //color based off the picture
	m_basicMeshes->DrawBoxMesh();

	//Calculator screen 
	scaleXYZ = glm::vec3(0.9f, 0.0f, 0.5f);

	//set x y and z rotations
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;

	//set x y and z positions
	positionXYZ = glm::vec3(-5.75f, 0.3f, 1.2f);

	//set transformations to memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.90f, 0.97f, 0.98f, 1.0f); //color based off of a powered on screen
	
	m_basicMeshes->DrawPlaneMesh();

	//Buttons
	scaleXYZ = glm::vec3(0.22f, 0.05f, 0.22f);
	//set x y and z rotations
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;

	SetShaderColor(0.69f, 0.74f, 0.73f, 1.0f); //color based off the white buttons

	//needed the buttons to align with the calculator by aligning with the rotation from the calculators center.
	glm::vec3 calcCenter = glm::vec3(-6.0f, 0.14f, 2.0f);
	float yawRads = glm::radians(YrotationDegrees);


	// where the grid starts, as an offset from center. these are the coordinates for the top left button
	float startOffX = -0.5f;   
	float startOffZ = 0.1f;   
	float topY = 0.28f; //how high the buttons will sit off the desk

	for (int r = 0; r < 5; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			// for each button we need to multiply it by 0.35f for the spacing
			float xOffset = startOffX + c * 0.35f;
			float zOffset = startOffZ + r * 0.35f;

			//allows the rotation to happen with each button in the grid
			float X = calcCenter.x + (xOffset * cos(yawRads) + zOffset * sin(yawRads));
			float Z = calcCenter.z + (-xOffset * sin(yawRads) + zOffset * cos(yawRads));

			positionXYZ = glm::vec3(X, topY, Z);

			SetTransformations(
				scaleXYZ,
				XrotationDegrees,
				YrotationDegrees,
				ZrotationDegrees,
				positionXYZ);

			m_basicMeshes->DrawBoxMesh();
		}
	}


	/***********************************
    **             pencil             **
    * *********************************/
	// Pencil body
	scaleXYZ = glm::vec3(0.08f, 3.8f, 0.08f);   
	XrotationDegrees = 90.0f;                  
	YrotationDegrees = -24.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.5f, 0.18f, 1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.65f, 0.74f, 0.76f, 1.0f); //color based off the picture
	SetShaderMaterial("glass");

	m_basicMeshes->DrawCylinderMesh();

	// Pencil tip
	scaleXYZ = glm::vec3(0.09f, 0.35f, 0.08f);
	positionXYZ = glm::vec3(-0.05f, 0.18f, 4.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.72f, 0.71f, 0.67f, 1.0f);//color based off the picture
	SetShaderMaterial("glass");
	
	m_basicMeshes->DrawConeMesh();

	/***********************************
    **              mug               **
    * *********************************/
	// Mug outer body
	scaleXYZ = glm::vec3(0.9f, 1.75f, 0.9f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.0f, 0.60f, 0.8f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetShaderColor(0.42f, 0.47f, 0.37f, 1.0f); // color based off the picture
	SetShaderMaterial("clay"); //sets the lighting material

	m_basicMeshes->DrawCylinderMesh();

	//Mug inner 
	scaleXYZ = glm::vec3(0.78f, 1.6f, 0.78f);
	positionXYZ = glm::vec3(3.0f, 0.76f, 0.8f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1f, 0.18f, 0.25f, 1.0f); //based off the picture
	
	m_basicMeshes->DrawCylinderMesh();

	//Mug handle
	scaleXYZ = glm::vec3(0.7f, 0.45f, 0.18f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(4.05f, 1.25f, 0.8f);

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetShaderColor(0.42f, 0.47f, 0.37f, 1.0f); //color based off the picture
	SetShaderMaterial("clay"); //sets the lighting material

	m_basicMeshes->DrawTorusMesh();
}


	
	



