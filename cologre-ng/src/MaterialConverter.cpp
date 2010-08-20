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

namespace cologreng{
//-----------------------------------------------------------------------------
CMaterialConverter::CMaterialConverter(Ogre::Log *_log, LocationsPtr _locations)  
    :CResourceConverter(_log)
    ,m_spLocations(_locations)
{
    //Ogre::MaterialPtr pOgreMat = Ogre::MaterialManager::getSingleton().load("OgreCore.material", "Custom");
}
//-----------------------------------------------------------------------------
CMaterialConverter::~CMaterialConverter(void)
{
}
//-----------------------------------------------------------------------------
int CMaterialConverter::convert(daeDatabase* pDatabase)
{
    logMessage("Converting materials");
    logMessage("-------------------------------------------------");

    unsigned int numElements = pDatabase->getElementCount(NULL, "material", NULL);
    for(unsigned int i = 0; i < numElements; i++)
    {
        daeElement* pElement = NULL;
        pDatabase->getElement(&pElement, i, NULL, "material", NULL);
        domMaterial* pMat = (domMaterial*)pElement;
        xsID id = pMat->getId();
        std::string materialName;

        if(!id)
        {
            materialName = _makeGenericMaterialName(m_uiElementCounter);
            id = materialName.c_str();
        }

        logMessage(utility::toString("Creating ogre material : ", id, " in group DaeCustom"));

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

    logMessage("\n\n\n");

    return 0;
}
//-----------------------------------------------------------------------------
void CMaterialConverter::addTechnique_COMMON(const domProfile_COMMON::domTechniqueRef techRef, Ogre::MaterialPtr pMat)
{
    logMessage(utility::toString("Material ", pMat->getName(), " : adding technique COMMON"));

    Ogre::Technique* pOgreTech = pMat->createTechnique();
    pOgreTech->setName(techRef->getSid());
    pOgreTech->removeAllPasses();
    Ogre::Pass* pOgrePass = pOgreTech->createPass();

    logMessage(utility::toString("Material ", pMat->getName(), " : adding pass"));

    domProfile_COMMON::domTechnique::domPhongRef phongRef = techRef->getPhong();
    if(phongRef)
    {
        _addPhongPass(phongRef, pOgrePass);
    }

    // TODO : should be ifelse ?
    domProfile_COMMON::domTechnique::domBlinnRef blinnRef = techRef->getBlinn();
    if(blinnRef)
    {
        _addBlinnPass(blinnRef, pOgrePass);
    }
}
//-----------------------------------------------------------------------------
void CMaterialConverter::convertTexture(const domCommon_color_or_texture_type_complexType::domTextureRef textureRef, Ogre::Pass* pOgrePass)
{
    xsNCName texName = textureRef->getTexture();
    std::string strTarget = texName;
    strTarget = "./" + strTarget;
    daeSIDResolver res(textureRef->getDocument()->getDomRoot(), strTarget.c_str(), NULL);

    daeElementRef elemRef = res.getElement();
    domCommon_newparam_type* pNewparam = NULL;
    if(elemRef)
        pNewparam = daeSafeCast<domCommon_newparam_type>(&(*elemRef));
    else
    {
        logMessage(cologreng::utility::toString("Could not resolve to sampler2d for texture ", strTarget));
        return;
    }

    // get the sampler2d element                        
    domFx_sampler2D_commonRef  sampler2dRef = pNewparam->getSampler2D();
    strTarget = sampler2dRef->getSource()->getValue();
    strTarget = "./" + strTarget;
    res.setTarget(strTarget.c_str());
    elemRef = res.getElement();

    if(elemRef)
        pNewparam = daeSafeCast<domCommon_newparam_type>(&(*elemRef));
    else
    {
        logMessage(cologreng::utility::toString("Could not resolve to surface for sampler2d ", strTarget));
        return;
    }

    // get the surface referenced in the sampler 
    domFx_surface_commonRef surfaceRef = pNewparam->getSurface();
    switch(surfaceRef->getType())
    {
    case FX_SURFACE_TYPE_ENUM_2D: _add2DTexturePass(surfaceRef, sampler2dRef, pOgrePass);  break;
    default: logMessage(utility::toString("Unknown surface type id was detected : ", surfaceRef->getType())); break;
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


    // set wrapping mode
    domFx_sampler2D_common_complexType::domWrap_sRef sRef = sampler2dRef->getWrap_s();
    domFx_sampler2D_common_complexType::domWrap_tRef tRef = sampler2dRef->getWrap_t();                               
    Ogre::TextureUnitState::TextureAddressingMode uTAM, vTAM, wTAM;
    uTAM = Ogre::TextureUnitState::TAM_WRAP; // default
    vTAM = Ogre::TextureUnitState::TAM_WRAP; // default
    wTAM = Ogre::TextureUnitState::TAM_WRAP; // default

    if(sRef->getValue())
        uTAM = _convertDomWrappingToOgreTextureAddressingMode(sRef->getValue());
    if(tRef->getValue())
        vTAM = _convertDomWrappingToOgreTextureAddressingMode(tRef->getValue());

    wTAM = Ogre::TextureUnitState::TAM_WRAP; // default

    pTextureUnitState->setTextureAddressingMode(uTAM, vTAM, wTAM);
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
std::string CMaterialConverter::_makeGenericMaterialName(unsigned int _id)
{
    return utility::toString("DAE_Material_", m_uiElementCounter);
}
//-----------------------------------------------------------------------------
Ogre::ColourValue CMaterialConverter::_convertDomColorToColourValue(domCommon_color_or_texture_type::domColorRef _colorRef)
{
    return Ogre::ColourValue(_colorRef->getValue()[0]
                            ,_colorRef->getValue()[1]
                            ,_colorRef->getValue()[2]
                            ,_colorRef->getValue()[3]);
}
//-----------------------------------------------------------------------------
void CMaterialConverter::_addBlinnPass(domProfile_COMMON::domTechnique::domBlinnRef blinnRef, Ogre::Pass* pOgrePass )
{
    logMessage("Blinn", HasLog::LEVEL1);

    domCommon_color_or_texture_typeRef typeRef;

    // ambient component
    typeRef = blinnRef->getAmbient();
    pOgrePass->setAmbient(_convertDomColorToColourValue(typeRef->getColor()));
    logMessage(utility::toString("Ambient : ", pOgrePass->getAmbient()), HasLog::LEVEL2);

    // diffuse component : color or texture
    typeRef = blinnRef->getDiffuse();
    if(typeRef->getColor())
    {
        pOgrePass->setDiffuse(_convertDomColorToColourValue(typeRef->getColor()));
        logMessage(utility::toString("Diffuse : ", pOgrePass->getDiffuse()), HasLog::LEVEL2);
    }
    else if(typeRef->getTexture())
    {
        convertTexture(typeRef->getTexture(), pOgrePass);
    }

    // specular        
    typeRef = blinnRef->getSpecular();
    pOgrePass->setSpecular(_convertDomColorToColourValue(typeRef->getColor()));
    logMessage(utility::toString("Specular : ", pOgrePass->getSpecular()), HasLog::LEVEL2);
}
//-----------------------------------------------------------------------------
void CMaterialConverter::_addPhongPass( domProfile_COMMON::domTechnique::domPhongRef phongRef, Ogre::Pass* pOgrePass )
{
    logMessage("Phong", HasLog::LEVEL1);
    domCommon_color_or_texture_typeRef typeRef;

    // ambient component
    typeRef = phongRef->getAmbient();
    pOgrePass->setAmbient(_convertDomColorToColourValue(typeRef->getColor()));
    logMessage(utility::toString("Ambient : ", pOgrePass->getAmbient()), HasLog::LEVEL1);

    // diffuse : color or texture
    typeRef = phongRef->getDiffuse();
    if(typeRef->getColor())
    {
        pOgrePass->setDiffuse(_convertDomColorToColourValue(typeRef->getColor()));
        logMessage(utility::toString("Diffuse : ", pOgrePass->getDiffuse()), HasLog::LEVEL1);
    } 
    else if(typeRef->getTexture())
    { 
        convertTexture(typeRef->getTexture(), pOgrePass);
    }

    // specular
    typeRef = phongRef->getSpecular();
    pOgrePass->setSpecular(_convertDomColorToColourValue(typeRef->getColor()));                                          
    logMessage(utility::toString("Specular : ", pOgrePass->getSpecular()), HasLog::LEVEL1);
}
//-----------------------------------------------------------------------------
void CMaterialConverter::_add2DTexturePass( domFx_surface_commonRef surfaceRef, domFx_sampler2D_commonRef _sampler2dRef, Ogre::Pass* pOgrePass )
{
    // get image handle
    daeElement *initFrom = surfaceRef->getChild("init_from");
    std::string imageFile;
    initFrom->getCharData(imageFile);

    // find image in library
    daeDocument *doc = initFrom->getDocument();
    daeDatabase *db = initFrom->getDAE()->getDatabase();
    domImage *pImg = daeSafeCast<domImage>(db->idLookup(imageFile, doc));

    if(pImg)
    {
        domImage::domInit_fromRef initFromRef = pImg->getInit_from();
        xsAnyURI imageURI = initFromRef->getValue();    
        PathBasename pathBasename = _getPathBasenameFromUri(imageURI);

        _addResourcesLocation(pathBasename.first);

        logMessage(cologreng::utility::toString("Loading 2D image : ", pathBasename.second));

        Ogre::TexturePtr pOgreTexture = Ogre::TextureManager::getSingleton().load(pathBasename.second, "DaeCustom", Ogre::TEX_TYPE_2D);
        Ogre::TextureUnitState *pOgreTextureUnitState = pOgrePass->createTextureUnitState(pathBasename.second, 0);
  
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

        setSamplerAttributes(_sampler2dRef, pOgreTextureUnitState);
    }
    else
    {
        logMessage(utility::toString("Failed to load domImage for : ", imageFile));
    }
}
//-----------------------------------------------------------------------------
void CMaterialConverter::_addResourcesLocation( const std::string &_path )
{
    // check if the resources location was already added
    if(m_spLocations->find(_path) == m_spLocations->end())
    {
        logMessage(cologreng::utility::toString("Adding location ", _path, " to global resource manager"));
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(_path, "FileSystem", "DaeCustom");
    }
    else                
    {
        logMessage(cologreng::utility::toString("Location ", _path, " already added to resource manager"));
    }
}
//-----------------------------------------------------------------------------
Ogre::TextureUnitState::TextureAddressingMode CMaterialConverter::_convertDomWrappingToOgreTextureAddressingMode(domFx_sampler_wrap_common _wrapValue)
{
    switch(_wrapValue)
    {
    case FX_SAMPLER_WRAP_COMMON_NONE: return Ogre::TextureUnitState::TAM_WRAP; // default ogre value
    case FX_SAMPLER_WRAP_COMMON_WRAP: return Ogre::TextureUnitState::TAM_WRAP;
    case FX_SAMPLER_WRAP_COMMON_MIRROR: return Ogre::TextureUnitState::TAM_MIRROR;
    case FX_SAMPLER_WRAP_COMMON_CLAMP: return Ogre::TextureUnitState::TAM_CLAMP;
    case FX_SAMPLER_WRAP_COMMON_BORDER: return Ogre::TextureUnitState::TAM_BORDER;        
    default: return Ogre::TextureUnitState::TAM_WRAP;  // default
    }
}
//-----------------------------------------------------------------------------
} // namespace cologreng
