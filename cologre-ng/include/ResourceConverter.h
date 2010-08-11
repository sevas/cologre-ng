#ifndef _RESOURCE_CONVERTER_H_
#define _RESOURCE_CONVERTER_H_

#include "Converter.h"
#include "dae.h"

///abstract base class for all resource converters
class CResourceConverter : public CConverter
{
public:
  ///Constructor
  CResourceConverter() ;
  ///Destructor
  virtual ~CResourceConverter();
  ///virtual function, has to be implemented by all derived classes
  virtual int convert(daeDatabase* pDatabase) = 0;

protected:
  ///to keep track of how many elements have been converted. since collada element names are optional, this is used to assign
  ///unique names to each created ogre resource, if collada element name is not present
  static unsigned int m_uiElementCounter;
};

#endif //_RESOURCE_CONVERTER_H_