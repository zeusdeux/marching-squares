#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "raylib.h"
#include "raymath.h"

#define ZDX_SIMPLEX_3D_IMPLEMENTATION
#include "./simplex3d.h"

typedef uint8_t  u8;
typedef uint32_t u32;
typedef float    f32;
typedef double   f64;

#define DeferScope(startExpr, endExpr) \
  for(int DeferScope_i__ = (startExpr, 0); DeferScope_i__ == 0; (DeferScope_i__++, endExpr))

#define WIDTH       1280
#define HEIGHT      720

#define GRIDLEN     10
#define HALFGRIDLEN (GRIDLEN/2)
#define COLS        (WIDTH/GRIDLEN+1)
#define ROWS        (HEIGHT/GRIDLEN+1)

#define POINTSCOUNT COLS*ROWS
#define POINTRADIUS 6.f

// Marching squares controls
#define DEFAULT_ISOVAL       -0.5f
// Simplex3D related
#define DEFAULT_FEATURESIZE  0.003f
#define DEFAULT_RATEOFCHANGE 0.17f

#define BGCOLOR            (Color){0,    0,    0,    0}
#define TEXTCOLOR(weight)  (Color){0xFF, 0xFF, 0xFF, 0xFF*(weight)}
#define LINECOLOR(weight)  (Color){0x00, 0x33, 0xAA, 0xFF*(weight)}
#define INV(n)             (1.f-(n))
#define QB(n)              ((n)*(n)*(n))
#define POINTCOLOR(weight) (Color){0xFF*(weight), 0xFF*QB(weight), 0xFF*(weight)*INV(weight)*2, 0xFF}

typedef Vector2 Point;

static Point points[POINTSCOUNT]  = {0};
static f32   weights[POINTSCOUNT] = {0};
static u8    states[POINTSCOUNT]  = {0};

void updatePoints(f32 isoVal, f64 t, f32 featureSize, f32 rateOfChange)
{
  for (u32 y = 0; y < ROWS; y++) {
    for (u32 x = 0; x < COLS; x++) {
      u32 idx = x+(y*COLS);
      f32 weight = simplex3d(x*GRIDLEN*featureSize, y*GRIDLEN*featureSize, t*rateOfChange);

      points[idx] = (Point){x*GRIDLEN, y*GRIDLEN*1.f};
      weights[idx] = weight;
      states[idx] = (u8)(weight < isoVal);
    }
  }
}

