/*This will be the scanner that I made earlier :) */
/*go max! */

/* The lexer for the scanner that determines which character is what token */
/* Max Krishka-Pachal */
/* CPSC 411 */
/* Final Project */
/* Scanner.l */
/* May 10th 2022 */

/***********************************************************************/
/*** THIS IS BASED ON SHANKAR GANESHS EXAMPLE CODE SCANNER.L FROM M1 ***/
/***********************************************************************/

%option noyywrap
%option yylineno
%option noinput
%option nounput

/* declarations */
%{
#include <stdio.h>
#include "utils.h"
#include "y.tab.h"

int strPtr = 0; 
int lineCounter = 1;
int lineStartCount[LINE_START];
int lineCountIndex = 1;
%}

/* definitions */

chara       [a-zA-Z_]
    /* characters are lowercase and uppercase letters as well as underscores */
digit       [0-9]
    /* digits are single numbers between 0 and 9 */

number      {digit}* 
    /* a number is any amount of digits */
id          {chara}({chara}|{digit})*
    /* an identfier is any combination of characters and or digits, for any length */
    /* "//"([^("\r"|"\n"|<<EOF>>)]*)+ */
comment     "//"(.*)$
string      \"([^\\\"]|\\.)*\"

reserved    "true"|"false"|"boolean"|"int"|"void"|"if"|"else"|"while"|"break"|"return"
operator    "+"|"-"|"*"|"/"|"%"|"<"|">"|"<="|">="|"="|"=="|"!="|"!"|"||"|"&&"
special     ";"|","|"("|")"|"{"|"}"

 /* comments are any words that appear after slash slash until a newline or end of file */

/* rules */
%%
[ \t\r\b\f\']+      ; /* ignore the whitespace */
\n                  { lineCounter++; }

{comment}           { } /* pretty fancy, I redesigned the way I'm doing strings now */
{string}            { yylval.name = strdup(yytext); return STR; }

 /* reserved words */
"true"              { return TRUE; }
"false"             { return FALSE; } /*the yylval is to return the string values to the parser */
"boolean"           { yylval.vType = BOOL_T; return BOOL; }
"int"               { yylval.vType = INT_T; return INT; }
"void"              { yylval.vType = VOID_T; return VOID; }
"if"                { lineStartCount[lineCountIndex] = lineCounter; lineCountIndex++; return IF; }
"else"              { lineStartCount[lineCountIndex] = lineCounter; lineCountIndex++; return ELSE; }
"while"             { lineStartCount[lineCountIndex] = lineCounter; lineCountIndex++; return WHILE; }
"break"             { return BREAK; }
"return"            { return RETURN; }

 /* operators */    /* these guys all have a string name to return with them */
"+"                 { yylval.name = strdup(yytext); return ADD; }
"-"                 { yylval.name = strdup(yytext); return SUB; }
"*"                 { yylval.name = strdup(yytext); return MULT; }
"/"                 { yylval.name = strdup(yytext); return DIV; }
"%"                 { yylval.name = strdup(yytext); return PERC; }
"<"                 { yylval.name = strdup(yytext); return LT; }
">"                 { yylval.name = strdup(yytext); return GT; }
"<="                { yylval.name = strdup(yytext); return LE; }
">="                { yylval.name = strdup(yytext); return GE; }
"="                 { yylval.name = strdup(yytext); return ASSIGN; }
"=="                { yylval.name = strdup(yytext); return EQ; }
"!="                { yylval.name = strdup(yytext); return NEQ; }
"!"                 { yylval.name = strdup(yytext); return NOT; }
"||"                { yylval.name = strdup(yytext); return OR; }
"&&"                { yylval.name = strdup(yytext); return AND; }

 /* special characters */
";"                 { yylval.name = strdup(yytext); return SEMICOLON; }
","                 { yylval.name = strdup(yytext); return COMMA; }
"("                 { yylval.name = strdup(yytext); return LBRACK; }
")"                 { yylval.name = strdup(yytext); return RBRACK; }
"{"                 { yylval.name = strdup(yytext); lineStartCount[lineCountIndex] = lineCounter; lineCountIndex++; return LCURLY; }
"}"                 { yylval.name = strdup(yytext); return RCURLY; }

{id}                { yylval.name = strdup(yytext); return ID; }
{number}            { yylval.name = strdup(yytext); return NUM; }

 /* end of file */  /* end of the file is just a 0 now because the parser does all the work */
<<EOF>>             { return 0; } /* detects the end of the file */

.                   { return ERROR; } /* all other characters that dont fit anywhere in here are illegal and a warning is printed */
%%

