#ifndef _CONTROLLER_CONVERTER_H_
#define _CONTROLLER_CONVERTER_H_ 

#include "ResourceConverter.h"

namespace cologreng{
///Parses collada controller libraries.
/**
  Parses collada controller libraries. Specifically, it takes collada "<skin>s" and applies
  vertex weights to ogre meshes. It also transforms all vertices of a mesh according to the bind shape matrix.
  \author Michael Weyel
*/
class CControllerConverter : public CResourceConverter
{
public:
  CControllerConverter(void);
  virtual ~CControllerConverter(void);

  /**Reads collada "<skin>s" and applies vertex weights to ogre meshes. It also transforms all vertices of a mesh according 
    to the bind shape matrix, if present. Meshes already have to have been created and stored as Ogre resources, so 
    if you directly call this function (rather than calling CColladaDatabase::convertResources()), make sure to call
    CGeometryConverter::convert() first.
    @param pDatabase daeDatabase to convert controllers from
    @return 0 if succeeded, 1 upon error, prints error message to stderr
  */
  int convert(daeDatabase* pDatabase);

protected:
  ///transforms the individual vertices of the mesh rather than applying a scene node transformation
  ///maybe should be moved to a helper function class or something, but since it is only needed here so far, I'll leave it in here
  void transformVertices(Ogre::MeshPtr pOgreMesh, Ogre::Matrix4* pMatTransform);
};

} // namespace cologreng

#endif //_CONTROLLER_CONVERTER_H_