
/*
 * CS-252 Spring 2013
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN GREAT LESS NEWLINE GREATGREAT GREATAMPERSAND GREATGREATAMPERSAND PIPE AMPERSAND

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <stdio.h>
#include "command.h"
void yyerror(const char * s);
int yylex();

%}

%%

goal:
	command_line
	;

command_line:
	simple_command
	| command_line simple_command
	;

simple_command:
	pipe_list iomodifier_list background_opt NEWLINE {
		/*printf("   Yacc: Execute command\n");*/
		Command::_currentCommand.execute();
	}
	| NEWLINE
	| error NEWLINE { yyerrok; }
	;

pipe_list:
    pipe_list PIPE command_and_args
    | command_and_args
    ;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
               /*printf("   Yacc: insert argument \"%s\"\n", $1);*/

	       Command::_currentSimpleCommand->insertArgument( $1 );\
	}
	;

command_word:
	WORD {
               /*printf("   Yacc: insert command \"%s\"\n", $1);*/

	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_list:
    iomodifier_list iomodifier_opt
    | /* can be empty */
    ;

iomodifier_opt:
	GREAT WORD {
		/*printf("   Yacc: insert output \"%s\"\n", $2);*/
        Command::_currentCommand._out_flag++;
		Command::_currentCommand._outFile = $2;
	}
    | GREATGREAT WORD {
		/*printf("   Yacc:  append output \"%s\"\n", $2);*/
        Command::_currentCommand._out_flag++;
        Command::_currentCommand._append = 1;
		Command::_currentCommand._outFile = $2;
	}
    | GREATGREATAMPERSAND WORD {
		/*printf("   Yacc: append output and stderr \"%s\"\n", $2);*/
        Command::_currentCommand._out_flag++;
        Command::_currentCommand._append = 1;
		Command::_currentCommand._outFile = $2;
        Command::_currentCommand._errFile = $2;
	}
    | GREATAMPERSAND WORD {
		/*printf("   Yacc: insert output and stderr  \"%s\"\n", $2);*/
        Command::_currentCommand._out_flag++;
		Command::_currentCommand._outFile = $2;
        Command::_currentCommand._errFile = $2;
	}
    | LESS WORD {
		/*printf("   Yacc: insert input \"%s\"\n", $2);*/
		Command::_currentCommand._inputFile = $2;
	} /* can be empty */
	;

background_opt:
    AMPERSAND {
        /*printf("     Yacc: background mode is on\n");*/
        Command::_currentCommand._background = 1;
    }
    | /* can be empty */
    ;

%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
