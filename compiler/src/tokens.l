%{


#include <string>
#include "node.h"

using namespace staple;

#include "parser.hpp"


/* handle locations */
int yycolumn = 1;
#define YY_USER_ACTION yylloc.filename = filename; \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; yylloc.last_column = yycolumn+yyleng-1; \
    yycolumn += yyleng;



#define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng)
#define TOKEN(t) (yylval.token = t)
extern "C" int yywrap() { }
%}

%x comment

%option yylineno

%%

"/*"                    BEGIN(comment);
<comment>[^*\n]*        /* eat anything that's not a '*' */
<comment>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
<comment>\n             yycolumn = 1;
<comment>"*"+"/"        BEGIN(INITIAL);

[ \t]                   ;
\n                      { yycolumn = 1; }
"extern"                return TOKEN(TEXTERN);
"class"                 return TOKEN(TCLASS);
"if"                    return TOKEN(TIF);
"else"                  return TOKEN(TELSE);
"return"                return TOKEN(TRETURN);
"@"                     return TOKEN(TAT);
"new"                   return TOKEN(TNEW);
"sizeof"                return TOKEN(TSIZEOF);
"include"               return TOKEN(TINCLUDE);
"extends"               return TOKEN(TEXTENDS);
\"([^\\\"]|\\.)*\"      SAVE_TOKEN; return TSTRINGLIT;
[a-zA-Z_][a-zA-Z0-9_]*  SAVE_TOKEN; return TIDENTIFIER;
[0-9]+\.[0-9]*          SAVE_TOKEN; return TDOUBLE;
[0-9]+                  SAVE_TOKEN; return TINTEGER;
"="                     return TOKEN(TEQUAL);
"=="                    return TOKEN(TCEQ);
"!="                    return TOKEN(TCNE);
"<"                     return TOKEN(TCLT);
"<="                    return TOKEN(TCLE);
">"                     return TOKEN(TCGT);
">="                    return TOKEN(TCGE);
"("                     return TOKEN(TLPAREN);
")"                     return TOKEN(TRPAREN);
"{"                     return TOKEN(TLBRACE);
"}"                     return TOKEN(TRBRACE);
"["                     return TOKEN(TLBRACKET);
"]"                     return TOKEN(TRBRACKET);
"."                     return TOKEN(TDOT);
","                     return TOKEN(TCOMMA);
"+"                     return TOKEN(TPLUS);
"-"                     return TOKEN(TMINUS);
"*"                     return TOKEN(TMUL);
"/"                     return TOKEN(TDIV);
";"                     return TOKEN(TSEMI);
"!"                     return TOKEN(TNOT);
"..."                   return TOKEN(TELLIPSIS);
.                       printf("Unknown token!\n"); yyterminate();

%%
