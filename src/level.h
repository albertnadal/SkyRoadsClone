#ifndef LEVEL_H
#define LEVEL_H

#include "common.h"

extern int totalSegments;
extern srRoadSegment segments[];

void loadLevel(int level);
void unloadCurrentLevel();

#endif