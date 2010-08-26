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
    ,mCurrentIndentLevel(0)
{
#ifdef _DEBUG
    assert(mLog);
#endif

}
//------------------------------------------------------------------------------
void HasLog::logMessage(const std::string &_msg) 
{
    std::string spaces = _makeIndent(mCurrentIndentLevel);
    mLog->logMessage(spaces + _msg);
}
//------------------------------------------------------------------------------
void HasLog::indent()
{
    if(mCurrentIndentLevel < 20)
        mCurrentIndentLevel++;
}
//------------------------------------------------------------------------------
void HasLog::dedent()
{
    if(mCurrentIndentLevel > 0)
        mCurrentIndentLevel--;
    else
        throw std::exception("Indent level can't be below 0");
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