int main(void)
{
  char text[15] = {0};

  bool drawFps      = false;
  bool drawPoints   = false;
  bool drawText     = false;
  bool drawContours = true;
  bool play         = true;

  // Marching squares controls
  f32 isoVal = DEFAULT_ISOVAL;
  // Simplex3D related controls
  f32 featureSize  = DEFAULT_FEATURESIZE;
  f32 rateOfChange = DEFAULT_RATEOFCHANGE;

  updatePoints(isoVal, 1, featureSize, rateOfChange);

  InitWindow(WIDTH, HEIGHT, "Marching squares contouring Simplex3D noise");
  SetTargetFPS(60);

  while(!WindowShouldClose()) {
    // Handle input
    {
      if (IsKeyPressed(KEY_Q)) {
        break;
      }

      if (IsKeyPressed(KEY_SPACE)) {
        play = !play;
      }

      if (IsKeyPressed(KEY_F)) {
        drawFps = !drawFps;
      }

      if (IsKeyPressed(KEY_D)) {
        drawPoints = !drawPoints;
      }

      if (IsKeyPressed(KEY_T)) {
        drawText = !drawText;
      }

      // reset
      if (IsKeyPressed(KEY_R)) {
        featureSize  = DEFAULT_FEATURESIZE;
        rateOfChange = DEFAULT_RATEOFCHANGE;
        isoVal       = DEFAULT_ISOVAL;
      }

      // feature size
      if (IsKeyPressed(KEY_P) || IsKeyPressedRepeat(KEY_P)) {
        featureSize += 0.0001f;
      }

      if (IsKeyPressed(KEY_O) || IsKeyPressedRepeat(KEY_O)) {
        featureSize -= 0.0001f;

        if (featureSize < 0.f) {
          featureSize = 0;
        }
      }

      // rate of change
      if (IsKeyPressed(KEY_L) || IsKeyPressedRepeat(KEY_L)) {
        rateOfChange += 0.005f;
      }

      if (IsKeyPressed(KEY_K) || IsKeyPressedRepeat(KEY_K)) {
        rateOfChange -= 0.005f;

        if (rateOfChange < 0) {
          rateOfChange = 0;
        }
      }

      // iso val
      if (IsKeyPressed(KEY_M) || IsKeyPressedRepeat(KEY_M)) {
        isoVal += 0.01f;

        if (isoVal > 1.f) {
          isoVal = 1.f;
        }
      }

      if (IsKeyPressed(KEY_N) || IsKeyPressedRepeat(KEY_N)) {
        isoVal -= 0.01f;

        if (isoVal < -1.f) {
          isoVal = -1.f;
        }
      }

      if (IsKeyPressed(KEY_C)) {
        drawContours = !drawContours;
      }
    }

    // Update state
    if (play) {
      f64 t = GetTime();
      updatePoints(isoVal, t, featureSize, rateOfChange);
    }

    // Draw
    {
      DeferScope(BeginDrawing(), EndDrawing()) {
        ClearBackground(BGCOLOR);

        if (drawPoints) {
          for (u32 i = 0; i < POINTSCOUNT; i++) {
            Point p = points[i];
            f32 weight = (weights[i]+1)/2.f;

            // TODO(mudit): Speed this up as it is mad slow
            DrawCircleV(p, POINTRADIUS, POINTCOLOR(weight));
          }
        }

        if (drawText) {
          for (u32 i = 0; i < POINTSCOUNT; i++) {
            Point p = points[i];
            f32 weight = (weights[i]+1)/2.f;
            int spErr = snprintf(text, 8, "%3.2f", weight);

            if (spErr < 0) {
              fprintf(stderr, "ERROR: Failed to write point weight to string - %s",
                      strerror(errno));
              return 1;
            }

            DrawText(text, p.x, p.y, 14, TEXTCOLOR(weight));
          }
        }

        if (drawContours) {
          for (u32 y = 0; y < ROWS-1; y++) {

            // TODO(mudit): a and d can be calc out here and then in
            // the loop we calc and use b and c and at end of inner
            // loop a = b, c = d
            for (u32 x = 0; x < COLS-1; x++) {
              int aIdx = x+(y*COLS);       // [x][y]
              int bIdx = x+1+(y*COLS);     // [x+1][y]
              int cIdx = x+1+((y+1)*COLS); // [x+1][y+1]
              int dIdx = x+((y+1)*COLS);   // [x][y+1]

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

              // this has duplications but I don't care for now
              switch(state) {
                case 0:  break;
                case 15: break;

                case 1: {
                  DrawLineV(abMid, daMid, LINECOLOR(1.f));
                } break;

                case 2: {
                  DrawLineV(abMid, bcMid, LINECOLOR(1.f));
                } break;

                case 3: {
                  DrawLineV(daMid, bcMid, LINECOLOR(1.f));
                } break;

                case 4: {
                  DrawLineV(bcMid, cdMid, LINECOLOR(1.f));
                } break;

                case 5: {
                  DrawLineV(abMid, daMid, LINECOLOR(1.f));
                  DrawLineV(bcMid, cdMid, LINECOLOR(1.f));
                } break;

                case 6: {
                  DrawLineV(abMid, cdMid, LINECOLOR(1.f));
                } break;

                case 7:
                case 8: {
                  DrawLineV(cdMid, daMid, LINECOLOR(1.f));
                } break;

                case 9: {
                  DrawLineV(abMid, cdMid, LINECOLOR(1.f));
                } break;

                case 10: {
                  DrawLineV(daMid, cdMid, LINECOLOR(1.f));
                  DrawLineV(abMid, bcMid, LINECOLOR(1.f));
                } break;

                case 11: {
                  DrawLineV(bcMid, cdMid, LINECOLOR(1.f));
                } break;

                case 12: {
                  DrawLineV(daMid, bcMid, LINECOLOR(1.f));
                } break;

                case 13: {
                  DrawLineV(abMid, bcMid, LINECOLOR(1.f));
                } break;

                case 14: {
                  DrawLineV(daMid, abMid, LINECOLOR(1.f));
                } break;

                default: {
                  fprintf(stderr, "Error: INVALID STATE %u\n", state);
                  return 1;
                } break;
              }
            }
          }
        }

        // Draw FPS on top of everything
        if (drawFps) {
          DrawFPS(20, 20);
        }
      }
    }
  }

  return 0;
}
