
/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "y.tab.h"
#include <unistd.h>

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

////////////  Start added code ///////////

extern "C" char * read_line();

int mygetc(FILE * f) {
    static char *p;
    char ch;

    if (!isatty(0)) {
        // stdin is not a tty. Call real getc
        return getc(f);
    }

    // stdin is a tty. Call our read_line.

    if (p==NULL || *p == 0) {
        char * s = read_line();
        p = s;
    }

    ch = *p;
    p++;

    return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////////// End added code ///////////


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

 `[^\n`]*`       {
        // backticks
        printf("cought backticks, yytext is :%s\n", yytext);
        char buffer[1024];
        char** arguments = (char **)malloc(strlen(yytext) * sizeof(char *) );

        int i = 0;
        int j = 0;
        int k = 0;

        char * tmp = (char *)malloc(strlen(yytext) * sizeof(char));
        for( i = 0; i < strlen(yytext); i++) {
            if(yytext[i] == '`') {
                continue;
            }
            else if(yytext[i] == ' ') {
                tmp[j] == '\0';
                j = 0;
                arguments[k++] = strdup(tmp);
            }
            else {
                tmp[j++] = yytext[i];
            }
        }

        arguments[k] = NULL;
        while(arguments[i] != NULL) {
            printf("%s\n", arguments[i]);
            i++;
        }
        return -1;

        int defaultin = dup(0);
        int defaultout = dup(1);

        int pid;
        int parent[2];
        int child[2];

        if( pipe(parent) == -1) {
            perror("pipe");
            exit(2);
        }
        if( pipe(child) == -1) {
            perror("pipe");
            exit(2);
        }

        // Parent process:
        // infd: fdPipe[0]

        pid = fork();

        if (pid == -1) {
            perror("subshell: fork\n");
            exit(2);
        }
        else if (pid == 0) {
            // Subshell process:
            // outfd: fdPipe[1]

            dup2(parent[1], 1);
            dup2(child[0], 0);
            close(parent[1]);
            close(child[0]);

            // run the command
            execvp("shell", NULL);

            perror("Unknown command when exec execvp\n");
            exit(1);
        }
        else {
            // Parent process:
            // infd: fdPipe[0]
            dup2(child[1], 1);
            dup2(parent[0], 0);
            close(child[1]);
            close(parent[0]);
            // write commands to child process
            while(*arguments) {
                write(1, arguments, 1);
                arguments++;
            }
            i = 0;
            char c = 0;
            while( read(0, buffer+i, 1 ) != 0) {
                printf("%c\n", *(buffer+i));
                i++;
            }

            dup2(defaultout, 1);
            dup2(defaultin, 0);
            close(defaultin);
            close(defaultout);
        }

        waitpid(0, NULL, 0);

        i = 0;
        k = strlen(buffer);
        while(i < k) {
            if(buffer[k - 1 - i] == '\n') {
                unput(' ');
            }
            else {
                unput(buffer[k - 1 - i]);
            }
            i++;
        }
    }

\"[^\n\"]*\"    {
        // quotes
        char* string = (char*)malloc(sizeof(char) * 1024);
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
        // escape characters
        char* string = (char*) malloc(sizeof(char) * 1024);
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

[^ \t\n\|<>&][^ \t\n\|<>&]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}

 .  {

        return NOTOKEN;
    }

%%
