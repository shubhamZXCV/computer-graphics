#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "file_utils.h"
#include "math_utils.h"
#include "OFFReader.h"
#define GL_SILENCE_DEPRECATION

/********************************************************************/
/*   Variables */

char theProgramTitle[] = "Sample";
int theWindowWidth = 800, theWindowHeight = 600;
int theWindowPositionX = 40, theWindowPositionY = 40;
bool isFullScreen = false;
bool isAnimating = true;
float rotation = 0.0f;
GLuint VBO, VAO;
// GLuint gWorldLocation;

/* Constants */
const int ANIMATION_DELAY = 20; /* milliseconds between rendering */
const char *pVSFileName = "shaders/shader.vs";
const char *pFSFileName = "shaders/shader.fs";
const char *pGSFileName = "shaders/shader.gs";

const char *offFile1 = "models/1grm.off";
const char *offFile2 = "models/2oar.off";
const char *offFile3 = "models/3sy7.off";
const char *offFile4 = "models/4hhb.off";

// Add after other global variables (around line 30)

// View mode flag
bool isDroneViewEnabled = false; // Start with normal view

// UI mode flag
bool isUIMode = true; // Start with UI mode enabled

// Camera position and orientation
float cameraX = 0.0f;
float cameraY = 0.0f;
float cameraZ = 3.0f; // Start camera a bit back from the model

// Camera orientation (look direction)
float yaw = -90.0f; // Initial look direction (facing negative Z)
float pitch = 0.0f;
float lastX = 0, lastY = 0; // Last mouse position
bool firstMouse = true;



// Movement speed
float cameraSpeed = 0.05;

OffModel *model1;
OffModel *model2;
OffModel *model3;
OffModel *model4;

// Add after other global variables

// Shader uniform locations
GLuint gModelLocation;
GLuint gViewLocation;
GLuint gWorldLocation;
GLuint viewPosLocation;
GLuint lightPosLocations[3];
GLuint lightColorLocations[3];
GLuint lightIntensityLocations[3];
GLuint lightEnabledLocations[3];
GLuint numActiveLightsLocation;
GLuint ambientStrengthLocation;
GLuint specularStrengthLocation;
GLuint shininessLocation;
GLuint useBlinnPhongLocation;
GLuint useDepthColorLocation;
GLuint minDepthLocation;
GLuint maxDepthLocation;

// Light properties
struct Light
{
	Vector3f position;
	Vector3f color;
	float intensity;
	bool enabled;
};

// Lighting variables
Light lights[3];
int numActiveLights = 3;
float ambientStrength = 0.1f;
float specularStrength = 0.5f;
float shininess = 32.0f;
bool useBlinnPhong = true;
bool useDepthColor = false;
float minDepth = 0.1f;
float maxDepth = 20.0f;

// Add these with your other global variables
bool isExploded = false;         // Whether the mesh is currently exploded
float explosionFactor = 0.0f;    // Current explosion amount
float explosionMax = 2.0f;       // Maximum explosion amount
bool isExploding = false;        // Whether explosion animation is in progress
float explosionSpeed = 0.02f;    // Speed of explosion animation

// Uniform location variables
GLuint explosionFactorLocation;
GLuint isExplodedLocation;

/********************************************************************
  Utility functions
 */

/* post: compute frames per second and display in window's title bar */

