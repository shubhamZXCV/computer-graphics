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
#include "gameLogic.hpp"
#define GL_SILENCE_DEPRECATION

#include <vector>

/********************************************************************/
/*   Variables */

char theProgramTitle[] = "Sample";
int theWindowWidth = 700, theWindowHeight = 700;
int theWindowPositionX = 40, theWindowPositionY = 40;
bool isFullScreen = false;
bool isAnimating = true;
float rotation = 0.0f;
GLuint VBO, VAO;
GLuint colorVBO;
GLuint gWorldLocation;
const int SEGMENTS = 32; // Number of segments in each circle

/* Constants */
const int ANIMATION_DELAY = 20; /* milliseconds between rendering */
const char *pVSFileName = "shaders/shader.vs";
const char *pFSFileName = "shaders/shader.fs";
const float CIRCLE_RADIUS = 0.1f; // Adjust this value to change circle size
int UNDO_REDO_LEFT = 5;

// colors
Vector3f LIGHTBLUE(0.0f, 0.0f, 1.0f);	// Blue for marbles
Vector3f BLACK(0.0f, 0.0f, 0.0f);		// BROWN for empty
Vector3f GRAY(0.5f, 0.5f, 0.5f);		// Gray for invalid
Vector3f BLUE(0.0f, 0.0f, 0.5f);		// LIGHTBLUE for selected
Vector3f GREEN(0.0f, 1.0f, 0.0f);		// Green for valid move
Vector3f BROWN(0.468f, 0.191f, 0.132f); // BROWN background

MarbleSolitaireBoard board;

/********************************************************************
  Utility functions
 */

/* post: compute frames per second and display in window's title bar */
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

void generateBoardSquare(std::vector<Vector3f> &vertices, std::vector<Vector3f> &colors)
{
	// Use the same coordinates as your marble grid
	float left = -0.8f - CIRCLE_RADIUS;
	float right = 0.8f + CIRCLE_RADIUS;
	float top = 0.8f + CIRCLE_RADIUS;
	float bottom = -0.8 - CIRCLE_RADIUS;
	float zDepth = 0.1f; // Place the square behind the marbles

	// Square vertices (as two triangles)
	vertices.push_back(Vector3f(left, bottom, zDepth));	 // Bottom left
	vertices.push_back(Vector3f(right, bottom, zDepth)); // Bottom right
	vertices.push_back(Vector3f(left, top, zDepth));	 // Top left
	vertices.push_back(Vector3f(right, top, zDepth));	 // Top right

	// Add colors for each vertex
	for (int i = 0; i < 4; i++)
	{
		colors.push_back(BROWN);
	}
}

void generateCircleVertices(float centerX, float centerY, float radius, int segments,
							std::vector<Vector3f> &vertices, std::vector<Vector3f> &colors,
							const Vector3f &color)
{
	// Center vertex
	vertices.push_back(Vector3f(centerX, centerY, 0.0f));
	colors.push_back(color);

	// Generate circle vertices
	for (int i = 0; i <= segments; i++)
	{
		float angle = 2.0f * M_PI * i / segments;
		float x = centerX + radius * cosf(angle);
		float y = centerY + radius * sinf(angle);
		vertices.push_back(Vector3f(x, y, 0.0f));
		colors.push_back(color);
	}
}

