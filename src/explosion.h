#ifndef EXPLOSION_H
#define EXPLOSION_H

#include "common.h"
#include <stdlib.h>

void createExplosionSpheres(b3WorldId worldId, srExplosionSphere explosionSpheres[], Vector3 initialPosition, Vector3 initialVolume, float minRadius, float maxRadius, float maxImpulse);
void destroyExplosionSpheres(srExplosionSphere explosionSpheres[]);

#endif