void computeNormals(OffModel *model)
{
	int i, j;

	// Initialize all vertex normals to zero
	for (i = 0; i < model->numberOfVertices; ++i)
	{
		model->vertices[i].normal.x = 0.0f;
		model->vertices[i].normal.y = 0.0f;
		model->vertices[i].normal.z = 0.0f;
		model->vertices[i].numIcidentTri = 0;
	}

	// Loop through each polygon (face)
	for (i = 0; i < model->numberOfPolygons; ++i)
	{
		Polygon poly = model->polygons[i];

		if (poly.noSides < 3)
			continue; // skip invalid faces

		// Get indices of 3 vertices
		int i0 = poly.v[0];
		int i1 = poly.v[1];
		int i2 = poly.v[2];

		// Get vertices
		Vertex v0 = model->vertices[i0];
		Vertex v1 = model->vertices[i1];
		Vertex v2 = model->vertices[i2];

		// Compute edge vectors
		float ux = v1.x - v0.x;
		float uy = v1.y - v0.y;
		float uz = v1.z - v0.z;

		float vx = v2.x - v0.x;
		float vy = v2.y - v0.y;
		float vz = v2.z - v0.z;

		// Cross product: u × v = normal
		float nx = uy * vz - uz * vy;
		float ny = uz * vx - ux * vz;
		float nz = ux * vy - uy * vx;

		// Normalize the face normal
		float length = sqrt(nx * nx + ny * ny + nz * nz);
		if (length == 0.0f)
			continue;

		nx /= length;
		ny /= length;
		nz /= length;

		// Add the face normal to each vertex normal
		for (j = 0; j < poly.noSides; ++j)
		{
			int vi = poly.v[j];
			model->vertices[vi].normal.x += nx;
			model->vertices[vi].normal.y += ny;
			model->vertices[vi].normal.z += nz;
			model->vertices[vi].numIcidentTri += 1;
		}
	}

	// Normalize vertex normals
	for (i = 0; i < model->numberOfVertices; ++i)
	{
		Vector3f *n = &model->vertices[i].normal;
		float length = sqrt(n->x * n->x + n->y * n->y + n->z * n->z);
		if (length > 0.0f)
		{
			n->x /= length;
			n->y /= length;
			n->z /= length;
		}
	}
}



// Compute view matrix based on camera position and orientation
Matrix4f ComputeViewMatrix()
{
	// Calculate camera direction vectors
	float dirX = cos(yaw * 3.14159f / 180.0f) * cos(pitch * 3.14159f / 180.0f);
	float dirY = sin(pitch * 3.14159f / 180.0f);
	float dirZ = sin(yaw * 3.14159f / 180.0f) * cos(pitch * 3.14159f / 180.0f);

	// Normalize the direction vector
	float length = sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
	dirX /= length;
	dirY /= length;
	dirZ /= length;

	// Calculate camera right vector (using world up = {0,1,0})
	float rightX = dirZ; // Cross product of (0,1,0) and direction vector
	float rightY = 0.0f;
	float rightZ = -dirX;

	// Normalize right vector
	length = sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
	if (length > 0.0f)
	{
		rightX /= length;
		rightZ /= length;
	}

	// Calculate camera up vector (cross product of right and direction)
	float upX = rightY * dirZ - rightZ * dirY;
	float upY = rightZ * dirX - rightX * dirZ;
	float upZ = rightX * dirY - rightY * dirX;

	// Create the view matrix
	Matrix4f View;

	// First row: right vector
	View.m[0][0] = rightX;
	View.m[0][1] = rightY;
	View.m[0][2] = rightZ;
	View.m[0][3] = -rightX * cameraX - rightY * cameraY - rightZ * cameraZ;

	// Second row: up vector
	View.m[1][0] = upX;
	View.m[1][1] = upY;
	View.m[1][2] = upZ;
	View.m[1][3] = -upX * cameraX - upY * cameraY - upZ * cameraZ;

	// Third row: negated direction vector
	View.m[2][0] = -dirX;
	View.m[2][1] = -dirY;
	View.m[2][2] = -dirZ;
	View.m[2][3] = dirX * cameraX + dirY * cameraY + dirZ * cameraZ;

	// Fourth row: homogeneous coordinates
	View.m[3][0] = 0.0f;
	View.m[3][1] = 0.0f;
	View.m[3][2] = 0.0f;
	View.m[3][3] = 1.0f;

	return View;
}

