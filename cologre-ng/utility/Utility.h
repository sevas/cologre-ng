#pragma once
#ifndef __110810_UTILITY_H__
#define __110810_UTILITY_H__


#include <string>

std::string makeFullUri(const std::string &_scheme, const std::string &_path);
std::string convertUriToPath(const std::string &_uri);

#endif 