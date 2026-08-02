#include <assert.h>
#include <box3d/box3d.h>
#include <ctype.h>
#include <raylib/raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SEGMENTS_PER_LEVEL 100
#define MAX_ROAD_OBJECTS_PER_SEGMENT 100
#define MAX_VISIBLE_SEGMENTS 2
#define DISTANCE_BETWEEN_SHIP_AND_CAMERA 10.0f
#define CAMERA_TARGET_Z_DISTANCE 19.0f
#define GRAVITY 4.0f * -9.80665f
#define TUNNEL_SLICES 9

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static const float SCR_WIDTH = 640.0f;
static const float SCR_HEIGHT = 480.0f;
static const char *WINDOW_TITLE = "SkyRoads Clone";

typedef enum {
    SR_ROAD_OBJECT_NONE = 0,
    SR_ROAD_OBJECT_LANE = 1,
    SR_ROAD_OBJECT_TUNNEL = 2
} srRoadObjectType;

typedef struct {
  Vector3 initialPosition;
  Vector3 size;
  Color color;
  srRoadObjectType type;
  b3BodyId box3DBodyId;
  Model model;
  bool isLast;
} srRoadObject;

typedef struct {
  int totalRoadObjects;
  srRoadObject roadObjects[MAX_ROAD_OBJECTS_PER_SEGMENT];
} srRoadSegment;

int totalSegments = 0;
int currentSegmentIdx = 0;
srRoadSegment segments[MAX_SEGMENTS_PER_LEVEL];

void loadLevel(int level) {
  char filename[64];
  snprintf(filename, sizeof(filename), "level%d.dat", level);

  FILE *fp = fopen(filename, "r");
  assert(fp != NULL && "The file with the level data does not exists.");

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
        if (currentRoadSegment != NULL && currentRoadSegment->totalRoadObjects > 0) {
            currentRoadSegment->roadObjects[currentRoadSegment->totalRoadObjects - 1].isLast = true;
        }

        assert(totalSegments < MAX_SEGMENTS_PER_LEVEL &&
               "Reached the max number of segments allowed per level.");

        currentRoadSegment = &segments[totalSegments];
        currentRoadSegment->totalRoadObjects = 0;
        totalSegments++;

        continue;
    }

    assert(currentRoadSegment != NULL &&
           "No first segment is defined before defining road objects.");

    assert(currentRoadSegment->totalRoadObjects < MAX_ROAD_OBJECTS_PER_SEGMENT &&
           "Reached the max number of road objects allowed per segment.");

    float px, py, pz;
    float sx, sy, sz;
    int colorValue;
    int type;
    int parsed = sscanf(p, "%f,%f,%f,%f,%f,%f,%d,%d", &px, &py, &pz, &sx, &sy,
                        &sz, &colorValue, &type);

    assert(parsed == 8 && "Found a road object with an invalid number of parameters. "
                          "Road objects must have 8 parameters.");

    srRoadObject *obj = &currentRoadSegment->roadObjects[currentRoadSegment->totalRoadObjects];

    obj->initialPosition = (Vector3){px, py, pz};
    obj->size = (Vector3){sx, sy, sz};
    obj->type = (srRoadObjectType)type;

    switch (colorValue) {
    case 0:
      obj->color = BLACK;
      break;
    case 1:
      obj->color = BLUE;
      break;
    case 2:
      obj->color = RED;
      break;
    case 3:
      obj->color = GREEN;
      break;
    case 4:
      obj->color = YELLOW;
      break;
    case 5:
      obj->color = ORANGE;
      break;
    case 6:
      obj->color = PURPLE;
      break;
    case 7:
      obj->color = PINK;
      break;
    case 8:
      obj->color = WHITE;
      break;
    case 9:
      obj->color = GRAY;
      break;
    case 10:
      obj->color = BROWN;
      break;
    default:
      obj->color = WHITE;
      break;
    }

    memset(&obj->box3DBodyId, 0, sizeof(obj->box3DBodyId));
    obj->isLast = false;
    currentRoadSegment->totalRoadObjects++;
  }

  if (currentRoadSegment != NULL && currentRoadSegment->totalRoadObjects > 0) {
      currentRoadSegment->roadObjects[currentRoadSegment->totalRoadObjects - 1].isLast = true;
  }

  fclose(fp);
}

