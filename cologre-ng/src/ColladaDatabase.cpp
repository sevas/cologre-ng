#include "cologre_ng_precompiled.h"
#include "ColladaDatabase.h"

#include <sstream>
#ifdef _DEBUG
#include <cassert>
#endif

#include "ResourceConverter.h"
#include "GeometryConverter.h"
#include "MaterialConverter.h"
#include "ControllerConverter.h"
#include "SceneConverter.h"
#include "../utility/Utility.h"

namespace cologreng{
//------------------------------------------------------------------------------
const std::string CColladaDatabase::s_LogFilename("cologre_ng.log");
//------------------------------------------------------------------------------
CColladaDatabase::CColladaDatabase()
{
    m_pDae = NULL;
    m_pDatabase = NULL;

    m_pLog = NULL;
    m_spLocations = LocationsPtr(new Locations);
}
//------------------------------------------------------------------------------
CColladaDatabase::~CColladaDatabase()
{
    if(m_pDae)
    {
        delete m_pDae;
        m_pDae = NULL;
    }
}
//------------------------------------------------------------------------------
int CColladaDatabase::load(std::string filename)
{
    _initLogger();

    std::stringstream s;
    s << "Loading DAE file : " << filename;
    _logMessage(s.str());

    m_pDae = new DAE();
    if(m_pDae->load(filename.c_str(), 0))
    {
        _logMessage(utility::toString("DAE object not initialized! Probably .dae file ", filename,  " not found."));
        return 1;
    }

    m_pDatabase = m_pDae->getDatabase();
    parseAsset();
    return 0;
}
//------------------------------------------------------------------------------
void CColladaDatabase::convertResources()
{
    CResourceConverter* pResConv = NULL;
    pResConv = new CGeometryConverter(m_pLog);
    pResConv->convert(m_pDatabase);
    delete pResConv;

    pResConv = new CMaterialConverter(m_pLog, m_spLocations);
    pResConv->convert(m_pDatabase);
    delete pResConv;

    pResConv = new CControllerConverter(m_pLog);
    pResConv->convert(m_pDatabase);
    delete pResConv;
}
//------------------------------------------------------------------------------
void CColladaDatabase::convertScene(Ogre::SceneManager* pOgreSceneManager)
{
  daeElement* pElement = NULL;
  m_pDatabase->getElement(&pElement, 0, NULL, "scene", NULL);
  CSceneConverter* pSceneConv = NULL;
  pSceneConv = new CSceneConverter();
  pSceneConv->convert(pElement, pOgreSceneManager);
  delete pSceneConv;
}
//------------------------------------------------------------------------------
void CColladaDatabase::parseAsset()
{
  daeElement* pElement = NULL;
  m_pDatabase->getElement(&pElement, 0, NULL, "asset", NULL);
  if(pElement)
  {
    domAsset* pAsset = daeSafeCast<domAsset>(pElement);
    domAsset::domUp_axisRef upAxisRef = pAsset->getUp_axis();
    if(upAxisRef->hasValue() && upAxisRef->getValue() == UPAXISTYPE_Z_UP)
      CConverter::m_zUp = true;
  }
}  
//-----------------------------------------------------------------------------
void CColladaDatabase::_initLogger()
{
    m_pLog = Ogre::LogManager::getSingletonPtr()->createLog(s_LogFilename);
}
//-----------------------------------------------------------------------------
void CColladaDatabase::_logMessage(const std::string &_msg)
{
#ifdef _DEBUG
    assert(m_pLog);
#endif
    m_pLog->logMessage(_msg);
}

} // namespace cologreng