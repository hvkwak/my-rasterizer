#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 VertexColor; // Output variable to send to Fragment Shader

// matrices for transformations
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Proj;
void main()
{
  gl_Position = Proj * View * Model * vec4(aPos, 1.0);
  // Pass the color data from the attribute to the fragment shader
  VertexColor = aColor;
}
