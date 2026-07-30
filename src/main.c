#include <assert.h>
#include <box3d/box3d.h>
#include <ctype.h>
#include <raylib/raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SEGMENTS_PER_LEVEL 100
#define MAX_LANES_PER_SEGMENT 100
#define MAX_VISIBLE_SEGMENTS 2
#define DISTANCE_BETWEEN_SHIP_AND_CAMERA 10.0f
#define CAMERA_TARGET_Z_DISTANCE 19.0f
#define GRAVITY 4.0f * -9.80665f

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static const float SCR_WIDTH = 640.0f;
static const float SCR_HEIGHT = 480.0f;
static const char *WINDOW_TITLE = "SkyRoads Clone";

typedef struct {
  Vector3 initialPosition;
  Vector3 size;
  Color color;
  b3BodyId box3DBodyId;
  bool isLast;
} srLane;

typedef struct {
  int totalLanes;
  srLane lanes[MAX_LANES_PER_SEGMENT];
} srRoadSegment;

int totalSegments = 0;
int currentSegmentIdx = 0;
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
        if (currentRoadSegment != NULL && currentRoadSegment->totalLanes > 0) {
            currentRoadSegment->lanes[currentRoadSegment->totalLanes - 1].isLast = true;
        }

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
    case 10:
      lane->color = BROWN;
      break;
    default:
      lane->color = WHITE;
      break;
    }

    memset(&lane->box3DBodyId, 0, sizeof(lane->box3DBodyId));
    lane->isLast = false;
    currentRoadSegment->totalLanes++;
  }

  if (currentRoadSegment != NULL && currentRoadSegment->totalLanes > 0) {
      currentRoadSegment->lanes[currentRoadSegment->totalLanes - 1].isLast = true;
  }

  fclose(fp);
}

void initSegment(int segmentIdx, b3WorldId worldId) {
  for (int j = 0; j < segments[segmentIdx].totalLanes; j++) {
    srLane *lane = &segments[segmentIdx].lanes[j];
    b3BodyDef laneBodyDef = b3DefaultBodyDef();
    laneBodyDef.type = b3_staticBody;
    laneBodyDef.position = (b3Pos){lane->initialPosition.x, lane->initialPosition.y, lane->initialPosition.z};
    lane->box3DBodyId = b3CreateBody(worldId, &laneBodyDef);
    b3BoxHull laneStaticBox = b3MakeBoxHull(lane->size.x * 0.5f, lane->size.y * 0.5f, lane->size.z * 0.5f);
    b3ShapeDef laneShapeDef = b3DefaultShapeDef();
    laneShapeDef.baseMaterial.restitution = 0.02f;
    laneShapeDef.baseMaterial.friction = 0.2f;
    laneShapeDef.density = 10.0f; // Set the box density to be non-zero, so it will be dynamic.
    b3CreateHullShape(lane->box3DBodyId, &laneShapeDef, &laneStaticBox.base);
  }
}

void initNextVisibleSegment(b3WorldId worldId) {
  int nextSegmentIdx = MAX_VISIBLE_SEGMENTS - currentSegmentIdx;
  initSegment(nextSegmentIdx, worldId);
}

