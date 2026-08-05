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

void unloadCurrentLevel() {
  for (int segmentIdx = 0; segmentIdx < totalSegments; segmentIdx++) {
    srRoadSegment *currentRoadSegment = &segments[segmentIdx];
    for (int roadObjectIdx = 0; roadObjectIdx < currentRoadSegment->totalRoadObjects; roadObjectIdx++) {
      srRoadObject *roadObject = &currentRoadSegment->roadObjects[roadObjectIdx];
      if (b3Body_IsValid(roadObject->box3DBodyId)) {
        b3DestroyBody(roadObject->box3DBodyId);
      }
    }
    currentRoadSegment->totalRoadObjects = 0;
  }

  totalSegments = 0;
}

void loadLevel(int level) {
  unloadCurrentLevel();

  char filename[64];
  snprintf(filename, sizeof(filename), "levels/level%d.dat", level);

  FILE *fp = fopen(filename, "r");
  assert(fp != NULL && "The file with the level data does not exist.");

  totalSegments = 0;
  srRoadSegment *currentRoadSegment = NULL;
  char line[512];

  while (fgets(line, sizeof(line), fp) != NULL) {
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

    if (*p == '#') {
      if (currentRoadSegment != NULL && currentRoadSegment->totalRoadObjects > 0) {
        currentRoadSegment->roadObjects[currentRoadSegment->totalRoadObjects - 1].isLast = true;
      }

      assert(totalSegments < MAX_SEGMENTS_PER_LEVEL && "Reached the max number of segments allowed per level.");
      currentRoadSegment = &segments[totalSegments];
      currentRoadSegment->totalRoadObjects = 0;
      totalSegments++;
      continue;
    }

    assert(currentRoadSegment != NULL && "No first segment is defined before defining road objects.");
    assert(currentRoadSegment->totalRoadObjects < MAX_ROAD_OBJECTS_PER_SEGMENT && "Reached the max number of road objects allowed per segment.");

    float px, py, pz;
    float sx, sy, sz;
    int colorValue, type;
    int parsed = sscanf(p, "%f,%f,%f,%f,%f,%f,%d,%d", &px, &py, &pz, &sx, &sy, &sz, &colorValue, &type);

    assert(parsed == 8 && "Found a road object with an invalid number of parameters.");
    srRoadObject *obj = &currentRoadSegment->roadObjects[currentRoadSegment->totalRoadObjects];

    obj->initialPosition = (Vector3){px, py, pz};
    obj->size = (Vector3){sx, sy, sz};
    obj->color = getColorFromId(colorValue);
    obj->type = (srRoadObjectType)type;
    obj->model = (Model){0};
    obj->isLast = false;
    memset(&obj->box3DBodyId, 0, sizeof(obj->box3DBodyId));
    currentRoadSegment->totalRoadObjects++;
  }

  if (currentRoadSegment != NULL && currentRoadSegment->totalRoadObjects > 0) {
    currentRoadSegment->roadObjects[currentRoadSegment->totalRoadObjects - 1].isLast = true;
  }

  fclose(fp);
}