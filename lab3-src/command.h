
#ifndef command_h
#define command_h

// Command Data Structure


struct SimpleCommand {
	// Available space for arguments currently preallocated
	int _numberOfAvailableArguments;

	// Number of arguments
	int _numberOfArguments;
	char ** _arguments;

	SimpleCommand();
	void insertArgument( char * argument );
    void expandWildcardsIfNecessary(char * arg);
    void expandWildcards(char * arg);
    };

struct Command {
	int _numberOfAvailableSimpleCommands;
	int _numberOfSimpleCommands;
	SimpleCommand ** _simpleCommands;
	char * _outFile;
	char * _inputFile;
	char * _errFile;
	int _background;
    int _append;
    int _out_flag;

	void prompt();
	void print();
	void execute();
    void execute_command();
	void clear();

	Command();
    void insertSimpleCommand( SimpleCommand * simpleCommand );

	static Command _currentCommand;
	static SimpleCommand *_currentSimpleCommand;
};

#endif
