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
#include "line_rasterizer.h"
#include "polygon_filler.h"
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
GLuint gWorldLocation;

/* Constants */
const int ANIMATION_DELAY = 20; /* milliseconds between rendering */
const char *pVSFileName = "shaders/shader.vs";
const char *pFSFileName = "shaders/shader.fs";
const char *pGSFileName = "shaders/shader.gs";

const char *offFile1 = "models/1grm.off";
const char *offFile2 = "models/2oar.off";
const char *offFile3 = "models/3sy7.off";
const char *offFile4 = "models/4hhb.off";

OffModel *model1;
OffModel *model2;
OffModel *model3;
OffModel *model4;

GLuint gSlicePlaneLocation;     // Uniform for slice planes
GLuint gNumPlanesLocation;      // Uniform for number of active planes
GLuint gSeparatePartsLocation;  // Uniform for toggling separation of parts
GLuint gSeparationDistLocation; // Uniform for separation distance

// Slicing plane configuration
int numActivePlanes = 0;          // Number of active slicing planes
Vector4f slicePlanes[4];          // Storage for plane equations (ax + by + cz + d = 0)
bool showSlicingControls = false; // Toggle for UI panel
bool separateParts = false;       // Toggle for separating sliced parts
float separationDistance = 0.05f; // Distance to separate sliced parts

bool showLineRasterizer = false;
int lineX0 = 100, lineY0 = 100, lineX1 = 400, lineY1 = 300;
float lineColor[3] = {1.0f, 0.0f, 0.0f}; // Red by default
std::vector<PixelPoint> currentLinePixels;
bool lineNeedsUpdate = true;

GLuint ShaderProgram;

bool showPolygonFiller = false;
std::vector<ImVec2> polygonVertices;
std::vector<PixelPoint> filledPolygonPixels;
bool polygonNeedsUpdate = true;
ImVec4 polygonFillColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f); // Initial fill color (blue)

// Variables for manual point entry
int manualPointX = 200;
int manualPointY = 200;
bool pointAdded = false;
float pointAddedTimer = 0.0f;
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

