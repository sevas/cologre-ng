#ifndef _MATERIAL_CONVERTER_H_
#define _MATERIAL_CONVERTER_H_

#include "ResourceConverter.h"
#include <dom/domProfile_COMMON.h>

#include <string>
#include <Ogre.h>
#include "common.h"
#include "HasLog.h"

namespace cologreng{
///Class for converting collada materials into ogre materials.
/**Class for converting collada materials into ogre materials. Creates Ogre materials for each collada "<material>" and 
  passes for each collada technique. Also loads texture files.
*/
class CMaterialConverter : public CResourceConverter
{
public:
    ///Constructor
    CMaterialConverter(Ogre::Log *_log, LocationsPtr _locations);
    ///Destructor
    virtual ~CMaterialConverter(void);

    /**Reads collada "<material>s" and creates Ogre materials accordingly. For each material, it creates a new profile for each technique
      of the "<effect>" referenced by a collada material and creates Ogre passes for each subtechnique. It also looks up images referenced
      in the profiles and loads them and stores them in the Ogre texture manager. This function is far from being complete, it creates only
      basic materials with phong or blinn shaders so far, fragment or vertex shader support has yet to be added.
      Created materials are stored in Ogres global MaterialManager. 
      An Ogre root object has to have been created BEFORE calling this function.
      @param pDatabase daeDatabase to convert material elements from
      @return 0 if succeeded, 1 upon error, prints error message to stderr
    */
    int convert(daeDatabase* pDatabase);


protected:
    PathBasename _getPathBasenameFromUri(xsAnyURI _uri);

protected:
    ///adds COMMON techniques as techniques of the passed Ogre material
    void addTechnique_COMMON(const domProfile_COMMON::domTechniqueRef techRef, Ogre::MaterialPtr pMat);
    ///converts a collada texture definition to an Ogre pass, adds texture path as resource location, loads texture image
    void convertTexture(const domCommon_color_or_texture_type_complexType::domTextureRef textureRef, Ogre::Pass* pOgrePass);
    ///translates the xsToken string to an Ogre image format, so far, only A8R8G8B8 is supported
    void setImageFormat(const xsToken colladaFormat, Ogre::TexturePtr pOgreTexture);
    ///for now it only sets texture filters
    void setSamplerAttributes(const domFx_sampler2D_commonRef sampler2dRef, Ogre::TextureUnitState* pTextureUnitState);


protected:
    LocationsPtr m_spLocations;
};

} // namespace cologreng

#endif //_MATERIAL_CONVERTER_H_
