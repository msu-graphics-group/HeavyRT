#include "rtao.h"
#include "render_common.h"

#include <cfloat>
#include <chrono>

using namespace LiteMath;

static inline float attenuationFunc(float r, float R) {
//  return std::clamp(r/R, 0.0f, 1.0f); // HBAO attenuation function
  return 0.0f;
}

static inline uint32_t float4ToRGBA(LiteMath::float4 v)
{
  int4 intV = int4(clamp(v, 0, 1)*255);
  uint32_t rgba = intV.x + (intV.y<<8) + (intV.z<<16) + (intV.w<<24);
  return rgba;
}

static inline uint32_t floatToGrayRGBA(float v)
{
  return float4ToRGBA(LiteMath::float4(v,v,v, 1.0f));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RTAO::kernel_PackXY(uint tidX, uint tidY, uint* out_pakedXY)
{
  //const uint offset   = BlockIndex2D(tidX, tidY, m_width);
  const uint offset   = SuperBlockIndex2DOpt(tidX, tidY, m_width);
  out_pakedXY[offset] = ((tidY << 16) & 0xFFFF0000) | (tidX & 0x0000FFFF);
}

void RTAO::PackXY(uint tidX, uint tidY)
{
  kernel_PackXY(tidX, tidY, m_packedXY.data());
}

void RTAO::PackXYBlock(uint tidX, uint tidY, uint a_passNum)
{
  #pragma omp parallel for default(shared)
  for(int y=0;y<tidY;y++)
    for(int x=0;x<tidX;x++)
      PackXY(x, y);
}

void RTAO::Clear(uint32_t a_width, uint32_t a_height, const char* a_what)
{
  PackXYBlock(a_width, a_height, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void RTAO::CalcAOBlock(uint32_t* a_outColor, uint32_t a_size, uint32_t a_passNumber)
{
  auto start = std::chrono::high_resolution_clock::now();
  
  #ifndef _DEBUG
  #ifndef ENABLE_METRICS
  #pragma omp parallel for
  #endif
  #endif
  for (int j = 0; j < int(a_size); ++j)
    for(uint32_t k=0;k<a_passNumber;k++)
      CalcAO(a_outColor, j);

  timeDataByName["CalcAOBlock"] = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count()/1000.f;
}

void RTAO::CalcAO(uint32_t* a_outColor, uint32_t tidX)
{
 
  float4 hitPosNorm;
  float  visibility;
  
  RTVPersistent_SetIter(0);
  kernel_TraceEyeRay2(tidX, &hitPosNorm, &visibility); // ==> (hitPos,hitNorm,visibility)
    
  for(uint32_t tidZ = 0; tidZ < m_aoRaysCount / RTVPersistent_Iters(); tidZ++) { // RTVPersistent_Iters()
    RTVPersistent_SetIter(tidZ % RTVPersistent_Iters());
    kernel_TraceAORay(tidX, tidZ, &hitPosNorm, &visibility); // ==> visibility
  }
  
  kernel_AO2Color(tidX, &hitPosNorm, &visibility, a_outColor); // ==> a_outColor
}

void RTAO::kernel_AO2Color(uint32_t tidX, const float4* positions, const float* visibility, uint32_t* out_color)
{
  const uint XY = m_packedXY[RTVPersistent_ThreadId(tidX)];
  const uint x  = (XY & 0x0000FFFF);
  const uint y  = (XY & 0xFFFF0000) >> 16;

  if(positions->y >= AO_HIT_BACK)
    out_color[y*m_width + x] = 0;
  else
  {
    const float visLocal     = (*visibility);
    const float sampleCount  = float(AO_PASS_COUNT * m_aoRaysCount);
    const float normCoeff    = 1.0f / sampleCount;
    const float power        = m_power;
    const float resAO        = std::pow(visLocal * normCoeff, power);
    out_color[y*m_width + x] = floatToGrayRGBA(resAO);
  }
}

static float3 calcTriangleNormal(const float3 *A_pos, const float3 *B_pos, const float3 *C_pos) {
  //CounterClockwise normal
  return cross((*B_pos) - (*A_pos), (*C_pos) - (*A_pos));
}

void RTAO::kernel_TraceEyeRay2(uint32_t tidX, float4* positions, float* visibility)
{
  const uint XY = m_packedXY[RTVPersistent_ThreadId(tidX)];
  const uint x  = (XY & 0x0000FFFF);
  const uint y  = (XY & 0xFFFF0000) >> 16;

  float3 rayDir1 = EyeRayDirNormalized((float(x)+0.5f)/float(m_width), (float(y)+0.5f)/float(m_height), m_projInv);
  float3 rayPos1 = float3(0,0,0);

  transform_ray3f(m_worldViewInv, &rayPos1, &rayDir1);

  const float4 rayPos = to_float4(rayPos1, m_zNearFar.x); // 0.0f
  const float4 rayDir = to_float4(rayDir1, m_zNearFar.y); // FLT_MAX
  *visibility    = 0.0f;

  if(x == 209 && y == 73)
  {
    int a = 2;
  }

  CRT_Hit hit = m_pAccelStruct->RayQuery_NearestHit(rayPos, rayDir);

  float3 hitNorm = float3(0,1,0);
  float3 hitPos  = to_float3(rayPos) + 0.9999995f*hit.t*to_float3(rayDir);

  const uint32_t geomId   = hit.geomId & GEOM_ID_MASK; 
  const uint32_t geomType = hit.geomId >> GEOM_ID_BITS; 

  if(geomId != GEOM_ID_MASK && geomType == 0)
  {
    const float2 uv       = float2(hit.coords[0], hit.coords[1]);
    const uint triOffset  = m_matIdOffsets[geomId];
    const uint vertOffset = m_vertOffset  [geomId];

    const uint A = m_triIndices[(triOffset + hit.primId)*3 + 0];
    const uint B = m_triIndices[(triOffset + hit.primId)*3 + 1];
    const uint C = m_triIndices[(triOffset + hit.primId)*3 + 2];

    const float3 A_pos = to_float3(m_vPos4f[A + vertOffset]);
    const float3 B_pos = to_float3(m_vPos4f[B + vertOffset]);
    const float3 C_pos = to_float3(m_vPos4f[C + vertOffset]);

    float3 triangleNormal = calcTriangleNormal(&A_pos, &B_pos, &C_pos);

    const float3 A_norm = to_float3(m_vNorm4f[A + vertOffset]);
    const float3 B_norm = to_float3(m_vNorm4f[B + vertOffset]);
    const float3 C_norm = to_float3(m_vNorm4f[C + vertOffset]);

    hitNorm = (1.0f - uv.x - uv.y)*A_norm + uv.y*B_norm + uv.x*C_norm;
   
    // transform surface point with matrix and flip normal if needed
    //
    hitNorm              = normalize(mymul3x3(m_normMatrices[hit.instId], hitNorm));
    triangleNormal       = normalize(mymul3x3(m_normMatrices[hit.instId], triangleNormal));

    if(dot(triangleNormal,hitNorm) < 0.0f)
      hitNorm *= -1.0f;

    const float dotp     = dot(to_float3(rayDir), triangleNormal);
    const float flipNorm = (dotp > 1e-5f) ? -1.0f : 1.0f; // beware of transparent materials which use normal sign to identity "inside/outside" glass for example
    hitNorm = hitNorm*flipNorm;
  }
  else if(geomId != GEOM_ID_MASK)
  {
    hitNorm = decode_normal(float2(hit.coords[0], hit.coords[1]));
  }
  else
  {
    hitPos.x = 0.0f;
    hitPos.y = AO_HIT_BACK;
    hitPos.z = 0.0f;
  }

  const uint normalCompressed = encodeNormal(hitNorm);
  *positions = to_float4(hitPos, as_float(normalCompressed));
}

void RTAO::kernel_TraceAORay(uint32_t tidX, uint32_t tidZ, const float4* positions, float* out_visibility)
{
  const float4 rayPos1 = *positions;
  if (rayPos1.y>=AO_HIT_BACK) // no hit point of screen
    return;

  const float3 normal = decodeNormal(as_uint(rayPos1.w)); // to_float3(*normals);
  
  const uint XY = m_packedXY[RTVPersistent_ThreadId(tidX)];
  const uint x  = (XY & 0x0000FFFF);
  const uint y  = (XY & 0xFFFF0000) >> 16;

  const uint32_t xTiled = x % AO_TILE_SIZE;
  const uint32_t yTiled = y % AO_TILE_SIZE;

  float2 uv      = m_aoRandomsTile[(yTiled * AO_TILE_SIZE + xTiled)*m_aoRaysCount + tidZ];
  float3 rayDir2 = MapSampleToCosineDistribution(uv.x, uv.y, normal, normal, 1.0f);
  float3 rayPos2 = OffsRayPos(to_float3(rayPos1), normal, rayDir2);
  
  ////////////////////////////////////////////////////////////////////////////////////////

  const float4 rayPos = to_float4(rayPos2, 0.0f); // rayPos.w
  const float4 rayDir = to_float4(rayDir2, m_aoMaxRadius);

  if (rayPos.y>=AO_HIT_BACK)
    return;
  
  #ifdef RTAO_USE_CLOSEST_HIT
  CRT_Hit hit = m_pAccelStruct->RayQuery_NearestHit(rayPos, rayDir);
  float visibility = 1.0f;
  if (hit.geomId != uint32_t(-1)) {
    visibility = attenuationFunc(hit.t, m_aoMaxRadius);
  }
  *out_visibility = *out_visibility + visibility;
  #else
  bool hit = m_pAccelStruct->RayQuery_AnyHit(rayPos, rayDir);
  if(hit)
    *out_visibility = *out_visibility + 1.0f;
  #endif
  *out_visibility = RTVPersistent_ReduceAdd1f(*out_visibility);
}
