#version 330

// Raylib's default and expected input vertex attributes
in vec2 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// this is custom
in float vertexWeight;

// Raylib's default and expected input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

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
  // NOTES(mudit): macos seems to have a hard limit on this so going
  // past 60 doesn't make any difference
  gl_PointSize = 8.0;

  fragColor = vec4(vertexWeight,
                   vertexWeight*vertexWeight*vertexWeight,
                   vertexWeight*(1-vertexWeight)*2,
                   1.0);
}
