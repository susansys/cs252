
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
/*#include <unistd.h>*/

/*static  void yyunput (int c,char *buf_ptr  );*/

/*void myunputc(int c) {*/
    /*unput(c);*/
/*}*/

/*////////////  Start added code ///////////*/

/*extern "C" char * read_line();*/

/*int mygetc(FILE * f) {*/
    /*static char *p;*/
    /*char ch;*/

    /*if (!isatty(0)) {*/
        /*// stdin is not a tty. Call real getc*/
        /*return getc(f);*/
    /*}*/

    /*// stdin is a tty. Call our read_line.*/

    /*if (p==NULL || *p == 0) {*/
        /*char * s = read_line();*/
        /*p = s;*/
    /*}*/

    /*ch = *p;*/
    /*p++;*/

    /*return ch;*/
/*}*/

/*#undef getc*/
/*#define getc(f) mygetc(f)*/

/*/////////// End added code ///////////*/


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
            if(yytext[i] == '"'){
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
            // if it's \\, then change it to
            if(yytext[i] == 92) {
                if(yytext[i+1] == 92) {
                    string[j] = yytext[i+1];
                    i++;
                    j++;
                }
            }
            // copy everything else to string
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
