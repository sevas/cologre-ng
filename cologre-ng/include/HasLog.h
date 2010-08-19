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
    void logMessage(const std::string &_msg) {mLog->logMessage(_msg);};

protected:
    Ogre::Log *mLog;
};
}
#endif 