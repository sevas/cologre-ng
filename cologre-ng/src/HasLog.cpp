#include "cologre_ng_precompiled.h"
#include "HasLog.h"

#include <sstream>
#include <string>

#ifdef _DEBUG
#include <cassert>
#endif

namespace cologreng{
//------------------------------------------------------------------------------
const std::string HasLog::sSpaces = std::string("    ");
//------------------------------------------------------------------------------
HasLog::HasLog(Ogre::Log *_log)
    :mLog(_log)
{
#ifdef _DEBUG
    assert(mLog);
#endif

}
//------------------------------------------------------------------------------
void HasLog::logMessage(const std::string &_msg, int _indentLevel) 
{
    std::string spaces = _makeIndent(_indentLevel);
    mLog->logMessage(spaces + _msg);
}
//------------------------------------------------------------------------------
std::string HasLog::_makeIndent(int _indentLevel)
{
    std::stringstream indent;
    while(_indentLevel--)
    {
        indent << sSpaces;
    }
    return indent.str();
}

} // namespace cologreng