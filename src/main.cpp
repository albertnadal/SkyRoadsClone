#include <raylib/raylib.h>
#include <box3d/box3d.h>

constexpr float SCR_WIDTH = 640.0f;
constexpr float SCR_HEIGHT = 480.0f;
constexpr const char *WINDOW_TITLE = "SkyRoads Clone";

int main() {
  // Construct a world object, which will hold and simulate the rigid bodies.
  b3WorldDef worldDef = b3DefaultWorldDef();
  worldDef.gravity = (b3Vec3){0.0f, -10.0f, 0.0f};

  b3WorldId worldId = b3CreateWorld(&worldDef);

  // Define the ground body.
  b3BodyDef groundBodyDef = b3DefaultBodyDef();
  groundBodyDef.position = (b3Pos){0.0f, -10.0f, 0.0f};

  // Call the body factory which allocates memory for the ground body
  // from a pool and creates the ground box shape (also from a pool).
  // The body is also added to the world.
  b3BodyId groundId = b3CreateBody(worldId, &groundBodyDef);

  // Define the ground box shape. The extents are the half-widths of the box.
  b3BoxHull groundBox = b3MakeBoxHull(50.0f, 10.0f, 50.0f);

  // Add the box shape to the ground body.
  b3ShapeDef groundShapeDef = b3DefaultShapeDef();
  b3CreateHullShape(groundId, &groundShapeDef, &groundBox.base);

  // Define the dynamic body. We set its position and call the body factory.
  b3BodyDef bodyDef = b3DefaultBodyDef();
  bodyDef.type = b3_dynamicBody;
  bodyDef.position = (b3Pos){0.0f, 4.0f, 0.0f};

  b3BodyId bodyId = b3CreateBody(worldId, &bodyDef);

  // Define another box shape for our dynamic body.
  b3BoxHull dynamicBox = b3MakeCubeHull(1.0f);

  // Define the dynamic body shape
  b3ShapeDef shapeDef = b3DefaultShapeDef();

  // Set the box density to be non-zero, so it will be dynamic.
  shapeDef.density = 1.0f;

  // Override the default friction.
  shapeDef.baseMaterial.friction = 0.3f;

  // Add the shape to the body.
  b3CreateHullShape(bodyId, &shapeDef, &dynamicBox.base);

  // Prepare for simulation. Typically we use a time step of 1/60 of a
  // second (60Hz) and 4 sub-steps. This provides a high quality simulation
  // in most game scenarios.
  float timeStep = 1.0f / 60.0f;
  int subStepCount = 4;

  b3Pos position = b3Body_GetPosition(bodyId);
  b3Quat rotation = b3Body_GetRotation(bodyId);

  // This is our little game loop.
  for (int i = 0; i < 90; ++i) {
    // Instruct the world to perform a single step of simulation.
    // It is generally best to keep the time step and iterations fixed.
    b3World_Step(worldId, timeStep, subStepCount);

    // Now print the position and angle of the body.
    position = b3Body_GetPosition(bodyId);
    rotation = b3Body_GetRotation(bodyId);
  }

  // When the world destructor is called, all bodies and joints are freed. This
  // can create orphaned ids, so be careful about your world management.
  b3DestroyWorld(worldId);

  InitWindow(SCR_WIDTH, SCR_HEIGHT, WINDOW_TITLE);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawFPS(16, 16);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
