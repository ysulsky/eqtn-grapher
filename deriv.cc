#include "deriv.h"
#include "func.h"
#include <vector>
#include <assert.h>

using namespace std;
using namespace Math;

#ifndef LN_10
#define LN_10  2.302585093
#endif

typedef vector< Variant > (*deriv_rule_func)
  (const vector< Variant >& RPN, var_enum differential, int top_pos);

static vector< Variant >
rpn_deriv (const vector< Variant >& RPN, var_enum differential, int top_pos);

static int
find_arg_bottom (const vector< Variant >& RPN, int top_pos)
{
  int level = -1;
  
  int i;
  for (i = top_pos; i >= 0; i--)
  {
    Variant cur = RPN[i];
    if (cur.type == Variant::CONSTANT || cur.type == Variant::VARIABLE)
    {
      if (! ++level)
	break;
    }
    else if (cur.is_infix_op())
      level -= 1;
  }
  
  assert (i >= 0);
  
  return i;
}

static vector< Variant >
sin_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // cos (arg) * deriv (arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "cos (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_cos);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);
  
  return ret;
}
  
static vector< Variant >
cos_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // 0 - sin (arg) * deriv (arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "0"
  ret.push_back (0.0);
  
  // push "sin (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_sin);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  // push op_minus
  ret.push_back (op_minus);
  
  return ret;
}
  
static vector< Variant >
tan_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // sec(arg)^2 * deriv(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "sec (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_sec);

  // push "2"
  ret.push_back (2.0);

  // push op_pow
  ret.push_back (op_pow);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  return ret;
}

static vector< Variant >
csc_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // 0 - csc(arg) * cot(arg) * deriv(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "0"
  ret.push_back (0.0);
  
  // push "csc (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_csc);

  // push "cot (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_cot);

  // push op_mult
  ret.push_back (op_mult);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  // push op_minus
  ret.push_back (op_minus);

  return ret;
}
 
static vector< Variant >
sec_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // sec(arg) * tan(arg) * deriv(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "sec (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_sec);

  // push "tan (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_tan);

  // push op_mult
  ret.push_back (op_mult);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  return ret;
}
  
static vector< Variant >
cot_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // 0 - csc(arg)^2 * deriv(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "0"
  ret.push_back (0.0);

  // push "csc (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_csc);

  // push "2"
  ret.push_back (2.0);
  
  // push op_pow
  ret.push_back (op_pow);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  // push op_minus
  ret.push_back (op_minus);

  return ret;
}
  
static vector< Variant >
asin_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // deriv(arg) / sqrt(1 - arg^2)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());
  
  // push "1"
  ret.push_back (1.0);

  // push "arg^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push op_minus
  ret.push_back (op_minus);
  
  // push op_sqrt
  ret.push_back (op_sqrt);

  // push op_div
  ret.push_back (op_div);
  
  return ret;
}
  
static vector< Variant >
acos_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // 0 - deriv(arg) / sqrt(1 - arg^2)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "0"
  ret.push_back (0.0);
  
  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());
  
  // push "1"
  ret.push_back (1.0);

  // push "arg^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push op_minus
  ret.push_back (op_minus);
  
  // push op_sqrt
  ret.push_back (op_sqrt);

  // push op_div
  ret.push_back (op_div);

  // push op_minus
  ret.push_back (op_minus);
  
  return ret;
}
  
static vector< Variant >
atan_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // deriv(arg) / (1 + arg^2)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());
  
  // push "1"
  ret.push_back (1.0);

  // push "arg^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push op_plus
  ret.push_back (op_plus);
  
  // push op_div
  ret.push_back (op_div);
  
  return ret;
}
  
static vector< Variant >
acsc_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // 0 - deriv(arg) / (abs(arg) * sqrt(arg^2 - 1))
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "0"
  ret.push_back (0.0);
  
  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());
  
  // push "abs (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_abs);
  
  // push "arg^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push "1"
  ret.push_back (1.0);

  // push op_minus
  ret.push_back (op_minus);
  
  // push op_sqrt
  ret.push_back (op_sqrt);
  
  // push op_mult
  ret.push_back (op_mult);
 
  // push op_div
  ret.push_back (op_div);

  // push op_minus
  ret.push_back (op_minus);
  
  return ret;
}

