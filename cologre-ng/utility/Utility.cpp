#include "cologre_ng_precompiled.h"
#include "Utility.h"
#ifdef _DEBUG
#include <cassert>
#endif
#include <sstream>

#include <uriparser/Uri.h>

namespace cologreng{
namespace utility{

//-----------------------------------------------------------------------------
std::string makeFullUri(const std::string &_scheme, const std::string &_path)
{
    std::string sep("://");
    return std::string(_scheme+sep+_path);
}
//-----------------------------------------------------------------------------
std::string convertUriToPath(const std::string &_uri)
{    
    size_t len = _uri.size() + 1 - 8;
    char *filename = new char[len];

    if(uriUriStringToWindowsFilenameA(_uri.c_str(), filename) != 0)
    {
        // TODO : error logging
    }
    

    std::string s(filename);
    delete[] filename;

    return s;
}
//-----------------------------------------------------------------------------
}}
