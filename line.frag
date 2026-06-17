#version 330 core

// Input fragment attributes (from fragment shader)
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

void main()
{
  finalColor = fragColor;
}
