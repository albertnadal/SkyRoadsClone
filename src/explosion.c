#include "explosion.h"

void createExplosionSpheres(b3WorldId worldId,
                            srExplosionSphere explosionSpheres[],
                            Vector3 initialPosition,
                            Vector3 initialVolume,
                            float minRadius,
                            float maxRadius,
                            float maxImpulse) {
  for (int i = 0; i < EXPLOSION_SPHERES_COUNT; i++) {
    float t = (float)rand() / (float)RAND_MAX;
    float radius = minRadius + t * (maxRadius - minRadius);
    explosionSpheres[i].initialPosition.x = initialPosition.x + ((float)GetRandomValue(-1000, 1000) / 1000.0f) * (initialVolume.x * 0.5f);
    explosionSpheres[i].initialPosition.y = initialPosition.y + ((float)GetRandomValue(-1000, 1000) / 1000.0f) * (initialVolume.y * 0.5f);
    explosionSpheres[i].initialPosition.z = initialPosition.z + ((float)GetRandomValue(-1000, 1000) / 1000.0f) * (initialVolume.z * 0.5f);
    explosionSpheres[i].radius = radius;
    explosionSpheres[i].alpha = 255;
    explosionSpheres[i].color = (Color){255, GetRandomValue(150, 230), 0, 255};

    b3BodyDef bodyDef = b3DefaultBodyDef();
    bodyDef.type = b3_dynamicBody;
    bodyDef.position = (b3Vec3){
      initialPosition.x,
      initialPosition.y,
      initialPosition.z
    };
    bodyDef.linearDamping = 2.0f;

    b3BodyId bodyId = b3CreateBody(worldId, &bodyDef);
    b3ShapeDef shapeDef = b3DefaultShapeDef();
    shapeDef.density = 0.008f;
    shapeDef.baseMaterial.friction = 0.02f;
    shapeDef.baseMaterial.restitution = 0.95f;

    b3Sphere sphere;
    sphere.center = (b3Vec3){0.0f, 0.0f, 0.0f};
    sphere.radius = radius;
    b3CreateSphereShape(bodyId, &shapeDef, &sphere);
    explosionSpheres[i].box3DBodyId = bodyId;

    Vector3 direction = {
      (float)GetRandomValue(-1000, 1000),
      (float)GetRandomValue(-1000, 1000),
      (float)GetRandomValue(-1000, 1000)
    };

    float length = sqrtf(
      direction.x * direction.x +
      direction.y * direction.y +
      direction.z * direction.z
    );

    if (length > 0.0f) {
      direction.x /= length;
      direction.y /= length;
      direction.z /= length;
    }

    float magnitude = ((float)GetRandomValue(1, 10) / 10.0f) * maxImpulse;
    b3Vec3 impulse = {
      direction.x * magnitude,
      direction.y * magnitude,
      direction.z * magnitude
    };
    b3Body_ApplyLinearImpulseToCenter(bodyId, impulse, true);
  }
}

void destroyExplosionSpheres(srExplosionSphere explosionSpheres[]) {
  for (int i = 0; i < EXPLOSION_SPHERES_COUNT; i++) {
    if (b3Body_IsValid(explosionSpheres[i].box3DBodyId)) {
      b3DestroyBody(explosionSpheres[i].box3DBodyId);
    }
  }
}