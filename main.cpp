#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <ft2build.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "spline.h"

#include FT_FREETYPE_H

FT_Library ft;
FT_Face face;

void
initFreeType ()
{
  if (FT_Init_FreeType (&ft))
    {
      std::cerr << "Error initializing FreeType" << std::endl;
      exit (EXIT_FAILURE);
    }

  if (FT_New_Face (ft, "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf", 0, &face))
    {
      std::cerr << "Error loading font" << std::endl;
      exit (EXIT_FAILURE);
    }

  FT_Set_Pixel_Sizes (face, 0, 48);
}

void
render_text (const char *text, float x, float y, float sx, float sy)
{
  const char *p;

  for (p = text; *p; p++)
    {
      if (FT_Load_Char (face, *p, FT_LOAD_RENDER))
        continue;

      FT_GlyphSlot g = face->glyph;

      glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, g->bitmap.width, g->bitmap.rows,
                    0, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

      float x2 = x + g->bitmap_left * sx;
      float y2 = -y - g->bitmap_top * sy;
      float w = g->bitmap.width * sx;
      float h = g->bitmap.rows * sy;

      GLfloat box[4][4] = {
        { x2, -y2, 0, 0 },
        { x2 + w, -y2, 1, 0 },
        { x2, -y2 - h, 0, 1 },
        { x2 + w, -y2 - h, 1, 1 },
      };

      glBufferData (GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
      glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

      x += (g->advance.x / 64) * sx;
      y += (g->advance.y / 64) * sy;
    }
}

// Vertex Shader source code
const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "uniform float size;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(size * aPos.x, size * "
                                 "aPos.y * -1, size * aPos.z, 1.0);\n"
                                 "}\0";
// Fragment Shader source code
const char *fragmentShaderSource = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "uniform vec4 color;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = color;\n"
                                   "}\n\0";

// Second Vertex Shader source code
const char *sosVertexShaderSource
    = "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = vec4(aPos.y, aPos.x * -1, aPos.z, 1.0);\n"
      "}\0";

// Second Fragment Shader source code
const char *sosFragmentShaderSource
    = "#version 330 core\n"
      "out vec4 FragColor;\n"
      "void main()\n"
      "{\n"
      "   FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
      "}\n\0";

// Frame Vertex Shader source code
const char *frameVertexShaderSource
    = "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
      "}\0";

// Second Fragment Shader source code
const char *frameFragmentShaderSource
    = "#version 330 core\n"
      "out vec4 FragColor;\n"
      "void main()\n"
      "{\n"
      "   FragColor = vec4(0.9, 0.9, 0.9, 1.0);\n"
      "}\n\0";

// Frame Vertex Shader source code
const char *floorVertexShaderSource
    = "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = vec4(aPos.x, aPos.y * -1, aPos.z, 1.0);\n"
      "}\0";

// Second Fragment Shader source code
const char *floorFragmentShaderSource
    = "#version 330 core\n"
      "out vec4 FragColor;\n"
      "void main()\n"
      "{\n"
      "   FragColor = vec4(1.0, 1.0, 0.0, 1.0);\n"
      "}\n\0";

GLuint shaderProgram, VAO, VBO;
GLuint sosShaderProgram, sosVAO, sosVBO;
GLuint frameShaderProgram, frameVAO, frameVBO;
GLuint floorShaderProgram, floorVAO, floorVBO;

struct Data
{
  int vertices;
  int top_bounce;
  int bottom_bounce;
  double angle_of_entry;
  std::vector<double> x;
  std::vector<double> y;
};

void
cubicSpline (const std::vector<double> &x, const std::vector<double> &y,
             int resolution, std::vector<double> &x_interp,
             std::vector<double> &y_interp)
{
  tk::spline s (x, y);
  for (int i = 0; i < resolution; i++)
    {
      x_interp.push_back (x[0] + i * (x[x.size () - 1] - x[0]) / resolution);
      y_interp.push_back (s (x_interp[i]));
    }
}

int
countItems (std::istringstream &iss)
{
  int count = 0;
  std::string item;

  // Count items
  while (iss >> item)
    {
      count++;
    }

  // Clear the stream and restore the position
  iss.clear ();
  iss.seekg (0);

  return count;
}

std::vector<Data>
readDataFromFile (const std::string &filename)
{
  std::vector<Data> dataVector;
  std::ifstream file (filename);

  if (!file.is_open ())
    {
      std::cerr << "Error opening file: " << filename << std::endl;
      return dataVector;
    }

  std::string line;
  Data currentData;
  bool firstPass = true;

  while (std::getline (file, line))
    {
      std::istringstream iss (line);

      if (countItems (iss) == 1)
        {
          // Singular value on its own line, create a new Data struct
          if (!firstPass)
            {
              // Push the previous Data struct into the vector
              dataVector.push_back (currentData);
            }
          firstPass = false;

          // Reset the currentData struct for the new block
          currentData = Data ();
          iss >> currentData.angle_of_entry;
        }
      else if (countItems (iss) == 3)
        {
          // Three values on the line (vertices, top_bounce, bottom_bounce)
          iss >> currentData.vertices >> currentData.top_bounce
              >> currentData.bottom_bounce;
        }
      else if (countItems (iss) == 2)
        {
          // Two values on the line, x and y coordinates
          double x, y;
          iss >> x >> y;
          currentData.x.push_back (x);
          currentData.y.push_back (y);
        }
    }

  // Push the last Data struct into the vector
  dataVector.push_back (currentData);

  return dataVector;
}

void insertOrUpdatePoint(std::vector<double>& xCoords, std::vector<double>& yCoords, double newX, double newY) {
    auto it = std::lower_bound(xCoords.begin(), xCoords.end(), newX);

    // Calculate the index where the new x should be inserted or updated
    size_t index = std::distance(xCoords.begin(), it);

    // Check if the x-coordinate already exists
    if (it != xCoords.end() && *it == newX) {
        // If it exists, overwrite the corresponding y-coordinate
        yCoords[index] = newY;
    } else {
        // If it doesn't exist, insert the new x and y coordinates at the calculated index
        xCoords.insert(it, newX);
        yCoords.insert(yCoords.begin() + index, newY);
    }
}

void
initRayDraw ()
{
  // Create Second Vertex Shader Object and get its reference
  GLuint vertexShader = glCreateShader (GL_VERTEX_SHADER);
  glShaderSource (vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader (vertexShader);

  // Create Second Fragment Shader Object and get its reference
  GLuint fragmentShader = glCreateShader (GL_FRAGMENT_SHADER);
  glShaderSource (fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader (fragmentShader);

  // Create Second Shader Program Object and get its reference
  shaderProgram = glCreateProgram ();
  glAttachShader (shaderProgram, vertexShader);
  glAttachShader (shaderProgram, fragmentShader);
  glLinkProgram (shaderProgram);

  // Delete the now useless Second Vertex and Fragment Shader objects
  glDeleteShader (vertexShader);
  glDeleteShader (fragmentShader);

  // Create Second Vertex Array Object and Vertex Buffer Object
  glGenVertexArrays (1, &VAO);
  glGenBuffers (1, &VBO);

  // Bind the Second VAO and VBO
  glBindVertexArray (VAO);
  glBindBuffer (GL_ARRAY_BUFFER, VBO);

  // Configure the Second Vertex Attribute
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float),
                         (void *)0);
  glEnableVertexAttribArray (0);

  // Unbind the Second VAO and VBO
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindVertexArray (0);
}

void
initSOSDraw ()
{
  // Create Second Vertex Shader Object and get its reference
  GLuint sosVertexShader = glCreateShader (GL_VERTEX_SHADER);
  glShaderSource (sosVertexShader, 1, &sosVertexShaderSource, NULL);
  glCompileShader (sosVertexShader);

  // Create Second Fragment Shader Object and get its reference
  GLuint sosFragmentShader = glCreateShader (GL_FRAGMENT_SHADER);
  glShaderSource (sosFragmentShader, 1, &sosFragmentShaderSource, NULL);
  glCompileShader (sosFragmentShader);

  // Create Second Shader Program Object and get its reference
  sosShaderProgram = glCreateProgram ();
  glAttachShader (sosShaderProgram, sosVertexShader);
  glAttachShader (sosShaderProgram, sosFragmentShader);
  glLinkProgram (sosShaderProgram);

  // Delete the now useless Second Vertex and Fragment Shader objects
  glDeleteShader (sosVertexShader);
  glDeleteShader (sosFragmentShader);

  // Create Second Vertex Array Object and Vertex Buffer Object
  glGenVertexArrays (1, &sosVAO);
  glGenBuffers (1, &sosVBO);

  // Bind the Second VAO and VBO
  glBindVertexArray (sosVAO);
  glBindBuffer (GL_ARRAY_BUFFER, sosVBO);

  // Configure the Second Vertex Attribute
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float),
                         (void *)0);
  glEnableVertexAttribArray (0);

  // Unbind the Second VAO and VBO
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindVertexArray (0);
}

