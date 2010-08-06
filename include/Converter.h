#ifndef _CONVERTER_H_
#define _CONVERTER_H_

#include "Ogre.h"

///errors which may occur during conversion
enum conversion_errors
{
  ALL_OK,
  ERROR_UNSUPPORTED
};

///global struct with conversion options. make sure to set these before calling any convert functions
struct conversionOptions
{
  ///Contructor
  conversionOptions(bool flipTextureHParam = false, bool flipTextureVParam = false, bool defaultLight = true) { flipTextureH = flipTextureHParam; flipTextureV = flipTextureVParam; addDefaultLight = defaultLight; }
  ///flip texture coordinates horizontally before writing to Ogre vertex buffer?
  bool flipTextureH;
  ///flip texture coordinates vertically before writing to Ogre vertex buffer?
  bool flipTextureV;
  ///add a default directional light when no light in scene?
  bool addDefaultLight;
};

///Base class for all converter classes.
/**Base class for all converter classes. Defines some functions that are usefull for all classes and static members
  that are read by all converters before converting.
*/
class CConverter
{
public:
  ///Constructor, abstract
  CConverter(void) ;
  ///Destructor
  virtual ~CConverter(void);

  ///the conversion options
  static conversionOptions m_convOptions;
  ///is z up in the collada file? If not, y-up is assumed. If set it the collada file, this will automatically be recognized
  ///by the CColladaDatabase::load function
  static bool m_zUp;

  ///flips z and y axes and negates z for rotations;
  Ogre::Quaternion flipAxes(Ogre::Quaternion* pQuat) const;
  ///flips z and y axes and negates z for translations;
  Ogre::Vector3 flipAxes(Ogre::Vector3 *pVec) const;
  ///timer for speed testing
  double timeStamp();
};

#endif //_CONVERTER_H_