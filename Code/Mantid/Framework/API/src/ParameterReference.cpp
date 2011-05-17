#include "MantidAPI/ParameterReference.h"
#include "MantidAPI/CompositeFunction.h"

namespace Mantid
{
namespace API
{

void ParameterReference::reset(IFitFunction* fun,size_t index)
{
  IFitFunction* fLocal = fun;
  int iLocal = static_cast<int>(index);
  CompositeFunction* cf = dynamic_cast<CompositeFunction*>(fun);
  while (cf)
  {
    int iFun = cf->functionIndex(iLocal);
    fLocal = cf->getFunction(iFun);
    iLocal = fLocal->parameterIndex(cf->parameterLocalName(iLocal));
    cf = dynamic_cast<CompositeFunction*>(fLocal);
  }

  m_function = fLocal;
  m_index = iLocal;
}

} // namespace API
} // namespace Mantid

