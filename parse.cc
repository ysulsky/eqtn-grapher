#include "parse.h"
#include "func.h"
#include <stdlib.h>
#include <string>
#include <vector>

using namespace std;
using namespace Math;

static vector< Variant >
rpn_process_string (const string& expr, int start, int len, int* paren_map);

const char* var_names[] = {"x", "y"};
const char* op_names[]  = {
  "sin(",          // op_sin
  "cos(",          // op_cos
  "tan(",          // op_tan
  "csc(",          // op_csc
  "sec(",          // op_sec
  "cot(",          // op_cot
  "asin(",         // op_asin
  "acos(",         // op_acos
  "atan(",         // op_atan
  "acsc(",         // op_acsc
  "asec(",         // op_asec
  "acot(",         // op_acot
  "sinh(",         // op_sinh
  "cosh(",         // op_cosh
  "tanh(",         // op_tanh
  "csch(",         // op_csch
  "sech(",         // op_sech
  "coth(",         // op_coth
  "asinh(",        // op_asinh
  "acosh(",        // op_acosh
  "atanh(",        // op_atanh
  "acsch(",        // op_acsch
  "asech(",        // op_asech
  "acoth(",        // op_acoth
  "log(",          // op_log
  "ln(",           // op_ln
  "exp(",          // op_exp
  "sqrt(",         // op_sqrt
  "abs(",          // op_abs
  "+",             // op_plus
  "-",             // op_minus
  "*",             // op_mult
  "/",             // op_div
  "^",             // op_pow
  "(",             // op_openparen
  ")",             // op_closeparen
  "deriv("         // op_differentiate
};

// look for a character in the string, skipping any parentheses
static int 
find_in_top_level (const char* str, char c, int len)
{
  int paren_level = 0;
  const char *cur = str;
  
  for (; cur < str + len; cur++)
  {
    if (*cur == '(')
      paren_level++;
    else if (*cur == ')')
      paren_level--;
    else if (!paren_level && *cur == c)
      return cur - str;
  }
  
  return -1;
}

static int
find_matching_str (const string& str, int pos, const char **search, int num)
{
  int i;
  for (i = 0; i < num; i++)
  {
    if (!str.compare (pos, strlen (search[i]), search[i]))
      break;
  }
  
  return i;
}

static int precedence (ops_enum op)
{
  switch (op)
  {
    case op_minus:
    case op_plus:
      return 1;
    case op_mult:
    case op_div:
      return 2;
    case op_pow:
      return 3;
    default:
      break;
  }
  return -1;
}

static void
rpn_process_infix_op (Variant& last_in_str, int& i, ops_enum op,
                      vector< ops_enum >& ops_stack,
          vector< Variant >& RPN)
{
  if (last_in_str.is_infix_op()) // can't have 2 in a row
    throw SyntaxException (i);
    
  int op_order = precedence (op);
        
  while (!ops_stack.empty() && 
    precedence (ops_stack.back()) >= op_order)
  {
    RPN.push_back (ops_stack.back());
    ops_stack.pop_back();
  }

  ops_stack.push_back (op);

  // record the current element
  last_in_str = op;
}

static void
rpn_process_constant (Variant& last_in_str, int& i, const string& expr,
                      int start, int len,
          vector< ops_enum >& ops_stack, vector< Variant >& RPN)
{
  // insert op_mult if necessary
  if (! last_in_str.is_infix_op())
    rpn_process_infix_op (last_in_str, i, op_mult, ops_stack, RPN);
  
  char cur = expr[i];
  
  bool decimal_used = (cur == '.');
  int j;
      
  for (j = i + 1; j < start + len; j++)
  {
    cur = expr[j];
        
    if (decimal_used && cur == '.')
      throw SyntaxException (j);
  
    if (!isdigit (cur) && cur != '.')
      break;
  }

  double val = atof (expr.substr (i, j-i).c_str());
  RPN.push_back (val);
      
  // record the current element
  last_in_str = val;
  
  // skip to the end
  i = j - 1;
}

static void
rpn_process_diff_op (Variant& last_in_str, int& i, const string& expr,
                     int start, int len, int *paren_map,
         vector< ops_enum >& ops_stack, vector< Variant >& RPN)
{
  // insert op_mult if necessary
  if (! last_in_str.is_infix_op())
    rpn_process_infix_op (last_in_str, i, op_mult, ops_stack, RPN);
  
  int after_op = i + strlen (op_names[ op_differentiate ]); // skip the op
  
  int comma_pos = 
    find_in_top_level (expr.c_str() + after_op, ',',
                 paren_map[after_op - 1] - after_op);
      
  if (comma_pos == -1)
    throw ArgumentException (i, paren_map[after_op - 1]);

  comma_pos += after_op; // in absolute coords
      
  var_enum differential =
    (var_enum) find_matching_str (expr, comma_pos + 1, var_names, NUM_VARS);
      
  if (differential == NUM_VARS)
    throw SyntaxException (comma_pos + 1);

  // make sure there's nothing appended
  if (strlen (var_names[ differential ]) !=
      paren_map[after_op - 1] - (comma_pos + 1))
    throw SyntaxException (comma_pos + 1);
      
  vector< Variant > diff_func_RPN =
    rpn_process_string (expr, after_op, comma_pos - after_op, paren_map);
      
  if (diff_func_RPN.empty())
    throw SyntaxException (comma_pos);

  Function diff_func = Function::FromRPN (diff_func_RPN);

  vector< Variant > add_to_RPN = 
    diff_func.differentiate (differential).get_rpn_stack();

  RPN.insert (RPN.end(), add_to_RPN.begin(), add_to_RPN.end());

  // record the current element
  last_in_str = op_differentiate;
  
  // skip to the end
  i = paren_map[after_op - 1];
}