b3BodyId createLaneBody(b3WorldId worldId, Vector3 lanePos, Vector3 laneSize) {
  b3BodyDef laneBodyDef = b3DefaultBodyDef();
  laneBodyDef.type = b3_staticBody;
  laneBodyDef.position = (b3Pos){lanePos.x, lanePos.y, lanePos.z};
  b3BodyId bodyId = b3CreateBody(worldId, &laneBodyDef);
  b3BoxHull laneStaticBox = b3MakeBoxHull(laneSize.x * 0.5f, laneSize.y * 0.5f, laneSize.z * 0.5f);
  b3ShapeDef laneShapeDef = b3DefaultShapeDef();
  laneShapeDef.baseMaterial.restitution = 0.02f;
  laneShapeDef.baseMaterial.friction = 0.2f;
  laneShapeDef.density = 10.0f; // Set the box density to be non-zero, so it will be dynamic.
  b3CreateHullShape(bodyId, &laneShapeDef, &laneStaticBox.base);
  return bodyId;
}

b3BodyId createTunnelBody(b3WorldId worldId, Vector3 tunnelPos, Vector3 tunnelSize) {
  b3BodyDef bodyDef = b3DefaultBodyDef();
  bodyDef.type = b3_staticBody;
  bodyDef.position = (b3Vec3){tunnelPos.x, tunnelPos.y, tunnelPos.z};
  b3BodyId bodyId = b3CreateBody(worldId, &bodyDef);
  b3ShapeDef shapeDef = b3DefaultShapeDef();
  shapeDef.baseMaterial.friction = 0.2f;
  shapeDef.baseMaterial.restitution = 0.0f;

  const float radius = tunnelSize.x * 0.5f;
  const float thickness = tunnelSize.x * 0.05f;
  const float deltaAngle = 20.0f * DEG2RAD;
  const float plateWidth = 2.0f * radius * tanf(deltaAngle * 0.5f);
  const float startAngle = -80.0f * DEG2RAD;

  for (int i = 0; i < TUNNEL_SLICES; i++) {
    float angle = startAngle + i * deltaAngle;
    float x = radius * sinf(angle);
    float y = radius * cosf(angle);

    b3BoxHull box = b3MakeBoxHull(plateWidth * 0.5f, thickness * 0.5f, tunnelSize.z * 0.5f);
    b3Transform transform;
    transform.p = (b3Vec3){x, y, 0.0f};
    transform.q = b3MakeQuatFromAxisAngle((b3Vec3){0.0f, 0.0f, 1.0f}, -angle);
    b3Vec3 scale = {1.0f, 1.0f, 1.0f};
    b3CreateTransformedHullShape(bodyId, &shapeDef, &box.base, transform, scale);
  }

  return bodyId;
}

Model createTunnelModel(Vector3 tunnelSize) {
  Mesh mesh = {0};
  mesh.vertexCount = TUNNEL_SLICES * 8;
  mesh.triangleCount = TUNNEL_SLICES * 12;
  mesh.vertices = malloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.normals  = malloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.indices  = malloc(mesh.triangleCount * 3 * sizeof(unsigned short));

  float radius = tunnelSize.x * 0.5f;
  float thickness = 0.15f;
  float segmentAngle = 20.0f * DEG2RAD;
  float pieceWidth = 2.0f * radius * tanf(segmentAngle * 0.5f);
  int vertexOffset = 0;
  int indexOffset = 0;

  for (int i = 0; i < TUNNEL_SLICES; i++) {
    float angle = (-80.0f + i * 20.0f) * DEG2RAD;
    float cx = radius * sinf(angle);
    float cy = radius * cosf(angle);
    float halfX = pieceWidth * 0.5f;
    float halfY = thickness * 0.5f;
    float halfZ = tunnelSize.z * 0.5f;

    Vector3 corners[8] = {
      {-halfX, -halfY, -halfZ},
      { halfX, -halfY, -halfZ},
      { halfX,  halfY, -halfZ},
      {-halfX,  halfY, -halfZ},

      {-halfX, -halfY,  halfZ},
      { halfX, -halfY,  halfZ},
      { halfX,  halfY,  halfZ},
      {-halfX,  halfY,  halfZ}
    };

    for (int v = 0; v < 8; v++) {
      float x = corners[v].x;
      float y = corners[v].y;
      float rx = x * cosf(angle) + y * sinf(angle);
      float ry = -x * sinf(angle) + y * cosf(angle);

      mesh.vertices[(vertexOffset + v) * 3 + 0] = cx + rx;
      mesh.vertices[(vertexOffset + v) * 3 + 1] = cy + ry;
      mesh.vertices[(vertexOffset + v) * 3 + 2] = corners[v].z;

      mesh.normals[(vertexOffset + v) * 3 + 0] = 0.0f;
      mesh.normals[(vertexOffset + v) * 3 + 1] = 1.0f;
      mesh.normals[(vertexOffset + v) * 3 + 2] = 0.0f;
    }

    unsigned short base = vertexOffset;
    unsigned short cubeIndices[36] = {
      0,1,2, 0,2,3,
      4,6,5, 4,7,6,

      0,4,5, 0,5,1,
      1,5,6, 1,6,2,

      2,6,7, 2,7,3,
      4,0,3, 4,3,7
    };

    for (int j = 0; j < 36; j++) {
      mesh.indices[indexOffset++] = base + cubeIndices[j];
    }

    vertexOffset += 8;
  }

  UploadMesh(&mesh, false);
  Model model = LoadModelFromMesh(mesh); // Model loaded to GPU

  free(mesh.vertices);
  free(mesh.normals);
  free(mesh.indices);
  return model;
}

