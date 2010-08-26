#pragma once
#ifndef __190810_HASLOG_H__
#define __190810_HASLOG_H__

#include <string>
#include <Ogre.h>

namespace cologreng{



class HasLog
{
public:
    HasLog(Ogre::Log *_log);
    virtual ~HasLog() {mLog = NULL;};
    void logMessage(const std::string &_msg);
    void indent();
    void dedent();

protected:
    std::string _makeIndent(int _indentLevel);

protected:
    Ogre::Log *mLog;
    int mCurrentIndentLevel;
    static const std::string sSpaces;
};
}
#endif 