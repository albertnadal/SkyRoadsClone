#include "lane.h"

b3BodyId createLaneBody(b3WorldId worldId, Vector3 lanePos, Vector3 laneSize, bool isExit)
{
  b3BodyDef laneBodyDef = b3DefaultBodyDef();
  laneBodyDef.type = b3_staticBody;
  laneBodyDef.position = (b3Pos){lanePos.x, lanePos.y, lanePos.z};

  b3BodyId bodyId = b3CreateBody(worldId, &laneBodyDef);

  b3BoxHull laneStaticBox = b3MakeBoxHull(
    laneSize.x * 0.5f,
    laneSize.y * 0.5f,
    laneSize.z * 0.5f
  );

  b3ShapeDef laneShapeDef = b3DefaultShapeDef();
  laneShapeDef.baseMaterial.restitution = 0.02f;
  laneShapeDef.baseMaterial.friction = 0.2f;
  laneShapeDef.density = 10.0f;
  laneShapeDef.isSensor = isExit;
  laneShapeDef.enableSensorEvents = isExit;

  b3CreateHullShape(bodyId, &laneShapeDef, &laneStaticBox.base);

  return bodyId;
}