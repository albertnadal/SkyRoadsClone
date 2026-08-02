#ifndef TUNNEL_H
#define TUNNEL_H

#include "common.h"

b3BodyId createTunnelBody(b3WorldId worldId, Vector3 tunnelPos, Vector3 tunnelSize);
Model createTunnelModel(Vector3 tunnelSize);
void drawTunnelWires(Vector3 tunnelPos, Vector3 tunnelSize, Color color);

#endif