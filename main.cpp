#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <vector>

// Vertex Shader source code
const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "uniform float size;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(size * aPos.x, size * "
                                 "aPos.y, size * aPos.z, 1.0);\n"
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
const char *secondVertexShaderSource = "#version 330 core\n"
                                       "layout (location = 0) in vec3 aPos;\n"
                                       "void main()\n"
                                       "{\n"
                                       "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                       "}\0";

// Second Fragment Shader source code
const char *secondFragmentShaderSource = "#version 330 core\n"
                                         "out vec4 FragColor;\n"
                                         "void main()\n"
                                         "{\n"
                                         "   FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
                                         "}\n\0";

GLuint secondShaderProgram, secondVAO, secondVBO;

struct Data
{
  int vertices;
  int top_bounce;
  int bottom_bounce;
  double angle_of_entry;
  std::vector<double> x;
  std::vector<double> y;
};

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

void SetupSecondViewport() {
    // Create Second Vertex Shader Object and get its reference
    GLuint secondVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(secondVertexShader, 1, &secondVertexShaderSource, NULL);
    glCompileShader(secondVertexShader);

    // Create Second Fragment Shader Object and get its reference
    GLuint secondFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(secondFragmentShader, 1, &secondFragmentShaderSource, NULL);
    glCompileShader(secondFragmentShader);

    // Create Second Shader Program Object and get its reference
    secondShaderProgram = glCreateProgram();
    glAttachShader(secondShaderProgram, secondVertexShader);
    glAttachShader(secondShaderProgram, secondFragmentShader);
    glLinkProgram(secondShaderProgram);

    // Delete the now useless Second Vertex and Fragment Shader objects
    glDeleteShader(secondVertexShader);
    glDeleteShader(secondFragmentShader);

    // Create Second Vertex Array Object and Vertex Buffer Object
    glGenVertexArrays(1, &secondVAO);
    glGenBuffers(1, &secondVBO);

    // Bind the Second VAO and VBO
    glBindVertexArray(secondVAO);
    glBindBuffer(GL_ARRAY_BUFFER, secondVBO);

    // Define the vertices of a simple triangle
    GLfloat secondVertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    // Introduce the vertices into the Second VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(secondVertices), secondVertices, GL_STATIC_DRAW);

    // Configure the Second Vertex Attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind the Second VAO and VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int
main ()
{

  std::vector<Data> dataVector = readDataFromFile ("test.ray");

  std::cout << dataVector.size () << std::endl;

  // Initialize GLFW
  glfwInit ();

  // Set GLFW_RESIZABLE to GLFW_FALSE to prevent window resizing
  glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);

  // Tell GLFW what version of OpenGL we are using
  // In this case we are using OpenGL 3.3
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
  // Tell GLFW we are using the CORE profile
  // So that means we only have the modern functions
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window
      = glfwCreateWindow (1865, 800, "ImGui + GLFW", NULL, NULL);

  // Error check if the window fails to create
  if (window == NULL)
    {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate ();
      return -1;
    }
  // Introduce the window into the current context
  glfwMakeContextCurrent (window);

  // Load GLAD so it configures OpenGL
  gladLoadGL ();
  // Specify the viewport of OpenGL in the Window
  // In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800
  // glViewport (0, 0, 800, 800);

  // Create Vertex Shader Object and get its reference
  GLuint vertexShader = glCreateShader (GL_VERTEX_SHADER);
  // Attach Vertex Shader source to the Vertex Shader Object
  glShaderSource (vertexShader, 1, &vertexShaderSource, NULL);
  // Compile the Vertex Shader into machine code
  glCompileShader (vertexShader);

  // Create Fragment Shader Object and get its reference
  GLuint fragmentShader = glCreateShader (GL_FRAGMENT_SHADER);
  // Attach Fragment Shader source to the Fragment Shader Object
  glShaderSource (fragmentShader, 1, &fragmentShaderSource, NULL);
  // Compile the Vertex Shader into machine code
  glCompileShader (fragmentShader);

  // Create Shader Program Object and get its reference
  GLuint shaderProgram = glCreateProgram ();
  // Attach the Vertex and Fragment Shaders to the Shader Program
  glAttachShader (shaderProgram, vertexShader);
  glAttachShader (shaderProgram, fragmentShader);
  // Wrap-up/Link all the shaders together into the Shader Program
  glLinkProgram (shaderProgram);

  // Delete the now useless Vertex and Fragment Shader objects
  glDeleteShader (vertexShader);
  glDeleteShader (fragmentShader);

  // Generate sine wave vertices
  const int numVertices = dataVector[0].vertices;
  // GLfloat vertices[numVertices * 3];
  std::vector<GLfloat> vertices;

  // Create reference containers for the Vartex Array Object and the Vertex
  // Buffer Object
  GLuint VAO, VBO;

  // Generate the VAO and VBO with only 1 object each
  glGenVertexArrays (1, &VAO);
  glGenBuffers (1, &VBO);

  // Make the VAO the current Vertex Array Object by binding it
  glBindVertexArray (VAO);

  // Bind the VBO specifying it's a GL_ARRAY_BUFFER
  glBindBuffer (GL_ARRAY_BUFFER, VBO);
  // Introduce the vertices into the VBO
  // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBufferData (GL_ARRAY_BUFFER, vertices.size () * sizeof (GLfloat),
                vertices.data (), GL_STATIC_DRAW);

  // Configure the Vertex Attribute so that OpenGL knows how to read the VBO
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float),
                         (void *)0);
  // Enable the Vertex Attribute so that OpenGL knows to use it
  glEnableVertexAttribArray (0);

  // Bind both the VBO and VAO to 0 so that we don't accidentally modify the
  // VAO and VBO we created
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindVertexArray (0);

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

  // Exporting variables to shaders
  glUseProgram (shaderProgram);
  glUniform1f (glGetUniformLocation (shaderProgram, "size"), size);
  glUniform4f (glGetUniformLocation (shaderProgram, "color"), color[0],
               color[1], color[2], color[3]);

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
  bool simPlaying = true;

  const char *vendor
      = reinterpret_cast<const char *> (glGetString (GL_VENDOR));
  const char *renderer
      = reinterpret_cast<const char *> (glGetString (GL_RENDERER));
  std::cout << "OpenGL Vendor: " << vendor << std::endl;
  std::cout << "OpenGL Renderer: " << renderer << std::endl;

  

  // Min and Max
  double minX
      = *std::min_element (dataVector[0].x.begin (), dataVector[0].x.end ());
  double maxX
      = *std::max_element (dataVector[0].x.begin (), dataVector[0].x.end ());
  double minY
      = *std::min_element (dataVector[0].y.begin (), dataVector[0].y.end ());
  double maxY
      = *std::max_element (dataVector[0].y.begin (), dataVector[0].y.end ());

  int ray = 0;

  bool enableVSync = true;

  SetupSecondViewport();

  // Main while loop
  while (!glfwWindowShouldClose (window))
    {

      glfwSwapInterval(enableVSync);

      glViewport (0, 0, 800, 800);

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

          float x = (((dataVector[ray].x[i] - minX) / (maxX - minX)) * 2.0f)
                    - 1.0f;
          float y = (((dataVector[ray].y[i] - minY) / (maxY - minY)) * 2.0f)
                    - 1.0f;

          vertices.push_back (x);
          vertices.push_back (y);
          vertices.push_back (0.0f);
        }

      // Specify the color of the background
      glClearColor (0.07f, 0.13f, 0.17f, 1.0f);
      // Clean the back buffer and assign the new color to it
      glClear (GL_COLOR_BUFFER_BIT);

      // Tell OpenGL a new frame is about to begin
      ImGui_ImplOpenGL3_NewFrame ();
      ImGui_ImplGlfw_NewFrame ();
      ImGui::NewFrame ();

      // Tell OpenGL which Shader Program we want to use
      glUseProgram (shaderProgram);
      // Bind the VAO so OpenGL knows to use it
      glBindVertexArray (VAO);

      // Bind the VBO specifying it's a GL_ARRAY_BUFFER
      glBindBuffer (GL_ARRAY_BUFFER, VBO);
      // Introduce the updated vertices into the VBO
      // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
      // GL_STATIC_DRAW);
      glBufferData (GL_ARRAY_BUFFER, vertices.size () * sizeof (GLfloat),
                    vertices.data (), GL_STATIC_DRAW);
      // glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(GLfloat),
      // vertices.data());

      // Draw the triangle using the GL_TRIANGLES primitive
      glDrawArrays (GL_LINE_STRIP, 0, dataVector[ray].vertices);

      if (tetherX)
        txStartPosX = rxStartPosX;
      if (tetherY)
        txStartPosY = rxStartPosY;

      ImGui::SetNextWindowPos (ImVec2 (io.DisplaySize.x - 265, 0));
      ImGui::SetNextWindowSize (ImVec2 (265, io.DisplaySize.y));
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
        }
      // ImGui::Dummy (ImVec2 (0.0f, 10.0f));
      // ImGui::SliderFloat ("Freq", &freq, 0.0, 10.0);
      // ImGui::Spacing ();
      // ImGui::SliderFloat ("Ampl", &ampl, -1.0, 1.0);
      // ImGui::Spacing ();
      // ImGui::SliderFloat ("Phase", &phase, -1.0 * freq, 1.0 * freq);

      // New section
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));
      ImGui::Separator ();
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      ImGui::Text ("Simulation Settings");
      ImGui::Spacing ();
      ImGui::Text ("Playback");
      ImGui::Spacing ();
      ImGui::Dummy (ImVec2 (12.5f, 0.0f));
      ImGui::SameLine ();
      char *mySimProgress;
      std::sprintf (
          mySimProgress, "%d/%d", ray + 1,
          dataVector.size ()); 
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

      // Add text anchored to the bottom of the side panel
      ImGui::SetCursorPosY (ImGui::GetWindowHeight ()
                            - (ImGui::GetStyle ().ItemSpacing.y + 58));
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

      glViewport (800, 0, 800, 800);

      // Render the second OpenGL viewport
      glUseProgram(secondShaderProgram);
      glBindVertexArray(secondVAO);
      glDrawArrays(GL_TRIANGLES, 0, 3);

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
  glDeleteVertexArrays (1, &VAO);
  glDeleteBuffers (1, &VBO);
  glDeleteProgram (shaderProgram);
  // Delete window before ending the program
  glfwDestroyWindow (window);
  // Terminate GLFW before ending the program
  glfwTerminate ();
  return 0;
}