static vector< Variant >
asec_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // deriv(arg) / (abs(arg) * sqrt(arg^2 - 1))
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());
  
  // push "abs (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_abs);
  
  // push "arg^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push "1"
  ret.push_back (1.0);

  // push op_minus
  ret.push_back (op_minus);
  
  // push op_sqrt
  ret.push_back (op_sqrt);
  
  // push op_mult
  ret.push_back (op_mult);
 
  // push op_div
  ret.push_back (op_div);
  
  return ret;
}

static vector< Variant >
acot_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // 0 - deriv(arg) / (1 + arg^2))
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "0"
  ret.push_back (0.0);
  
  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push "1"
  ret.push_back (1.0);
  
  // push "arg^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push op_plus
  ret.push_back (op_minus);
 
  // push op_div
  ret.push_back (op_div);
  
  // push op_minus
  ret.push_back (op_minus);
  
  return ret;
}

static vector< Variant >
sinh_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // cosh(arg) * deriv(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "cosh(arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_cosh);
  
  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  return ret;
}

static vector< Variant >
cosh_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // sinh(arg) * deriv(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "sinh(arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_sinh);
  
  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  return ret;
}

static vector< Variant >
tanh_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // sech(arg)^2 * deriv(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "sech(arg)^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_sech);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  return ret;
}

static vector< Variant >
csch_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // 0 - csch(arg) * coth(arg) * deriv(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "0"
  ret.push_back (0.0);
  
  // push "csch(arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_csch);

  // push "coth(arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_coth);

  // push op_mult
  ret.push_back (op_mult);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  // push op_minus
  ret.push_back (op_minus);

  return ret;
}

static vector< Variant >
sech_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // 0 - sech(arg) * tanh(arg) * deriv(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "0"
  ret.push_back (0.0);
  
  // push "sech(arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_sech);

  // push "tanh(arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_tanh);

  // push op_mult
  ret.push_back (op_mult);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  // push op_minus
  ret.push_back (op_minus);

  return ret;
}

static vector< Variant >
coth_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // 0 - csch(arg)^2 * deriv(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "0"
  ret.push_back (0.0);
  
  // push "csch(arg)^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_csch);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  // push op_minus
  ret.push_back (op_minus);

  return ret;
}
  
static vector< Variant >
asinh_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // deriv(arg) / sqrt(arg^2 + 1)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push arg^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push "1"
  ret.push_back (1.0);

  // push op_plus
  ret.push_back (op_plus);

  // push op_sqrt
  ret.push_back (op_sqrt);

  // push op_div
  ret.push_back (op_div);
  
  return ret;
}

static vector< Variant >
acosh_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // deriv(arg) / sqrt(arg^2 - 1)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push arg^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push "1"
  ret.push_back (1.0);

  // push op_minus
  ret.push_back (op_minus);

  // push op_sqrt
  ret.push_back (op_sqrt);

  // push op_div
  ret.push_back (op_div);
  
  return ret;
}

static vector< Variant >
atanh_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // deriv(arg) / (1 - arg^2)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push "1"
  ret.push_back (1.0);

  // push arg^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push op_minus
  ret.push_back (op_minus);

  // push op_div
  ret.push_back (op_div);
  
  return ret;
}

static vector< Variant >
acsch_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // 0 - deriv(arg) / (abs(arg) * sqrt(1 + arg^2))
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "0"
  ret.push_back (0.0);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push "abs (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_abs);

  // push "1"
  ret.push_back (1.0);

  // push "arg^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);
 
  // push op_plus
  ret.push_back (op_plus);
 
  // push op_sqrt
  ret.push_back (op_sqrt);
 
  // push op_mult
  ret.push_back (op_mult);
 
  // push op_div
  ret.push_back (op_div);
 
  // push op_minus
  ret.push_back (op_minus);

  return ret;
}

static vector< Variant >
asech_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // 0 - deriv(arg) / (arg * sqrt(1 - arg^2))
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "0"
  ret.push_back (0.0);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push "abs (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_abs);

  // push "1"
  ret.push_back (1.0);

  // push "arg^2"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);
 
  // push op_minus
  ret.push_back (op_minus);
 
  // push op_sqrt
  ret.push_back (op_sqrt);
 
  // push op_mult
  ret.push_back (op_mult);
 
  // push op_div
  ret.push_back (op_div);
 
  // push op_minus
  ret.push_back (op_minus);

  return ret;
}

static vector< Variant >
acoth_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // same as atanh_rule
  return atanh_rule (RPN, differential, top_pos);
}