static void
rpn_process_paren_op (Variant& last_in_str, int& i, const string& expr,
                      int *paren_map, ops_enum op,
                      vector< ops_enum >& ops_stack, vector< Variant >& RPN)
{
  // insert op_mult if necessary
  if (! last_in_str.is_infix_op())
    rpn_process_infix_op (last_in_str, i, op_mult, ops_stack, RPN);
  
  int after_op = i + strlen (op_names[ op ]); // skip to position after op
  
  vector< Variant > add_to_RPN =
    rpn_process_string (expr, after_op, paren_map[after_op - 1] - after_op,
                    paren_map);
         
  if (add_to_RPN.empty())
    throw ArgumentException (i, paren_map[after_op - 1]);
        
  RPN.insert (RPN.end(), add_to_RPN.begin(), add_to_RPN.end());

  if (op != op_openparen) // op is sin(), abs(), sqrt()...
    RPN.push_back (op);

  // record the current element
  last_in_str = op;
  
  // skip to the end
  i = paren_map[after_op - 1];
}

static void
rpn_process_variable (Variant& last_in_str, int& i, var_enum var,
                      vector< ops_enum >& ops_stack, vector< Variant >& RPN)
{
  // insert op_mult if necessary
  if (! last_in_str.is_infix_op())
    rpn_process_infix_op (last_in_str, i, op_mult, ops_stack, RPN);
  
  RPN.push_back (var);

  // record the current element
  last_in_str = var;

  // skip to the end
  i += strlen (var_names[ (int)var ]) - 1;
}

static vector< Variant >
rpn_process_string (const string& expr, int start, int len, int* paren_map)
{
  vector< Variant > RPN;
  vector< ops_enum > ops_stack;

  Variant last_in_str (op_plus); // imagine there's a plus in front

  for (int i = start; i < start + len; i++)
  {
    char cur = expr[i];
    if (cur == '.' || isdigit (cur))
    { // start of a constant
      rpn_process_constant (last_in_str, i, expr, start, len, ops_stack, RPN);
    }
    else
    { // not a constant
      ops_enum op = (ops_enum) find_matching_str (expr, i, op_names, NUM_OPS);

      if (op != NUM_OPS)
      { // it's an operation
        switch (op)
        {
          case op_plus:
          case op_minus:
          case op_mult:
          case op_div:
          case op_pow:
            rpn_process_infix_op (last_in_str, i, op, ops_stack, RPN);
            break;
          case op_differentiate:
            rpn_process_diff_op (last_in_str, i, expr, start, len, paren_map,
                                 ops_stack, RPN);
            break;
          default: // (assumed to be parentheses or sin(), sqrt(), ...)
            rpn_process_paren_op (last_in_str, i, expr, paren_map, op,
                                  ops_stack, RPN);
            break;
        }
      }
      else
      { // it should be a variable
        var_enum var = (var_enum) find_matching_str (expr, i, var_names,
                                                     NUM_VARS);
        if (var == NUM_VARS) // not a variable either, give up
          throw SyntaxException (i);
        rpn_process_variable (last_in_str, i, var, ops_stack, RPN);
      }
    }
  }

  if (last_in_str.is_infix_op()) // can't end with an infix op
    throw SyntaxException (start + len - 1);

  for (int i = ops_stack.size() - 1; i >= 0; i--)
    RPN.push_back (ops_stack[i]);
  
  return RPN;
}

vector< Variant >
parse_into_RPN (string& expr)
{
  vector< Variant > RPN_stack;
  
  // remove all spaces
  int i = 0;
  while ((i = expr.find (' ', i)) != string::npos)
    expr.erase (expr.begin() + i);

  // add a 0 before unary minuses/pluses
  if (expr[0] == '-' || expr[0] == '+')
    expr.insert (0, "0");

  i = 0;
  while ((i = expr.find ("(-", i)) != string::npos)
    expr.insert (i + 1, "0");

  i = 0;
  while ((i = expr.find ("(+", i)) != string::npos)
    expr.insert (i + 1, "0");

  // create an array A of length expr.size(), with values of -1 except
  // where the corresponding character is a '('. At such a pos.
  // j, expr[ A[j] ] = the closing parenthesis of expr[ j ]

  int *paren_map = new int[ expr.size() ];
  vector< int > open_paren_stack;
  
  for (i = 0; i < expr.size(); i++)
  {
    if (expr[i] == '(')
      open_paren_stack.push_back (i);
    else if (expr[i] == ')')
    {
      if (open_paren_stack.empty())
	throw SyntaxException (i);

      int open_pos = open_paren_stack.back();
      open_paren_stack.pop_back();
      
      paren_map[ i ] = open_pos;
      paren_map[ open_pos ] = i;
    }
    else
      paren_map[i] = -1;
  }

  if (!open_paren_stack.empty())
    throw SyntaxException (open_paren_stack.back());

  RPN_stack = rpn_process_string (expr, 0, expr.size(), paren_map);
  
  delete [] paren_map;
  
  return RPN_stack;
}

