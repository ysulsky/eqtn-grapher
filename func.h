#ifndef _FUNC_H_
#define _FUNC_H_

#include <vector>
#include <string>

namespace Math
{
  enum var_enum
  {
    var_x,
    var_y,
    NUM_VARS
  };

  enum ops_enum
  {
    op_sin,
    op_cos,
    op_tan,
    op_csc,
    op_sec,
    op_cot,
    op_asin,
    op_acos,
    op_atan,
    op_acsc,
    op_asec,
    op_acot,
    op_sinh,
    op_cosh,
    op_tanh,
    op_csch,
    op_sech,
    op_coth,
    op_asinh,
    op_acosh,
    op_atanh,
    op_acsch,
    op_asech,
    op_acoth,
    op_log,
    op_ln,
    op_exp,
    op_sqrt,
    op_abs,
    op_plus,
    op_minus,
    op_mult,
    op_div,
    op_pow,
    op_openparen,
    op_closeparen,
    op_differentiate,
    NUM_OPS           // last item 
  };

  class Variant
  {
  public:

    enum {
      OP,
      CONSTANT,
      VARIABLE
    } type;

    union
    {
      ops_enum op;
      double   val;
      var_enum var;
    };

    Variant (void) {}
    Variant (double d)   { type = CONSTANT; val = d; }
    Variant (var_enum v) { type = VARIABLE; var = v; }
    Variant (ops_enum o) { type = OP; op = o; }

    bool is_infix_op() const
    {
      if (type != OP) return false;
      switch (op)
      {
	case op_plus:
        case op_minus:
	case op_mult:
	case op_div:
	case op_pow:
	  return true;
	default: break;
      }
      return false;
    }
  };

  class SyntaxException
  {
  public:
    int pos;
    SyntaxException (int i = -1)
    { pos = i; }
  };

  class ArgumentException
  {
  public:
    int pos_start, pos_end;
    ArgumentException (int start, int end)
    {
      pos_start = start;
      pos_end   = end;
    }
  };

  class Function
  {
    std::vector< Variant > RPN_stack;
    double *eval_stack;
   
  public:
    Function (void) { eval_stack = NULL; }
    Function (std::string expr) throw (SyntaxException, ArgumentException);
    Function (const Function& other){ eval_stack = NULL; *this = other; }
    
    ~Function (void)
    { delete [] eval_stack; }
    
    Function& operator= (const char *expr)
    { return (*this = std::string (expr)); }
    Function& operator= (const std::string expr)
    { return (*this = Function (expr)); }
    Function& operator= (const Function& other);
    
    Function differentiate (var_enum var) const;
    double operator() (double *var_values) const;

    std::vector< Variant > get_rpn_stack (void)
    { return RPN_stack; }

    // don't use this
    static Function FromRPN (const std::vector< Variant >& RPN_stack);
  };
}

#endif