static vector< Variant >
log_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // deriv(arg) / (ln(10) * arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push "arg"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);

  // push op_div
  ret.push_back (op_div);

  // push "ln(10)"
  ret.push_back (LN_10);

  // push op_div
  ret.push_back (op_div);

  return ret;
}

static vector< Variant >
ln_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // deriv(arg) / arg
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push "arg"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);

  // push op_div
  ret.push_back (op_div);

  return ret;
}

static vector< Variant >
exp_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // exp(arg) * deriv(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "exp (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_exp);
  
  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push op_mult
  ret.push_back (op_mult);

  return ret;
}

static vector< Variant >
sqrt_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // deriv(arg) / (2 * sqrt(arg))
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());

  // push "2"
  ret.push_back (2.0);

  // push op_div
  ret.push_back (op_div);
  
  // push "sqrt (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_sqrt);
  
  // push op_div
  ret.push_back (op_div);

  return ret;
}

static vector< Variant >
abs_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // arg * deriv(arg) / abs(arg)
  vector< Variant > ret;
  
  int arg_bottom = find_arg_bottom (RPN, top_pos - 1);
  
  // push "arg"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);

  // push "deriv (arg)"
  vector< Variant > chain_rule = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain_rule.begin(), chain_rule.end());
  
  // push op_mult
  ret.push_back (op_mult);
  
  // push "abs (arg)"
  ret.insert (ret.end(), RPN.begin() + arg_bottom, RPN.begin() + top_pos);
  ret.push_back (op_abs);
  
  // push op_div
  ret.push_back (op_div);

  return ret;
}

static vector< Variant >
plus_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // deriv(arg1) + deriv(arg2)
  vector< Variant > ret;
  
  int arg2_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "deriv (arg1)"
  vector< Variant > chain1 = rpn_deriv (RPN, differential, arg2_bottom - 1);
  ret.insert (ret.end(), chain1.begin(), chain1.end());
  
  // push "deriv (arg2)"
  vector< Variant > chain2 = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain2.begin(), chain2.end());
  
  // push op_plus
  ret.push_back (op_plus);

  return ret;
}

static vector< Variant >
minus_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // deriv(arg1) - deriv(arg2)
  vector< Variant > ret;
  
  int arg2_bottom = find_arg_bottom (RPN, top_pos - 1);

  // push "deriv (arg1)"
  vector< Variant > chain1 = rpn_deriv (RPN, differential, arg2_bottom - 1);
  ret.insert (ret.end(), chain1.begin(), chain1.end());
  
  // push "deriv (arg2)"
  vector< Variant > chain2 = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain2.begin(), chain2.end());
  
  // push op_minus
  ret.push_back (op_minus);

  return ret;
}

static vector< Variant >
mult_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // arg1 * deriv(arg2) + deriv(arg1) * arg2
  vector< Variant > ret;
  
  int arg2_bottom = find_arg_bottom (RPN, top_pos - 1);
  int arg1_bottom = find_arg_bottom (RPN, arg2_bottom - 1);

  // push "arg1"
  ret.insert (ret.end(), RPN.begin() + arg1_bottom, RPN.begin() + arg2_bottom);

  // push "deriv (arg2)"
  vector< Variant > chain2 = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain2.begin(), chain2.end());
  
  // push op_mult
  ret.push_back (op_mult);

  // push "deriv (arg1)
  vector< Variant > chain1 = rpn_deriv (RPN, differential, arg2_bottom - 1);
  ret.insert (ret.end(), chain1.begin(), chain1.end());
  
  // push "arg2"
  ret.insert (ret.end(), RPN.begin() + arg2_bottom, RPN.begin() + top_pos);
  
  // push op_mult
  ret.push_back (op_mult);

  // push op_plus
  ret.push_back (op_plus);
  
  return ret;
}