void computeFPS()
{
	static int frameCount = 0;
	static int lastFrameTime = 0;
	static char *title = NULL;
	int currentTime;

	if (!title)
		title = (char *)malloc((strlen(theProgramTitle) + 20) * sizeof(char));
	frameCount++;
	currentTime = 0;
	if (currentTime - lastFrameTime > 1000)
	{
		sprintf(title, "%s [ FPS: %4.2f ]",
				theProgramTitle,
				frameCount * 1000.0 / (currentTime - lastFrameTime));
		lastFrameTime = currentTime;
		frameCount = 0;
	}
}

void CreateVertexBuffer(OffModel *model)
{
	// Generate and bind VAO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Create vertex buffer
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// Allocate buffer memory and copy vertex data
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * model->numberOfVertices,
				 model->vertices, GL_STATIC_DRAW);

	// Set up position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
						  (void *)offsetof(Vertex, x));

	// Set up normal attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
						  (void *)offsetof(Vertex, normal));

	// Create index buffers for each polygon
	GLuint EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	// Calculate total number of indices needed
	int totalIndices = 0;
	for (int i = 0; i < model->numberOfPolygons; i++)
	{
		if (model->polygons[i].noSides >= 3)
		{
			totalIndices += (model->polygons[i].noSides - 2) * 3; // Triangulation
		}
	}

	// Allocate and fill index array
	GLuint *indices = new GLuint[totalIndices];
	int indexOffset = 0;

	for (int i = 0; i < model->numberOfPolygons; i++)
	{
		Polygon poly = model->polygons[i];
		if (poly.noSides >= 3)
		{
			// Triangulate the polygon (simple fan triangulation)
			for (int j = 1; j < poly.noSides - 1; j++)
			{
				indices[indexOffset++] = poly.v[0];
				indices[indexOffset++] = poly.v[j];
				indices[indexOffset++] = poly.v[j + 1];
			}
		}
	}

	// Upload indices to GPU
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, totalIndices * sizeof(GLuint),
				 indices, GL_STATIC_DRAW);

	// Store the total count for drawing
	model->totalIndices = totalIndices;

	delete[] indices;

	glBindVertexArray(0);
}

static void AddShader(GLuint ShaderProgram, const char *pShaderText, GLenum ShaderType)
{
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0)
	{
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}

	const GLchar *p[1];
	p[0] = pShaderText;
	GLint Lengths[1];
	Lengths[0] = strlen(pShaderText);
	glShaderSource(ShaderObj, 1, p, Lengths);
	glCompileShader(ShaderObj);
	GLint success;
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}

	glAttachShader(ShaderProgram, ShaderObj);
}

using namespace std;

