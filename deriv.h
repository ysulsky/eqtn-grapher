#ifndef _DERIV_H_
#define _DERIV_H_

#include <vector>
#include "func.h"

Math::Function
  deriv (const std::vector< Math::Variant >& RPN, Math::var_enum differential);

#endif
