#include "cologre_ng_precompiled.h"
#include "Converter.h"

CConverter::CConverter(void)
{
}

CConverter::~CConverter(void)
{
}


Ogre::Quaternion CConverter::flipAxes(Ogre::Quaternion *pQuat) const
{
  Ogre::Quaternion quat;
  quat.x = pQuat->x;
  quat.y = pQuat->z;
  quat.z = -pQuat->y;
  quat.w = pQuat->w;
  //Ogre::Vector3 axis;
	//Ogre::Radian w;
	//quat.ToAngleAxis(w, axis);
	//quat.FromAngleAxis(-w, axis);

  return quat;
}

Ogre::Vector3 CConverter::flipAxes(Ogre::Vector3 *pVec) const
{
  Ogre::Vector3 vec;
  vec.x = pVec->x;
  vec.y = pVec->z;
  vec.z = -pVec->y;
  return vec;
}

double CConverter::timeStamp()
{
#if defined (WIN32)
  __int64 freq, count;
  //check for high resolution timer
  if(QueryPerformanceFrequency((LARGE_INTEGER*)&freq))
  {
    //high resolution timer available - use it!
    double dResolution = 1.0 / (double)freq;
    QueryPerformanceCounter((LARGE_INTEGER*)&count);
    double dSeconds = (double)count * dResolution;
    static long secsFirstCall = (long)dSeconds;
    return dSeconds - (double)secsFirstCall;
  }
  else
  {
    return 0.0;
  }
#else
  static struct timeval time;
  gettimeofday(&time, 0);
  static long secsFirstCall=time.tv_sec;
  return (double)(time.tv_sec-secsFirstCall) + (double)time.tv_usec/(1000.0*1000.0);
#endif
}