// Replace your CompileShaders function with this updated version
static void CompileShaders()
{
	GLuint ShaderProgram = glCreateProgram();

	if (ShaderProgram == 0)
	{
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	string vs, gs, fs;

	if (!ReadFile(pVSFileName, vs))
	{
		exit(1);
	}

	if (!ReadFile(pGSFileName, gs))
	{
		exit(1);
	}

	if (!ReadFile(pFSFileName, fs))
	{
		exit(1);
	}

	AddShader(ShaderProgram, vs.c_str(), GL_VERTEX_SHADER);
	AddShader(ShaderProgram, gs.c_str(), GL_GEOMETRY_SHADER);
	AddShader(ShaderProgram, fs.c_str(), GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = {0};

	glLinkProgram(ShaderProgram);
	glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
	if (Success == 0)
	{
		glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	glBindVertexArray(VAO);
	glValidateProgram(ShaderProgram);
	glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
	if (!Success)
	{
		glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program1: '%s'\n", ErrorLog);
		exit(1);
	}

	glUseProgram(ShaderProgram);

	// Get uniform locations for transforms
	gWorldLocation = glGetUniformLocation(ShaderProgram, "gWorld");
	gModelLocation = glGetUniformLocation(ShaderProgram, "gModel");
	gViewLocation = glGetUniformLocation(ShaderProgram, "gView");

	// Get camera position uniform location
	viewPosLocation = glGetUniformLocation(ShaderProgram, "viewPos");

	// Get light uniform locations
	for (int i = 0; i < 3; i++)
	{
		char buffer[100];
		sprintf(buffer, "lightPositions[%d]", i);
		lightPosLocations[i] = glGetUniformLocation(ShaderProgram, buffer);

		sprintf(buffer, "lightColors[%d]", i);
		lightColorLocations[i] = glGetUniformLocation(ShaderProgram, buffer);

		sprintf(buffer, "lightIntensities[%d]", i);
		lightIntensityLocations[i] = glGetUniformLocation(ShaderProgram, buffer);

		sprintf(buffer, "lightEnabled[%d]", i);
		lightEnabledLocations[i] = glGetUniformLocation(ShaderProgram, buffer);
	}

	numActiveLightsLocation = glGetUniformLocation(ShaderProgram, "numActiveLights");
	ambientStrengthLocation = glGetUniformLocation(ShaderProgram, "ambientStrength");
	specularStrengthLocation = glGetUniformLocation(ShaderProgram, "specularStrength");
	shininessLocation = glGetUniformLocation(ShaderProgram, "shininess");
	useBlinnPhongLocation = glGetUniformLocation(ShaderProgram, "useBlinnPhong");
	useDepthColorLocation = glGetUniformLocation(ShaderProgram, "useDepthColor");
	minDepthLocation = glGetUniformLocation(ShaderProgram, "minDepth");
	maxDepthLocation = glGetUniformLocation(ShaderProgram, "maxDepth");

	// Add explosion uniforms
	 explosionFactorLocation = glGetUniformLocation(ShaderProgram, "explosionFactor");
	 isExplodedLocation = glGetUniformLocation(ShaderProgram, "isExploded");
}

/********************************************************************
 Callback Functions
 */

void onInit(int argc, char *argv[])
{
	/* by default the back ground color is black */
	model1 = readOffFile(offFile4);

	computeNormals(model1);

	cout << model1->numberOfVertices << endl;
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Create the vertex buffer for the model
	CreateVertexBuffer(model1);

	// Initialize lights
	// Light 1: Red light from right
	lights[0].position = Vector3f(5.0f, 0.0f, 0.0f);
	lights[0].color = Vector3f(1.0f, 0.0f, 0.0f); // Red
	lights[0].intensity = 1.0f;
	lights[0].enabled = true;

	// Light 2: Green light from top
	lights[1].position = Vector3f(0.0f, 5.0f, 0.0f);
	lights[1].color = Vector3f(0.0f, 1.0f, 0.0f); // Green
	lights[1].intensity = 1.0f;
	lights[1].enabled = true;

	// Light 3: Blue light from front
	lights[2].position = Vector3f(0.0f, 0.0f, 5.0f);
	lights[2].color = Vector3f(0.0f, 0.0f, 1.0f); // Blue
	lights[2].intensity = 1.0f;
	lights[2].enabled = true;

	CompileShaders();

	/* set to draw in window based on depth  */
	glEnable(GL_DEPTH_TEST);
}

float normalViewDistance = 8.0f;
static void onDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Create transformation matrices
	Matrix4f World;
	Matrix4f Scale;
	Matrix4f Rotation;
	Matrix4f Translation;
	Matrix4f View;
	Matrix4f Projection;
	Matrix4f Model;

	// Scale the model to fit in the viewport
	float scale = 1.0f / (model1->extent * 1.2f);
	Scale.InitScaleTransform(scale, scale, scale);

	// Rotation around Y axis
	Rotation.InitRotateTransform(0.0f, rotation, 0.0f);

	// Center the model
	float centerX = -0.5f * (model1->minX + model1->maxX);
	float centerY = -0.5f * (model1->minY + model1->maxY);
	float centerZ = -0.5f * (model1->minZ + model1->maxZ);
	Translation.InitTranslationTransform(centerX, centerY, centerZ);

	// Calculate model matrix
	Model = Rotation * Scale * Translation;

	

	// Choose view mode
	if (isDroneViewEnabled)
	{
		View = ComputeViewMatrix();
		Projection.InitPerspProjTransform(45.0f, (float)theWindowWidth / (float)theWindowHeight, 0.1f, 100.0f);
	}
	else
	{
		View.initIdentity();
		View.m[2][3] = -normalViewDistance;
		Projection.InitOrthoProjTransform(-1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 10.0f);
	}

	// Send camera position to shader for specular calculation
	glUniform3f(viewPosLocation, cameraX, cameraY, cameraZ);

	// Send light information to shader
	for (int i = 0; i < 3; i++)
	{
		glUniform3f(lightPosLocations[i], lights[i].position.x, lights[i].position.y, lights[i].position.z);
		glUniform3f(lightColorLocations[i], lights[i].color.x, lights[i].color.y, lights[i].color.z);
		glUniform1f(lightIntensityLocations[i], lights[i].intensity);
		glUniform1i(lightEnabledLocations[i], lights[i].enabled);
	}

	// Send lighting parameters to shader
	glUniform1i(numActiveLightsLocation, numActiveLights);
	glUniform1f(ambientStrengthLocation, ambientStrength);
	glUniform1f(specularStrengthLocation, specularStrength);
	glUniform1f(shininessLocation, shininess);
	glUniform1i(useBlinnPhongLocation, useBlinnPhong);
	glUniform1i(useDepthColorLocation, useDepthColor);
	glUniform1f(minDepthLocation, minDepth);
	glUniform1f(maxDepthLocation, maxDepth);

	// Send explosion parameters to shader
	glUniform1f(explosionFactorLocation, explosionFactor);
	glUniform1i(isExplodedLocation, isExploded ? 1 : 0);

	// Combined world transformation
	World = Projection * View;

	// Send matrices to shader
	glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &World.m[0][0]);
	glUniformMatrix4fv(gModelLocation, 1, GL_TRUE, &Model.m[0][0]);
	glUniformMatrix4fv(gViewLocation, 1, GL_TRUE, &View.m[0][0]);

	// Draw the model
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, model1->totalIndices, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	// Only change camera direction in drone view and when not in UI mode
	if (!isDroneViewEnabled || isUIMode)
	{
		return;
	}

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // Reversed: y ranges bottom to top
	lastX = xpos;
	lastY = ypos;

	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// Prevent looking too far up or down
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		// Left button pressed
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		// Left button released
	}
}

