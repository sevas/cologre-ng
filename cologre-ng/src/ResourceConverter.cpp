#include "cologre_ng_precompiled.h"
#include "ResourceConverter.h"

#include "../utility/Utility.h"

namespace cologreng{
//------------------------------------------------------------------------------
unsigned int CResourceConverter::m_uiElementCounter = 0;

conversionOptions CConverter::m_convOptions(true, false);
bool CConverter::m_zUp(false);
//------------------------------------------------------------------------------
CResourceConverter::CResourceConverter(Ogre::Log *_log)
    :HasLog(_log)
{
    m_uiElementCounter++;
}
//------------------------------------------------------------------------------
CResourceConverter::~CResourceConverter()
{
}
//------------------------------------------------------------------------------
int CResourceConverter::convert(daeDatabase* pDatabase)
{
    if(!pDatabase)
    {
        logMessage(utility::toString("Element param not valid, aborting!"));
        return 1;
    }
    return 0;
}
//------------------------------------------------------------------------------
} //namespace cologreng