void
initFrameDraw ()
{
  // Create Second Vertex Shader Object and get its reference
  GLuint frameVertexShader = glCreateShader (GL_VERTEX_SHADER);
  glShaderSource (frameVertexShader, 1, &frameVertexShaderSource, NULL);
  glCompileShader (frameVertexShader);

  // Create Second Fragment Shader Object and get its reference
  GLuint frameFragmentShader = glCreateShader (GL_FRAGMENT_SHADER);
  glShaderSource (frameFragmentShader, 1, &frameFragmentShaderSource, NULL);
  glCompileShader (frameFragmentShader);

  // Create Second Shader Program Object and get its reference
  frameShaderProgram = glCreateProgram ();
  glAttachShader (frameShaderProgram, frameVertexShader);
  glAttachShader (frameShaderProgram, frameFragmentShader);
  glLinkProgram (frameShaderProgram);

  // Delete the now useless Second Vertex and Fragment Shader objects
  glDeleteShader (frameVertexShader);
  glDeleteShader (frameFragmentShader);

  // Create Second Vertex Array Object and Vertex Buffer Object
  glGenVertexArrays (1, &frameVAO);
  glGenBuffers (1, &frameVBO);

  // Bind the Second VAO and VBO
  glBindVertexArray (frameVAO);
  glBindBuffer (GL_ARRAY_BUFFER, frameVBO);

  // Configure the Second Vertex Attribute
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float),
                         (void *)0);
  glEnableVertexAttribArray (0);

  // Unbind the Second VAO and VBO
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindVertexArray (0);
}

