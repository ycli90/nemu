/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <string.h>
#include <debug.h>
#include <memory/vaddr.h>

#define MAX_TOKEN_NUM 10000

enum {
  TK_NOTYPE = 256, TK_NUMBER, TK_HEX_NUMBER, TK_REG, TK_EQ, TK_NOT_EQ, TK_AND, TK_POS, TK_NEG, TK_DEREF
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"-", '-'},           // minus
  {"\\*", '*'},         // multiply
  {"/", '/'},           // divide
  {"\\(", '('},         // left parenthesis
  {"\\)", ')'},         // right parenthesis
  {"0[xX][0-9a-fA-F]+", TK_HEX_NUMBER},  // hexadecimal number
  {"[[:digit:]]+", TK_NUMBER},           // decimal number
  {"\\$[a-z0-9]+", TK_REG},
  {"==", TK_EQ},        // equal
  {"!=", TK_NOT_EQ},    // not equal
  {"&&", TK_AND},       // logical and
  {"\\+", TK_POS},      // positive
  {"-", TK_NEG},        // negative
  {"\\*", TK_DEREF},    // dereference
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[MAX_TOKEN_NUM] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  if (e == NULL) {
    printf("empty expression\n");
    return false;
  }

  while (e[position] != '\0') {
    Assert(nr_token < MAX_TOKEN_NUM, "expression too long");
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // printf("match rules[%d] = \"%s\" at position %d with len %d: %.*s\n",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        int type = rules[i].token_type;
        if (type != TK_NOTYPE) {
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool possible_binary_op_prior_type(int type) {
  return type == ')' || type == TK_NUMBER || type == TK_HEX_NUMBER || type == TK_REG;
}

bool is_operand(int type) {
  return type == TK_NUMBER || type == TK_HEX_NUMBER || type == TK_REG;
}

// also check parentheses
int eval_count_operand(int left, int right, bool *success) {
  int n_operand = 0;
  int level = 0;
  for (int ind = left; ind <= right; ind++) {
    if (is_operand(tokens[ind].type) && level == 0) {
      n_operand++;
    } else if (tokens[ind].type == '(') {
      level++;
    } else if (tokens[ind].type == ')') {
      if (level == 0) {
        printf("unmatched )\n");
        *success = false;
        return 0;
      }
      level--;
      if (level == 0) {
        n_operand++;
      }
    }
  }
  if (level > 0) {
    printf("unclosed (\n");
    *success = false;
    return 0;
  }
  return n_operand;
}

bool is_binary_operator(int token_type) {
  return token_type == '+' || token_type == '-' || token_type == '*' || token_type == '/'
    || token_type == TK_EQ || token_type == TK_NOT_EQ || token_type == TK_AND;
}

bool is_unary_operator(int token_type) {
  return token_type == TK_POS || token_type == TK_NEG || token_type == TK_DEREF;
}

int binary_operator_precedence(int op) {
  switch (op) {
    case '*': case '/': return 1;
    case '+': case '-': return 2;
    case TK_EQ: case TK_NOT_EQ: return 3;
    case TK_AND: return 4;
    default: return 0;
  }
}

// return true if current_op should be evaluated later
bool binary_operator_cmp(int current_op, int previous_op) {
  if (previous_op == -1) return true;
  int p_current = binary_operator_precedence(current_op);
  int p_previous = binary_operator_precedence(previous_op);
  return p_current >= p_previous;
}

int find_main_operator(int left, int right, bool *success) {
  int level = 0;
  int ind = left;
  int main_op_ind = -1;
  int main_op_type = -1;
  while (ind <= right) {
    int token_type = tokens[ind].type;
    if (token_type == '(') {
      level++;
    } else if (token_type == ')') {
      level--;
    } else if (level == 0 && is_binary_operator(token_type)) {
      if (binary_operator_cmp(token_type, main_op_type)) {
        main_op_ind = ind;
        main_op_type = token_type;
      }
    }
    ind++;
  }
  return main_op_ind;
}

void print_tokens(int left, int right) {
  printf("eval: ");
  for (int i = left; i <= right; i++) {
    printf("%s ", tokens[i].str);
  }
  printf("\n");
}

uint32_t eval(int left, int right, bool *success) {
  // print_tokens(left, right);
  if (left > right) {
    printf("invalid expression\n");
    *success = false;
    return 0;
  }
  int n_operand = eval_count_operand(left, right, success);
  if (!*success) return 0;
  // printf("n_operand: %d\n", n_operand);
  if (n_operand == 0) {
    printf("invalid expression, no operand\n");
    *success = false;
    return 0;
  }
  if (n_operand == 1) {
    if (tokens[left].type == '(' && tokens[right].type == ')') {
      return eval(left + 1, right - 1, success);
    } else {
      if (is_operand(tokens[left].type)) {
        Assert(left == right, "single operand parse error, left %d right %d", left, right);
        if (tokens[left].type == TK_REG) {
          return isa_reg_str2val(tokens[left].str + 1, success);
        } else if (tokens[left].type == TK_NUMBER) {
          return atoi(tokens[left].str);
        } else if (tokens[left].type == TK_HEX_NUMBER) {
          int t = 0;
          sscanf(tokens[left].str, "%x", &t);
          return t;
        } else {
          printf("invalid operand: %s\n", tokens[left].str);
          return 0;
        }
      } else if (tokens[left].type == TK_POS) {
        return eval(left + 1, right, success);
      } else if (tokens[left].type == TK_NEG) {
        return -eval(left + 1, right, success);
      } else if (tokens[left].type == TK_DEREF) {
        uint32_t addr = eval(left + 1, right, success);
        return vaddr_read(addr, 4);
      } else {
        printf("invalid unary operator: %s\n", tokens[left].str);
        return 0;
      }
    }
  } else {
    int op_ind = find_main_operator(left, right, success);
    if (!*success) return 0;
    // printf("main binary operator at position %d: %s\n", op_ind, tokens[op_ind].str);
    uint32_t val1 = eval(left, op_ind - 1, success);
    uint32_t val2 = eval(op_ind + 1, right, success);
    switch (tokens[op_ind].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      case TK_EQ: return val1 == val2;
      case TK_NOT_EQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      default: Assert(0, "unknown operator %s", tokens[op_ind].str);
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  
  for (int i = 0; i < nr_token; i++) {
    if (i == 0 || !possible_binary_op_prior_type(tokens[i - 1].type)) {
      if (tokens[i].type == '*') {
        tokens[i].type = TK_DEREF;
      } else if (tokens[i].type == '+') {
        tokens[i].type = TK_POS;
      } else if (tokens[i].type == '-') {
        tokens[i].type = TK_NEG;
      }
    }
  }

  word_t result = eval(0, nr_token - 1, success);
  return result;
}
