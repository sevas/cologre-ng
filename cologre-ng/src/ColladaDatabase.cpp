#include "cologre_ng_precompiled.h"
#include "ColladaDatabase.h"

#include <sstream>


#include "ResourceConverter.h"
#include "GeometryConverter.h"
#include "MaterialConverter.h"
#include "ControllerConverter.h"
#include "SceneConverter.h"

CColladaDatabase::CColladaDatabase()
{
    m_pDae = NULL;
    m_pDatabase = NULL;

    m_pLogger = NULL;
    m_spLocations = LocationsPtr(new Locations);
}

CColladaDatabase::~CColladaDatabase()
{
  if(m_pDae)
  {
    delete m_pDae;
    m_pDae = NULL;
  }
}

int CColladaDatabase::load(std::string filename)
{
    m_pLogger = Ogre::LogManager::getSingletonPtr()->createLog("cologre_ng.log");

    m_pDae = new DAE();
    if(m_pDae->load(filename.c_str(), 0))
    {
        std::stringstream s;
        s << "DAE object not initialized! Probably .dae file " << filename << " not found.";
        m_pLogger->logMessage(s.str());

        return 1;
    }
    m_pDatabase = m_pDae->getDatabase();
    parseAsset();
    return 0;
}

void CColladaDatabase::convertResources()
{
    CResourceConverter* pResConv = NULL;
    pResConv = new CGeometryConverter();
    pResConv->convert(m_pDatabase);
    delete pResConv;

    pResConv = new CMaterialConverter(m_pLogger, m_spLocations);
    pResConv->convert(m_pDatabase);
    delete pResConv;

    pResConv = new CControllerConverter();
    pResConv->convert(m_pDatabase);
    delete pResConv;
}

void CColladaDatabase::convertScene(Ogre::SceneManager* pOgreSceneManager)
{
  daeElement* pElement = NULL;
  m_pDatabase->getElement(&pElement, 0, NULL, "scene", NULL);
  CSceneConverter* pSceneConv = NULL;
  pSceneConv = new CSceneConverter();
  pSceneConv->convert(pElement, pOgreSceneManager);
  delete pSceneConv;
}

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
  