void
initFloorDraw ()
{
  // Create Second Vertex Shader Object and get its reference
  GLuint floorVertexShader = glCreateShader (GL_VERTEX_SHADER);
  glShaderSource (floorVertexShader, 1, &floorVertexShaderSource, NULL);
  glCompileShader (floorVertexShader);

  // Create Second Fragment Shader Object and get its reference
  GLuint floorFragmentShader = glCreateShader (GL_FRAGMENT_SHADER);
  glShaderSource (floorFragmentShader, 1, &floorFragmentShaderSource, NULL);
  glCompileShader (floorFragmentShader);

  // Create Second Shader Program Object and get its reference
  floorShaderProgram = glCreateProgram ();
  glAttachShader (floorShaderProgram, floorVertexShader);
  glAttachShader (floorShaderProgram, floorFragmentShader);
  glLinkProgram (floorShaderProgram);

  // Delete the now useless Second Vertex and Fragment Shader objects
  glDeleteShader (floorVertexShader);
  glDeleteShader (floorFragmentShader);

  // Create Second Vertex Array Object and Vertex Buffer Object
  glGenVertexArrays (1, &floorVAO);
  glGenBuffers (1, &floorVBO);

  // Bind the Second VAO and VBO
  glBindVertexArray (floorVAO);
  glBindBuffer (GL_ARRAY_BUFFER, floorVBO);

  // Configure the Second Vertex Attribute
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float),
                         (void *)0);
  glEnableVertexAttribArray (0);

  // Unbind the Second VAO and VBO
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindVertexArray (0);
}

