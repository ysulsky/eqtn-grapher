#ifndef _PARSE_H_
#define _PARSE_H_

#include <vector>
#include <string>
#include "func.h"

// warning: expr will be altered
std::vector< Math::Variant > parse_into_RPN (std::string& expr);

extern const char* op_names [ Math::NUM_OPS ];
extern const char* var_names[ Math::NUM_VARS ];
  
#endif
