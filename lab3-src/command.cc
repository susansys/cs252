
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}

	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;

	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
    _append = 0;
    _out_flag = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}

	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}

		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

    if ( _errFile ) {
        free( _errFile );
     }

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
    _append = 0;
    _out_flag = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");

	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );

}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
        clear();
		prompt();
		return;
	}

	// Print contents of Command data structure
	print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
    execute_command();
	// Clear to prepare for next command
	clear();

	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::execute_command()
{
    if( !strcmp(_simpleCommands[0]->_arguments[0], "exit")) {
        printf("Bye!\n");
        exit( 1 );
    }
    if (_out_flag > 1) {
        printf("Ambiguous output redirect\n");
    }
    int defaultin = dup(0);
    int defaultout = dup(1);
    int defaulterr = dup(2);

    int pid;
    int fdpip[_numberOfSimpleCommands-1][2];
    // Pipe initialization
    for ( int i = 0; i<_numberOfSimpleCommands-1; i++) {
        if (pipe(fdpip[i]) == -1) {
            perror("pipe");
            exit(2);
        }
    }

    int infd;
    int outfd;
    int errfd;

    // Redirect input
    if(_inputFile) {
        infd = open(_inputFile, O_RDONLY);
        if(infd < 0) {
            perror("inputfile");
            exit(1);
        }
    }
    else {
        infd = dup(defaultin);
    }

    // Redirect output
    if(_outFile) {

        // >> out
        if(_append) {
            outfd = open(_outFile, O_RDWR |O_CREAT |O_APPEND, 0666);
            if(outfd < 0) {
                perror("outfile");
                exit(1);
            }

            // >>& out
            if(_errFile) {
                errfd = open(_errFile, O_RDWR |O_CREAT |O_APPEND, 0666);
                if(errfd < 0) {
                    perror("errfile");
                    exit(1);
                }
                //dup2(outfd, 2);
            }
        }
        else {

            // > out
            outfd = open(_outFile, O_RDWR |O_CREAT |O_TRUNC, 0666);
            if(outfd < 0) {
                perror("outfile");
                exit(1);
            }
            // >& out
            if(_errFile) {
                errfd = open(_errFile, O_RDWR |O_CREAT |O_APPEND, 0666);
                if(errfd < 0) {
                    perror("errfile");
                    exit(1);
                }
                //dup2(outfd, 2);
            }
        }
    }
    else{
        // Use default output and err;
        outfd = dup(defaultout);
        errfd = dup(defaulterr);
    }

    for (int i = 0; i < _numberOfSimpleCommands; i++) {
        // For first command
        if ( i == 0) {
            // in: infd
            // out: depends on if it's the only command
            // err: errfd
            dup2(infd, 0);

            // only 1 command
            // out: outfd
            if (_numberOfSimpleCommands == 1) {
                dup2(outfd, 1);
            }
            // more than 1 command
            // out: pipe
            else {
                dup2(fdpip[i][1], 1);
                close(fdpip[i][1]);
            }
            dup2(errfd, 2);
        }
        // For none last commands
        // in: pipe
        // out: pipe
        // err: errfd
        else if (i != _numberOfSimpleCommands - 1) {
            dup2(fdpip[i-1][0], 0);
            dup2(fdpip[i][1], 1);
            close(fdpip[i-1][0]);
            close(fdpip[i][1]);
            dup2(errfd, 2);
        }
        // For last command
        // in: pipe
        // out: fdout
        // err: errfd
        else {
            dup2(fdpip[i-1][0], 0);
            close(fdpip[i-1][0]);
            dup2(outfd, 1);
            dup2(errfd, 2);
        }

        // Create new process for the first command
        pid = fork();

        if (pid == -1) {
            perror("cat_grep: fork\n");
            exit(2);
        }

        if (pid == 0) {
            // close file descriptors
            close(defaultin);
            close(defaultout);
            close(defaulterr);

            for (int j = 0; j < _numberOfSimpleCommands-1; j++) {
                close(fdpip[j][0]);
                close(fdpip[j][1]);
            }
            if(_inputFile){
                close(infd);
            }
            if(_outFile){
                close(outfd);
            }
            if(_errFile){
                close(errfd);
            }

            execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);

            // if it returns, something is wrong
            perror("Unknown command when exec execvp\n");
            exit( 2 );
        }
        dup2(defaultin, 0);
        dup2(defaultout, 1);
        dup2(defaulterr, 2);

    }
    for ( int i = 0; i < _numberOfSimpleCommands - 1; i++) {
        close(fdpip[i][0]);
        close(fdpip[i][1]);
    }

    // Restore fd
    dup2(defaultin, 0);
    dup2(defaultout, 1);
    dup2(defaulterr, 2);
    close(defaultin);
    close(defaultout);
    close(defaulterr);

    if(_inputFile){
        close(infd);
    }
    if(_outFile){
        close(outfd);
    }
    if(_errFile){
        close(errfd);
    }

    if(!_background) {
        if(waitpid(pid, 0, 0) == -1) {
            perror("waitpid");
        }
    }
}

void
Command::prompt()
{
    if(isatty(0)) {
	    printf("myshell>");
    }
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

main()
{
	Command::_currentCommand.prompt();
	yyparse();
}
