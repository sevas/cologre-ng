#include "cologre_ng_precompiled.h"
#include "Utility.h"
#include <vector>
#include <numeric>

std::string convertUriToPath(const std::string &_uri)
{
    std::vector<std::string> UriFragments = Ogre::StringUtil::split(_uri, "%20");

    return std::accumulate( UriFragments.begin(), UriFragments.end(), std::string(" ") );

}
