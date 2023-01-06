#include "errors.h"

/*****************************************************************
 * Generate a generic Type Mismatch error of the "expected xx,
 * observed yy" variety.
 */
TypeMismatch::TypeMismatch(const std::string &theOperator,
  const std::string &expectedType, const std::string &observedType)
  : Error(" Type mismatch for " + theOperator + " operator. Type expected " + expectedType
          + ", found " + observedType)
{
}


/*****************************************************************
 * Generate a generic Type Mismatch error of the "operator x
 * cannot be applied to type y" variety.
 */
TypeMismatch::TypeMismatch(const std::string &theOperator,
  const std::string &observedType)
  : Error("Operator <" + theOperator + "> cannot be applied to type "
          + observedType)
{
}

TypeMismatch::~TypeMismatch()
{
}
