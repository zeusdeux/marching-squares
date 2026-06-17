#version 330 core

// Raylib's default and expected input vertex attributes
in vec2 vertexPosition;

// this is custom
in float vertexWeight;

// this is custom
uniform vec2 resolution;

// Output vertex attributes (to fragment shader)
out vec4 fragColor;

void main()
{
  vec2 ndc = (vertexPosition/resolution)*2.0 - 1.0;
  // NOTES(mudit): (0,0) in opengl is bottom left instead of top
  // right of the window hence the flip here
  ndc.y *= -1.0;
  gl_Position = vec4(ndc, 0.0, 1.0);

  float wtuv = (vertexWeight+1.0)*0.5;
  fragColor = vec4(0.2, 0.7, 1.0, 1.0);
}
