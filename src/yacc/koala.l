/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

%{

#include "parser.h"

#define YY_EXTRA_TYPE ParserState *

#define yyps ((ParserState *)yyextra)

#define YY_USER_ACTION \
do { \
  yylloc->first_line = yyps->line.row; \
  yylloc->last_line = yyps->line.row; \
  yylloc->first_column = yyps->line.col + 1; \
  yylloc->last_column = yylloc->first_column + strlen(yytext); \
  Lexer_DoUserAction(yyps, yytext); \
} while (0);

#define YY_INPUT(buf, result, max_size) \
do { \
  result = Lexer_DoYYInput(yyps, buf, max_size, yyin); \
  if (result <= 0) \
    result = YY_NULL; \
} while (0);

#define RETURN(token) \
  yyps->lastToken = token; \
  return token;

static int Lexer_NeedInsertSemiColon(ParserState *ps)
{
  static int tokens[] = {
    ID, INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL,
    BREAK, CONTINUE, FALLTHROUGH, TOKEN_RETURN,
    ')', ']', '}',
    CHAR, BYTE, INTEGER, FLOAT, STRING, BOOL, ANY, ELLIPSIS
  };
  int lastToken = ps->lastToken;
  for ( int i = 0; i < nr_elts(tokens); i++) {
    if (tokens[i] == lastToken) return 1;
  }
  return 0;
}

#define NEWLINE() \
do { \
  if (Lexer_NeedInsertSemiColon(yyps)) { \
    RETURN(';'); \
  } \
} while (0);

%}

%option bison-bridge
%option reentrant
/* %option header-file="koala_lex.h" */
%option bison-locations
%option yylineno
%option nounput
%option noinput
%option noyywrap
%option nointeractive

D   [0-9]
H   [a-fA-F0-9]
L   [a-zA-Z_]

%%

[\+\-\*\/\%&\|=><,.;\{\}\[\]\(\)^~:] {
  RETURN(yytext[0]);
}

"=="             { RETURN(EQ);              }
"!="             { RETURN(NE);              }
">="             { RETURN(GE);              }
"<="             { RETURN(LE);              }
"and"            { RETURN(AND);             }
"or"             { RETURN(OR);              }
"not"            { RETURN(NOT);             }
"&&"             { RETURN(AND);             }
"||"             { RETURN(OR);              }
"!"              { RETURN(NOT);             }
">>"             { RETURN(RSHIFT);          }
"<<"             { RETURN(LSHIFT);          }
":="             { RETURN(TYPELESS_ASSIGN); }
"+="             { RETURN(PLUS_ASSGIN);     }
"-="             { RETURN(MINUS_ASSIGN);    }
"*="             { RETURN(MULT_ASSIGN);     }
"/="             { RETURN(DIV_ASSIGN);      }
"%="             { RETURN(MOD_ASSIGN);      }
"&="             { RETURN(AND_ASSIGN);      }
"|="             { RETURN(OR_ASSIGN);       }
"^="             { RETURN(XOR_ASSIGN);      }
">>="            { RETURN(RSHIFT_ASSIGN);   }
"<<="            { RETURN(LSHIFT_ASSIGN);   }
"..."            { RETURN(ELLIPSIS);        }
"**"             { RETURN(POWER);           }
"..<"            { RETURN(DOTDOTLESS);      }
">.."            { RETURN(GREATERDOTDOT);   }

"if"             { RETURN(IF);              }
"else"           { RETURN(ELSE);            }
"while"          { RETURN(WHILE);           }
"do"             { RETURN(DO);              }
"for"            { RETURN(FOR);             }
"switch"         { RETURN(SWITCH);          }
"case"           { RETURN(CASE);            }
"fallthrough"    { RETURN(FALLTHROUGH);     }
"break"          { RETURN(BREAK);           }
"continue"       { RETURN(CONTINUE);        }
"default"        { RETURN(DEFAULT);         }
"var"            { RETURN(VAR);             }
"func"           { RETURN(FUNC);            }
"return"         { RETURN(TOKEN_RETURN);    }
"class"          { RETURN(CLASS);           }
"trait"          { RETURN(TRAIT);           }
"extends"        { RETURN(EXTENDS);         }
"with"           { RETURN(WITH);            }
"const"          { RETURN(CONST);           }
"package"        { RETURN(PACKAGE);         }
"import"         { RETURN(IMPORT);          }
"go"             { RETURN(GO);              }
"defer"          { RETURN(DEFER);           }
"typealias"      { RETURN(TYPEALIAS);       }

"char"           { RETURN(CHAR);            }
"byte"           { RETURN(BYTE);            }
"int"            { RETURN(INTEGER);         }
"float"          { RETURN(FLOAT);           }
"bool"           { RETURN(BOOL);            }
"string"         { RETURN(STRING);          }
"error"          { RETURN(ERROR);           }
"Any"            { RETURN(ANY);             }

"self"           { RETURN(SELF);            }
"super"          { RETURN(SUPER);           }
"true"           { RETURN(TOKEN_TRUE);      }
"false"          { RETURN(TOKEN_FALSE);     }
"nil"            { RETURN(TOKEN_NIL);       }

"[]"+ {
  yylval->Dims = yyleng/2;
  return DIMS;
}

([1-9][0-9]*|[0]) {
  yylval->IVal = atoi(yytext);
  RETURN(INT_LITERAL);
}

0[xX]{H}+ {
  yylval->IVal = (int64)strtoul(yytext, NULL, 16);
  RETURN(INT_LITERAL);
}

0[0-7]* {
  yylval->IVal = (int64)strtoul(yytext, NULL, 8);
  RETURN(INT_LITERAL);
}

[0-9]+[.][0-9]+ {
  yylval->FVal = atof(yytext);
  RETURN(FLOAT_LITERAL);
}

(\"(([^\"]|\\\")*[^\\])?\") {
  yylval->SVal = AtomString_New_NStr(yytext + 1, yyleng - 2);
  RETURN(STRING_LITERAL);
}

{L}({L}|{D})* {
  assert(yyleng < 64);
  yylval->SVal = AtomString_New_NStr(yytext, yyleng + 1);
  RETURN(ID);
}

[\n] {
  NEWLINE();
}

(\'.\') { RETURN(CHAR_LITERAL); }

[\t\r ]+ {}

"/*"(([^\*]*(\*[^\/])?)*)"*/" {}

"#"[^\n]* {}

"//"[^\n]* {}

. {}

%%