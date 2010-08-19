#pragma once
#ifndef __180810_COMMON_H__
#define __180810_COMMON_H__

#include <utility>
#include <set>
#include <string>
#include <boost/shared_ptr.hpp>

namespace cologreng{

typedef std::set<std::string>   Locations;
typedef boost::shared_ptr<Locations>   LocationsPtr;
	
typedef std::pair<std::string, std::string>   PathBasename;

} //namespace cologreng

#endif 