// Add before main function
void processInput(GLFWwindow *window)
{
	// Only process camera movement in drone view and when not in UI mode
	if (!isDroneViewEnabled || isUIMode)
	{
		return;
	}

	// Calculate camera direction vectors
	float dirX = cos(yaw * 3.14159f / 180.0f) * cos(pitch * 3.14159f / 180.0f);
	float dirY = sin(pitch * 3.14159f / 180.0f);
	float dirZ = sin(yaw * 3.14159f / 180.0f) * cos(pitch * 3.14159f / 180.0f);

	// Normalize the direction vector
	float length = sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
	dirX /= length;
	dirY /= length;
	dirZ /= length;

	// Calculate camera right vector (perpendicular to direction and world up)
	float rightX = dirZ; // Cross product simplification for world up = (0,1,0)
	float rightY = 0.0f;
	float rightZ = -dirX;

	// Move forward/backward
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		cameraX += dirX * cameraSpeed;
		cameraY += dirY * cameraSpeed;
		cameraZ += dirZ * cameraSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		cameraX -= dirX * cameraSpeed;
		cameraY -= dirY * cameraSpeed;
		cameraZ -= dirZ * cameraSpeed;
	}

	// Move left/right
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		cameraX -= rightX * cameraSpeed;
		cameraZ -= rightZ * cameraSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		cameraX += rightX * cameraSpeed;
		cameraZ += rightZ * cameraSpeed;
	}

	// Move up/down (world space)
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		cameraY += cameraSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		cameraY -= cameraSpeed;
	}
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) // Only react on key press
	{
		switch (key)
		{
		case GLFW_KEY_R:
			// Reset rotation
			rotation = 0;

			// Reset camera position if in drone view
			if (isDroneViewEnabled)
			{
				cameraX = 0.0f;
				cameraY = 0.0f;
				cameraZ = 3.0f;
				yaw = -90.0f;
				pitch = 0.0f;
			}
			break;

		case GLFW_KEY_TAB:
			// Toggle UI mode (for mouse control)
			isUIMode = !isUIMode;

			// Set cursor mode based on UI mode
			if (isUIMode || !isDroneViewEnabled)
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
			else
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				firstMouse = true; // Reset mouse tracking
			}
			break;

		case GLFW_KEY_Q:
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, true);
			break;
		}
	}
}

