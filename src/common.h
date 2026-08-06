#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <raylib/raylib.h>
#include <box3d/box3d.h>

#define DEBUG false
#define MAX_SEGMENTS_PER_LEVEL 100
#define MAX_ROAD_OBJECTS_PER_SEGMENT 100
#define MAX_VISIBLE_SEGMENTS 2
#define DISTANCE_BETWEEN_SHIP_AND_CAMERA 15.0f
#define CAMERA_TARGET_Z_DISTANCE 19.0f
#define GRAVITY 3 * -9.80665f
#define TUNNEL_SLICES 9
#define EXPLOSION_SPHERES_COUNT 50
#define INITIAL_SHIP_POSITION (Vector3){5.5f, 1.5f, 10.0f}
#define SHIP_FALL_LIMIT_Y -60.0f

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static const float SCR_WIDTH  = 1280.0f;
static const float SCR_HEIGHT = 800.0f;
static const float RES_WIDTH  = 1280.0f; //320
static const float RES_HEIGHT = 800.0f; //200
static const char WINDOW_TITLE[] = "SkyRoads Clone";

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

typedef struct {
  Vector3 initialPosition;
  float radius;
  Color color;
  unsigned char alpha;
  b3BodyId box3DBodyId;
} srExplosionSphere;

#endif