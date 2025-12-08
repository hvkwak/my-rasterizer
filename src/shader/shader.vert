#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 VertexColor; // Output variable to send to Fragment Shader

// matrices for transformations
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
void main()
{
  gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
  gl_PointSize = 2.0; // size in pixels (you can tweak this)
  // Pass the color data from the attribute to the fragment shader
  VertexColor = aColor;
}
