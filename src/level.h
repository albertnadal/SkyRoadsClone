#ifndef LEVEL_H
#define LEVEL_H

#include "common.h"
#include "lane.h"
#include "tunnel.h"

void initLevelObjects(b3WorldId worldId, srRoadObject objects[], int* totalObjects);
void loadLevel(int level, b3WorldId worldId, srRoadObject objects[], int* totalObjects);
void unloadCurrentLevel(srRoadObject objects[], int* totalObjects);

#endif