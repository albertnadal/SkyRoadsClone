#include <assert.h>
#include <stdio.h>
#include "common.h"
#include "level.h"
#include "lane.h"
#include "tunnel.h"

int totalSegments;
int currentSegmentIdx;
srRoadSegment segments[MAX_SEGMENTS_PER_LEVEL];

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
  shipBodyDef.position = (b3Pos){shipPos.x, shipPos.y, shipPos.z};

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
  shipShapeDef.density = 200.0f;

  b3BoxHull shipBox = b3MakeBoxHull(shipSize.x * 0.5f, shipSize.y * 0.5f, shipSize.z * 0.5f);
  b3CreateHullShape(shipBodyId, &shipShapeDef, &shipBox.base);

  float sphereRadius = 0.15f;
  float offsetX = shipSize.x * 0.30f;
  float offsetZ = shipSize.z * 0.30f;
  float offsetY = -shipSize.y * 0.50f;

  b3Sphere sphere;
  sphere.radius = sphereRadius;
  sphere.center = (b3Vec3){-offsetX, offsetY, -offsetZ};
  b3CreateSphereShape(shipBodyId, &shipShapeDef, &sphere);

  sphere.center = (b3Vec3){offsetX, offsetY, -offsetZ};
  b3CreateSphereShape(shipBodyId, &shipShapeDef, &sphere);

  sphere.center = (b3Vec3){-offsetX, offsetY, offsetZ};
  b3CreateSphereShape(shipBodyId, &shipShapeDef, &sphere);

  sphere.center = (b3Vec3){offsetX, offsetY, offsetZ};
  b3CreateSphereShape(shipBodyId, &shipShapeDef, &sphere);

  return shipBodyId;
}

int main() {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(SCR_WIDTH, SCR_HEIGHT, WINDOW_TITLE);
  SetTargetFPS(60);

  RenderTexture2D target = LoadRenderTexture(RES_WIDTH, RES_HEIGHT);
  SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

  Texture2D background = LoadTexture("images/level1.png");
  Rectangle bgSize = {0, 0, (float)background.width, (float)background.height};

  b3WorldDef worldDef = b3DefaultWorldDef();
  worldDef.gravity = (b3Vec3){0.0f, GRAVITY, 0.0f};
  worldDef.restitutionThreshold = 0.1f;
  b3WorldId worldId = b3CreateWorld(&worldDef);

  Vector3 shipPos = (Vector3){5.5f, 1.5f, 10.0f};
  Vector3 shipSize = (Vector3){1.33f, 0.5f, 0.7f};
  b3BodyId shipBodyId = createShipBody(worldId, shipPos, shipSize);
  b3Pos shipPosition;
  b3Vec3 shipSpeed;
  b3Vec3 shipEngineForce = {0.0f, 0.0f, 0.0f},
         shipLateralForce = {0.0f, 0.0f, 0.0f};
  Model shipModel = LoadModel("models/ship.glb");
  bool shipOnGround = false;
  b3ContactEvents events;

  // Prepare for simulation. Typically we use a time step of 1/60 of a
  // second (60Hz) and 4 sub-steps. This provides a high quality simulation
  // in most game scenarios.
  float timeStep = 1.0f / 60.0f;
  int subStepCount = 4;

  loadLevel(1);
  for (int s = 0; s < MAX_VISIBLE_SEGMENTS; s++) {
    initSegment(s, worldId);
  }

  Camera3D camera = {0};
  camera.position = (Vector3){5.5f, 7.5f, shipPos.z + DISTANCE_BETWEEN_SHIP_AND_CAMERA};
  camera.target = (Vector3){5.5f, 2.5f, camera.position.z - CAMERA_TARGET_Z_DISTANCE};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 40.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  while (!WindowShouldClose()) {

    if (IsKeyDown(KEY_LEFT)) {
      shipLateralForce.x = -1500.0f;
    } else if (IsKeyDown(KEY_RIGHT)) {
      shipLateralForce.x = 1500.0f;
    } else {
      shipLateralForce.x = 0.0f;
    }

    if (IsKeyDown(KEY_UP)) {
      shipEngineForce.z = -2000.0f;
    } else if (IsKeyDown(KEY_DOWN)) {
      shipEngineForce.z = 200.0f;
    }

    if (IsKeyReleased(KEY_UP) || IsKeyReleased(KEY_DOWN)) {
      shipEngineForce.z = 0.0f;
    }

    if (IsKeyPressed(KEY_SPACE)) {
      b3Body_ApplyForceToCenter(shipBodyId, (b3Pos){0.0f, 90000.0f, 0.0f}, true);
    }

    b3Body_ApplyForceToCenter(shipBodyId, shipEngineForce, true);
    b3Body_ApplyForceToCenter(shipBodyId, shipLateralForce, true);

    b3World_Step(worldId, timeStep, subStepCount);
    events = b3World_GetContactEvents(worldId);
    shipSpeed = b3Body_GetLinearVelocity(shipBodyId);

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

    shipPosition = b3Body_GetPosition(shipBodyId);

    BeginTextureMode(target);
    ClearBackground(BLACK);
    DrawTexturePro(background, bgSize, bgSize, (Vector2){0,0}, 0.0f, WHITE);

    BeginMode3D(camera);

    DrawModelEx(
      shipModel,
      (Vector3){shipPosition.x, shipPosition.y - 0.25f, shipPosition.z},
      (Vector3){0.0f, 0.0f, 0.0f},
      0.0f,
      (Vector3){0.3f, 0.3f, 0.3f},
      WHITE
    );

    //DrawCube((Vector3){shipPosition.x, shipPosition.y, shipPosition.z}, shipSize.x, shipSize.y, shipSize.z, GREEN);
    if (DEBUG) {
      DrawCubeWires((Vector3){shipPosition.x, shipPosition.y, shipPosition.z}, shipSize.x, shipSize.y, shipSize.z, BLACK);
    }

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
    EndTextureMode();

    BeginDrawing();
    DrawTexturePro(
        target.texture,
        (Rectangle){0, 0, RES_WIDTH, -RES_HEIGHT},
        (Rectangle){0, 0, SCR_WIDTH, SCR_HEIGHT},
        (Vector2){0,0},
        0.0f,
        WHITE
    );
    EndDrawing();
    // IF Z position of the Ship is higher than the Z position of the last lane of the current segment then 
    // call the function initNextVisibleSegment(worldId) to load the bodies of the next visible segment.
  }

  UnloadModel(shipModel);
  UnloadTexture(background);
  CloseWindow();
  b3DestroyWorld(worldId);
  return 0;
}
