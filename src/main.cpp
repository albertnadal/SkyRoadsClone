#include <box3d/box3d.h>
#include <raylib/raylib.h>

constexpr float SCR_WIDTH = 640.0f;
constexpr float SCR_HEIGHT = 480.0f;
constexpr const char *WINDOW_TITLE = "SkyRoads Clone";

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

  b3Vec3 shipForce = { 0.0f, 0.0f, -10.0f };

  while (!WindowShouldClose()) {
    b3Body_ApplyForceToCenter(shipBodyId, shipForce, true);        
    b3World_Step(worldId, timeStep, subStepCount);
    b3Pos lanePosition = b3Body_GetPosition(laneBodyId);
    b3Pos shipPosition = b3Body_GetPosition(shipBodyId);

    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(camera);
      DrawGrid(20, 1.0f);
      DrawCube((Vector3){lanePosition.x, lanePosition.y, lanePosition.z}, laneSize.x, laneSize.y, laneSize.z, BLUE);
      DrawCubeWires((Vector3){lanePosition.x, lanePosition.y, lanePosition.z}, laneSize.x, laneSize.y, laneSize.z, BLACK);

      DrawCube((Vector3){shipPosition.x, shipPosition.y, shipPosition.z}, shipSize.x, shipSize.y, shipSize.z, GREEN);
      DrawCubeWires((Vector3){shipPosition.x, shipPosition.y, shipPosition.z}, shipSize.x, shipSize.y, shipSize.z, BLACK);
    EndMode3D();
    DrawFPS(16, 16);
    EndDrawing();
  }

  CloseWindow();
  b3DestroyWorld(worldId);
  return 0;
}
