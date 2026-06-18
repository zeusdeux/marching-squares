#version 330 core

// Raylib's default input attributes and uniform
// Commented ones are unused in this project and
// are for my own knowledge
// in vec2 vertexTexCoord;
// in vec3 vertexNormal;
// in vec4 vertexColor;
// uniform mat4 mvp;
// uniform mat4 matModel;
// uniform mat4 matNormal;

// vertexPosition is one of the default attributes
// in raylib and it's in use for passing in Points
in vec2 vertexPosition;

// These are custom
in float vertexWeight;
uniform vec2 resolution;
uniform float pointSize;

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
  gl_PointSize = pointSize;

  // NOTES(mudit): shift sampled simplex4d weight from -1..1 to 0..1
  float wtuv = (vertexWeight+1.0)*0.5;

  fragColor = vec4(wtuv,
                   wtuv*wtuv*wtuv,
                   wtuv*(1-wtuv)*2,
                   1.0);
}