void
drawFrame (std::vector<GLfloat> frameVertices)
{
  glUseProgram (frameShaderProgram);
  glBindVertexArray (frameVAO);
  glBindBuffer (GL_ARRAY_BUFFER, frameVBO);
  glBufferData (GL_ARRAY_BUFFER, frameVertices.size () * sizeof (GLfloat),
                frameVertices.data (), GL_STATIC_DRAW);
  glDrawArrays (GL_LINE_LOOP, 0, frameVertices.size () / 3);
}

void
drawFloor (std::vector<GLfloat> floorVertices)
{
  glUseProgram (floorShaderProgram);
  glBindVertexArray (floorVAO);
  glBindBuffer (GL_ARRAY_BUFFER, floorVBO);
  glBufferData (GL_ARRAY_BUFFER, floorVertices.size () * sizeof (GLfloat),
                floorVertices.data (), GL_STATIC_DRAW);
  glDrawArrays (GL_LINE_STRIP, 0, floorVertices.size () / 3);
}

void
drawRay (int ray, std::vector<Data> dataVector, std::vector<GLfloat> vertices)
{
  glUseProgram (shaderProgram);
  glBindVertexArray (VAO);
  glBindBuffer (GL_ARRAY_BUFFER, VBO);
  glBufferData (GL_ARRAY_BUFFER, vertices.size () * sizeof (GLfloat),
                vertices.data (), GL_STATIC_DRAW);
  glDrawArrays (GL_LINE_STRIP, 0, dataVector[ray].vertices);
}

void
drawSOS (std::vector<GLfloat> sosVertices)
{
  // Render the second OpenGL viewport
  glUseProgram (sosShaderProgram);
  glBindVertexArray (sosVAO);
  glBindBuffer (GL_ARRAY_BUFFER, sosVBO);
  glBufferData (GL_ARRAY_BUFFER, sosVertices.size () * sizeof (GLfloat),
                sosVertices.data (), GL_STATIC_DRAW);
  glDrawArrays (GL_LINE_STRIP, 0, sosVertices.size () / 3);
}

void
deleteRayDraw ()
{
  glDeleteVertexArrays (1, &VAO);
  glDeleteBuffers (1, &VBO);
  glDeleteProgram (shaderProgram);
}

void
deleteSOSDraw ()
{
  glDeleteVertexArrays (1, &sosVAO);
  glDeleteBuffers (1, &sosVBO);
  glDeleteProgram (sosShaderProgram);
}

void
deleteFrameDraw ()
{
  glDeleteVertexArrays (1, &frameVAO);
  glDeleteBuffers (1, &frameVBO);
  glDeleteProgram (frameShaderProgram);
}

void
deleteFloorDraw ()
{
  glDeleteVertexArrays (1, &floorVAO);
  glDeleteBuffers (1, &floorVBO);
  glDeleteProgram (floorShaderProgram);
}

