#pragma once
#ifndef __110810_UTILITY_H__
#define __110810_UTILITY_H__

#include <string>
#include <sstream>

namespace cologreng{
namespace utility{

std::string makeFullUri(const std::string &_scheme, const std::string &_path);
std::string convertUriToPath(const std::string &_uri);


template<typename T >
inline std::string toString (T p)
{
    std::ostringstream stream;
    stream << p;
    return stream.str();
}

inline const std::string & toString (const std::string & _value)
{
    return _value;
}

template<typename T1,  typename T2 >
inline std::string toString (T1 p1, T2 p2)
{
    std::ostringstream stream;
    stream << p1 << p2;
    return stream.str();
}

template<typename T1,  typename T2,  typename T3 >
inline std::string toString (T1 p1, T2 p2, T3 p3)
{
    std::ostringstream stream;
    stream << p1 << p2 << p3;
    return stream.str();
}

template<typename T1,  typename T2,  typename T3, typename T4 >
inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4)
{
    std::ostringstream stream;
    stream << p1 << p2 << p3 << p4;
    return stream.str();
}

template<typename T1,  typename T2,  typename T3, typename T4, typename T5 >
inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    std::ostringstream stream;
    stream << p1 << p2 << p3 << p4 << p5;
    return stream.str();
}

template<typename T1,  typename T2,  typename T3, typename T4, typename T5, typename T6 >
inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    std::ostringstream stream;
    stream << p1 << p2 << p3 << p4 << p5 << p6;
    return stream.str();
}

template<typename T1,  typename T2,  typename T3, typename T4, typename T5, typename T6, typename T7 >
inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    std::ostringstream stream;
    stream << p1 << p2 << p3 << p4 << p5 << p6 << p7;
    return stream.str();
}

template<typename T1,  typename T2,  typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    std::ostringstream stream;
    stream << p1 << p2 << p3 << p4 << p5 << p6 << p7 << p8;
    return stream.str();
}


template<typename T1,  typename T2,  typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9 >
inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    std::ostringstream stream;
    stream << p1 << p2 << p3 << p4 << p5 << p6 << p7 << p8 << p9;
    return stream.str();
}

template< >
inline std::string toString<bool> (bool _value)
{
    return _value ? "true" : "false";
}


}}

#endif 