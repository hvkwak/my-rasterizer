#version 330 core
in vec3 VertexColor; // matches output at shader.vert
out vec4 FragColor;

void main()
{
	FragColor = vec4(VertexColor, 1.0);
}
