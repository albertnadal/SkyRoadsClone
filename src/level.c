#include "level.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Color getColorFromId(int colorId) {
  switch (colorId) {
    case 0:  return BLACK;
    case 1:  return BLUE;
    case 2:  return RED;
    case 3:  return GREEN;
    case 4:  return YELLOW;
    case 5:  return ORANGE;
    case 6:  return PURPLE;
    case 7:  return PINK;
    case 8:  return WHITE;
    case 9:  return GRAY;
    case 10: return BROWN;
    default: return WHITE;
  }
}

void unloadCurrentLevel(srRoadObject objects[], int* totalObjects) {
  for (int roadObjectIdx = 0; roadObjectIdx < *totalObjects; roadObjectIdx++) {
    srRoadObject *roadObject = &objects[roadObjectIdx];
    if (b3Body_IsValid(roadObject->box3DBodyId)) {
      b3DestroyBody(roadObject->box3DBodyId);
    }
  }
  *totalObjects = 0;
}

void initLevelObjects(b3WorldId worldId, srRoadObject objects[], int* totalObjects) {
  for (int j = 0; j < *totalObjects; j++) {
    srRoadObject *obj = &objects[j];

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

void loadLevel(int level, b3WorldId worldId, srRoadObject objects[], int* totalObjects) {
  unloadCurrentLevel(objects, totalObjects);

  char filename[64];
  snprintf(filename, sizeof(filename), "levels/level%d.dat", level);

  FILE *fp = fopen(filename, "r");
  assert(fp != NULL && "The file with the level data does not exist.");

  char line[512];

  while (fgets(line, sizeof(line), fp) != NULL) {
    assert(*totalObjects < MAX_ROAD_OBJECTS_PER_LEVEL && "Reached the max number of road objects allowed per level.");

    line[strcspn(line, "\r\n")] = '\0';
    char *comment = strstr(line, "//");
    if (comment != NULL) {
      *comment = '\0';
    }

    char *p = line;

    while (isspace((unsigned char)*p)) {
      p++;
    }

    if (*p == '\0') {
      continue;
    }

    float px, py, pz;
    float sx, sy, sz;
    int colorValue, type;
    int parsed = sscanf(p, "%f,%f,%f,%f,%f,%f,%d,%d", &px, &py, &pz, &sx, &sy, &sz, &colorValue, &type);

    assert(parsed == 8 && "Found a road object with an invalid number of parameters.");
    srRoadObject *obj = &objects[*totalObjects];

    obj->initialPosition = (Vector3){px, py, pz};
    obj->size = (Vector3){sx, sy, sz};
    obj->color = getColorFromId(colorValue);
    obj->type = (srRoadObjectType)type;
    obj->model = (Model){0};
    memset(&obj->box3DBodyId, 0, sizeof(obj->box3DBodyId));
    (*totalObjects)++;
  }
  fclose(fp);
  initLevelObjects(worldId, objects, totalObjects);
}