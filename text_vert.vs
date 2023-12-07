#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform mat4 projection;

void main()
{
    // Swap X and Y components to rotate 90 degrees to the left
    gl_Position = projection * vec4(vertex.yx, 0.0, 1.0);
    TexCoords = vec2(vertex.z, 1.0 - vertex.w);
}
