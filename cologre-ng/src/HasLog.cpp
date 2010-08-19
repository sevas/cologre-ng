#include "cologre_ng_precompiled.h"
#include "HasLog.h"

#ifdef _DEBUG
#include <cassert>
#endif

namespace cologreng{

HasLog::HasLog(Ogre::Log *_log)
    :mLog(_log)
{
#ifdef _DEBUG
    assert(mLog);
#endif

}
} // namespace cologreng