void initSegment(int segmentIdx, b3WorldId worldId) {
  for (int j = 0; j < segments[segmentIdx].totalRoadObjects; j++) {
    srRoadObject *obj = &segments[segmentIdx].roadObjects[j];

    if(obj->type == SR_ROAD_OBJECT_LANE) {
      obj->box3DBodyId = createLaneBody(worldId, obj->initialPosition, obj->size);
    } else if (obj->type == SR_ROAD_OBJECT_TUNNEL) {
      obj->box3DBodyId = createTunnelBody(worldId, obj->initialPosition, obj->size);
      obj->model = createTunnelModel(obj->size);
    } else {
      assert(false && "Unknown road object type specified in the level data file.");
    }
  }
}

void initNextVisibleSegment(b3WorldId worldId) {
  int nextSegmentIdx = MAX_VISIBLE_SEGMENTS - currentSegmentIdx;
  initSegment(nextSegmentIdx, worldId);
}

b3BodyId createShipBody(b3WorldId worldId, Vector3 shipPos, Vector3 shipSize)
{
  b3BodyDef shipBodyDef = b3DefaultBodyDef();
  shipBodyDef.type = b3_dynamicBody;
  shipBodyDef.position = (b3Pos){
    shipPos.x,
    shipPos.y,
    shipPos.z
  };

  b3BodyId shipBodyId = b3CreateBody(worldId, &shipBodyDef);
  b3MotionLocks shipBodyLocks = {0};

  shipBodyLocks.angularX = true;
  shipBodyLocks.angularY = true;
  shipBodyLocks.angularZ = true;

  b3Body_SetMotionLocks(shipBodyId, shipBodyLocks);
  b3ShapeDef shipShapeDef = b3DefaultShapeDef();

  shipShapeDef.enableContactEvents = true;
  shipShapeDef.enableHitEvents = true;
  shipShapeDef.baseMaterial.restitution = 0.0f;
  shipShapeDef.baseMaterial.friction = 0.2f;
  shipShapeDef.density = 20.0f;

  b3BoxHull shipBox = b3MakeBoxHull(
    shipSize.x * 0.5f,
    shipSize.y * 0.5f,
    shipSize.z * 0.5f
  );

  b3CreateHullShape(
    shipBodyId,
    &shipShapeDef,
    &shipBox.base
  );

  float sphereRadius = 0.15f;
  float offsetX = shipSize.x * 0.30f;
  float offsetZ = shipSize.z * 0.30f;
  float offsetY = -shipSize.y * 0.50f;

  b3Sphere sphere;
  sphere.radius = sphereRadius;
  sphere.center = (b3Vec3){
    -offsetX,
    offsetY,
    -offsetZ
  };

  b3CreateSphereShape(
    shipBodyId,
    &shipShapeDef,
    &sphere
  );

  sphere.center = (b3Vec3){
    offsetX,
    offsetY,
    -offsetZ
  };

  b3CreateSphereShape(
    shipBodyId,
    &shipShapeDef,
    &sphere
  );

  sphere.center = (b3Vec3){
    -offsetX,
    offsetY,
    offsetZ
  };

  b3CreateSphereShape(
    shipBodyId,
    &shipShapeDef,
    &sphere
  );

  sphere.center = (b3Vec3){
    offsetX,
    offsetY,
    offsetZ
  };

  b3CreateSphereShape(
    shipBodyId,
    &shipShapeDef,
    &sphere
  );

  return shipBodyId;
}

