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