static vector< Variant >
div_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // (arg2 * deriv(arg1) - deriv(arg2) * arg1) / (arg2)^2
  vector< Variant > ret;
  
  int arg2_bottom = find_arg_bottom (RPN, top_pos - 1);
  int arg1_bottom = find_arg_bottom (RPN, arg2_bottom - 1);

  // push "arg2"
  ret.insert (ret.end(), RPN.begin() + arg2_bottom, RPN.begin() + top_pos);

  // push "deriv (arg1)"
  vector< Variant > chain1 = rpn_deriv (RPN, differential, arg2_bottom - 1);
  ret.insert (ret.end(), chain1.begin(), chain1.end());

  // push op_mult
  ret.push_back (op_mult);

  // push deriv (arg2)"
  vector< Variant > chain2 = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain2.begin(), chain2.end());
  
  // push "arg1"
  ret.insert (ret.end(), RPN.begin() + arg1_bottom, RPN.begin() + arg2_bottom);
  
  // push op_mult
  ret.push_back (op_mult);

  // push op_minus
  ret.push_back (op_minus);

  // push "(arg2)^2"
  ret.insert (ret.end(), RPN.begin() + arg2_bottom, RPN.begin() + top_pos);
  ret.push_back (2.0);
  ret.push_back (op_pow);

  // push op_div
  ret.push_back (op_div);

  return ret;
}

static vector< Variant >
pow_rule (const vector< Variant >& RPN, var_enum differential, int top_pos)
{ // arg1^arg2 * (arg2 * deriv(arg1) / arg1 + deriv(arg2) * ln(arg1))
  vector< Variant > ret;
  
  int arg2_bottom = find_arg_bottom (RPN, top_pos - 1);
  int arg1_bottom = find_arg_bottom (RPN, arg2_bottom - 1);

  // push "arg1"
  ret.insert (ret.end(), RPN.begin() + arg1_bottom, RPN.begin() + arg2_bottom);
  
  // push "arg2"
  ret.insert (ret.end(), RPN.begin() + arg2_bottom, RPN.begin() + top_pos);
  
  // push op_pow
  ret.push_back (op_pow);
  
  // push "arg2"
  ret.insert (ret.end(), RPN.begin() + arg2_bottom, RPN.begin() + top_pos);
  
  // push "deriv (arg1)"
  vector< Variant > chain1 = rpn_deriv (RPN, differential, arg2_bottom - 1);
  ret.insert (ret.end(), chain1.begin(), chain1.end());
  
  // push op_mult
  ret.push_back (op_mult);
  
  // push "arg1"
  ret.insert (ret.end(), RPN.begin() + arg1_bottom, RPN.begin() + arg2_bottom);
  
  // push op_div
  ret.push_back (op_div);
  
  // push "deriv (arg2)"
  vector< Variant > chain2 = rpn_deriv (RPN, differential, top_pos - 1);
  ret.insert (ret.end(), chain2.begin(), chain2.end());
  
  // push "ln (arg1)"
  ret.insert (ret.end(), RPN.begin() + arg1_bottom, RPN.begin() + arg2_bottom);
  ret.push_back (op_ln);
  
  // push op_mult
  ret.push_back (op_mult);
  
  // push op_plus
  ret.push_back (op_plus);
  
  // push op_mult
  ret.push_back (op_mult);

  return ret;
}


static deriv_rule_func deriv_rule[ NUM_OPS ] = {
  sin_rule,
  cos_rule,
  tan_rule,
  csc_rule,
  sec_rule,
  cot_rule,
  asin_rule,
  acos_rule,
  atan_rule,
  acsc_rule,
  asec_rule,
  acot_rule,
  sinh_rule,
  cosh_rule,
  tanh_rule,
  csch_rule,
  sech_rule,
  coth_rule,
  asinh_rule,
  acosh_rule,
  atanh_rule,
  acsch_rule,
  asech_rule,
  acoth_rule,
  log_rule,
  ln_rule,
  exp_rule,
  sqrt_rule,
  abs_rule,
  plus_rule,
  minus_rule,
  mult_rule,
  div_rule,
  pow_rule,
  NULL,            // open paren
  NULL,            // close paren
  NULL,            // deriv
};

static vector< Variant >
rpn_deriv (const vector< Variant >& RPN, var_enum differential, int top_pos)
{
  vector< Variant > ret;
  
  Variant top = RPN[ top_pos ];
  
  if (top.type == Variant::CONSTANT)
  { // derivative of a constant is 0
    ret.push_back (0.0);
  }
  else if (top.type == Variant::VARIABLE)
  { // return 1 only if it's the differential
    ret.push_back ((top.var == differential) ? 1.0 : 0.0);
  }
  else
  { // use the rule of the operation
    ret = deriv_rule[ top.op ] (RPN, differential, top_pos);
  }

  return ret;
}

Function
deriv (const vector< Variant >& RPN, var_enum differential)
{
  return Function::FromRPN (rpn_deriv (RPN, differential, RPN.size() - 1));
}

