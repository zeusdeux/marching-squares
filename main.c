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

#define WIDTH 1280
#define HEIGHT 720

#define GRIDLEN     10
#define HALFGRIDLEN (GRIDLEN/2)
#define COLS        (WIDTH/GRIDLEN+1)
#define ROWS        (HEIGHT/GRIDLEN+1)

#define POINTSCOUNT     COLS*ROWS
#define POINTRADIUS     5.f
#define POINTFMT        "(x, y, weight, addr) = (%6.1f, %6.1f, %1.1f, %p)"
#define POINTFMTARGS(p) (p).pos.x, (p).pos.y, (p).weight, ((void *)&(p))

#define BGCOLOR            (Color){0,    0,    0,    0}
#define POINTCOLOR(weight) (Color){0xFF, 0,    0,    0xFF*(weight)}
#define TEXTCOLOR(weight)  (Color){0xFF, 0xFF, 0xFF, 0xFF*(weight)}
#define LINECOLOR(weight)  (Color){0x00, 0x33, 0xAA, 0xFF*(weight)}

// Simplex3D related
#define DEFAULT_FEATURESIZE 0.006f
#define DEFAULT_RATEOFCHANGE 0.1f
#define DEFAULT_ISOVAL 0.f

typedef struct {
  Vector2 pos;
  f32 weight;
} Point;

static Point points[POINTSCOUNT] = {0};

void updatePoints(f64 t, f32 featureSize, f32 rateOfChange)
{
  for (u32 y = 0; y < ROWS; y++) {
    for (u32 x = 0; x < COLS; x++) {
      points[x+(y*COLS)] = (Point){
        (Vector2){x*GRIDLEN*1.f, y*GRIDLEN*1.f},
        simplex3d(x*GRIDLEN*featureSize, y*GRIDLEN*featureSize, t*rateOfChange),
      };
    }
  }
}

u8 getState(f32 isoVal, const Point a, const Point b, const Point c, const Point d)
{
  u8 state = 0;

  if (a.weight > isoVal) {
    state |= 1;
  }

  if (b.weight > isoVal) {
    state |= 2;
  }

  if (c.weight > isoVal) {
    state |= 4;
  }

  if (d.weight > isoVal) {
    state |= 8;
  }

  #if 0
  printf("POINT: CLOCKWISE FROM LEFT CORNER");
  printf("\n\ta: "POINTFMT, POINTFMTARGS(a));
  printf("\n\tb: "POINTFMT, POINTFMTARGS(b));
  printf("\n\tc: "POINTFMT, POINTFMTARGS(c));
  printf("\n\td: "POINTFMT"\n", POINTFMTARGS(d));
  printf("\tSTATE: %u\n", state);
  #endif

  return state;
}


int main(void)
{
  char text[10] = {0};
  bool drawDebug = false;
  bool drawText = false;
  bool play = true;

  // marching squares controls
  f32 isoVal = DEFAULT_ISOVAL;
  // Simplex3D related controls
  f32 featureSize  = DEFAULT_FEATURESIZE;
  f32 rateOfChange = DEFAULT_RATEOFCHANGE;

  updatePoints(1, featureSize, rateOfChange);

  InitWindow(WIDTH, HEIGHT, "Marching squares contouring simplex3d noise");
  SetTargetFPS(60);

  while(!WindowShouldClose()) {
    if (IsKeyPressed(KEY_Q)) {
      break;
    }

    if (IsKeyPressed(KEY_SPACE)) {
      play = !play;
    }

    if (IsKeyPressed(KEY_D)) {
      drawDebug = !drawDebug;
    }

    if (IsKeyPressed(KEY_T)) {
      drawText = !drawText;
    }

    if (IsKeyPressed(KEY_R)) {
      featureSize = DEFAULT_FEATURESIZE;
      rateOfChange = DEFAULT_RATEOFCHANGE;
      isoVal = DEFAULT_ISOVAL;
    }

    if (IsKeyPressed(KEY_P) || IsKeyPressedRepeat(KEY_P)) {
      featureSize += 0.001f;
    }

    if (IsKeyPressed(KEY_O) || IsKeyPressedRepeat(KEY_O)) {
      featureSize -= 0.001f;

      if (featureSize < 0) {
        featureSize = DEFAULT_RATEOFCHANGE;
      }
    }

    if (IsKeyPressed(KEY_L) || IsKeyPressedRepeat(KEY_L)) {
      rateOfChange += 0.01f;
    }

    if (IsKeyPressed(KEY_K) || IsKeyPressedRepeat(KEY_K)) {
      rateOfChange -= 0.01f;

      if (rateOfChange < 0) {
        rateOfChange = DEFAULT_RATEOFCHANGE;
      }
    }

    if (IsKeyPressed(KEY_M) || IsKeyPressedRepeat(KEY_M)) {
      isoVal += 0.01f;
    }


    if (IsKeyPressed(KEY_N) || IsKeyPressedRepeat(KEY_N)) {
      isoVal -= 0.01f;

      if (isoVal < -1) {
        isoVal = -1;
      }
    }

    if (play) {
      f64 t = GetTime();
      updatePoints(t, featureSize, rateOfChange);
    }

    DeferScope(BeginDrawing(), EndDrawing()) {
      ClearBackground(BGCOLOR);

      if (drawDebug) {
        for (u32 i = 0; i < POINTSCOUNT; i++) {
          Point p = points[i];

          DrawCircleV(p.pos, POINTRADIUS, POINTCOLOR((p.weight+1)/2));
        }
      }

      if (drawText) {
        for (u32 i = 0; i < POINTSCOUNT; i++) {
          Point p = points[i];
          int spErr = snprintf(text, 6, "%1.2f", p.weight);

          if (spErr < 0) {
            fprintf(stderr, "ERROR: Failed to write point weight to string - %s", strerror(errno));
            return 1;
          }

          DrawText(text, p.pos.x, p.pos.y, 14, TEXTCOLOR(p.weight));
        }
      }

      for (u32 y = 0; y < ROWS-1; y++) {
        for (u32 x = 0; x < COLS-1; x++) {
          Point a = points[x+(y*COLS)];       // [x][y]
          Point b = points[x+1+(y*COLS)];     // [x+1][y]
          Point c = points[x+1+((y+1)*COLS)]; // [x+1][y+1]
          Point d = points[x+((y+1)*COLS)];   // [x][y+1]

          // Get lerp'd mid points of each line segment a->b, b->c, c->d, d->a
          // TODO(mudit): Calc only once as these midpoints do not change
          // per pair of points
          f32 abT = (isoVal - a.weight)/(b.weight - a.weight);
          Vector2 abMid = {Lerp(a.pos.x, b.pos.x, abT), a.pos.y};

          f32 bcT = (isoVal - b.weight)/(c.weight - b.weight);
          Vector2 bcMid = {b.pos.x, Lerp(b.pos.y, c.pos.y, bcT)};

          f32 cdT = (isoVal - c.weight)/(d.weight - c.weight);
          Vector2 cdMid = {Lerp(c.pos.x, d.pos.x, cdT), c.pos.y};

          f32 daT = (isoVal - d.weight)/(a.weight - d.weight);
          Vector2 daMid = {d.pos.x, Lerp(d.pos.y, a.pos.y, daT)};

          u8 state = getState(isoVal, a, b, c, d);

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
              printf("Error: INVALID STATE %u\n", state);
              return 1;
            } break;
          }
        }
      }
    }
  }

  return 0;
}