static Vector3 rotateAroundZ(Vector3 p, float angle) {
  float c = cosf(angle);
  float s = sinf(angle);

  return (Vector3){
    p.x * c - p.y * s,
    p.x * s + p.y * c,
    p.z
  };
}

static void drawBoxWiresRotated(Vector3 center, Vector3 size, float rotation, Color color) {
  float hx = size.x * 0.5f;
  float hy = size.y * 0.5f;
  float hz = size.z * 0.5f;

  Vector3 vertices[8] = {
    {-hx, -hy, -hz},
    { hx, -hy, -hz},
    { hx,  hy, -hz},
    {-hx,  hy, -hz},

    {-hx, -hy,  hz},
    { hx, -hy,  hz},
    { hx,  hy,  hz},
    {-hx,  hy,  hz}
  };

  for(int i = 0; i < 8; i++) {
    vertices[i] = rotateAroundZ(vertices[i], rotation);
    vertices[i].x += center.x;
    vertices[i].y += center.y;
    vertices[i].z += center.z;
  }

  int edges[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0}, // bottom face
    {4,5}, {5,6}, {6,7}, {7,4}, // upper face
    {0,4}, {1,5}, {2,6}, {3,7} // laterals
  };

  for(int i = 0; i < 12; i++) {
    DrawLine3D(vertices[edges[i][0]], vertices[edges[i][1]], color);
  }
}

void drawTunnelWires(Vector3 tunnelPos, Vector3 tunnelSize, Color color) {
  float radius = tunnelSize.x * 0.5f;
  float thickness = tunnelSize.x * 0.06f;
  float segmentAngle = 20.0f * DEG2RAD;
  float pieceWidth = 2.0f * radius * tanf(segmentAngle * 0.5f);

  for(int i = 0; i < TUNNEL_SLICES; i++) {
    float angle = (-80.0f + i * 20.0f) * DEG2RAD;
    float x = radius * sinf(angle);
    float y = radius * cosf(angle);
    float rotation = -angle;

    Vector3 pieceCenter = {
      tunnelPos.x + x,
      tunnelPos.y + y,
      tunnelPos.z
    };

    drawBoxWiresRotated(pieceCenter, (Vector3){pieceWidth, thickness, tunnelSize.z}, rotation, color);
  }
}

int main() {
  b3WorldDef worldDef = b3DefaultWorldDef();
  worldDef.gravity = (b3Vec3){0.0f, GRAVITY, 0.0f};
  worldDef.restitutionThreshold = 0.1f;
  b3WorldId worldId = b3CreateWorld(&worldDef);

  Vector3 shipPos = (Vector3){5.5f, 1.5f, 10.0f};
  Vector3 shipSize = (Vector3){1.0f, 1.0f, 1.0f};
  b3BodyId shipBodyId = createShipBody(worldId, shipPos, shipSize);

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
      b3Body_ApplyForceToCenter(shipBodyId, (b3Pos){0.0f, 25000.0f, 0.0f}, true);
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
      for (int j = 0; j < segments[i].totalRoadObjects; j++) {
        srRoadObject *obj = &segments[i].roadObjects[j];
        b3Pos pos = b3Body_GetPosition(obj->box3DBodyId);

        if(obj->type == SR_ROAD_OBJECT_LANE) {
          DrawCube((Vector3){pos.x, pos.y, pos.z}, obj->size.x, obj->size.y, obj->size.z, obj->color);
          DrawCubeWires((Vector3){pos.x, pos.y, pos.z}, obj->size.x, obj->size.y, obj->size.z, BLACK);
        } else if (obj->type == SR_ROAD_OBJECT_TUNNEL) {
          DrawModel(obj->model, (Vector3){pos.x, pos.y, pos.z}, 1.0f, obj->color);
          drawTunnelWires((Vector3){pos.x, pos.y, pos.z}, (Vector3){obj->size.x, obj->size.y, obj->size.z}, BLACK);
        }
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
