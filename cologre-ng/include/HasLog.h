#pragma once
#ifndef __190810_HASLOG_H__
#define __190810_HASLOG_H__

#include <string>
#include <Ogre.h>

namespace cologreng{



class HasLog
{
public:
    typedef enum{LEVEL0=0, LEVEL1, LEVEL2, LEVEL3} IndentLevels;

public:
    HasLog(Ogre::Log *_log);
    virtual ~HasLog() {mLog = NULL;};
    void logMessage(const std::string &_msg, int _indentLevel=0);

protected:
    std::string _makeIndent(int _indentLevel);

protected:
    Ogre::Log *mLog;
    static const std::string sSpaces;
};
}
#endif 