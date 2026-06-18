#include <stdio.h>
#include <stdlib.h>
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

#ifndef assertm
#define assertm(cond, ...)                                                                               \
  (cond) ?                                                                                               \
         ((void)0)                                                                                       \
       : (fprintf(stderr, "%s:%d:\t[%s] FAILED ASSERTION: %s => ", __FILE__, __LINE__, __func__, #cond), \
          fprintf(stderr, "\n\tREASON: "),                                                               \
          fprintf(stderr, __VA_ARGS__),                                                                  \
          fprintf(stderr, "\n"),                                                                         \
          abort())
#endif // assertm

#define DeferScope(startExpr, endExpr) \
  for(int DeferScope_i__ = (startExpr, 0); DeferScope_i__ == 0; (DeferScope_i__++, endExpr))

#define WIDTH  1280
#define HEIGHT 720
#define COLS   ((WIDTH/GRID_W)+1)
#define ROWS   ((HEIGHT/GRID_W)+1)

#ifndef GRID_W
#define GRID_W    3    // used in debug build
#endif // GRID_W

#ifndef POINTSIZE
#define POINTSIZE 8.0f // used in debug build
#endif // POINTSIZE

#define POINTSCOUNT   (COLS*ROWS)
#define MAXLINESCOUNT ((ROWS-1)*(COLS-1)*2) // each square can contain at max 2 lines

typedef uint8_t  u8;
typedef uint32_t u32;
typedef int32_t  i32;
typedef float    f32;
typedef double   f64;
typedef Vector2  Point;

static Point lines[MAXLINESCOUNT] = {0};
static Point points[POINTSCOUNT]  = {0};
static f32   weights[POINTSCOUNT] = {0};
static u8    states[POINTSCOUNT]  = {0};

static f32 featureSize  = 0.003f;
static f32 rateOfChange = 0.17f;
static f32 isoVal       = -0.5f;
static u32 linesCount   = 0;
static const Point res  = {WIDTH, HEIGHT};

void updateState(f32 t, bool calcLines)
{
  f32 Z = t*rateOfChange;
  f32 sizeFactor = GRID_W*featureSize;

  for (u32 y = 0; y < ROWS; y++) {
    f32 Y = y*sizeFactor;
    u32 yIdx = (y*COLS);

    for (u32 x = 0; x < COLS; x++) {
      f32 X = x*sizeFactor;
      f32 weight = simplex3d(X, Y, Z);

      u32 idx = x+yIdx;
      weights[idx] = weight;
      states[idx]  = (u8)(weight < isoVal);
    }
  }

  linesCount = 0;

  if (calcLines) {
    u32 lineIdx = 0;

    // TODO(mudit): Merge these loops into the one above where
    // we sample simplex3d for weights per point
    for (u32 y = 0; y < ROWS-1; y++) {
      u32 yIdx = (y*COLS);
      u32 yNextIdx = (y+1)*COLS;
      // TODO(mudit): a and d can be calc out here and then in
      // the loop we calc and use b and c and at end of inner
      // loop a = b, c = d
      for (u32 x = 0; x < COLS-1; x++) {
        u32 aIdx = x+yIdx;       // [x][y]
        u32 bIdx = x+1+yIdx;     // [x+1][y]
        u32 cIdx = x+1+yNextIdx; // [x+1][y+1]
        u32 dIdx = x+yNextIdx;   // [x][y+1]

        Point a = points[aIdx];
        Point b = points[bIdx];
        Point c = points[cIdx];
        Point d = points[dIdx];

        f32 aWt = weights[aIdx];
        f32 bWt = weights[bIdx];
        f32 cWt = weights[cIdx];
        f32 dWt = weights[dIdx];

        f32 abT     = (isoVal - aWt)/(bWt - aWt);
        Point abMid = {Lerp(a.x, b.x, abT), a.y};
        f32 bcT     = (isoVal - bWt)/(cWt - bWt);
        Point bcMid = {b.x, Lerp(b.y, c.y, bcT)};
        f32 cdT     = (isoVal - cWt)/(dWt - cWt);
        Point cdMid = {Lerp(c.x, d.x, cdT), c.y};
        f32 daT     = (isoVal - dWt)/(aWt - dWt);
        Point daMid = {d.x, Lerp(d.y, a.y, daT)};

        u8 aSt = states[aIdx];
        u8 bSt = states[bIdx];
        u8 cSt = states[cIdx];
        u8 dSt = states[dIdx];

        u8 state = aSt | bSt << 1 | cSt << 2 | dSt << 3;

        switch(state) {
          case 0:
          case 15: break;

          case 1:
          case 14: {
            lines[lineIdx++] = abMid;
            lines[lineIdx++] = daMid;
            // DrawLineV(abMid, daMid, LINECOLOR(1.f));
          } break;

          case 2:
          case 13: {
            lines[lineIdx++] = abMid;
            lines[lineIdx++] = bcMid;
            // DrawLineV(abMid, bcMid, LINECOLOR(1.f));
          } break;

          case 3:
          case 12: {
            lines[lineIdx++] = daMid;
            lines[lineIdx++] = bcMid;
            // DrawLineV(daMid, bcMid, LINECOLOR(1.f));
          } break;

          case 4:
          case 11: {
            lines[lineIdx++] = bcMid;
            lines[lineIdx++] = cdMid;
            // DrawLineV(bcMid, cdMid, LINECOLOR(1.f));
          } break;

          case 5: {
            lines[lineIdx++] = abMid;
            lines[lineIdx++] = daMid;
            // DrawLineV(abMid, daMid, LINECOLOR(1.f));
            lines[lineIdx++] = bcMid;
            lines[lineIdx++] = cdMid;
            // DrawLineV(bcMid, cdMid, LINECOLOR(1.f));
          } break;

          case 6:
          case 9: {
            lines[lineIdx++] = abMid;
            lines[lineIdx++] = cdMid;
            // DrawLineV(abMid, cdMid, LINECOLOR(1.f));
          } break;

          case 7:
          case 8: {
            lines[lineIdx++] = cdMid;
            lines[lineIdx++] = daMid;
            // DrawLineV(cdMid, daMid, LINECOLOR(1.f));
          } break;

          case 10: {
            lines[lineIdx++] = daMid;
            lines[lineIdx++] = cdMid;
            // DrawLineV(daMid, cdMid, LINECOLOR(1.f));
            lines[lineIdx++] = abMid;
            lines[lineIdx++] = bcMid;
            // DrawLineV(abMid, bcMid, LINECOLOR(1.f));
          } break;

          default: {
            assertm(0, "Error: INVALID STATE %u\n", state);
          } break;
        }
      }
    }

    assertm(lineIdx < MAXLINESCOUNT,
            "Expected max generated lines count to be %d, Received: %d", MAXLINESCOUNT, lineIdx);

    linesCount = lineIdx - 1;
  }
}

int main(void)
{
  char pointCountText[64] = {0};
  snprintf(pointCountText, 64, "%d points", POINTSCOUNT);

  char linesCountText[64] = {0};
  snprintf(linesCountText, 64, "%d lines", linesCount);

  f64 t             = 0;
  bool drawFps      = false;
  bool drawPoints   = true;
  bool drawContours = false;
  bool play         = true;

  // Setup points aka their screen positions
  for (u32 y = 0; y < ROWS; y++) {
    for (u32 x = 0; x < COLS; x++) {
      u32 idx = x+(y*COLS);
      points[idx] = (Point){x*GRID_W*1.f, y*GRID_W*1.f};
    }
  }

  // This is a fun one!
  // SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
  // this will cause a panic at resolutions higher than 1280x720
  // SetConfigFlags(FLAG_WINDOW_UNDECORATED);
  SetConfigFlags(FLAG_WINDOW_HIGHDPI);
  SetConfigFlags(FLAG_MSAA_4X_HINT);

  DeferScope(InitWindow(WIDTH, HEIGHT, "Marching squares contouring Simplex3D noise"), CloseWindow()) {
    SetTargetFPS(120);

    // USED VBOs SETUP
    // Point positions VBO
    u32 posVbo;
    glGenBuffers(1, &posVbo);
    glBindBuffer(GL_ARRAY_BUFFER, posVbo);
      // NOTES(mudit): GL_STATIC_DRAW as the points data does not change
      glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // NOTES(mudit): Screen space vertices of the contour lines coming
    // from the marching squares algo
    u32 linesVbo;
    glGenBuffers(1, &linesVbo);
    glBindBuffer(GL_ARRAY_BUFFER, linesVbo);
      // NOTES(mudit): GL_DYNAMIC_DRAW as the weights change every frame
      glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // NOTES(mudit): This VBO is shared by both linesVao and pointsVao
    // Sampled weights from simplex3D noise per point
    u32 weightsVbo;
    glGenBuffers(1, &weightsVbo);
    glBindBuffer(GL_ARRAY_BUFFER, weightsVbo);
      // NOTES(mudit): GL_DYNAMIC_DRAW as the weights change every frame
      glBufferData(GL_ARRAY_BUFFER, sizeof(weights), weights, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    // SHARED SHADER UNIFORM, ATTRIB LOCS and IDs
    i32 resolutionUniformLoc = -1;
    i32 pointSizeUniformLoc = -1;
    i32 vertexPositionAttribLoc = -1;
    i32 vertexWeightAttribLoc = -1;


    // NOTES(mudit): Draws the contouring lines coming from marching
    // squares stored in the lines array
    Shader lineShader = LoadShader("line.vert", "line.frag");
    // NOTES(mudit): Raylib silently fails when using LoadShader with
    // non-existent file paths for example and even IsShaderValid()
    // returns 1 as raylib silently loads its default shader and returns
    // that as the value for LoadShader(...). Hence the check below
    assertm(IsShaderValid(lineShader) && lineShader.id != rlGetShaderIdDefault(), "Invalid shader: line shader");

    resolutionUniformLoc = GetShaderLocation(lineShader, "resolution");
      assertm(resolutionUniformLoc != -1, "Could not find uniform `resolution` in line shader");
      SetShaderValue(lineShader, resolutionUniformLoc, &res, SHADER_UNIFORM_VEC2);
    resolutionUniformLoc = -1;

    // LINES VAO SETUP
    u32 linesVao;
    glGenVertexArrays(1, &linesVao);
    DeferScope(glBindVertexArray(linesVao), glBindVertexArray(0)) {
      DeferScope(glBindBuffer(GL_ARRAY_BUFFER, linesVbo), glBindBuffer(GL_ARRAY_BUFFER, 0)) {
        vertexPositionAttribLoc = lineShader.locs[SHADER_LOC_VERTEX_POSITION];
          assertm(vertexPositionAttribLoc != -1, "Line shader: Could not find attribute `vertexPosition`");
          glVertexAttribPointer(vertexPositionAttribLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
          glEnableVertexAttribArray(vertexPositionAttribLoc);
        vertexPositionAttribLoc = -1;
      }

      DeferScope(glBindBuffer(GL_ARRAY_BUFFER, weightsVbo), glBindBuffer(GL_ARRAY_BUFFER, 0)) {
        vertexWeightAttribLoc = GetShaderLocationAttrib(lineShader, "vertexWeight");
          assertm(vertexWeightAttribLoc != -1, "Line shader: Could not find attribute `vertexWeight`");
          glVertexAttribPointer(vertexWeightAttribLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
          glEnableVertexAttribArray(vertexWeightAttribLoc);
        vertexWeightAttribLoc = -1;
      }
    }


    // NOTES(mudit): Draws the sampled weights from simplex3d in the
    // grid given by points array
    Shader pointShader = LoadShader("point.vert", "point.frag");
    assertm(IsShaderValid(pointShader) && pointShader.id != rlGetShaderIdDefault(), "Invalid shader: point shader");

    // NOTES(mudit): GetShaderLocation gets a uniform location by name
    // whereas to get an attribute location, use GetShaderLocationAttrib(...)
    resolutionUniformLoc = GetShaderLocation(pointShader, "resolution");
      assertm(resolutionUniformLoc != -1, "Point shader: Could not find uniform `resolution`");
      SetShaderValue(pointShader, resolutionUniformLoc, &res, SHADER_UNIFORM_VEC2);
    resolutionUniformLoc = -1;

    float pointSize = POINTSIZE;
    pointSizeUniformLoc = GetShaderLocation(pointShader, "pointSize");
      assertm(pointSizeUniformLoc != -1, "Point shader: Could not find uniform `pointSize`");
      SetShaderValue(pointShader, pointSizeUniformLoc, &pointSize, SHADER_UNIFORM_FLOAT);
    pointSizeUniformLoc = -1;

    // NOTES(mudit): Learnt from examples/others/raylib_opengl_interop.c
    // VAO -> basically holds the config for the VBOs that are enabled
    // below it. This is so that we can directly use all VBOs
    // referenced by this VAO via glBindVertexArray(vao); prior to
    // issuing a draw call rather than having to call:
    // glBindBuffer -> glVertexAttribPointer -> glEnableVertexAttribArray -> glDrawArrays -> glBindBuffer(..., 0) aka disable
    // wherein the first three calls need to happen for each vbo and the
    // attrib it's being passed to in the vertex shader
    u32 pointsVao;
    glGenVertexArrays(1, &pointsVao);
    // NOTES(mudit): glBindVertexArray(0) to unbind enabled vertex array
    // to ensure we don't mess up or change the data in the vao we are
    // currently configuring
    DeferScope(glBindVertexArray(pointsVao), glBindVertexArray(0)) {
      // NOTES(mudit): glBindBuffer(GL_ARRAY_BUFFER, 0) to unbind buffer
      // so that we don't mess up the data inside the last bound buffer
      // by subsequent calls to glBufferData(...) for example
      DeferScope(glBindBuffer(GL_ARRAY_BUFFER, posVbo), glBindBuffer(GL_ARRAY_BUFFER, 0)) {
        vertexPositionAttribLoc = pointShader.locs[SHADER_LOC_VERTEX_POSITION];
          assertm(vertexPositionAttribLoc != -1, "Point shader: Could not find attribute `vertexPosition`");
          // NOTES(mudit): set vertexPosition attribute of vertex shader. This
          // attrib name "vertexPosition" is required by default by raylib
          // internally. Also, stride is 0 instead of sizeof(*points) aka
          // sizeof(float)*2 as 0 tells opengl that the data is tightly packed
          // i.e., points here contains (x,y)(x,y)(x,y).. and not
          // (x,y,texCoords)(x,y,texCoords)(x,y,texCoords).. or something else
          // where each vertex is more than just a 2D vertex position
          glVertexAttribPointer(vertexPositionAttribLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
          // NOTES(mudit): These don't need to be disabled as the state of
          // this vertex attrib is stored in our vao and we unbind the whole
          // vao after draw anyway. Even if we didn't, and we switch out vaos
          // for multiple draws, vaos don't leak into each other as to use a
          // different vao, we have to bind it first which is equivalent to
          // glBindVertexArray(0) only that we pass the other vao instead of 0
          glEnableVertexAttribArray(vertexPositionAttribLoc);
        vertexPositionAttribLoc = -1;
      }

      DeferScope(glBindBuffer(GL_ARRAY_BUFFER, weightsVbo), glBindBuffer(GL_ARRAY_BUFFER, 0)) {
        vertexWeightAttribLoc = GetShaderLocationAttrib(pointShader, "vertexWeight");
          assertm(vertexWeightAttribLoc != -1, "Point shader: Could not find attribute `vertexWeight`");
          glVertexAttribPointer(vertexWeightAttribLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
          glEnableVertexAttribArray(vertexWeightAttribLoc);
        vertexWeightAttribLoc = -1;
      }
    }

    // NOTES(mudit): Without this, setting gl_PointSize in vertex shader
    // does not work and instead you must use glPointSize(float size) in
    // CPU side code before calling glDrawArrays(GL_POINTS, ...) for
    // example.
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SetExitKey(KEY_Q);

    while(!WindowShouldClose()) {
      // inputs
      {
        // modifiers
        bool rightControl = IsKeyDown(KEY_RIGHT_CONTROL);

        // animation control
        {
          bool keyC = IsKeyPressed(KEY_C);

          if (!rightControl && keyC) {
            drawContours = !drawContours;
          }

          if (rightControl && keyC) {
            drawPoints = !drawPoints;
          }

          bool keyS = IsKeyPressed(KEY_S) || IsKeyPressedRepeat(KEY_S);

          if (!rightControl && keyS) {
            featureSize += 0.0001f;
          }

          if (rightControl && keyS) {
            featureSize -= 0.0001f;

            if (featureSize < 0.f) {
              featureSize = 0;
            }
          }

          bool keyT = IsKeyPressed(KEY_T) || IsKeyPressedRepeat(KEY_T);

          if (!rightControl && keyT) {
            rateOfChange += 0.01f;
          }

          if (rightControl && keyT) {
            rateOfChange -= 0.01f;

            if (rateOfChange < 0) {
              rateOfChange = 0;
            }
          }

          bool keyI = IsKeyPressed(KEY_I) || IsKeyPressedRepeat(KEY_I);

          if (!rightControl && keyI) {
            isoVal += 0.01f;

            if (isoVal > 1.f) {
              isoVal = 1.f;
            }
          }

          if (rightControl && keyI) {
            isoVal -= 0.01f;

            if (isoVal < -1.f) {
              isoVal = -1.f;
            }
          }

          bool keyP = IsKeyPressed(KEY_P) || IsKeyPressedRepeat(KEY_P);

          if (!rightControl && keyP) {
            pointSize += 1.f;

            // max gl_PointSize on macos is 64.0
            if (pointSize > 64.f) {
              pointSize = 64.f;
            }
          }

          if (rightControl && keyP) {
            pointSize -= 1.f;

            // min gl_PointSize on macos is 1.f but that
            // just makes everything disappear so clamping
            // at 2.0f
            if (pointSize < 2.f) {
              pointSize = 2.f;
            }
          }

          if (rightControl && IsKeyPressed(KEY_G)) {
            featureSize  = 0.003f;
            rateOfChange = 0.17f;
            isoVal       = -0.5f;
            pointSize    = POINTSIZE;
          }
        }

        // perf/debug
        if (IsKeyPressed(KEY_F)) {
          drawFps = !drawFps;
        }

        // play/pause
        if (IsKeyPressed(KEY_SPACE)) {
          play = !play;
        }

        if (play) {
          t = (f32)GetTime();
          updateState(t, drawContours);

        }
      }

      // draw
      DeferScope(BeginDrawing(), EndDrawing()) {
        ClearBackground(BLACK);
        // NOTES(mudit): Force raylib to flush its internal batch
        rlDrawRenderBatchActive();

        if (drawPoints) {
          // NOTES(mudit): Bind the vbo we want to update and update it
          // using glBufferSubData. We don't need to unbind it as we
          // unbind the VAO altogether
          glBindBuffer(GL_ARRAY_BUFFER, weightsVbo);
          glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(weights), weights);

          DeferScope(glUseProgram(pointShader.id), glUseProgram(0)) {
            pointSizeUniformLoc = GetShaderLocation(pointShader, "pointSize");
            glUniform1f(pointSizeUniformLoc, pointSize);

            glBindVertexArray(pointsVao);
            glDrawArrays(GL_POINTS, 0, POINTSCOUNT);
            glBindVertexArray(0);
          }
        }

        if (drawContours) {
          glBindBuffer(GL_ARRAY_BUFFER, linesVbo);
          glBufferSubData(GL_ARRAY_BUFFER, 0, linesCount*sizeof(*lines), lines);

          DeferScope(glUseProgram(lineShader.id), glUseProgram(0)) {
            glBindVertexArray(linesVao);
            glDrawArrays(GL_LINES, 0, linesCount);
            glBindVertexArray(0);
          }
        }

        if (drawFps) {
          snprintf(linesCountText, 64, "%d lines", linesCount);

          DrawFPS(20, 20);
          DrawText(pointCountText, 20, 40, 20, LIME);
          DrawText(linesCountText, 20, 60, 20, LIME);
        }
      }
    }

    // cleanup
    {
      glDeleteVertexArrays(1, &linesVao);
      glDeleteBuffers(1, &linesVbo);

      glDeleteVertexArrays(1, &pointsVao);
      glDeleteBuffers(1, &posVbo);
      glDeleteBuffers(1, &weightsVbo);

      UnloadShader(pointShader);
      UnloadShader(lineShader);
    }
  }

  return 0;
}
