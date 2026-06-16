#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define GL_SILENCE_DEPRECATION
#define GRAPHICS_API_OPENGL_33
#include <OpenGL/gl3.h>
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#define ZDX_SIMPLEX_3D_IMPLEMENTATION
#include "./simplex3d.h"

#define DeferScope(startExpr, endExpr) \
  for(int DeferScope_i__ = (startExpr, 0); DeferScope_i__ == 0; (DeferScope_i__++, endExpr))

#define WIDTH 1280
#define HEIGHT 720

#define GRID_W 3
#define COLS ((WIDTH/GRID_W)+1)
#define ROWS ((HEIGHT/GRID_W)+1)
#define POINTSCOUNT (COLS*ROWS)

#define BGCOLOR (Color){0}

typedef uint8_t  u8;
typedef uint32_t u32;
typedef int32_t  i32;
typedef float    f32;
typedef double   f64;
typedef Vector2  Point;

static Point points[POINTSCOUNT]  = {0};
static f32   weights[POINTSCOUNT] = {0};
static f32   featureSize          = 0.003f;
static f32   rateOfChange         = 0.17f;

void updateWeights(f32 t)
{
  f32 Z = t*rateOfChange;
  f32 sizeFactor = GRID_W*featureSize;

  for (u32 y = 0; y < ROWS; y++) {
    f32 Y = y*sizeFactor;
    u32 yIdx = (y*COLS);

    for (u32 x = 0; x < COLS; x++) {
      f32 X = x*sizeFactor;
      f32 weight = simplex3d(X, Y, Z);
      // NOTES(mudit): shift sampled weight from -1..1 to 0..1 as it's
      // used to calculate fragment color in the vertex shader
      weights[x+yIdx] = (weight+1.f)*0.5f;
    }
  }
}

int main(void)
{
  char pointCountText[64] = {0};
  snprintf(pointCountText, 64, "%d points", POINTSCOUNT);

  f64 t = 0;
  bool drawFps = false;

  for (u32 y = 0; y < ROWS; y++) {
    for (u32 x = 0; x < COLS; x++) {
      u32 idx = x+(y*COLS);
      points[idx] = (Point){x*GRID_W, y*GRID_W*1.f};
    }
  }

  // This is a fun one!
  // SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
  // this will cause a panic at resolutions higher than 1280x720
  // SetConfigFlags(FLAG_WINDOW_UNDECORATED);
  SetConfigFlags(FLAG_WINDOW_HIGHDPI);
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(WIDTH, HEIGHT, "Custom shaders example");
  SetTargetFPS(120);

  Shader pointShader = LoadShader("point.vert", "point.frag");

  if (!IsShaderValid(pointShader)) {
    TraceLog(LOG_ERROR, "Invalid shader");
    return 1;
  }

  Point res = {WIDTH, HEIGHT};
  // NOTES(mudit): GetShaderLocation gets a uniform location by name
  // whereas to get an attribute location, use GetShaderLocationAttrib(...)
  i32 resolutionUniformLoc = GetShaderLocation(pointShader, "resolution");
  SetShaderValue(pointShader, resolutionUniformLoc, &res, SHADER_UNIFORM_VEC2);

  // NOTES(mudit): Learnt from examples/others/raylib_opengl_interop.c
  // VAO -> basically holds the config for the VBOs that are enabled
  // below it. This is so that we can directly use all VBOs
  // referenced by this VAO via glBindVertexArray(vao); prior to
  // issuing a draw call rather than having to call:
  // glBindBuffer -> glVertexAttribPointer -> glEnableVertexAttribArray -> glDrawArrays -> glBindBuffer(..., 0) aka disable
  // wherein the first three calls need to happen for each vbo and the
  // attrib it's being passed to in the vertex shader
  u32 vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  u32 posVbo;
  glGenBuffers(1, &posVbo);
  glBindBuffer(GL_ARRAY_BUFFER, posVbo);
  // NOTES(mudit): GL_STATIC_DRAW as the points data does not change
  glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
  // NOTES(mudit): set vertexPosition attribute of vertex shader. This
  // attrib name "vertexPosition" is required by default by raylib
  // internally. Also, stride is 0 instead of sizeof(*points) aka
  // sizeof(float)*2 as 0 tells opengl that the data is tightly packed
  // i.e., points here contains (x,y)(x,y)(x,y).. and not
  // (x,y,texCoords)(x,y,texCoords)(x,y,texCoords).. or something else
  // where each vertex is more than just a 2D vertex position
  glVertexAttribPointer(pointShader.locs[SHADER_LOC_VERTEX_POSITION], 2, GL_FLOAT, GL_FALSE, 0, 0);
  // NOTES(mudit): These don't need to be disabled as the state of
  // this vertex attrib is stored in our vao and we unbind the whole
  // vao after draw anyway. Even if we didn't, and we switch out vaos
  // for multiple draws, vaos don't leak into each other as to use a
  // different vao, we have to bind it first which is equivalent to
  // glBindVertexArray(0) only that we pass the other vao instead of 0
  glEnableVertexAttribArray(pointShader.locs[SHADER_LOC_VERTEX_POSITION]);

  u32 weightsVbo;
  glGenBuffers(1, &weightsVbo);
  glBindBuffer(GL_ARRAY_BUFFER, weightsVbo);
  // NOTES(mudit): GL_DYNAMIC_DRAW as the weights change every frame
  glBufferData(GL_ARRAY_BUFFER, sizeof(weights), weights, GL_DYNAMIC_DRAW);

  i32 vertexWeightLoc = GetShaderLocationAttrib(pointShader, "vertexWeight");
  glVertexAttribPointer(vertexWeightLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexWeightLoc);

  // NOTES(mudit): reset enabled buffer to none so that we don't mess
  // up the data inside the last bound buffer by mistake
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  // NOTES(mudit): reset enabled vertex array to none to ensure we
  // don't mess up or change the data in the vao we are currently
  // configuring
  glBindVertexArray(0);

  // NOTES(mudit): Without this, setting gl_PointSize in vertex shader
  // does not work and instead you must use glPointSize(float size) in
  // CPU side code before calling glDrawArrays(GL_POINTS, ...) for
  // example.
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  while(!WindowShouldClose()) {
    if (IsKeyPressed(KEY_F)) {
      drawFps = !drawFps;
    }

    t = (f32)GetTime();
    updateWeights(t);

    DeferScope(BeginDrawing(), EndDrawing()) {
      ClearBackground(BGCOLOR);
      // NOTES(mudit): Force raylib to flush its internal batch
      rlDrawRenderBatchActive();

      // NOTES(mudit): Bind the vbo we want to update and update it
      // using glBufferSubData. We don't need to unbind it as we
      // unbind the VAO altogether
      glBindBuffer(GL_ARRAY_BUFFER, weightsVbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(weights), weights);

      DeferScope(glUseProgram(pointShader.id), glUseProgram(0)) {
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, POINTSCOUNT);
        glBindVertexArray(0);
      }

      if (drawFps) {
        DrawFPS(20, 20);
        DrawText(pointCountText, 20, 40, 20, LIME);
      }
    }
  }

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &posVbo);
  glDeleteBuffers(1, &weightsVbo);

  UnloadShader(pointShader);
  CloseWindow();

  return 0;
}
