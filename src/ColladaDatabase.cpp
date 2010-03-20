#include "ColladaDatabase.h"
#include "ResourceConverter.h"
#include "GeometryConverter.h"
#include "MaterialConverter.h"
#include "ControllerConverter.h"
#include "SceneConverter.h"

CColladaDatabase::CColladaDatabase()
{
  m_pDae = NULL;
  m_pDatabase = NULL;
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
  m_pDae = new DAE();
  if(m_pDae->load(filename.c_str(), 0))
  {
    std::cerr << "DAE object not initialized! Probably .dae file " << filename << " not found." << std::endl;
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

  pResConv = new CMaterialConverter();
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
  
