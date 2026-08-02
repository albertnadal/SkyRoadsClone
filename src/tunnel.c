#include "tunnel.h"

#include <stdlib.h>
#include <math.h>

static Vector3 rotateAroundZ(Vector3 p, float angle) {
  float c = cosf(angle);
  float s = sinf(angle);

  return (Vector3){
    p.x * c - p.y * s,
    p.x * s + p.y * c,
    p.z
  };
}

static void drawBoxWiresRotated(Vector3 center, Vector3 size, float rotation, Color color) {
  float hx = size.x * 0.5f;
  float hy = size.y * 0.5f;
  float hz = size.z * 0.5f;

  Vector3 vertices[8] = {
    {-hx, -hy, -hz},
    { hx, -hy, -hz},
    { hx,  hy, -hz},
    {-hx,  hy, -hz},

    {-hx, -hy,  hz},
    { hx, -hy,  hz},
    { hx,  hy,  hz},
    {-hx,  hy,  hz}
  };

  for (int i = 0; i < 8; i++) {
    vertices[i] = rotateAroundZ(vertices[i], rotation);
    vertices[i].x += center.x;
    vertices[i].y += center.y;
    vertices[i].z += center.z;
  }

  int edges[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0},
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}
  };

  for (int i = 0; i < 12; i++) {
    DrawLine3D(vertices[edges[i][0]],
           vertices[edges[i][1]],
           color);
  }
}

b3BodyId createTunnelBody(b3WorldId worldId, Vector3 tunnelPos, Vector3 tunnelSize) {
  b3BodyDef bodyDef = b3DefaultBodyDef();
  bodyDef.type = b3_staticBody;
  bodyDef.position = (b3Vec3){tunnelPos.x, tunnelPos.y, tunnelPos.z};

  b3BodyId bodyId = b3CreateBody(worldId, &bodyDef);

  b3ShapeDef shapeDef = b3DefaultShapeDef();
  shapeDef.baseMaterial.friction = 0.2f;
  shapeDef.baseMaterial.restitution = 0.0f;

  const float radius = tunnelSize.x * 0.5f;
  const float thickness = tunnelSize.x * 0.05f;
  const float deltaAngle = 20.0f * DEG2RAD;
  const float plateWidth = 2.0f * radius * tanf(deltaAngle * 0.5f);
  const float startAngle = -80.0f * DEG2RAD;

  for (int i = 0; i < TUNNEL_SLICES; i++) {
    float angle = startAngle + i * deltaAngle;
    float x = radius * sinf(angle);
    float y = radius * cosf(angle);

    b3BoxHull box = b3MakeBoxHull(
      plateWidth * 0.5f,
      thickness * 0.5f,
      tunnelSize.z * 0.5f);

    b3Transform transform;
    transform.p = (b3Vec3){x, y, 0.0f};
    transform.q = b3MakeQuatFromAxisAngle(
      (b3Vec3){0.0f, 0.0f, 1.0f},
      -angle);

    b3Vec3 scale = {1.0f, 1.0f, 1.0f};

    b3CreateTransformedHullShape(
      bodyId,
      &shapeDef,
      &box.base,
      transform,
      scale);
  }

  return bodyId;
}

Model createTunnelModel(Vector3 tunnelSize) {
  Mesh mesh = {0};

  mesh.vertexCount = TUNNEL_SLICES * 8;
  mesh.triangleCount = TUNNEL_SLICES * 12;

  mesh.vertices = malloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.normals  = malloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.indices  = malloc(mesh.triangleCount * 3 * sizeof(unsigned short));

  float radius = tunnelSize.x * 0.5f;
  float thickness = 0.15f;
  float segmentAngle = 20.0f * DEG2RAD;
  float pieceWidth = 2.0f * radius * tanf(segmentAngle * 0.5f);

  int vertexOffset = 0;
  int indexOffset = 0;

  for (int i = 0; i < TUNNEL_SLICES; i++) {

    float angle = (-80.0f + i * 20.0f) * DEG2RAD;
    float cx = radius * sinf(angle);
    float cy = radius * cosf(angle);

    float halfX = pieceWidth * 0.5f;
    float halfY = thickness * 0.5f;
    float halfZ = tunnelSize.z * 0.5f;

    Vector3 corners[8] = {
      {-halfX, -halfY, -halfZ},
      { halfX, -halfY, -halfZ},
      { halfX,  halfY, -halfZ},
      {-halfX,  halfY, -halfZ},

      {-halfX, -halfY,  halfZ},
      { halfX, -halfY,  halfZ},
      { halfX,  halfY,  halfZ},
      {-halfX,  halfY,  halfZ}
    };

    for (int v = 0; v < 8; v++) {
      float x = corners[v].x;
      float y = corners[v].y;
      float rx = x * cosf(angle) + y * sinf(angle);
      float ry = -x * sinf(angle) + y * cosf(angle);

      mesh.vertices[(vertexOffset + v) * 3 + 0] = cx + rx;
      mesh.vertices[(vertexOffset + v) * 3 + 1] = cy + ry;
      mesh.vertices[(vertexOffset + v) * 3 + 2] = corners[v].z;

      mesh.normals[(vertexOffset + v) * 3 + 0] = 0.0f;
      mesh.normals[(vertexOffset + v) * 3 + 1] = 1.0f;
      mesh.normals[(vertexOffset + v) * 3 + 2] = 0.0f;
    }

    unsigned short base = vertexOffset;
    unsigned short cubeIndices[36] = {
      0,1,2, 0,2,3,
      4,6,5, 4,7,6,
      0,4,5, 0,5,1,
      1,5,6, 1,6,2,
      2,6,7, 2,7,3,
      4,0,3, 4,3,7
    };

    for (int j = 0; j < 36; j++) {
      mesh.indices[indexOffset++] = base + cubeIndices[j];
    }

    vertexOffset += 8;
  }

  UploadMesh(&mesh, false);

  Model model = LoadModelFromMesh(mesh);

  free(mesh.vertices);
  free(mesh.normals);
  free(mesh.indices);

  return model;
}

void drawTunnelWires(Vector3 tunnelPos, Vector3 tunnelSize, Color color) {
  float radius = tunnelSize.x * 0.5f;
  float thickness = tunnelSize.x * 0.06f;
  float segmentAngle = 20.0f * DEG2RAD;
  float pieceWidth = 2.0f * radius * tanf(segmentAngle * 0.5f);

  for (int i = 0; i < TUNNEL_SLICES; i++) {
    float angle = (-80.0f + i * 20.0f) * DEG2RAD;
    float x = radius * sinf(angle);
    float y = radius * cosf(angle);
    float rotation = -angle;

    Vector3 pieceCenter = {
      tunnelPos.x + x,
      tunnelPos.y + y,
      tunnelPos.z
    };

    drawBoxWiresRotated(
      pieceCenter,
      (Vector3){pieceWidth, thickness, tunnelSize.z},
      rotation,
      color);
  }
}