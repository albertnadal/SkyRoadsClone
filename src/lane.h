#ifndef LANE_H
#define LANE_H

#include "common.h"

b3BodyId createLaneBody(b3WorldId worldId, Vector3 lanePos, Vector3 laneSize, bool isExit);

#endif