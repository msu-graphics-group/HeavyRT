#include "CrossRT.h"
#include <string>
#include <iostream>

ISceneObject* CreateEmbreeRT(const char* a_implName);
ISceneObject* MakeBruteForceRT(const char* a_implName);
ISceneObject *MakeBVH2CommonLoftRT(const char *a_implName, const char* a_buildName);

ISceneObject* CreateSceneRT(const char* a_implName, const char* a_buildName, const char* a_layoutName)
{
  const std::string className(a_implName); //
  if (className.find("BruteForce") != std::string::npos)
    return MakeBruteForceRT(a_implName);
  else if (className.find("BVH2CommonLoft") != std::string::npos || className.find("BVH2CommonLOFT") != std::string::npos || className.find("BVH2_LOFT") != std::string::npos)
    return MakeBVH2CommonLoftRT(a_implName, a_buildName);
  else
  {
    std::cout << "[CreateSceneRT]: Use Embree for '" << className.c_str() << "' " << std::endl;
    return CreateEmbreeRT(a_implName);
  }
}

void DeleteSceneRT(ISceneObject* a_impl) { delete a_impl; }

MetricStats ISceneObject::GetStats() 
{
  MetricStats stats = {};
  const double rays = std::max(double(m_stats.raysNumber), 1.0);
  stats.avgNC  = float(double(m_stats.NC)/rays);
  stats.avgLC  = float(double(m_stats.LC)/rays);
  stats.avgTC  = float(double(m_stats.TC)/rays);
  for (int i = 0; i < TREELET_ARR_SIZE; i++) {
    stats.avgLJC[i] = float(double(m_stats.LJC[i])/rays);
    stats.avgCMC[i] = float(double(m_stats.CMC[i])/rays);
    stats.avgWSS[i] = uint32_t(m_stats.WSS[i].size());
  }
  stats.avgBLB = float(double(m_stats.BLB)/rays);
  stats.avgSOC = float(double(m_stats.SOC)/rays);
  stats.avgSBL = float(double(m_stats.SBL)/rays);
  stats.bvhTotalSize  = m_stats.bvhTotalSize;
  stats.geomTotalSize = m_stats.geomTotalSize;
  return stats;
}

void ISceneObject::ResetStats()
{
  m_stats.clear();
}