int main() {
  b3WorldDef worldDef = b3DefaultWorldDef();
  worldDef.gravity = (b3Vec3){0.0f, GRAVITY, 0.0f};
  worldDef.restitutionThreshold = 0.1f;
  b3WorldId worldId = b3CreateWorld(&worldDef);

  Vector3 shipSize = (Vector3){1.0f, 1.0f, 1.0f};
  Vector3 shipPos = (Vector3){5.5f, 1.5f, 10.0f};

  b3BodyDef shipBodyDef = b3DefaultBodyDef();
  shipBodyDef.type = b3_dynamicBody;
  shipBodyDef.position = (b3Pos){shipPos.x, shipPos.y, shipPos.z};
  b3BodyId shipBodyId = b3CreateBody(worldId, &shipBodyDef);
  b3MotionLocks shipBodyLocks = {0};
  shipBodyLocks.angularX = true;
  shipBodyLocks.angularY = true;
  shipBodyLocks.angularZ = true;
  b3Body_SetMotionLocks (shipBodyId, shipBodyLocks);
  b3BoxHull shipStaticBox = b3MakeBoxHull(shipSize.x * 0.5f, shipSize.y * 0.5f, shipSize.z * 0.5f);
  b3ShapeDef shipShapeDef = b3DefaultShapeDef();
  shipShapeDef.enableContactEvents = true;
  shipShapeDef.enableHitEvents = true;
  shipShapeDef.baseMaterial.restitution = 0.0f;
  shipShapeDef.baseMaterial.friction = 0.2f;
  shipShapeDef.density = 20.0f; // Set the box density to be non-zero, so it will be dynamic.
  b3CreateHullShape(shipBodyId, &shipShapeDef, &shipStaticBox.base);

  b3ContactEvents events;

  // Prepare for simulation. Typically we use a time step of 1/60 of a
  // second (60Hz) and 4 sub-steps. This provides a high quality simulation
  // in most game scenarios.
  float timeStep = 1.0f / 60.0f;
  int subStepCount = 4;

  InitWindow(SCR_WIDTH, SCR_HEIGHT, WINDOW_TITLE);
  SetTargetFPS(60);

  Camera3D camera = {0};
  camera.position = (Vector3){5.5f, 6.0f, shipPos.z + DISTANCE_BETWEEN_SHIP_AND_CAMERA};
  camera.target = (Vector3){5.5f, 1.0f, camera.position.z - CAMERA_TARGET_Z_DISTANCE};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 70.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  b3Vec3 engineForce = {0.0f, 0.0f, 0.0f},
         lateralForce = {0.0f, 0.0f, 0.0f};

  loadLevel(1);
  for (int s = 0; s < MAX_VISIBLE_SEGMENTS; s++) {
    initSegment(s, worldId);
  }

  bool shipOnGround = false;

  while (!WindowShouldClose()) {

    if (IsKeyDown(KEY_LEFT)) {
      lateralForce.x = -300.0f;
    } else if (IsKeyDown(KEY_RIGHT)) {
      lateralForce.x = 300.0f;
    } else {
      lateralForce.x = 0.0f;
    }

    if (IsKeyDown(KEY_UP)) {
      engineForce.z = -300.0f;
    } else if (IsKeyDown(KEY_DOWN)) {
      engineForce.z = 20.0f;
    }

    if (IsKeyReleased(KEY_UP) || IsKeyReleased(KEY_DOWN)) {
      engineForce.z = 0.0f;
    }

    if (IsKeyPressed(KEY_SPACE)) {
      b3Body_ApplyForceToCenter(shipBodyId, (b3Pos){0.0f, 20000.0f, 0.0f}, true);
    }

    b3Body_ApplyForceToCenter(shipBodyId, engineForce, true);
    b3Body_ApplyForceToCenter(shipBodyId, lateralForce, true);

    b3World_Step(worldId, timeStep, subStepCount);
    events = b3World_GetContactEvents(worldId);

    for(int i = 0; i < events.hitCount; i++) {
      b3ContactHitEvent *hit = &events.hitEvents[i];
      if(hit->normal.z == 1.0f) {
        printf("FRONTAL COLLISION\n");
        // TODO: The ship had a frontal collision with a static object.
        //       Destroy the ship and restart the level.
      }
    }

    for(int i = 0; i < events.beginCount; i++) {
      b3ContactBeginTouchEvent *event = &events.beginEvents[i];
      if (b3Contact_IsValid(event->contactId)) {
        b3ContactData data = b3Contact_GetData(event->contactId);
        for(int j = 0; j < data.manifoldCount; j++) {
          if(data.manifolds[j].normal.y == 1.0f) {
            printf("SHIP IS ON THE GROUND\n");
            shipOnGround = true;
          }
        }
      }
    }

    for(int i = 0; i < events.endCount; i++) {
      b3ContactEndTouchEvent *event = &events.endEvents[i];
      if (b3Shape_IsValid(event->shapeIdA) && b3Shape_IsValid(event->shapeIdB)) {
        b3AABB aabbA = b3Shape_GetAABB(event->shapeIdA);
        b3AABB aabbB = b3Shape_GetAABB(event->shapeIdB);

        if((aabbA.upperBound.y < aabbB.lowerBound.y) ||
           ((aabbA.upperBound.y > aabbB.lowerBound.y) && (aabbA.lowerBound.z > aabbB.upperBound.z)) ||
           ((aabbA.upperBound.y > aabbB.lowerBound.y) && (aabbA.lowerBound.x > aabbB.upperBound.x)) ||
           ((aabbA.upperBound.y > aabbB.lowerBound.y) && (aabbA.upperBound.x < aabbB.lowerBound.x))) {
            printf("SHIP IS NOT ON THE GROUND\n");
          shipOnGround = false;
        }
      }
    }


    b3Pos shipPosition = b3Body_GetPosition(shipBodyId);

    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(camera);
    //DrawGrid(80, 1.0f);

    DrawCube((Vector3){shipPosition.x, shipPosition.y, shipPosition.z},
             shipSize.x, shipSize.y, shipSize.z, GREEN);
    DrawCubeWires((Vector3){shipPosition.x, shipPosition.y, shipPosition.z},
                  shipSize.x, shipSize.y, shipSize.z, BLACK);

    camera.position.z = shipPosition.z + DISTANCE_BETWEEN_SHIP_AND_CAMERA;
    camera.target.z = camera.position.z - CAMERA_TARGET_Z_DISTANCE;

    for (int i = currentSegmentIdx; i < MIN(currentSegmentIdx + MAX_VISIBLE_SEGMENTS, totalSegments); i++ ) {
      for (int j = 0; j < segments[i].totalLanes; j++) {
        srLane *lane = &segments[i].lanes[j];
        b3Pos pos = b3Body_GetPosition(lane->box3DBodyId);
        DrawCube((Vector3){pos.x, pos.y, pos.z}, lane->size.x, lane->size.y, lane->size.z, lane->color);
        DrawCubeWires((Vector3){pos.x, pos.y, pos.z}, lane->size.x, lane->size.y, lane->size.z, BLACK);
      }
    }

    EndMode3D();
    DrawFPS(16, 16);
    EndDrawing();
    // IF Z position of the Ship is higher than the Z position of the last lane of the current segment then 
    // call the function initNextVisibleSegment(worldId) to load the bodies of the next visible segment.
  }

  CloseWindow();
  b3DestroyWorld(worldId);
  return 0;
}
