#include "func.h"
#include "parse.h"
#include "deriv.h"
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <string>

using namespace Math;
using namespace std;

// some simple math functions
static double csc   (double d) { return 1.0 / sin(d); }
static double sec   (double d) { return 1.0 / cos(d); }
static double cot   (double d) { return 1.0 / tan(d); }
static double csch  (double d) { return 1.0 / sinh(d); }
static double sech  (double d) { return 1.0 / cosh(d); }
static double coth  (double d) { return 1.0 / tanh(d); }
static double acsc  (double d) { return asin (1.0 / d); }
static double asec  (double d) { return acos (1.0 / d); }
static double acot  (double d) { return atan (1.0 / d); }
//static double asinh (double d) { return log (d + sqrt (d*d + 1.0)); }
//static double acosh (double d) { return log (d + sqrt (d*d - 1.0)); }
//static double atanh (double d) { return 0.5 * log ((1.0 + d) / (1.0 - d)); }
static double acsch (double d)
                    { return log ((1.0 / d) + sqrt (1.0 + d*d) / fabs (d)); }
static double asech (double d) { return log ((1.0 + sqrt (1.0 - d*d)) / d); }
static double acoth (double d) { return 0.5 * log ((d + 1.0) / (d - 1.0)); }

static double (*op_funcs[])(double arg) = {
  sin,             // op_sin
  cos,             // op_cos
  tan,             // op_tan
  csc,             // op_csc
  sec,             // op_sec
  cot,             // op_cot
  asin,            // op_asin
  acos,            // op_acos
  atan,            // op_atan
  acsc,            // op_acsc
  asec,            // op_asec
  acot,            // op_acot
  sinh,            // op_sinh
  cosh,            // op_cosh
  tanh,            // op_tanh
  csch,            // op_csch
  sech,            // op_sech
  coth,            // op_coth
  asinh,           // op_asinh
  acosh,           // op_acosh
  atanh,           // op_atanh
  acsch,           // op_acsch
  asech,           // op_asech
  acoth,           // op_acoth
  log10,           // op_log
  log,             // op_ln
  exp,             // op_exp
  sqrt,            // op_sqrt
  fabs,            // op_abs
  NULL,            // op_plus
  NULL,            // op_minus
  NULL,            // op_mult
  NULL,            // op_div
  NULL,            // op_pow
  NULL,            // op_openparen
  NULL,            // op_closeparen
  NULL             // op_differentiate
};

static int
calc_max_eval_stack_size (const vector< Variant >& RPN_stack)
{
  int max_eval_stack_size = 0;
  int cur_eval_stack_size = 0;
  for (int i = 0; i < RPN_stack.size(); i++)
  {
    Variant cur = RPN_stack[i];
    if (cur.type == Variant::CONSTANT || cur.type == Variant::VARIABLE)
      max_eval_stack_size = max (++cur_eval_stack_size, max_eval_stack_size);
    else if (cur.is_infix_op())
      cur_eval_stack_size--;
  }

  return  max_eval_stack_size;
}

namespace Math {

Function::Function (string expr) throw (ArgumentException, SyntaxException)
{
  // in case there's an exception, so the destructor still works
  eval_stack = NULL;

  // create the RPN representation of the function
  RPN_stack = parse_into_RPN (expr);

  // allocate the evaluation stack
  eval_stack = new double [calc_max_eval_stack_size (RPN_stack)];
}

Function
Function::FromRPN (const vector< Variant >& RPN_stack)
{
  Function f;
  f.RPN_stack = RPN_stack;
  f.eval_stack = new double [calc_max_eval_stack_size (RPN_stack)];
  return f;
}

Function&
Function::operator= (const Function& other)
{
  if (this == &other)
    return *this;

  delete [] eval_stack;

  RPN_stack = other.RPN_stack;
  eval_stack = new double [calc_max_eval_stack_size (RPN_stack)];

  return *this;
}

double
Function::operator() (double *var_values) const
{
  int eval_stack_size = 0;
  
  for (int i = 0; i < RPN_stack.size(); i++)
  {
    Variant cur = RPN_stack[i];
    
    if (cur.type == Variant::CONSTANT)
      eval_stack[ eval_stack_size++ ] = cur.val;
    else if (cur.type == Variant::VARIABLE)
      eval_stack[ eval_stack_size++ ] = var_values[ (int)cur.var ];
    else
    {
      double ans;
      
      switch (cur.op)
      {
	case op_plus:
	  ans = eval_stack [eval_stack_size - 2] +
	        eval_stack [eval_stack_size - 1];
	  eval_stack_size--;
	  break;

	case op_minus:
	  ans = eval_stack [eval_stack_size - 2] -
	        eval_stack [eval_stack_size - 1];
	  eval_stack_size--;
	  break;

        case op_div:
	  { // we rely on 0 / NaN == 0
	    double arg1 = eval_stack [eval_stack_size - 2];
	    
	    // TODO: only do this if arg1 is a constant
	    ans = (arg1 == 0.0) ? 0.0 : arg1 / eval_stack [eval_stack_size - 1];
	    eval_stack_size--;
	  }
	  break;

        case op_mult:
	  { // we rely on 0 * NaN == 0
	    double arg1 = eval_stack [eval_stack_size - 2];
	    double arg2 = eval_stack [eval_stack_size - 1];
	    
	    // TODO: only do this if the zero is a constant
	    ans =  (arg1 == 0.0 || arg2 == 0.0) ? 0.0 : arg1 * arg2;
	    eval_stack_size--;
	  }
	  break;
	
	case op_pow:
	  ans = pow (eval_stack [eval_stack_size - 2],
	             eval_stack [eval_stack_size - 1]);
	  eval_stack_size--;
	  break;
	
	default:
	  ans = op_funcs[ (int)cur.op ] (eval_stack[ eval_stack_size - 1 ]);
	  break;
      }

      eval_stack[ eval_stack_size - 1 ] = ans;
    }
  }

  assert (eval_stack_size == 1);
  return eval_stack[0];
}

Function
Function::differentiate (var_enum var) const
{
  return deriv (RPN_stack, var);
}

} // namespace Math

