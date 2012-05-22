#ifndef __CONVERTER_H__
#define __CONVERTER_H__

#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <typeinfo>  
 
class conversionError : public std::runtime_error
{
public:
    conversionError(const std::string& msg) 
        : std::runtime_error(msg)
    { }
};

 template<typename T>
 inline std::string to_string(T const& val)
 {
   std::ostringstream ost;
   if (!(ost << val))
     throw conversionError("Error while converting to string");
   return ost.str();
 }
 
 template<typename T>
 inline void convert(std::string const& str, T& val)
 {
   std::istringstream i(str);
   if (!(i >> val))
     throw conversionError("Error converting string to type");
 }
 
template<typename T>
inline T string_to(std::string const& str)
{
   T val;
   convert(str, val);
   return val;
}

#endif /* __CONVERTER_H__ */