int
main ()
{
  // Rays
  std::vector<Data> dataVector = readDataFromFile ("test_floor_E.ray");

  // Floor shape
  std::vector<double> floorVectorX = { 0, 300, 1000 };
  std::vector<double> floorVectorY = { 30, 20, 25 };

  // Speed of sound spline
  std::vector<double> sosVectorX = { 0, 10, 20, 25, 30 };
  std::vector<double> sosVectorY = { 1540, 1530, 1532, 1533, 1535 };

  // Bezier curve
  std::vector<double> xSOSCurve;
  std::vector<double> ySOSCurve;

  std::cout << dataVector.size () << std::endl;

  std::cout << "VERT:\t" << dataVector[0].vertices << std::endl;
  std::cout << "TOP:\t" << dataVector[0].top_bounce << std::endl;
  std::cout << "BOT:\t" << dataVector[0].bottom_bounce << std::endl;
  std::cout << "X Sz:\t" << dataVector[0].x.size () << std::endl;
  std::cout << "Y Sz:\t" << dataVector[0].y.size () << std::endl;

  // Initialize GLFW
  glfwInit ();
  glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create Window
  GLFWwindow *window
      = glfwCreateWindow (1880, 800, "Bellhop OpenGL", NULL, NULL);
  if (window == NULL)
    {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate ();
      return -1;
    }
  glfwMakeContextCurrent (window);

  // Load GLAD so it configures OpenGL
  gladLoadGL ();

  // Generate sine wave vertices
  std::vector<GLfloat> vertices;
  std::vector<GLfloat> sosVertices;
  std::vector<GLfloat> frameVertices = {
    .85, .85, 0.0, .85, -.85, 0.0, -.85, -.85, 0.0, -.85, .85, 0.0,
  };
  std::vector<GLfloat> floorVertices;

  // Initialize ImGUI
  IMGUI_CHECKVERSION ();
  ImGui::CreateContext ();
  ImGuiIO &io = ImGui::GetIO ();
  (void)io;
  ImGui::StyleColorsDark ();
  ImGui_ImplGlfw_InitForOpenGL (window, true);
  ImGui_ImplOpenGL3_Init ("#version 330");

  // Variables to be changed in the ImGUI window
  bool drawTriangle = true;
  float size = 1.0f;
  float color[4] = { 0.8f, 0.3f, 0.02f, 1.0f };
  // Variables to store the selected radio button
  int selectedRadioButton = 0;

  // Timing variables
  double lastTime = glfwGetTime ();
  int frameCount = 0;
  int lastFrameCount = 0;

  // Start TX Pos
  int txStartPosX = 0;
  int txStartPosY = 0;

  // Start RX Pos
  int rxStartPosX = 0;
  int rxStartPosY = 0;

  // Tethered Bools
  bool tetherX = false;
  bool tetherY = false;

  // Sim vals
  bool showPlayback = true;
  bool simPlaying = true;
  bool drawingRays = true;

  bool splineDrawn = false;

  double addSosX = 0;
  double addSosY = 0;

  const char *vendor
      = reinterpret_cast<const char *> (glGetString (GL_VENDOR));
  const char *renderer
      = reinterpret_cast<const char *> (glGetString (GL_RENDERER));
  std::cout << "OpenGL Vendor: " << vendor << std::endl;
  std::cout << "OpenGL Renderer: " << renderer << std::endl;

  initFreeType ();

  int ray = 0;

  bool enableVSync = true;

  initRayDraw ();
  initSOSDraw ();
  initFrameDraw ();
  initFloorDraw ();

  // Find set min and max
  double minX = 10000;
  double maxX = 0;
  double minY = 10000;
  double maxY = 0;
  for (int i = 0; i < dataVector.size (); i++)
    {
      double temp_minX = *std::min_element (dataVector[i].x.begin (),
                                            dataVector[i].x.end ());
      double temp_maxX = *std::max_element (dataVector[i].x.begin (),
                                            dataVector[i].x.end ());
      double temp_minY = *std::min_element (dataVector[i].y.begin (),
                                            dataVector[i].y.end ());
      double temp_maxY = *std::max_element (dataVector[i].y.begin (),
                                            dataVector[i].y.end ());

      if (temp_minX < minX)
        minX = temp_minX;
      if (temp_maxX > maxX)
        maxX = temp_maxX;
      if (temp_minY < minY)
        minY = temp_minY;
      if (temp_maxY > maxY)
        maxY = temp_maxY;
    }

  // Specify the color of the background
  glClearColor (0.07f, 0.13f, 0.17f, 1.0f);
  // Clean the back buffer and assign the new color to it
  glClear (GL_COLOR_BUFFER_BIT);

  // Main while loop
  while (!glfwWindowShouldClose (window))
    {

      if (showPlayback)
        glfwSwapInterval (enableVSync);
      else
        glfwSwapInterval (0);

      glViewport (0, 0, 950, 800);

      // Tell OpenGL a new frame is about to begin
      ImGui_ImplOpenGL3_NewFrame ();
      ImGui_ImplGlfw_NewFrame ();
      ImGui::NewFrame ();

      if (simPlaying)
        ray++;
      ray %= dataVector.size ();

      // Measure speed
      double currentTime = glfwGetTime ();
      frameCount++;
      if (currentTime - lastTime >= 1.0)
        { // If last prinf() was more than 1 sec ago
          lastFrameCount = frameCount;
          frameCount = 0;
          lastTime = currentTime;
        }

      vertices.clear ();
      for (int i = 0; i < dataVector[ray].vertices; i++)
        {

          float x = (((dataVector[ray].x[i] - minX) / (maxX - minX)) * 1.7f)
                    - 0.85f;
          float y = (((dataVector[ray].y[i] - minY) / (maxY - minY)) * 1.7f)
                    - 0.85f;

          vertices.push_back (x);
          vertices.push_back (y);
          vertices.push_back (0.0f);
        }

      if (!splineDrawn)
        {
          std::cout << "Spline" << std::endl;
          // Print out x and y sline vals side by side
          for (int i = 0; i < sosVectorX.size (); i++)
            {
              std::cout << sosVectorX[i] << "\t" << sosVectorY[i] << std::endl;
            }

          sosVertices.clear ();
          xSOSCurve.clear ();
          ySOSCurve.clear ();
          cubicSpline (sosVectorX, sosVectorY, 100, xSOSCurve, ySOSCurve);

          double sosMinX
              = *std::min_element (xSOSCurve.begin (), xSOSCurve.end ());
          double sosMaxX
              = *std::max_element (xSOSCurve.begin (), xSOSCurve.end ());
          double sosMinY
              = *std::min_element (ySOSCurve.begin (), ySOSCurve.end ());
          double sosMaxY
              = *std::max_element (ySOSCurve.begin (), ySOSCurve.end ());
          for (int i = 0; i < xSOSCurve.size (); i++)
            {

              float x
                  = (((xSOSCurve[i] - sosMinX) / (sosMaxX - sosMinX)) * 1.7f)
                    - .85f;
              float y
                  = (((ySOSCurve[i] - sosMinY) / (sosMaxY - sosMinY)) * 1.7f)
                    - .85f;

              sosVertices.push_back (x);
              sosVertices.push_back (y);
              sosVertices.push_back (0.0f);
            }
          splineDrawn = true;
        }

      floorVertices.clear ();
      for (int i = 0; i < floorVectorX.size (); i++)
        {

          float x
              = (((floorVectorX[i] - minX) / (maxX - minX)) * 1.7f) - 0.85f;
          float y
              = (((floorVectorY[i] - minY) / (maxY - minY)) * 1.7f) - 0.85f;

          floorVertices.push_back (x);
          floorVertices.push_back (y);
          floorVertices.push_back (0.0f);
        }

      if (showPlayback)
        {
          // Clear screen
          glClearColor (0.07f, 0.13f, 0.17f, 1.0f);
          glClear (GL_COLOR_BUFFER_BIT);

          drawFrame (frameVertices);
          drawFloor (floorVertices);
          drawRay (ray, dataVector, vertices);
        }
      else
        {
          if (drawingRays)
            {
              int numOfDrawnRays = 0;

              if (numOfDrawnRays == 0)
                {
                  drawFrame (frameVertices);
                  drawFloor (floorVertices);
                }

              drawRay (ray, dataVector, vertices);
              numOfDrawnRays++;

              if (numOfDrawnRays == dataVector.size () - 1)
                {
                  drawingRays = false;
                  numOfDrawnRays = 0;
                }
            }
        }

      if (tetherX)
        txStartPosX = rxStartPosX;
      if (tetherY)
        txStartPosY = rxStartPosY;

      render_text ("Hello", 0.0f, 0.0f, 1.0f, 1.0f);

      ImGui::SetNextWindowPos (ImVec2 (io.DisplaySize.x - 280, 0));
      ImGui::SetNextWindowSize (ImVec2 (280, io.DisplaySize.y));
      // ImGUI window creation
      ImGui::Begin ("Acoustic Ray Casting Parameters", NULL,
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
                        | ImGuiWindowFlags_NoCollapse
                        | ImGuiWindowFlags_AlwaysAutoResize);

      // Main content
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));
      ImGui::Text ("Select a Computation Method:");
      ImGui::Spacing ();
      // First radio button
      ImGui::RadioButton ("##C++", &selectedRadioButton, 0);
      ImGui::SameLine ();
      ImGui::Text ("C++");
      ImGui::SameLine ();
      // Second radio button
      ImGui::RadioButton ("##CUDA", &selectedRadioButton, 1);
      ImGui::SameLine ();
      ImGui::Text ("CUDA");
      ImGui::SameLine ();
      // Third radio button
      ImGui::RadioButton ("##OptiX", &selectedRadioButton, 2);
      ImGui::SameLine ();
      ImGui::Text ("OptiX");
      // Display the selected mode of computeMode
      const char *computeMode[] = { "C++", "CUDA", "OptiX" };
      // std::cout << "Selected Compute Mode: " <<
      // computeMode[selectedRadioButton] << std::endl;

      // New section
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));
      ImGui::Separator ();
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      ImGui::Text ("Adjust Endpoints:");
      ImGui::Spacing ();
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      ImGui::Text ("TX");
      ImGui::Spacing ();
      ImGui::Text ("X");
      ImGui::SameLine ();
      ImGui::SliderInt ("##tx_x", &txStartPosX, 0, io.DisplaySize.x - 265);
      ImGui::SameLine ();
      ImGui::Text ("px");
      ImGui::Text ("Y");
      ImGui::SameLine ();
      ImGui::SliderInt ("##tx_y", &txStartPosY, 0, io.DisplaySize.y);
      ImGui::SameLine ();
      ImGui::Text ("px");
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      ImGui::Text ("RX");
      ImGui::Spacing ();
      ImGui::Text ("X");
      ImGui::SameLine ();
      ImGui::SliderInt ("##rx_x", &rxStartPosX, 0, io.DisplaySize.x - 265);
      ImGui::SameLine ();
      ImGui::Text ("px");
      ImGui::Text ("Y");
      ImGui::SameLine ();
      ImGui::SliderInt ("##rx_y", &rxStartPosY, 0, io.DisplaySize.y);
      ImGui::SameLine ();
      ImGui::Text ("px");
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      // Display a lock icon based on the tethered variable
      ImGui::Checkbox ("Lock X", &tetherX);
      ImGui::SameLine ();
      ImGui::Checkbox ("Lock Y", &tetherY);

      // New section
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));
      ImGui::Separator ();
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      ImGui::Text ("Speed of Sound");
      ImGui::SameLine ();
      ImGui::Dummy (ImVec2 (65.0f, 0.0f));
      ImGui::SameLine ();
      if (ImGui::Button ("Reset", ImVec2 (75, 0)))
        {
          // Code to execute when Button 1 is clicked
          std::cout << "Code to reset speed of sound goes here" << std::endl;
        }
      ImGui::Spacing ();
      ImGui::Text ("Add Point");
      ImGui::Spacing ();
      ImGui::Text ("X");
      ImGui::SameLine ();
      ImGui::PushItemWidth (60);
      ImGui::SameLine ();
      ImGui::InputDouble ("##sos_x", &addSosX);
      ImGui::PushItemWidth (60);
      ImGui::SameLine ();
      ImGui::Text ("Y");
      ImGui::SameLine ();
      ImGui::InputDouble ("##sos_y", &addSosY);
      ImGui::SameLine ();
      ImGui::Dummy (ImVec2 (5.0f, 0.0f));
      ImGui::SameLine ();
      if (ImGui::Button ("Add", ImVec2 (75, 0)))
        {
          // Code to execute when Button 1 is clicked
          std::cout << "Code to add point goes here" << std::endl;
          insertOrUpdatePoint (sosVectorX, sosVectorY, addSosX, addSosY);
          splineDrawn = false;
        }

      // New section
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));
      ImGui::Separator ();
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      ImGui::Text ("Simulation Settings");
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));
      // First radio button
      ImGui::RadioButton ("##eigenerays", &selectedRadioButton, 0);
      ImGui::SameLine ();
      ImGui::Text ("Eigenrays");
      ImGui::SameLine ();
      // Second radio button
      ImGui::RadioButton ("##rays", &selectedRadioButton, 1);
      ImGui::SameLine ();
      ImGui::Text ("Rays");
      ImGui::Spacing ();
      ImGui::Checkbox ("Show Playback", &showPlayback);
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      if (showPlayback)
        {
          ImGui::Text ("Playback");
          ImGui::Spacing ();
          ImGui::Dummy (ImVec2 (12.5f, 0.0f));
          ImGui::SameLine ();
          char mySimProgress[20];
          std::sprintf (mySimProgress, "%d/%ld", ray + 1, dataVector.size ());
          ImGui::PushItemWidth (ImGui::GetWindowWidth () - 50);
          ImGui::SliderInt ("##playback", &ray, 0, dataVector.size () - 1,
                            mySimProgress, ImGuiSliderFlags_NoInput);
          ImGui::Spacing ();

          ImGui::Dummy (ImVec2 (21.5f, 0.0f));
          ImGui::SameLine ();
          ImGui::BeginDisabled (simPlaying);
          if (ImGui::Button ("|<<", ImVec2 (50, 0)))
            {
              ray--;
              ray = (ray < 0) ? 0 : ray;
            }
          ImGui::EndDisabled ();
          ImGui::SameLine ();
          if (ImGui::Button (simPlaying ? "||" : "|>", ImVec2 (75, 0)))
            {
              // Play/Pause
              simPlaying = !simPlaying;
            }
          ImGui::SameLine ();
          ImGui::BeginDisabled (simPlaying);
          if (ImGui::Button (">>|", ImVec2 (50, 0)))
            {
              ray++;
              ray = (ray >= dataVector.size ()) ? dataVector.size () - 1 : ray;
            }
          ImGui::EndDisabled ();
          ImGui::Spacing ();
          ImGui::Text ("Simulation Characteristics");
          ImGui::Spacing ();
          ImGui::Text ("Vertices:        %d", dataVector[ray].vertices);
          ImGui::Spacing ();
          ImGui::Text ("Top Bounces:     %d", dataVector[ray].top_bounce);
          ImGui::Spacing ();
          ImGui::Text ("Bottom Bounce:   %d", dataVector[ray].bottom_bounce);
        }

      // Add text anchored to the bottom of the side panel
      ImGui::SetCursorPosY (ImGui::GetWindowHeight ()
                            - (ImGui::GetStyle ().ItemSpacing.y + 60));
      // Center the buttons horizontally
      ImGui::SetCursorPosX ((ImGui::GetWindowWidth () - 200.0f) * 0.5f);
      // Add the first button
      if (ImGui::Button ("Export Simulation", ImVec2 (200, 0)))
        {
          // Code to execute when Button 1 is clicked
          std::cout << "Code to export simulation variables goes here"
                    << std::endl;
        }
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      ImGui::Text ("FPS: ");
      ImGui::SameLine ();
      const char *myFPS = std::to_string (lastFrameCount).c_str ();
      ImGui::Text ("%s", myFPS);
      ImGui::SameLine ();
      ImGui::Dummy (ImVec2 (20.0, 0.0f));
      ImGui::SameLine ();
      ImGui::Checkbox ("Enable VSync", &enableVSync);

      // Ends the window
      ImGui::End ();

      // Export variables to shader
      glUseProgram (shaderProgram);
      glUniform1f (glGetUniformLocation (shaderProgram, "size"), size);
      glUniform4f (glGetUniformLocation (shaderProgram, "color"), color[0],
                   color[1], color[2], color[3]);

      // Renders the ImGUI elements
      ImGui::Render ();
      ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());

      glViewport (950, 0, 650, 800);

      drawFrame (frameVertices);

      // Render the second OpenGL viewport
      drawSOS (sosVertices);

      // Swap the back buffer with the front buffer
      glfwSwapBuffers (window);
      // Take care of all GLFW events
      glfwPollEvents ();
    }

  // Deletes all ImGUI instances
  ImGui_ImplOpenGL3_Shutdown ();
  ImGui_ImplGlfw_Shutdown ();
  ImGui::DestroyContext ();

  // Delete all the objects we've created
  deleteRayDraw ();
  deleteSOSDraw ();
  deleteFrameDraw ();
  deleteFloorDraw ();

  // Delete window before ending the program
  glfwDestroyWindow (window);
  // Terminate GLFW before ending the program
  glfwTerminate ();
  return 0;
}