#include<imgui/imgui.h>
#include<imgui/imgui_impl_glfw.h>
#include<imgui/imgui_impl_opengl3.h>

#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<math.h>

// Vertex Shader source code
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform float size;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(size * aPos.x, size * aPos.y, size * aPos.z, 1.0);\n"
"}\0";
//Fragment Shader source code
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 color;\n"
"void main()\n"
"{\n"
"   FragColor = color;\n"
"}\n\0";


int main()
{
	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Tell GLFW we are using the CORE profile
	// So that means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a GLFWwindow object of 800 by 800 pixels, naming it "YoutubeOpenGL"
	GLFWwindow* window = glfwCreateWindow(800, 800, "ImGui + GLFW", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Introduce the window into the current context
	glfwMakeContextCurrent(window);

	//Load GLAD so it configures OpenGL
	gladLoadGL();
	// Specify the viewport of OpenGL in the Window
	// In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, 800, 800);



	// Create Vertex Shader Object and get its reference
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// Attach Vertex Shader source to the Vertex Shader Object
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(vertexShader);

	// Create Fragment Shader Object and get its reference
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Attach Fragment Shader source to the Fragment Shader Object
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(fragmentShader);

	// Create Shader Program Object and get its reference
	GLuint shaderProgram = glCreateProgram();
	// Attach the Vertex and Fragment Shaders to the Shader Program
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	// Wrap-up/Link all the shaders together into the Shader Program
	glLinkProgram(shaderProgram);

	// Delete the now useless Vertex and Fragment Shader objects
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);



	// Vertices coordinates
	GLfloat vertices[] =
	{
		-0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // Lower left corner
		0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // Lower right corner
		0.0f, 0.5f * float(sqrt(3)) * 2 / 3, 0.0f // Upper corner
	};

	// Create reference containers for the Vartex Array Object and the Vertex Buffer Object
	GLuint VAO, VBO;

	// Generate the VAO and VBO with only 1 object each
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// Make the VAO the current Vertex Array Object by binding it
	glBindVertexArray(VAO);

	// Bind the VBO specifying it's a GL_ARRAY_BUFFER
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Introduce the vertices into the VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Configure the Vertex Attribute so that OpenGL knows how to read the VBO
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	// Enable the Vertex Attribute so that OpenGL knows to use it
	glEnableVertexAttribArray(0);

	// Bind both the VBO and VAO to 0 so that we don't accidentally modify the VAO and VBO we created
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	// Variables to be changed in the ImGUI window
	bool drawTriangle = true;
	float size = 1.0f;
	float color[4] = { 0.8f, 0.3f, 0.02f, 1.0f };
    // Variables to store the selected radio button
    int selectedRadioButton = 0;

	// Exporting variables to shaders
	glUseProgram(shaderProgram);
	glUniform1f(glGetUniformLocation(shaderProgram, "size"), size);
	glUniform4f(glGetUniformLocation(shaderProgram, "color"), color[0], color[1], color[2], color[3]);

    // Timing variables
    double lastTime = glfwGetTime();
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

	const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	std::cout << "OpenGL Vendor: " << vendor << std::endl;
	std::cout << "OpenGL Renderer: " << renderer << std::endl;


	// Main while loop
	while (!glfwWindowShouldClose(window))
	{

        // Measure speed
        double currentTime = glfwGetTime();
        frameCount++;
        if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1 sec ago
            lastFrameCount = frameCount;
            frameCount = 0;
            lastTime = currentTime;
        }


		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and assign the new color to it
		glClear(GL_COLOR_BUFFER_BIT);

		// Tell OpenGL a new frame is about to begin
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		

		// Tell OpenGL which Shader Program we want to use
		glUseProgram(shaderProgram);
		// Bind the VAO so OpenGL knows to use it
		glBindVertexArray(VAO);
		// Only draw the triangle if the ImGUI checkbox is ticked
		if (drawTriangle)
			// Draw the triangle using the GL_TRIANGLES primitive
			glDrawArrays(GL_TRIANGLES, 0, 3);

		if(tetherX) txStartPosX = rxStartPosX;
		if(tetherY) txStartPosY = rxStartPosY;


        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 265, 0));
        ImGui::SetNextWindowSize(ImVec2(265, io.DisplaySize.y));
		// ImGUI window creation
		ImGui::Begin("Acoustic Ray Casting Parameters", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
		// // Text that appears in the window
		// ImGui::Text("Hello there adventurer!");
		// // Checkbox that appears in the window
		// ImGui::Checkbox("Draw Triangle", &drawTriangle);
		// // Slider that appears in the window
		// ImGui::SliderFloat("Size", &size, 0.5f, 2.0f);
		// // Fancy color editor that appears in the window
		// ImGui::ColorEdit4("Color", color);
		
		// Main content
		ImGui::Dummy(ImVec2(0.0f, 10.0f));
        ImGui::Text("Select a Computation Method:");
		ImGui::Spacing();
		// First radio button
        ImGui::RadioButton("##C++", &selectedRadioButton, 0);
        ImGui::SameLine();  
        ImGui::Text("C++");
        ImGui::SameLine();  
        // Second radio button
        ImGui::RadioButton("##CUDA", &selectedRadioButton, 1);
        ImGui::SameLine();  
        ImGui::Text("CUDA");
        ImGui::SameLine();  
        // Third radio button
        ImGui::RadioButton("##OptiX", &selectedRadioButton, 2);
        ImGui::SameLine();  
        ImGui::Text("OptiX");
        // Display the selected mode of computeMode
        const char* computeMode[] = { "C++", "CUDA", "OptiX" };
        // std::cout << "Selected Compute Mode: " << computeMode[selectedRadioButton] << std::endl;

		// New section
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::Text("Adjust Endpoints:");
		ImGui::Spacing();
		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::Text("TX");
		ImGui::Spacing();
		ImGui::Text("X");
		ImGui::SameLine();
		ImGui::SliderInt("##tx_x", &txStartPosX, 0, io.DisplaySize.x - 265);
		ImGui::SameLine();
		ImGui::Text("px");
		ImGui::Text("Y");
		ImGui::SameLine();
		ImGui::SliderInt("##tx_y", &txStartPosY, 0, io.DisplaySize.y);
		ImGui::SameLine();
		ImGui::Text("px");
		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::Text("RX");
		ImGui::Spacing();
		ImGui::Text("X");
		ImGui::SameLine();
		ImGui::SliderInt("##rx_x", &rxStartPosX, 0, io.DisplaySize.x - 265);
		ImGui::SameLine();
		ImGui::Text("px");
		ImGui::Text("Y");
		ImGui::SameLine();
		ImGui::SliderInt("##rx_y", &rxStartPosY, 0, io.DisplaySize.y);
		ImGui::SameLine();
		ImGui::Text("px");
		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		// Display a lock icon based on the tethered variable
		ImGui::Checkbox("Lock X", &tetherX);
		ImGui::SameLine();
		ImGui::Checkbox("Lock Y", &tetherY);

        // Add text anchored to the bottom of the side panel
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (ImGui::GetStyle().ItemSpacing.y + 58));

		// Center the buttons horizontally
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 200.0f) * 0.5f);

		// Add the first button
		if (ImGui::Button("Export Simulation", ImVec2(200, 0))) {
			// Code to execute when Button 1 is clicked
			std::cout << "Code to export simulation variables goes here" << std::endl;
		}

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

        ImGui::Text("FPS: ");
        ImGui::SameLine();
        const char * myFPS = std::to_string(lastFrameCount).c_str();
        ImGui::Text("%s", myFPS);

		// Ends the window
		ImGui::End();

		// Export variables to shader
		glUseProgram(shaderProgram);
		glUniform1f(glGetUniformLocation(shaderProgram, "size"), size);
		glUniform4f(glGetUniformLocation(shaderProgram, "color"), color[0], color[1], color[2], color[3]);

		// Renders the ImGUI elements
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}

	// Deletes all ImGUI instances
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Delete all the objects we've created
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProgram);
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}