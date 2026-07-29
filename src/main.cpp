#include <assert.h>
#include <box3d/box3d.h>
#include <ctype.h>
#include <raylib/raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SEGMENTS_PER_LEVEL 100
#define MAX_LANES_PER_SEGMENT 100

static const float SCR_WIDTH = 640.0f;
static const float SCR_HEIGHT = 480.0f;
static const char *WINDOW_TITLE = "SkyRoads Clone";

typedef struct {
  Vector3 initialPosition;
  Vector3 size;
  Color color;
  b3BodyId box3DBody;
} srLane;

typedef struct {
  int totalLanes;
  srLane lanes[MAX_LANES_PER_SEGMENT];
} srRoadSegment;

int totalSegments = 0;
int currentSegment = 0;
srRoadSegment segments[MAX_SEGMENTS_PER_LEVEL];

void loadLevel(int level) {
  char filename[64];
  snprintf(filename, sizeof(filename), "level%d.dat", level);

  FILE *fp = fopen(filename, "r");
  assert(fp != NULL && "El fitxer de nivell no existeix.");

  totalSegments = 0;
  srRoadSegment *currentRoadSegment = NULL;
  char line[512];

  while (fgets(line, sizeof(line), fp) != NULL) {
    line[strcspn(line, "\r\n")] = '\0';

    char *comment = strstr(line, "//");
    if (comment != NULL)
      *comment = '\0';

    char *p = line;
    while (isspace((unsigned char)*p))
      ++p;

    if (*p == '\0')
      continue;

    if (*p == '#') {
      assert(totalSegments < MAX_SEGMENTS_PER_LEVEL &&
             "Reached the max number of segments allowed per level.");

      currentRoadSegment = &segments[totalSegments];
      currentRoadSegment->totalLanes = 0;
      totalSegments++;
      continue;
    }

    assert(currentRoadSegment != NULL &&
           "No first segment is defined before defining lanes.");

    assert(currentRoadSegment->totalLanes < MAX_LANES_PER_SEGMENT &&
           "Reached the max number of lanes allowed per segment.");

    float px, py, pz;
    float sx, sy, sz;
    int colorValue;
    int type;
    int parsed = sscanf(p, "%f,%f,%f,%f,%f,%f,%d,%d", &px, &py, &pz, &sx, &sy,
                        &sz, &colorValue, &type);

    assert(parsed == 8 && "Found a lane with an invalid number of parameters. "
                          "Lanes must have 8 parameters.");

    srLane *lane = &currentRoadSegment->lanes[currentRoadSegment->totalLanes];

    lane->initialPosition = (Vector3){px, py, pz};
    lane->size = (Vector3){sx, sy, sz};

    switch (colorValue) {
    case 0:
      lane->color = BLACK;
      break;
    case 1:
      lane->color = BLUE;
      break;
    case 2:
      lane->color = RED;
      break;
    case 3:
      lane->color = GREEN;
      break;
    case 4:
      lane->color = YELLOW;
      break;
    case 5:
      lane->color = ORANGE;
      break;
    case 6:
      lane->color = PURPLE;
      break;
    case 7:
      lane->color = PINK;
      break;
    case 8:
      lane->color = WHITE;
      break;
    case 9:
      lane->color = GRAY;
      break;
    default:
      lane->color = WHITE;
      break;
    }

    memset(&lane->box3DBody, 0, sizeof(lane->box3DBody));
    currentRoadSegment->totalLanes++;
  }

  fclose(fp);
}

int main() {
  b3WorldDef worldDef = b3DefaultWorldDef();
  worldDef.gravity = (b3Vec3){0.0f, -9.80665f, 0.0f};

  b3WorldId worldId = b3CreateWorld(&worldDef);

  Vector3 laneSize = (Vector3){3.0f, 1.0f, 6.0f};
  Vector3 lanePos = (Vector3){5.0f, 0.5f, 0.0f};

  b3BodyDef laneBodyDef = b3DefaultBodyDef();
  laneBodyDef.type = b3_staticBody;
  laneBodyDef.position = (b3Pos){lanePos.x, lanePos.y, lanePos.z};
  b3BodyId laneBodyId = b3CreateBody(worldId, &laneBodyDef);
  b3BoxHull laneStaticBox = b3MakeBoxHull(laneSize.x * 0.5f, laneSize.y * 0.5f, laneSize.z * 0.5f);
  b3ShapeDef laneShapeDef = b3DefaultShapeDef();
  laneShapeDef.baseMaterial.friction = 0.3f;
  laneShapeDef.density = 0.0f; // Set the box density to be non-zero, so it will be dynamic.
  b3CreateHullShape(laneBodyId, &laneShapeDef, &laneStaticBox.base);

  Vector3 shipSize = (Vector3){1.0f, 1.0f, 1.0f};
  Vector3 shipPos = (Vector3){5.5f, 1.5f, 2.0f};

  b3BodyDef shipBodyDef = b3DefaultBodyDef();
  shipBodyDef.type = b3_dynamicBody;
  shipBodyDef.position = (b3Pos){shipPos.x, shipPos.y, shipPos.z};
  b3BodyId shipBodyId = b3CreateBody(worldId, &shipBodyDef);
  b3BoxHull shipStaticBox = b3MakeBoxHull(shipSize.x * 0.5f, shipSize.y * 0.5f, shipSize.z * 0.5f);
  b3ShapeDef shipShapeDef = b3DefaultShapeDef();
  shipShapeDef.density = 1.0f; // Set the box density to be non-zero, so it will be dynamic.
  b3CreateHullShape(shipBodyId, &shipShapeDef, &shipStaticBox.base);

  // Prepare for simulation. Typically we use a time step of 1/60 of a
  // second (60Hz) and 4 sub-steps. This provides a high quality simulation
  // in most game scenarios.
  float timeStep = 1.0f / 60.0f;
  int subStepCount = 4;

  InitWindow(SCR_WIDTH, SCR_HEIGHT, WINDOW_TITLE);
  SetTargetFPS(60);

  Camera3D camera = {0};
  camera.position = (Vector3){5.0f, 4.0f, 10.0f};
  camera.target = (Vector3){5.0f, 1.0f, 1.0f};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  b3Vec3 shipForce = {0.0f, 0.0f, -10.0f};

  loadLevel(1);

  while (!WindowShouldClose()) {
    b3Body_ApplyForceToCenter(shipBodyId, shipForce, true);
    b3World_Step(worldId, timeStep, subStepCount);
    b3Pos lanePosition = b3Body_GetPosition(laneBodyId);
    b3Pos shipPosition = b3Body_GetPosition(shipBodyId);

    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(camera);
    DrawGrid(20, 1.0f);
    DrawCube((Vector3){lanePosition.x, lanePosition.y, lanePosition.z},
             laneSize.x, laneSize.y, laneSize.z, BLUE);
    DrawCubeWires((Vector3){lanePosition.x, lanePosition.y, lanePosition.z},
                  laneSize.x, laneSize.y, laneSize.z, BLACK);

    DrawCube((Vector3){shipPosition.x, shipPosition.y, shipPosition.z},
             shipSize.x, shipSize.y, shipSize.z, GREEN);
    DrawCubeWires((Vector3){shipPosition.x, shipPosition.y, shipPosition.z},
                  shipSize.x, shipSize.y, shipSize.z, BLACK);
    EndMode3D();
    DrawFPS(16, 16);
    EndDrawing();
  }

  CloseWindow();
  b3DestroyWorld(worldId);
  return 0;
}
