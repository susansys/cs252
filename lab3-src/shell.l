
/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include "y.tab.h"

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

%}

%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}

">" 	{
		return GREAT;
	}

"<"     {
        return LESS;
    }

">>"    {
        return GREATGREAT;
    }

">&"    {
        return GREATAMPERSAND;
    }

">>&"   {
        return GREATGREATAMPERSAND;
    }

"|"     {
        return PIPE;
    }

"&"     {
        return AMPERSAND;
    }

 \"[^\n\"]*\"    {
        // quotes
        char* string = (char*)malloc(sizeof(char) * 1028);
        int i = 0;
        int j = 0;
        for(i = 0; yytext[i] != '\0'; i++){
            if(yytext[i] == 34){
                continue;
            }
            string[j] = yytext[i];
            j++;
        }
        yylval.string_val = strdup(string);
        return WORD;
    }

[^ \t\n&<>\|]*\\[^ \t\n]*    {
        /* escape characters */
        char* string = (char*) malloc(1024*sizeof(char));
        int i = 0;
        int j = 0;
        for(i = 0; i != strlen(yytext); i++){
            if(yytext[i] == 92){
                // means escape char was hit
                if(yytext[i+1] == 92){
                    string[j] = yytext[i+1];
                    i++;
                    j++;
                }
            }
            else {
                string[j] = yytext[i];
                j++;
            }
        }
        yylval.string_val = strdup(string);
        return WORD;
    }

[^ \t\n][^ \t\n]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}
