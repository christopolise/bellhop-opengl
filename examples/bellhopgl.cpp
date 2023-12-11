#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <bhc/bhc.hpp>
#include "../src/common_run.hpp"
#include "../src/common_setup.hpp"

#include "spline.h"
#include "shader.h"


#include FT_FREETYPE_H

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

// Frame Vertex Shader source code
const char *textVertexShaderSource
    = "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = vec4(aPos.x, aPos.y * -1, aPos.z, 1.0);\n"
      "}\0";

// Second Fragment Shader source code
const char *textFragmentShaderSource
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

GLuint textShaderProgram, textVAO, textVBO;
GLuint textShaderProgramVert, textVertVAO, textVertVBO;
FT_Library ft;
FT_Face face;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

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
  int line_no = 0;
  while (std::getline (file, line))
    {
      std::istringstream iss (line);

      if (line_no < 7)
        {
          line_no++;
          continue;
        }

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

void
insertOrUpdatePoint (std::vector<double> &xCoords,
                     std::vector<double> &yCoords, double newX, double newY)
{
  auto it = std::lower_bound (xCoords.begin (), xCoords.end (), newX);

  // Calculate the index where the new x should be inserted or updated
  size_t index = std::distance (xCoords.begin (), it);

  // Check if the x-coordinate already exists
  if (it != xCoords.end () && *it == newX)
    {
      // If it exists, overwrite the corresponding y-coordinate
      yCoords[index] = newY;
    }
  else
    {
      // If it doesn't exist, insert the new x and y coordinates at the
      // calculated index
      xCoords.insert (it, newX);
      yCoords.insert (yCoords.begin () + index, newY);
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
initTextDraw(Shader &shader, glm::mat4 &projection)
{
  // FreeType
  // --------
  // All functions return a value different than 0 whenever an error occurred
  if (FT_Init_FreeType(&ft))
  {
      std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
      return -1;
  }

// find path to font
  std::string font_name = "../fonts/MesloLGS-NF-Regular.ttf";
  if (font_name.empty())
  {
      std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
      return -1;
  }

// load font as face
  FT_Face face;
  if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
      std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
      return -1;
  }
  else {
      // set size to load glyphs as
      FT_Set_Pixel_Sizes(face, 0, 48);

      // disable byte-alignment restriction
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      // load first 128 characters of ASCII set
      for (unsigned char c = 0; c < 128; c++)
      {
          // Load character glyph 
          if (FT_Load_Char(face, c, FT_LOAD_RENDER))
          {
              std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
              continue;
          }
          // generate texture
          unsigned int texture;
          glGenTextures(1, &texture);
          glBindTexture(GL_TEXTURE_2D, texture);
          glTexImage2D(
              GL_TEXTURE_2D,
              0,
              GL_RED,
              face->glyph->bitmap.width,
              face->glyph->bitmap.rows,
              0,
              GL_RED,
              GL_UNSIGNED_BYTE,
              face->glyph->bitmap.buffer
          );
          // set texture options
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          // now store character for later use
          Character character = {
              texture,
              glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
              glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
              static_cast<unsigned int>(face->glyph->advance.x)
          };
          Characters.insert(std::pair<char, Character>(c, character));
      }
      glBindTexture(GL_TEXTURE_2D, 0);
  }
  // destroy FreeType once we're finished
  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  
  // configure VAO/VBO for texture quads
  // -----------------------------------
  glGenVertexArrays(1, &textVAO);
  glGenBuffers(1, &textVBO);
  glBindVertexArray(textVAO);
  glBindBuffer(GL_ARRAY_BUFFER, textVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return 0;
}

void
drawText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
  // activate corresponding render state	
    shader.use();
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void
deleteText()
{
  glDeleteVertexArrays (1, &textVAO);
  glDeleteBuffers (1, &textVBO);
  glDeleteProgram (textShaderProgram);
}

void clearRegion(int x, int y, int width, int height)
{
  glViewport(x, y, width, height);
  glEnable(GL_SCISSOR_TEST);
  glScissor(x, y, width, height);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
}

void getRays(std::vector<Data> &dataVector, bhc::bhcOutputs<false, false> outputs){
  for (int r = 0; r < outputs.rayinfo->NRays; ++r) {
        dataVector.push_back(Data());
        dataVector[r].vertices = outputs.rayinfo->results[r].Nsteps;
        dataVector[r].top_bounce = outputs.rayinfo->results[r].ray[dataVector[r].vertices - 1].NumTopBnc;
        dataVector[r].bottom_bounce = outputs.rayinfo->results[r].ray[dataVector[r].vertices - 1].NumBotBnc;
        dataVector[r].angle_of_entry = outputs.rayinfo->results[r].SrcDeclAngle;
        for(int j = 0; j < outputs.rayinfo->results[r].Nsteps; j++) {
            outputs.rayinfo->results[r].ray[j].x = RayToOceanX(outputs.rayinfo->results[r].ray[j].x,outputs.rayinfo->results[r].org);
            dataVector[r].x.push_back(outputs.rayinfo->results[r].ray[j].x.x);
            dataVector[r].y.push_back(outputs.rayinfo->results[r].ray[j].x.y);
        }
    }
}

void OutputCallback(const char *message)
{
    // std::cout << "Out: " << message << std::endl << std::flush;
}

void PrtCallback(const char *message) { 
    // std::cout << message << std::flush; 
    }

int
main (int argc, char **argv)
{
  // Find set min and max
  double minX = 10000;
  double maxX = 0;
  double minY = 10000;
  double maxY = 0;

  // Load environment
  bhc::bhcParams<false> params;
  bhc::bhcOutputs<false, false> outputs;
  bhc::bhcInit init;
  if (argc < 2 || argc > 3) exit(1);
  init.FileRoot       = argv[1];
  init.outputCallback = OutputCallback;
  init.prtCallback    = PrtCallback;
  bhc::setup(init, params, outputs);

  maxY =  params.Bdry->Bot.hs.Depth;
  std::cout << "Max Y:\t" << maxY << std::endl;

  bhc::run(params, outputs);

  // Rays
  // std::vector<Data> dataVector = readDataFromFile (argv[1] + std::string (".ray"));
  std::vector<Data> dataVector;
  getRays(dataVector, outputs);

  // Floor shape
  std::vector<double> floorVectorX;
  std::vector<double> floorVectorY;
  for (int i = 1; i < params.bdinfo->bot.NPts - 1; i++) {
        floorVectorX.push_back(params.bdinfo->bot.bd[i].x.x);
        floorVectorY.push_back(params.bdinfo->bot.bd[i].x.y);
  }

  // Speed of sound spline
  std::vector<double> sosVectorX;
  std::vector<double> sosVectorY;
  for (int i = 0; i < params.ssp->NPts; i++) {
        sosVectorX.push_back(params.ssp->z[i]);
        sosVectorY.push_back(params.ssp->alphaR[i]);
  }

  std::vector<double> resetSosVectorX;
  std::vector<double> resetSosVectorY;
  for (int i = 0; i < params.ssp->NPts; i++) {
        resetSosVectorX.push_back(params.ssp->z[i]);
        resetSosVectorY.push_back(params.ssp->alphaR[i]);
  }

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

  // OpenGL state
  // ------------
  glEnable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
  int selectedComputeMode = 0;
  int selectedRayMode = 0;

  // Timing variables
  double lastTime = glfwGetTime ();
  int frameCount = 0;
  int lastFrameCount = 0;

  // Start TX Pos
  // float txStartPosX = params.Pos->Sx[0];
  float txStartPosY = params.Pos->Sz[0];
  // Start RX Pos
  float rxStartPosX = params.Pos->Rr[0];
  float rxStartPosY = params.Pos->Rz[0];

  // Sim vals
  bool showPlayback = true;
  bool simPlaying = true;
  bool drawingRays = true;

  bool splineDrawn = false;

  double addSosX = 0;
  double addSosY = 0;

  bool staticDrawAxes = false;

  int numOfRays = 50;

  const char *vendor
      = reinterpret_cast<const char *> (glGetString (GL_VENDOR));
  const char *renderer
      = reinterpret_cast<const char *> (glGetString (GL_RENDERER));
  std::cout << "OpenGL Vendor: " << vendor << std::endl;
  std::cout << "OpenGL Renderer: " << renderer << std::endl;

  int ray = 0;

  bool enableVSync = true;

  Shader shader("../examples/text.vs", "../examples/text.fs");
  glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(1880), 0.0f, static_cast<float>(800));
  shader.use();
  glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

  initRayDraw ();
  initSOSDraw ();
  initFrameDraw ();
  initFloorDraw ();
  initTextDraw (shader, projection);

  // Specify the color of the background
  glClearColor (0.07f, 0.13f, 0.17f, 1.0f);
  // Clean the back buffer and assign the new color to it
  glClear (GL_COLOR_BUFFER_BIT);

  // Main while loop
  while (!glfwWindowShouldClose (window))
    {

      // Run Bellhop algorithm
      bhc::run(params, outputs);
      // Update rays
      dataVector.clear ();
      // std::cout << "Getting rays..." << std::endl;
      getRays(dataVector, outputs);
      // std::cout << "Got rays" << std::endl;

      if (showPlayback)
        glfwSwapInterval (enableVSync);
      else
        glfwSwapInterval (0);

      glViewport (0, 0, 950, 800);

      // Tell OpenGL a new frame is about to begin
      ImGui_ImplOpenGL3_NewFrame ();
      ImGui_ImplGlfw_NewFrame ();
      ImGui::NewFrame ();

      if (simPlaying && !dataVector.empty ())
      {
        ray++;
        ray %= dataVector.size ();
      }
      // if (!dataVector.empty ())

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
      if (!dataVector.empty ())
      {
        for (int i = 0; i < dataVector[ray].vertices; i++)
        {

          float x = (((dataVector[ray].x[i] - 0) / (1000 - 0)) * 1.7f)
                    - 0.85f;
          float y = (((dataVector[ray].y[i] - 0) / (maxY - 0)) * 1.7f)
                    - 0.85f;

          vertices.push_back (x);
          vertices.push_back (y);
          vertices.push_back (0.0f);
        }
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
              = (((floorVectorX[i] - 0) / (1000 - 0)) * 1.7f) - 0.85f;
          float y
              = (((floorVectorY[i] - 0) / (maxY - 0)) * 1.7f) - 0.85f;

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
          if (!dataVector.empty ())
            drawRay (ray, dataVector, vertices);
        }
      else
        {
          if (drawingRays)
            {
              int numOfDrawnRays = 0;

              if (numOfDrawnRays == 0)
                {
                  // clearRegion (70, 50, 900, 700);
                  // glViewport (0, 0, 950, 800);
                  drawFrame (frameVertices);
                  drawFloor (floorVertices);
                }

              if (!dataVector.empty ())
                drawRay (ray, dataVector, vertices);
              numOfDrawnRays++;

              if (numOfDrawnRays == dataVector.size () - 1)
                {
                  drawingRays = false;
                  numOfDrawnRays = 0;
                }
            }
          
        }

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
      ImGui::RadioButton ("##C++", &selectedComputeMode, 0);
      ImGui::SameLine ();
      ImGui::Text ("C++");
      ImGui::SameLine ();
      // Second radio button
      ImGui::RadioButton ("##CUDA", &selectedComputeMode, 1);
      ImGui::SameLine ();
      ImGui::Text ("CUDA");
      ImGui::SameLine ();
      // Display the selected mode of computeMode
      const char *computeMode[] = { "C++", "CUDA" };
      const char *rayMode[] = { "eigenrays", "rays" };

      // New section
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));
      ImGui::Separator ();
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      ImGui::Text ("Adjust Endpoints:");
      ImGui::Spacing ();
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      ImGui::Text ("TX");
      ImGui::Spacing ();
      // ImGui::Text ("X");
      // ImGui::SameLine ();
      // ImGui::SliderInt ("##tx_x", &txStartPosX, 0, maxX);
      // ImGui::SameLine ();
      // ImGui::Text ("px");
      ImGui::Text ("Y");
      ImGui::SameLine ();
      ImGui::SliderFloat ("##tx_y", &txStartPosY, 0, maxY - .1);
      ImGui::SameLine ();
      ImGui::Text ("px");
      if (selectedRayMode == 1)
      {
        ImGui::Text ("Rays");
        ImGui::SameLine ();
        ImGui::SliderInt("##rays", &numOfRays, 1, 1000);
      }
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      ImGui::Text ("RX");
      ImGui::Spacing ();
      ImGui::Text ("X");
      ImGui::SameLine ();
      ImGui::SliderFloat ("##rx_x", &rxStartPosX, 0.1, 1000);
      ImGui::SameLine ();
      ImGui::Text ("px");
      ImGui::Text ("Y");
      ImGui::SameLine ();
      ImGui::SliderFloat ("##rx_y", &rxStartPosY, 0, maxY - .1);
      ImGui::SameLine ();
      ImGui::Text ("px");

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
          sosVectorX.clear ();
          sosVectorY.clear ();
          sosVectorX = resetSosVectorX;
          sosVectorY = resetSosVectorY;
          // Print sos vectors
          std::cout << "Reset Spline" << std::endl;
          for (int i = 0; i < sosVectorX.size (); i++)
            {
              std::cout << sosVectorX[i] << "\t" << sosVectorY[i] << std::endl;
            }
          // clearRegion (950, 0, 650, 800);
          // clearRegion (0, 0, 1000, 800);
          glClear (GL_COLOR_BUFFER_BIT);
          splineDrawn = false;
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
          clearRegion (950, 0, 650, 800);
          clearRegion (0, 0, 1000, 800);
          dataVector.clear();
          splineDrawn = false;
        }

      // New section
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));
      ImGui::Separator ();
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));

      ImGui::Text ("Simulation Settings");
      ImGui::Dummy (ImVec2 (0.0f, 10.0f));
      // First radio button
      ImGui::RadioButton ("##eigenerays", &selectedRayMode, 0);
      ImGui::SameLine ();
      ImGui::Text ("Eigenrays");
      ImGui::SameLine ();
      // Second radio button
      ImGui::RadioButton ("##rays", &selectedRayMode, 1);
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

          ImGui::Dummy (ImVec2 (32.5f, 0.0f));
          ImGui::SameLine ();

          if (!dataVector.empty ())
          {
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
          }
          ImGui::EndDisabled ();
          ImGui::Spacing ();
          ImGui::Text ("Simulation Characteristics");
          ImGui::Spacing ();

          if(! dataVector.empty())
          {
            ImGui::Text ("Vertices:        %d", dataVector[ray].vertices);
            ImGui::Spacing ();
            ImGui::Text ("Angle of Entry:  %f", dataVector[ray].angle_of_entry);
            ImGui::Spacing ();
            ImGui::Text ("Top Bounces:     %d", dataVector[ray].top_bounce);
            ImGui::Spacing ();
            ImGui::Text ("Bottom Bounce:   %d", dataVector[ray].bottom_bounce);
          }
          else
          {
            ImGui::Text ("No Data");
          }
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

      clearRegion (950, 0, 650, 800);
      drawFrame (frameVertices);

      // Render the second OpenGL viewport
      drawSOS (sosVertices);

      // glViewport (0, 0, 1880, 800);
      clearRegion (0, 0, 1880 - 280, 59); // Clear bottom bar
      clearRegion (0, 800-59, 1880 - 280, 59 ); // Clear top bar
      clearRegion (0, 0, 70, 800); // Clear left bar
      clearRegion ((1880 / 2) - 12, 0, 70, 800); // Clear right bar
      glViewport (0, 0, 1880, 800);
      // Draw Once
      // if (showPlayback)
      // {
        drawText(shader, "Bellhop Algorithm", 350.0f, 760.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
        drawText(shader, "Distance (m)", 425.0f, 30.0f, 0.35f, glm::vec3(1.0f, 1.0f, 1.0f));
        drawText(shader, "Speed of Sound", 1175.0f, 760.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
        drawText(shader, "Speed (m/s)", 1225.0f, 30.0f, 0.35f, glm::vec3(1.0f, 1.0f, 1.0f));
        char yMaxStr[100];
        std::sprintf (yMaxStr, "%.1f", maxY);
        drawText(shader, yMaxStr, 15.0, 60.0, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f)); // Y Max
        drawText(shader, "0.0", 10.0, 735.0, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f)); // Y Min
        drawText(shader, "0.0", 70.0, 30.0, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f)); // X Min
        drawText(shader, "1000.0", 840.0, 30.0, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f)); // X Max

        double sosMinX
            = *std::min_element (xSOSCurve.begin (), xSOSCurve.end ());
        double sosMaxX
            = *std::max_element (xSOSCurve.begin (), xSOSCurve.end ());
        double sosMinY
            = *std::min_element (ySOSCurve.begin (), ySOSCurve.end ());
        double sosMaxY
            = *std::max_element (ySOSCurve.begin (), ySOSCurve.end ());
        char sosMinXStr[10];
        char sosMaxXStr[10];
        char sosMinYStr[10];
        char sosMaxYStr[10];
        std::sprintf (sosMinXStr, "%.1f", sosMinX);
        std::sprintf (sosMaxXStr, "%.1f", sosMaxX);
        std::sprintf (sosMinYStr, "%.1f", sosMinY);
        std::sprintf (sosMaxYStr, "%.1f", sosMaxY);
        drawText(shader, sosMinYStr, 1000, 30.0, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f)); // SOS X Min
        drawText(shader, sosMaxYStr, 1525, 30.0, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f)); // SOS X Max
        drawText(shader, sosMaxXStr, 950, 60.0, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f)); // SOS Y Max
        drawText(shader, sosMinXStr, 950, 735.0, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f)); // SOS Y Min

      // Swap the back buffer with the front buffer
      glfwSwapBuffers (window);
      // Take care of all GLFW events
      glfwPollEvents ();

      // Check to see if TX and RX positions are updated
      if (txStartPosY != params.Pos->Sz[0])
        { 
          params.Pos->Sz[0] = txStartPosY;
          clearRegion (70, 50, 900, 700);
          glViewport (0, 0, 950, 800);
        }
      if (rxStartPosX != params.Pos->Rr[0])
        { 
          params.Pos->Rr[0] = rxStartPosX;
          clearRegion (70, 50, 900, 700);
          glViewport (0, 0, 950, 800);
        }
      if (rxStartPosY != params.Pos->Rz[0])
        { 
          params.Pos->Rz[0] = rxStartPosY;
          clearRegion (70, 50, 900, 700);
          glViewport (0, 0, 950, 800);
        }

      // check to see if Ray mode needs to change
      if (params.Beam->RunType[0] =='E' && selectedRayMode == 1 ) {
        clearRegion (70, 50, 900, 700);
        glViewport (0, 0, 950, 800);
        params.Beam->RunType[0] = 'R';
        params.Angles->alpha.n = numOfRays;
        bhc::extsetup_rayelevations<false>(params,numOfRays);
        params.Angles->alpha.inDegrees = true;

        params.Angles->alpha.angles[0] = RL(-80.0);
        params.Angles->alpha.angles[1] = RL(80.0);
        params.Angles->alpha.angles[2] = FL(-999.9);
        bhc::SubTab(params.Angles->alpha.angles, params.Angles->alpha.n);
            // }
      }
      if (selectedRayMode == 1 && (params.Angles->alpha.n != numOfRays)) {
        params.Angles->alpha.n = numOfRays;
        bhc::extsetup_rayelevations<false>(params,numOfRays);
        params.Angles->alpha.inDegrees = true;

        params.Angles->alpha.angles[0] = RL(-80.0);
        params.Angles->alpha.angles[1] = RL(80.0);
        params.Angles->alpha.angles[2] = FL(-999.9);
        bhc::SubTab(params.Angles->alpha.angles, params.Angles->alpha.n);
      }

      if (params.Beam->RunType[0] =='R' && selectedRayMode == 0 ) {
        clearRegion (70, 50, 900, 700);
        glViewport (0, 0, 950, 800);
        params.Beam->RunType[0] = 'E';
        // params.Angles->alpha.n = 5000;
        params.Angles->alpha.inDegrees = true;
        // int n = params.Angles->alpha.n;
        bhc::extsetup_rayelevations<false>(params,5000);
        params.Angles->alpha.n = 5000;
        params.Angles->alpha.angles[0] = RL(-80.0);
        params.Angles->alpha.angles[1] = RL(80.0);
        params.Angles->alpha.angles[2] = FL(-999.9);
        bhc::SubTab(params.Angles->alpha.angles, params.Angles->alpha.n);
 
      }
      if ( params.ssp->NPts != sosVectorX.size()) {
        params.ssp->NPts = sosVectorX.size();
        params.ssp->Nz = params.ssp->NPts;
        for(int i = 0; i < params.ssp->NPts; ++i) {
          params.ssp->z[i] = sosVectorX[i];
          params.ssp->alphaR[i] = sosVectorY[i];
          params.ssp->betaR[i] = 1;
          params.ssp->rho[i] = 0;
          params.ssp->alphaI[i] = 0;
        }
        params.ssp->rangeInKm = false;
        params.ssp->dirty = true;
        std::cout <<"updating ssp"<<std::endl;
      }

    }

  // free memory used by bellhop
  bhc::finalize(params, outputs);

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