static void CreateVertexBuffer()
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	std::vector<Vector3f> vertices;
	std::vector<Vector3f> colors;

	generateBoardSquare(vertices, colors);

	for (int i = 0; i < 7; i++)
	{
		for (int j = 0; j < 7; j++)
		{
			float x = -0.8f + (j * (1.6f / 6));
			float y = 0.8f - (i * (1.6f / 6));

			Vector3f color;
			if (board.getCellState(i, j) == MARBLE)
			{
				color = BLUE; // Blue for marbles
			}
			else if (board.getCellState(i, j) == EMPTY)
			{
				color = BROWN; // BROWN for empty
			}
			else
			{
				color = BROWN; // Gray for invalid
			}

			generateCircleVertices(x, y, CIRCLE_RADIUS, SEGMENTS, vertices, colors, color);
		}
	}

	// Create and bind vertex buffer
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vector3f),
				 vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Create and bind color buffer
	glGenBuffers(1, &colorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(Vector3f),
				 colors.data(), GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void updateCircleColor(int gridI, int gridJ, const Vector3f &color)
{
	int circleIndex = gridI * 7 + gridJ;
	int startVertex = 4 + circleIndex * (SEGMENTS + 2); // Offset by 4 for the square vertices
	int numVertices = SEGMENTS + 2;

	std::vector<Vector3f> newColors(numVertices, color);

	glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
	glBufferSubData(GL_ARRAY_BUFFER,
					startVertex * sizeof(Vector3f),
					numVertices * sizeof(Vector3f),
					newColors.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
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

static void CompileShaders()
{
	GLuint ShaderProgram = glCreateProgram();

	if (ShaderProgram == 0)
	{
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	string vs, fs;

	if (!ReadFile(pVSFileName, vs))
	{
		exit(1);
	}

	if (!ReadFile(pFSFileName, fs))
	{
		exit(1);
	}

	AddShader(ShaderProgram, vs.c_str(), GL_VERTEX_SHADER);
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
	gWorldLocation = glGetUniformLocation(ShaderProgram, "gWorld");
	// glBindVertexArray(0);
}

/********************************************************************
 Callback Functions
 */

void onInit(int argc, char *argv[])
{
	/* by default the back ground color is BROWN */
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	CreateVertexBuffer();
	CompileShaders();

	/* set to draw in window based on depth  */
	glEnable(GL_DEPTH_TEST);
}

static void onDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Matrix4f World;

	// Scale down the points to fit in the visible area
	float scale = 1.0f;
	World.m[0][0] = scale * cosf(rotation);
	World.m[0][1] = -scale * sinf(rotation);
	World.m[0][2] = 0.0f;
	World.m[0][3] = 0.0f;
	World.m[1][0] = scale * sinf(rotation);
	World.m[1][1] = scale * cosf(rotation);
	World.m[1][2] = 0.0f;
	World.m[1][3] = 0.0f;
	World.m[2][0] = 0.0f;
	World.m[2][1] = 0.0f;
	World.m[2][2] = 1.0f;
	World.m[2][3] = 0.0f;
	World.m[3][0] = 0.0f;
	World.m[3][1] = 0.0f;
	World.m[3][2] = 0.0f;
	World.m[3][3] = 1.0f;

	glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &World.m[0][0]);

	glBindVertexArray(VAO);
	// Draw the board square first (4 vertices as triangle strip)
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Draw all circles
	for (int i = 0; i < 49; i++)
	{
		int startIdx = 4 + i * (SEGMENTS + 2); // Offset by 4 for the square vertices
		glDrawArrays(GL_TRIANGLE_FAN, startIdx, SEGMENTS + 2);
	}
	glBindVertexArray(0);

	/* check for any errors when rendering */
	GLenum errorCode = glGetError();
	if (errorCode == GL_NO_ERROR)
	{
	}
	else
	{
		fprintf(stderr, "OpenGL rendering error %d\n", errorCode);
	}
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	std::cout << "Mouse position: " << xpos << ", " << ypos << std::endl;
	// Handle mouse movement
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)

{
	ImGuiIO &io = ImGui::GetIO();
	io.AddMouseButtonEvent(button, action == GLFW_PRESS);
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{

		// Check if ImGui is handling this click
		ImGuiIO &io = ImGui::GetIO();
		if (io.WantCaptureMouse)
		{
			cout << "imgui button was clicked" << endl;
			return; // Exit if ImGui is handling the click
		}

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		// Convert pixel coordinates to OpenGL coordinates
		int width, height;
		glfwGetWindowSize(window, &width, &height);

		// Convert window coordinates to grid indices
		int i = (int)((ypos - 180) / 50);
		int j = (int)((xpos - 180) / 50);

		cout << "you clicked on " << i << " " << j << endl;
		CellState state = board.getCellState(i, j);
		if (state == EMPTY)
		{
			cout << "cell is empty" << endl;
		}
		else if (state == MARBLE)
		{
			cout << "cell has marble" << endl;
		}
		else
		{
			cout << "cell is invalid" << endl;
			return;
		}

		if (board.hasSelection())
		{
			if (i == board.getSelectedRow() && j == board.getSelectedCol())
			{
				board.clearSelection();
				cout << "you unselected cell: " << i << " " << j << endl;

				updateCircleColor(i, j, BLUE);

				return;
			}
			int fromRow = board.getSelectedRow();
			int fromCol = board.getSelectedCol();
			int toRow = i;
			int toCol = j;

			bool isValid = board.makeMove(fromRow, fromCol, toRow, toCol);
			if (isValid)
			{

				updateCircleColor((board.getSelectedRow() + i) / 2, (board.getSelectedCol() + j) / 2, BROWN);

				updateCircleColor(i, j, BLUE);

				updateCircleColor(board.getSelectedRow(), board.getSelectedCol(), BROWN);

				board.clearSelection();
			}
			else
			{
				cout << "Invalid move" << endl;

				updateCircleColor(board.getSelectedRow(), board.getSelectedCol(), BLUE);

				board.clearSelection();
			}
		}
		else
		{

			if (state == EMPTY)
			{
				return;
			}

			updateCircleColor(i, j, LIGHTBLUE);

			board.selectCell(i, j);
		}

		// Left button pressed
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		// Left button released
	}
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) // Only react on key press
	{
		switch (key)
		{
		case GLFW_KEY_R:
			rotation = 0;
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

void updateBoardVisuals()
{
	for (int i = 0; i < 7; i++)
	{
		for (int j = 0; j < 7; j++)
		{
			CellState state = board.getCellState(i, j);
			Vector3f color;
			if (state == MARBLE)
			{
				color = BLUE; // Blue for marbles
			}
			else if (state == EMPTY)
			{
				color = BROWN; // BROWN for empty
			}
			else
			{
				color = BROWN; // Gray for invalid
			}
			updateCircleColor(i, j, color);
		}
	}
}

void RenderImGui()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Get window size for positioning
	int width, height;
	glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

	// Top-left window
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(200, 100));
	ImGui::Begin("Game Controls"); // Start window

	// Add buttons inside the window
	if (ImGui::Button("Reset Game"))
	{
		board.reset();
		UNDO_REDO_LEFT = 5;
		updateBoardVisuals();
	}
	if (ImGui::Button("Undo"))
	{

		if (UNDO_REDO_LEFT >0)
		{
			

			if (board.undoMove())
			{
				UNDO_REDO_LEFT = UNDO_REDO_LEFT - 1;
				std::cout << "Undo successful. Remaining marbles: " << board.getRemainingMarbles() << std::endl;
				updateBoardVisuals();
			}
			else
			{
				std::cout << "No moves to undo!" << std::endl;
			}
		}
		else
		{
			std::cout << "No moves to undo!" << std::endl;
		}
	}

	if (ImGui::Button("redo"))
	{
		if (UNDO_REDO_LEFT > 0)
		{

			if (board.redoMove())
			{
				UNDO_REDO_LEFT--;
				updateBoardVisuals();
			}
		}
	}

	ImGui::End(); // End left window

	// Top-right window
	ImGui::SetNextWindowPos(ImVec2(width - 200, 0));
	ImGui::SetNextWindowSize(ImVec2(200, 100));
	ImGui::Begin("Game Stats"); // Start window

	ImGui::Text("Marbles: %d", board.getRemainingMarbles());
	ImGui::Text("Elapsed Time: %ld", board.getElapsedTime());
	ImGui::Text("UNDO/REDO LEFT: %d", UNDO_REDO_LEFT);

	ImGui::End(); // End right window

	ImGui::SetNextWindowPos(ImVec2(0, height - 100));
	ImGui::SetNextWindowSize(ImVec2(200, 100));
	ImGui::Begin("Game Results"); // Start window

	if (board.isGameOver())
	{
		ImGui::Text("Game Over!");
	}

	if (board.getRemainingMarbles() == 1 && board.getCellState(3, 3) == MARBLE)
	{
		ImGui::Text("You won!");
	}

	ImGui::End(); // End right window

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
	// glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// Event loop
	while (!glfwWindowShouldClose(window))
	{
		// Clear the screen to BROWN
		// rotation += 0.01;
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		onDisplay();

		RenderImGui();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Terminate GLFW
	glfwTerminate();
	return 0;
}
