#include "cologre_ng_precompiled.h"
#include "ResourceConverter.h"

unsigned int CResourceConverter::m_uiElementCounter = 0;

conversionOptions CConverter::m_convOptions(false, false);
bool CConverter::m_zUp(false);

CResourceConverter::CResourceConverter()
{
  m_uiElementCounter++;
}

CResourceConverter::~CResourceConverter()
{
}

int CResourceConverter::convert(daeDatabase* pDatabase)
{
  if(!pDatabase)
  {
    std::cerr << "Element param not valid, aborting!" << std::endl;
    return 1;
  }
  return 0;
}