static void CompileShaders()
{
    ShaderProgram = glCreateProgram();

    if (ShaderProgram == 0)
    {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

    string vs, fs, gs;

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
    gWorldLocation = glGetUniformLocation(ShaderProgram, "gWorld");
    gSlicePlaneLocation = glGetUniformLocation(ShaderProgram, "slicePlanes");
    gNumPlanesLocation = glGetUniformLocation(ShaderProgram, "numPlanes");
    gSeparatePartsLocation = glGetUniformLocation(ShaderProgram, "separateParts");
    gSeparationDistLocation = glGetUniformLocation(ShaderProgram, "separationDistance");
    // glBindVertexArray(0);
}

/********************************************************************
 Callback Functions
 */

void onInit(int argc, char *argv[])
{
    /* by default the back ground color is black */
    model1 = readOffFile(offFile1);

    computeNormals(model1);

    cout << model1->numberOfVertices << endl;
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Initialize slicing planes to default values
    numActivePlanes = 0;
    for (int i = 0; i < 4; i++)
    {
        slicePlanes[i] = Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // Create the vertex buffer for the model
    CreateVertexBuffer(model1);

    CompileShaders();

    /* set to draw in window based on depth  */
    glEnable(GL_DEPTH_TEST);

    // Initialize line rasterization values
    lineX0 = 100;
    lineY0 = 100;
    lineX1 = 400;
    lineY1 = 300;
    lineColor[0] = 1.0f; // Red
    lineColor[1] = 0.0f; // Green
    lineColor[2] = 0.0f; // Blue
    lineNeedsUpdate = true;
}

static void onDisplay()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Create transformation matrices
    Matrix4f World;
    Matrix4f Scale;
    Matrix4f Rotation;
    Matrix4f Translation;
    Matrix4f Projection;

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

    // Create orthographic projection
    Projection.InitOrthoProjTransform(-1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 10.0f);

    // Combine all transformations
    World = Projection * Rotation * Scale * Translation;

    // Send the transformation matrix to the shader
    glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &World.m[0][0]);

    // Send slicing planes data to shader
    glUniform1i(gNumPlanesLocation, numActivePlanes);
    // Send separation controls to shader
    glUniform1i(gSeparatePartsLocation, separateParts ? 1 : 0);
    glUniform1f(gSeparationDistLocation, separationDistance);
    if (numActivePlanes > 0)
    {
        glUniform4fv(gSlicePlaneLocation, numActivePlanes, &slicePlanes[0].x);
    }

    // Render the model
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, model1->totalIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Check for OpenGL errors
    GLenum errorCode = glGetError();
    if (errorCode != GL_NO_ERROR)
    {
        fprintf(stderr, "OpenGL rendering error %d\n", errorCode);
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    // std::cout << "Mouse position: " << xpos << ", " << ypos << std::endl;
    // Handle mouse movement
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

std::string getQuadrantDescription(int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0;
    int dy = y1 - y0;

    if (dx > 0 && dy > 0)
        return "1st quadrant (right & up)";
    if (dx < 0 && dy > 0)
        return "2nd quadrant (left & up)";
    if (dx < 0 && dy < 0)
        return "3rd quadrant (left & down)";
    if (dx > 0 && dy < 0)
        return "4th quadrant (right & down)";
    if (dx == 0 && dy != 0)
        return "Vertical line";
    if (dx != 0 && dy == 0)
        return "Horizontal line";
    return "Same point";
}

std::string getSlopeDescription(float slope)
{
    if (std::isinf(slope))
        return "Vertical (infinite)";
    if (slope == 0)
        return "Horizontal (zero)";
    if (slope > 0 && slope < 1)
        return "Gentle positive (0 < m < 1)";
    if (slope >= 1)
        return "Steep positive (m >= 1)";
    if (slope < 0 && slope > -1)
        return "Gentle negative (-1 < m < 0)";
    if (slope <= -1)
        return "Steep negative (m <= -1)";
    return "Unknown";
}

// Render ImGui
void RenderImGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create a window for model controls
    ImGui::Begin("Model Controls");

    // Add rotation controls if you want
    ImGui::SliderFloat("Rotation", &rotation, 0.0f, 360.0f);
    ImGui::Checkbox("Animate", &isAnimating);

    // Toggle for showing slicing controls
    ImGui::Checkbox("Show Slicing Controls", &showSlicingControls);

    if (numActivePlanes > 0) {
        ImGui::Separator();
        ImGui::Text("Slice Separation");
        ImGui::Checkbox("Separate Sliced Parts", &separateParts);
        if (separateParts) {
            ImGui::SliderFloat("Separation Distance", &separationDistance, 0.01f, 0.5f);
        }
    }

    // Slicing planes controls section
    if (showSlicingControls)
    {
        ImGui::Separator();
        ImGui::Text("Slicing Planes");

        // Number of active planes slider
        ImGui::SliderInt("Number of Planes", &numActivePlanes, 0, 4);

        // Only show controls for active planes
        for (int i = 0; i < numActivePlanes; i++)
        {
            ImGui::PushID(i); // Unique ID for each plane's controls

            ImGui::Text("Plane %d", i + 1);

            // Plane equation: ax + by + cz + d = 0
            ImGui::SliderFloat("A (x coefficient)", &slicePlanes[i].x, -1.0f, 1.0f);
            ImGui::SliderFloat("B (y coefficient)", &slicePlanes[i].y, -1.0f, 1.0f);
            ImGui::SliderFloat("C (z coefficient)", &slicePlanes[i].z, -1.0f, 1.0f);
            ImGui::SliderFloat("D (constant)", &slicePlanes[i].w, -1.0f, 1.0f);

            // Normalize button for the plane
            if (ImGui::Button("Normalize Plane"))
            {
                // Normalize the plane normal (a,b,c)
                float length = sqrt(slicePlanes[i].x * slicePlanes[i].x +
                                    slicePlanes[i].y * slicePlanes[i].y +
                                    slicePlanes[i].z * slicePlanes[i].z);
                if (length > 0.0001f)
                {
                    slicePlanes[i].x /= length;
                    slicePlanes[i].y /= length;
                    slicePlanes[i].z /= length;
                    slicePlanes[i].w /= length;
                }
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        // Reset all planes button
        if (ImGui::Button("Reset All Planes"))
        {
            numActivePlanes = 0;
            for (int i = 0; i < 4; i++)
            {
                slicePlanes[i] = Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
    }

    // Toggle for showing line rasterizer controls
    ImGui::Checkbox("Show Line Rasterizer", &showLineRasterizer);

    // Line rasterization controls section
    if (showLineRasterizer)
    {
        // Update line pixels if needed
        if (lineNeedsUpdate)
        {
            currentLinePixels = LineRasterizer::rasterizeLine(lineX0, lineY0, lineX1, lineY1);
            lineNeedsUpdate = false;
        }

        // Create a visualization window for the line
        ImGui::Begin("Line Visualization", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        // Calculate canvas size and position
        const float padding = 10.0f;
        const float canvasWidth = 500.0f;  // Set appropriate width
        const float canvasHeight = 400.0f; // Set appropriate height

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasEnd = ImVec2(canvasPos.x + canvasWidth, canvasPos.y + canvasHeight);

        // Display the canvas rectangle
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(canvasPos, canvasEnd, IM_COL32(50, 50, 50, 255));
        drawList->AddRect(canvasPos, canvasEnd, IM_COL32(255, 255, 255, 255));

        // Draw a grid (optional)
        const int gridSize = 20; // Size of each grid cell
        for (float x = canvasPos.x; x <= canvasEnd.x; x += gridSize)
        {
            drawList->AddLine(
                ImVec2(x, canvasPos.y), ImVec2(x, canvasEnd.y),
                IM_COL32(100, 100, 100, 40));
        }
        for (float y = canvasPos.y; y <= canvasEnd.y; y += gridSize)
        {
            drawList->AddLine(
                ImVec2(canvasPos.x, y), ImVec2(canvasEnd.x, y),
                IM_COL32(100, 100, 100, 40));
        }

        // Draw the original line
        ImU32 lineImGuiColor = IM_COL32(
            (int)(lineColor[0] * 255),
            (int)(lineColor[1] * 255),
            (int)(lineColor[2] * 255),
            150);

        // Scale the line to fit in the canvas
        float scaleX = canvasWidth / theWindowWidth;
        float scaleY = canvasHeight / theWindowHeight;

        ImVec2 p1 = ImVec2(canvasPos.x + lineX0 * scaleX, canvasPos.y + lineY0 * scaleY);
        ImVec2 p2 = ImVec2(canvasPos.x + lineX1 * scaleX, canvasPos.y + lineY1 * scaleY);
        drawList->AddLine(p1, p2, lineImGuiColor, 1.0f);

        // Draw start and end points with larger markers
        drawList->AddCircleFilled(p1, 4.0f, IM_COL32(255, 0, 0, 255));
        drawList->AddCircleFilled(p2, 4.0f, IM_COL32(0, 255, 0, 255));

        // Draw the Bresenham pixels
        ImU32 pixelColor = IM_COL32(
            (int)(lineColor[0] * 255),
            (int)(lineColor[1] * 255),
            (int)(lineColor[2] * 255),
            255);

        const float pixelSize = 5.0f; // Size of each rasterized pixel

        for (const auto &pixel : currentLinePixels)
        {
            float px = canvasPos.x + pixel.x * scaleX;
            float py = canvasPos.y + pixel.y * scaleY;

            // Draw filled square for each pixel
            drawList->AddRectFilled(
                ImVec2(px - pixelSize / 2, py - pixelSize / 2),
                ImVec2(px + pixelSize / 2, py + pixelSize / 2),
                pixelColor);
        }

        // Reserve space for the canvas
        ImGui::InvisibleButton("canvas", ImVec2(canvasWidth, canvasHeight));

        // Add line customization controls below the canvas
        ImGui::Separator();
        ImGui::Text("Line Customization");

        // Start point controls
        ImGui::PushID("StartPoint");
        ImGui::Text("Start Point (x0, y0):");
        bool startChanged = false;
        startChanged |= ImGui::SliderInt("X0", &lineX0, 0, theWindowWidth - 1);
        startChanged |= ImGui::SliderInt("Y0", &lineY0, 0, theWindowHeight - 1);
        ImGui::PopID();

        // End point controls
        ImGui::PushID("EndPoint");
        ImGui::Text("End Point (x1, y1):");
        bool endChanged = false;
        endChanged |= ImGui::SliderInt("X1", &lineX1, 0, theWindowWidth - 1);
        endChanged |= ImGui::SliderInt("Y1", &lineY1, 0, theWindowHeight - 1);
        ImGui::PopID();

        // Line color control with color picker
        ImGui::Text("Line Color:");
        bool colorChanged = ImGui::ColorEdit3("##LineColor", lineColor);

        // Line information display
        ImGui::Separator();
        ImGui::Text("Line Information:");

        // Calculate and display slope
        float dx = lineX1 - lineX0;
        float dy = lineY1 - lineY0;
        float slope = dx != 0 ? dy / dx : INFINITY;
        ImGui::Text("Slope: %.2f", slope);

        // Display line characteristics
        ImGui::Text("Direction: %s", getQuadrantDescription(lineX0, lineY0, lineX1, lineY1).c_str());
        ImGui::Text("Slope Type: %s", getSlopeDescription(slope).c_str());
        ImGui::Text("Line Length: %.2f pixels", sqrt(dx * dx + dy * dy));
        ImGui::Text("Pixel Count: %zu", currentLinePixels.size());

        // Update button
        if (startChanged || endChanged || colorChanged || ImGui::Button("Update Line"))
        {
            lineNeedsUpdate = true;
        }

        // Show pixel coordinates when hovering
        if (ImGui::IsItemHovered())
        {
            ImVec2 mousePos = ImGui::GetMousePos();
            int pixelX = (int)((mousePos.x - canvasPos.x) / scaleX);
            int pixelY = (int)((mousePos.y - canvasPos.y) / scaleY);

            ImGui::BeginTooltip();
            ImGui::Text("Pixel: (%d, %d)", pixelX, pixelY);

            // Check if this pixel is part of the rasterized line
            bool isOnLine = false;
            for (const auto &p : currentLinePixels)
            {
                if (p.x == pixelX && p.y == pixelY)
                {
                    isOnLine = true;
                    break;
                }
            }

            if (isOnLine)
            {
                ImGui::Text("On rasterized line");
            }
            ImGui::EndTooltip();
        }

        // Add this line to end the "Line Visualization" window
        ImGui::End();
    }

    // Add this after the line rasterizer checkbox in RenderImGui()
    ImGui::Checkbox("Show Polygon Filler", &showPolygonFiller);

    // Add this section after the line rasterizer controls in RenderImGui()
    if (showPolygonFiller)
    {
        // Create a visualization window for the polygon fill
        ImGui::Begin("Polygon Fill Visualization", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        // Calculate canvas size and position
        const float padding = 10.0f;
        const float canvasWidth = 500.0f;
        const float canvasHeight = 400.0f;

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasEnd = ImVec2(canvasPos.x + canvasWidth, canvasPos.y + canvasHeight);

        // Display the canvas rectangle
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(canvasPos, canvasEnd, IM_COL32(50, 50, 50, 255));
        drawList->AddRect(canvasPos, canvasEnd, IM_COL32(255, 255, 255, 255));

        // Draw grid (like in line rasterizer)
        const int gridSize = 20;
        for (float x = canvasPos.x; x <= canvasEnd.x; x += gridSize)
        {
            drawList->AddLine(
                ImVec2(x, canvasPos.y), ImVec2(x, canvasEnd.y),
                IM_COL32(100, 100, 100, 40));
        }
        for (float y = canvasPos.y; y <= canvasEnd.y; y += gridSize)
        {
            drawList->AddLine(
                ImVec2(canvasPos.x, y), ImVec2(canvasEnd.x, y),
                IM_COL32(100, 100, 100, 40));
        }

        // Scale factors for the canvas
        float scaleX = canvasWidth / theWindowWidth;
        float scaleY = canvasHeight / theWindowHeight;

        // Mouse position relative to canvas
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 relativeMousePos(
            (mousePos.x - canvasPos.x) / scaleX,
            (mousePos.y - canvasPos.y) / scaleY);

        // Handle mouse clicks to add polygon vertices
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            ImGui::IsWindowHovered() &&
            mousePos.x >= canvasPos.x && mousePos.x <= canvasEnd.x &&
            mousePos.y >= canvasPos.y && mousePos.y <= canvasEnd.y)
        {

            // Add vertex to the polygon
            ImVec2 newVertex(
                (mousePos.x - canvasPos.x) / scaleX,
                (mousePos.y - canvasPos.y) / scaleY);
            polygonVertices.push_back(newVertex);
            polygonNeedsUpdate = true;

            // Set timer for feedback message
            pointAdded = true;
            pointAddedTimer = 1.0f; // Show message for 1 second
        }

        // Update filled pixels if needed
        if (polygonNeedsUpdate && polygonVertices.size() >= 3)
        {
            filledPolygonPixels = PolygonFiller::fillPolygon(polygonVertices, theWindowWidth, theWindowHeight);
            polygonNeedsUpdate = false;
        }

        // Draw the polygon outline
        if (polygonVertices.size() > 0)
        {
            for (size_t i = 0; i < polygonVertices.size(); i++)
            {
                size_t next = (i + 1) % polygonVertices.size();
                ImVec2 p1(
                    canvasPos.x + polygonVertices[i].x * scaleX,
                    canvasPos.y + polygonVertices[i].y * scaleY);
                ImVec2 p2(
                    canvasPos.x + polygonVertices[next].x * scaleX,
                    canvasPos.y + polygonVertices[next].y * scaleY);
                drawList->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), 1.0f);

                // Draw vertex points
                drawList->AddCircleFilled(p1, 4.0f, IM_COL32(255, 255, 0, 255));
            }
        }

        // Draw the filled pixels
        ImU32 fillImGuiColor = IM_COL32(
            (int)(polygonFillColor.x * 255),
            (int)(polygonFillColor.y * 255),
            (int)(polygonFillColor.z * 255),
            (int)(polygonFillColor.w * 255));

        const float pixelSize = 5.0f; // Size of each filled pixel
        for (const auto &pixel : filledPolygonPixels)
        {
            float px = canvasPos.x + pixel.x * scaleX;
            float py = canvasPos.y + pixel.y * scaleY;

            drawList->AddRectFilled(
                ImVec2(px - pixelSize / 2, py - pixelSize / 2),
                ImVec2(px + pixelSize / 2, py + pixelSize / 2),
                fillImGuiColor);
        }

        // Reserve space for the canvas
        ImGui::InvisibleButton("polygon_canvas", ImVec2(canvasWidth, canvasHeight));

        // Controls below the canvas
        ImGui::Separator();
        ImGui::Text("Polygon Controls");

        // Manual point entry section
        ImGui::Text("Manual Point Entry:");
        ImGui::SetNextItemWidth(100);
        ImGui::InputInt("X", &manualPointX, 1, 10);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::InputInt("Y", &manualPointY, 1, 10);
        ImGui::SameLine();

        // Keep point coordinates within the window bounds
        manualPointX = std::max(0, std::min(manualPointX, theWindowWidth - 1));
        manualPointY = std::max(0, std::min(manualPointY, theWindowHeight - 1));

        if (ImGui::Button("Add Point"))
        {
            polygonVertices.push_back(ImVec2(manualPointX, manualPointY));
            polygonNeedsUpdate = true;
            pointAdded = true;
            pointAddedTimer = 1.0f;
        }

        // Show "Point Added" feedback if needed
        if (pointAdded)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, pointAddedTimer), "Point Added!");

            // Decrease timer
            pointAddedTimer -= ImGui::GetIO().DeltaTime;
            if (pointAddedTimer <= 0.0f)
            {
                pointAdded = false;
            }
        }

        // Fill color control
        ImGui::Text("Fill Color:");
        if (ImGui::ColorEdit4("##PolygonFillColor", (float *)&polygonFillColor))
        {
            polygonNeedsUpdate = true;
        }

        // Show the list of points with delete buttons
        if (polygonVertices.size() > 0)
        {
            ImGui::Separator();
            ImGui::Text("Current Vertices:");
            ImGui::BeginChild("PointsList", ImVec2(250, 150), true);

            for (int i = 0; i < polygonVertices.size(); i++)
            {
                ImGui::PushID(i);
                ImGui::Text("Point %d: (%.0f, %.0f)", i + 1, polygonVertices[i].x, polygonVertices[i].y);
                ImGui::SameLine();
                if (ImGui::Button("X"))
                {
                    polygonVertices.erase(polygonVertices.begin() + i);
                    polygonNeedsUpdate = true;
                }
                ImGui::PopID();
            }

            ImGui::EndChild();
        }

        // Controls for polygon manipulation
        if (ImGui::Button("Clear Polygon"))
        {
            polygonVertices.clear();
            filledPolygonPixels.clear();
        }

        // Show information
        ImGui::Separator();
        ImGui::Text("Polygon Information:");
        ImGui::Text("Vertices: %zu", polygonVertices.size());
        ImGui::Text("Filled Pixels: %zu", filledPolygonPixels.size());
        ImGui::Text("Click on canvas or use manual entry to add vertices");

        // Show current mouse position over canvas
        if (ImGui::IsWindowHovered() &&
            mousePos.x >= canvasPos.x && mousePos.x <= canvasEnd.x &&
            mousePos.y >= canvasPos.y && mousePos.y <= canvasEnd.y)
        {

            int canvasMouseX = (int)((mousePos.x - canvasPos.x) / scaleX);
            int canvasMouseY = (int)((mousePos.y - canvasPos.y) / scaleY);

            ImGui::Text("Mouse Position: (%d, %d)", canvasMouseX, canvasMouseY);
        }

        // End the polygon visualization window
        ImGui::End();
    }

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
        // Adjust rotation speed
        if (isAnimating)
        {
            rotation += 0.5f; // Smoother speed for rotation
            if (rotation > 360.0f)
            {
                rotation -= 360.0f;
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
