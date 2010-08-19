#include "cologre_ng_precompiled.h"
#include "MaterialConverter.h"

#ifdef _DEBUG
#include <cassert>
#endif

#include <dae/daeSIDResolver.h>
#include <dae/daeIDRef.h>
#include <dom/domEffect.h>
#include <dom/domMaterial.h>     

#include "../Utility/Utility.h"

//-----------------------------------------------------------------------------
CMaterialConverter::CMaterialConverter(Ogre::Log *_log, LocationsPtr _locations)  
    :CResourceConverter()
    ,m_spLocations(_locations)
    ,m_pLog(_log)
{
#ifdef _DEBUG
    assert(m_pLog);
#endif  
    //Ogre::MaterialPtr pOgreMat = Ogre::MaterialManager::getSingleton().load("OgreCore.material", "Custom");
}
//-----------------------------------------------------------------------------
CMaterialConverter::~CMaterialConverter(void)
{
}
//-----------------------------------------------------------------------------
int CMaterialConverter::convert(daeDatabase* pDatabase)
{
    _logMessage("Converting materials");
    _logMessage("-------------------------------------------------");


    unsigned int numElements = pDatabase->getElementCount(NULL, "material", NULL);
    for(unsigned int i = 0; i < numElements; i++)
    {
        daeElement* pElement = NULL;
        pDatabase->getElement(&pElement, i, NULL, "material", NULL);
        domMaterial* pMat = (domMaterial*)pElement;
        xsID id = pMat->getId();
        if(!id)
        {
            std::stringstream strTmp;
            strTmp << "Material_" << m_uiElementCounter << std::endl;
            id = strTmp.str().c_str();
        }

        Ogre::MaterialPtr pOgreMat = Ogre::MaterialManager::getSingleton().create(id, "DaeCustom");
        pOgreMat->removeAllTechniques();

        domInstance_effectRef fxRef = pMat->getInstance_effect();
        daeElementRef fxElementRef = fxRef->getUrl().getElement();
        if(fxElementRef->getElementType() == COLLADA_TYPE::EFFECT)
        {
            domEffect* pFx = (domEffect*) (&(*fxElementRef));
            domFx_profile_abstract_Array fxProfileArray = pFx->getFx_profile_abstract_array();
            for(unsigned int i = 0; i < fxProfileArray.getCount(); i++)
            {
                switch((*(fxProfileArray[i])).getElementType())
                {
                case COLLADA_TYPE::PROFILE_COMMON:
                    {
                        domProfile_COMMON* pProfileCommon = (domProfile_COMMON*) &(*fxProfileArray[i]);
                        domProfile_COMMON::domTechniqueRef techRef = pProfileCommon->getTechnique();
                        addTechnique_COMMON(techRef, pOgreMat);

                        break;
                    }

                default:
                    break;
                }
            }
        }
    }

    _logMessage("\n\n\n");

    return 0;
}
//-----------------------------------------------------------------------------
void CMaterialConverter::addTechnique_COMMON(const domProfile_COMMON::domTechniqueRef techRef, Ogre::MaterialPtr pMat)
{
  Ogre::Technique* pOgreTech = pMat->createTechnique();
  pOgreTech->setName(techRef->getSid());
  pOgreTech->removeAllPasses();
  Ogre::Pass* pOgrePass = pOgreTech->createPass();

  domProfile_COMMON::domTechnique::domPhongRef phongRef = techRef->getPhong();
  if(phongRef)
  {
    domCommon_color_or_texture_typeRef typeRef;
    domCommon_color_or_texture_type::domColorRef colorRef;
    domCommon_color_or_texture_type::domTextureRef textureRef;

    typeRef = phongRef->getAmbient();
    colorRef = typeRef->getColor();
    pOgrePass->setAmbient(colorRef->getValue()[0], colorRef->getValue()[1], colorRef->getValue()[2]);

    typeRef = phongRef->getDiffuse();
    if(colorRef = typeRef->getColor())
      pOgrePass->setDiffuse(colorRef->getValue()[0], colorRef->getValue()[1], colorRef->getValue()[2], colorRef->getValue()[3]);
    else if(textureRef = typeRef->getTexture())
      convertTexture(textureRef, pOgrePass);

    typeRef = phongRef->getSpecular();
    colorRef = typeRef->getColor();
    pOgrePass->setSpecular(colorRef->getValue()[0], colorRef->getValue()[1], colorRef->getValue()[2],  colorRef->getValue()[3]);
      
  }

  domProfile_COMMON::domTechnique::domBlinnRef blinnRef = techRef->getBlinn();
  if(blinnRef)
  {
    domCommon_color_or_texture_typeRef typeRef;
    domCommon_color_or_texture_type::domColorRef colorRef;
    domCommon_color_or_texture_type::domTextureRef textureRef;

    typeRef = blinnRef->getAmbient();
    colorRef = typeRef->getColor();
    pOgrePass->setAmbient(colorRef->getValue()[0], colorRef->getValue()[1], colorRef->getValue()[2]);

    typeRef = blinnRef->getDiffuse();
    if(colorRef = typeRef->getColor())
      pOgrePass->setDiffuse(colorRef->getValue()[0], colorRef->getValue()[1], colorRef->getValue()[2], colorRef->getValue()[3]);
    else if(textureRef = typeRef->getTexture())
      convertTexture(textureRef, pOgrePass);

    typeRef = blinnRef->getSpecular();
    colorRef = typeRef->getColor();
    pOgrePass->setSpecular(colorRef->getValue()[0], colorRef->getValue()[1], colorRef->getValue()[2],  colorRef->getValue()[3]);
  }
}
//-----------------------------------------------------------------------------
void CMaterialConverter::convertTexture(const domCommon_color_or_texture_type_complexType::domTextureRef textureRef, Ogre::Pass* pOgrePass)
{
    daeElementRef elemRef;
    domFx_sampler2D_commonRef sampler2dRef;
    domFx_surface_commonRef surfaceRef;
    domImage* pImg = NULL;
    Ogre::TexturePtr pOgreTexture;
    Ogre::TextureUnitState* pOgreTextureUnitState = NULL;

    xsNCName texName = textureRef->getTexture();
    std::string strTarget = texName;
    strTarget = "./" + strTarget;
    daeSIDResolver res(textureRef->getDocument()->getDomRoot(), strTarget.c_str(), NULL);
    elemRef = res.getElement();

    domCommon_newparam_type* pNewparam = NULL;
    if(elemRef)
        pNewparam = daeSafeCast<domCommon_newparam_type>(&(*elemRef));
    else
    {
        _logMessage(cologreng::utility::toString("Could not resolve to sampler2d for texture ", strTarget));
        return;
    }

    sampler2dRef = pNewparam->getSampler2D();
    strTarget = sampler2dRef->getSource()->getValue();
    strTarget = "./" + strTarget;
    res.setTarget(strTarget.c_str());
    elemRef = res.getElement();

    if(elemRef)
        pNewparam = daeSafeCast<domCommon_newparam_type>(&(*elemRef));
    else
    {
        _logMessage(cologreng::utility::toString("Could not resolve to surface for sampler2d ", strTarget));
        return;
    }

    surfaceRef = pNewparam->getSurface();
    switch(surfaceRef->getType())
    {
    case FX_SURFACE_TYPE_ENUM_2D:
        {

            // get image handle
            daeElement *initFrom = surfaceRef->getChild("init_from");
                           
            std::string imageFile;
            initFrom->getCharData(imageFile);
        
            // find image in library
            daeDocument *doc = initFrom->getDocument();
            daeDatabase *db = initFrom->getDAE()->getDatabase();

            pImg = daeSafeCast<domImage>(db->idLookup(imageFile, doc));

                                            
            if(pImg)
            {
                domImage::domInit_fromRef initFromRef = pImg->getInit_from();

                xsAnyURI imageURI = initFromRef->getValue();    
                PathBasename pathBasename = _getPathBasenameFromUri(imageURI);

                if(m_spLocations->find(pathBasename.first) == m_spLocations->end())
                {
                    _logMessage(cologreng::utility::toString("Adding location ", pathBasename.first, " to global resource manager"));
                    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(pathBasename.first, "FileSystem", "DaeCustom");
                }
                else                
                {
                    _logMessage(cologreng::utility::toString("Location ", pathBasename.first, " already added to resource manager"));
                }
                    
                _logMessage(cologreng::utility::toString("Loading 2D image : ", pathBasename.second));

                pOgreTexture = Ogre::TextureManager::getSingleton().load(pathBasename.second, "DaeCustom", Ogre::TEX_TYPE_2D);
                pOgreTextureUnitState = pOgrePass->createTextureUnitState(pathBasename.second, 0);
            }
            break;
        }
    default:
        break;
    }

    if(!(pOgreTexture.isNull()))
    {
        xsToken imgFormat;
        domFx_surface_common_complexType::domFormatRef formatRef;
        if(formatRef = surfaceRef->getFormat())
        {
            imgFormat = formatRef->getValue();
            setImageFormat(imgFormat, pOgreTexture);
        }
        else if(imgFormat = pImg->getFormat())
            setImageFormat(imgFormat, pOgreTexture);

        unsigned int imgHeight = pImg->getHeight();
        if(imgHeight)
            pOgreTexture->setHeight(imgHeight);

        unsigned int imgWidth = pImg->getWidth();
        if(imgWidth)
            pOgreTexture->setHeight(imgWidth);

        setSamplerAttributes(sampler2dRef, pOgreTextureUnitState);
    }
}
//-----------------------------------------------------------------------------
void CMaterialConverter::setImageFormat(const xsToken colladaFormat, Ogre::TexturePtr pOgreTexture)
{
  if(colladaFormat == "A8R8G8B8")
    pOgreTexture->setFormat(Ogre::PF_A8R8G8B8);
}
//-----------------------------------------------------------------------------
void CMaterialConverter::setSamplerAttributes(const domFx_sampler2D_commonRef sampler2dRef, Ogre::TextureUnitState *pTextureUnitState)
{
  Ogre::FilterOptions foMinFilter = Ogre::FO_ANISOTROPIC;
  Ogre::FilterOptions foMagFilter = Ogre::FO_ANISOTROPIC;
  Ogre::FilterOptions foMipFilter = Ogre::FO_LINEAR;

  if(domFx_sampler2D_common_complexType::domMinfilterRef minFilerRef = sampler2dRef->getMinfilter())
  {
    domFx_sampler_filter_common filter = minFilerRef->getValue();
    switch(filter)
    {
    case FX_SAMPLER_FILTER_COMMON_NONE:
      break;
    case FX_SAMPLER_FILTER_COMMON_NEAREST:
      foMinFilter = Ogre::FO_POINT;
      break;
    case FX_SAMPLER_FILTER_COMMON_LINEAR:
      foMinFilter = Ogre::FO_LINEAR;
      break;
    case FX_SAMPLER_FILTER_COMMON_NEAREST_MIPMAP_NEAREST:
      foMinFilter = Ogre::FO_POINT;
      foMipFilter = Ogre::FO_POINT;
      break;
    case FX_SAMPLER_FILTER_COMMON_NEAREST_MIPMAP_LINEAR:
      foMinFilter = Ogre::FO_POINT;
      foMipFilter = Ogre::FO_LINEAR;
      break;
    case FX_SAMPLER_FILTER_COMMON_LINEAR_MIPMAP_NEAREST:
      foMinFilter = Ogre::FO_LINEAR;
      foMipFilter = Ogre::FO_POINT;
      break;
    case FX_SAMPLER_FILTER_COMMON_LINEAR_MIPMAP_LINEAR:
      foMinFilter = Ogre::FO_LINEAR;
      foMipFilter = Ogre::FO_LINEAR;
      break;
    }
  }

  if(domFx_sampler2D_common_complexType::domMagfilterRef magFilerRef = sampler2dRef->getMagfilter())
  {
    domFx_sampler_filter_common filter = magFilerRef->getValue();
    switch(filter)
    {
    case FX_SAMPLER_FILTER_COMMON_NONE:
      break;
    case FX_SAMPLER_FILTER_COMMON_NEAREST:
      foMagFilter = Ogre::FO_POINT;
      break;
    case FX_SAMPLER_FILTER_COMMON_LINEAR:
      foMagFilter = Ogre::FO_LINEAR;
      break;
    }
  }

  if(domFx_sampler2D_common_complexType::domMipfilterRef mipFilerRef = sampler2dRef->getMipfilter())
  {
    domFx_sampler_filter_common filter = mipFilerRef->getValue();
    switch(filter)
    {
    case FX_SAMPLER_FILTER_COMMON_NONE:
      break;
    case FX_SAMPLER_FILTER_COMMON_NEAREST:
      foMipFilter = Ogre::FO_POINT;
      break;
    case FX_SAMPLER_FILTER_COMMON_LINEAR:
      foMipFilter = Ogre::FO_LINEAR;
      break;
    }
  }
  pTextureUnitState->setTextureFiltering(foMinFilter, foMagFilter, foMipFilter);

  //for later : maybe support texture wrapping modes, for now just leave it at the ogre default
}
//-----------------------------------------------------------------------------
PathBasename CMaterialConverter::_getPathBasenameFromUri(xsAnyURI _uri)
{
    std::string uriPath = _uri.getPath();
    std::string uriScheme = _uri.getScheme();

    std::string fullURI = cologreng::utility::makeFullUri(uriScheme, uriPath); 
    std::string qualifiedName = cologreng::utility::convertUriToPath(fullURI);

    std::string basename, path;
    Ogre::StringUtil::splitFilename(qualifiedName, basename, path);

    return PathBasename(path, basename);
}
//-----------------------------------------------------------------------------