// Initialize ImGui
void InitImGui(GLFWwindow *window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}

// Render ImGui
void RenderImGui()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Create a control panel window
	ImGui::Begin("Control Panel");

	// View mode toggle button
	if (ImGui::Button(isDroneViewEnabled ? "Switch to Normal View" : "Switch to Drone View"))
	{
		isDroneViewEnabled = !isDroneViewEnabled;

		// Reset camera when switching to drone view
		if (isDroneViewEnabled)
		{
			cameraX = 0.0f;
			cameraY = 0.0f;
			cameraZ = 10.0f;
			yaw = -90.0f;
			pitch = 0.0f;

			// If not in UI mode, capture cursor for camera control
			if (!isUIMode)
			{
				glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				firstMouse = true;
			}
		}
		else
		{
			// Always show cursor in normal view
			glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	// Display current mode
	ImGui::Text("Current mode: %s", isDroneViewEnabled ? "Drone View" : "Normal View");

	// Show UI/camera mode status
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
					   "Press TAB to %s",
					   isUIMode ? "use camera controls" : "use UI controls");

	// Normal view controls
	if (!isDroneViewEnabled)
	{
		ImGui::SliderFloat("Rotation", &rotation, 0.0f, 360.0f);
		ImGui::Checkbox("Auto Rotate", &isAnimating);
		ImGui::SliderFloat("Camera Distance", &normalViewDistance, 3.0f, 20.0f);
	}

	// Drone view controls info
	if (isDroneViewEnabled)
	{
		ImGui::Separator();
		ImGui::Text("Drone View Controls:");
		ImGui::Text("W/A/S/D - Move camera");
		ImGui::Text("Mouse - Look around");
		ImGui::Text("Space/Shift - Move up/down");
		ImGui::Text("R - Reset position");
		ImGui::Text("TAB - Toggle UI/camera mode");
	}

	// Lighting Controls Section
	ImGui::Separator();
	ImGui::Text("Lighting Controls");

	// Shading model
	ImGui::Checkbox("Use Blinn-Phong", &useBlinnPhong);

	// Global lighting parameters
	ImGui::SliderFloat("Ambient", &ambientStrength, 0.0f, 0.5f);
	ImGui::SliderFloat("Specular Strength", &specularStrength, 0.0f, 1.0f);
	ImGui::SliderFloat("Shininess", &shininess, 1.0f, 128.0f);

	// Depth coloring
	ImGui::Checkbox("Color by Depth", &useDepthColor);
	if (useDepthColor)
	{
		ImGui::SliderFloat("Min Depth", &minDepth, 0.1f, maxDepth);
		ImGui::SliderFloat("Max Depth", &maxDepth, minDepth, 50.0f);
	}

	// Individual light controls
	for (int i = 0; i < 3; i++)
	{
		char header[32];
		sprintf(header, "Light %d", i + 1);

		if (ImGui::CollapsingHeader(header))
		{
			char enabledLabel[32], intensityLabel[32];
			sprintf(enabledLabel, "Enabled##%d", i);
			sprintf(intensityLabel, "Intensity##%d", i);

			ImGui::Checkbox(enabledLabel, &lights[i].enabled);

			ImGui::ColorEdit3("Color", &lights[i].color.x);
			ImGui::SliderFloat(intensityLabel, &lights[i].intensity, 0.0f, 2.0f);

			// Light position controls
			ImGui::Text("Position");
			char xLabel[32], yLabel[32], zLabel[32];
			sprintf(xLabel, "X##%d", i);
			sprintf(yLabel, "Y##%d", i);
			sprintf(zLabel, "Z##%d", i);

			ImGui::SliderFloat(xLabel, &lights[i].position.x, -10.0f, 10.0f);
			ImGui::SliderFloat(yLabel, &lights[i].position.y, -10.0f, 10.0f);
			ImGui::SliderFloat(zLabel, &lights[i].position.z, -10.0f, 10.0f);
		}
	}

	ImGui::Separator();
	ImGui::Text("Blow Up View");

	// Explosion toggle button
	if (!isExploding)
	{
		if (ImGui::Button(isExploded ? "Collapse Mesh" : "Explode Mesh"))
		{
			isExploding = true;
		}
	}

	// Show explosion progress
	if (isExploding)
	{
		ImGui::Text("Animating... %.1f%%", (isExploded ? (1.0f - explosionFactor / explosionMax) * 100.0f : (explosionFactor / explosionMax) * 100.0f));

		// Add cancel button
		if (ImGui::Button("Cancel Animation"))
		{
			isExploding = false;
		}
	}

	// When exploded and not animating, show manual control
	if (isExploded && !isExploding)
	{
		ImGui::SliderFloat("Explosion Factor", &explosionFactor, 0.0f, explosionMax);
	}

	// Explosion speed control
	ImGui::SliderFloat("Animation Speed", &explosionSpeed, 0.001f, 0.05f);

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Define main function
int main(int argc, char *argv[])
{
	// Initialize GLFW
	glfwInit();

	// Define version and compatibility settings
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create OpenGL window and context
	GLFWwindow *window = glfwCreateWindow(theWindowWidth, theWindowHeight, "OpenGL", NULL, NULL);
	glfwMakeContextCurrent(window);
	// Set up mouse callback
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Check for window creation failure
	if (!window)
	{
		// Terminate GLFW
		glfwTerminate();
		return 0;
	}

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	glewInit();
	printf("GL version: %s\n",
		   glGetString(GL_VERSION));
	onInit(argc, argv);

	// Initialize ImGui
	InitImGui(window);

	// Set GLFW callback functions
	glfwSetKeyCallback(window, key_callback);

	// Event loop
	while (!glfwWindowShouldClose(window))
	{

		processInput(window);

		// Adjust rotation speed (only in normal view)
		if (isAnimating)
		{
			rotation += 0.5f; // Smoother speed for rotation
			if (rotation > 360.0f)
			{
				rotation -= 360.0f;
			}
		}

		// Handle explosion animation
		if (isExploding) {
			if (isExploded) {
				// Collapsing the mesh
				explosionFactor -= explosionSpeed;
				if (explosionFactor <= 0.0f) {
					explosionFactor = 0.0f;
					isExploding = false;
					isExploded = false;
				}
			} else {
				// Exploding the mesh
				explosionFactor += explosionSpeed;
				if (explosionFactor >= explosionMax) {
					explosionFactor = explosionMax;
					isExploding = false;
					isExploded = true;
				}
			}
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		onDisplay();

		RenderImGui();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Terminate GLFW
	glfwTerminate();
	return 0;
}
