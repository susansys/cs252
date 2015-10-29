/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_BUFFER_LINE 2048

// Buffer where line is stored
int line_length;;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
char history[100][1024] = {};
int historyCount = 0;
int historyIndex = 0;


void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {
  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  int len = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32 && ch!=127 && ch!=27) {
      // It is a printable character. 

		if (len != line_length) {
			// shift all chars in buffer and add new one
			int x = line_length-1;
			for (x = line_length-1; x >= len; x--) {
				char t = line_buffer[x];
				line_buffer[x] = line_buffer[x-1];
				line_buffer[x+1] = t;
			}

			// place new char in buffer
			line_buffer[len] = ch;

			// 27 91 67
			int boo = line_length - len;
			for (x = 0; x < boo; x++) {
				char a = 27; char b = 91; char c = 67;
				write(1,&a,1); write(1,&b,1); write(1,&c,1);
			}

			// Print backspaces
			int i = 0;
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}

			// Print spaces on top
			for (i =0; i < line_length; i++) {
			  ch = ' ';
			  write(1,&ch,1);
			}

			// Print backspaces
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}	
			
			// echo line
			line_length++;
			write(1, line_buffer, line_length);

			// Print backspaces
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}	
			line_buffer[line_length] ='\0';
			
			len++;

			for (x = 0; x < len; x++) {
				char a = 27; char b = 91; char c = 67;
				write(1,&a,1); write(1,&b,1); write(1,&c,1);
			}			
		}
		else {

			  // Do echo
			  write(1,&ch,1);

			  // If max number of character reached return.
			  if (line_length==MAX_BUFFER_LINE-2) break; 

			  // add char to buffer.
			  line_buffer[line_length]=ch;
			  line_length++;
			  len++;
		}
    }
	
	// home key
	else if (ch==1) {
		// Print backspaces
		int i = 0;
		
		for (i =0; i < len; i++) {
			ch = 8;
			write(1,&ch,1);
		}	

		len = 0;
	}
    // other end
	else if (ch == 5) {
		// 27 91 67
			int boo = line_length - len;
			int x = 0;
			for (x = 0; x < boo; x++) {
				char a = 27; char b = 91; char c = 67;
				write(1,&a,1); write(1,&b,1); write(1,&c,1);
			}
			len = line_length;
	}

	// other backspace
	else if (ch == 4) {

				if (len == -1) {
				}
				else {
					// shift all chars in buffer and add new one
					// int x = len + 1;
					char test = line_buffer[len];
					if (test == NULL) {}
					else { 
						int x = len+1;
						for (; x < line_length; x++) {
							char t = line_buffer[x];
							line_buffer[x] = line_buffer[x+1];
							line_buffer[x-1] = t;
						}

						// 27 91 67
						int boo = line_length - len;
						for (x = 0; x < boo; x++) {
							char a = 27; char b = 91; char c = 67;
							write(1,&a,1); write(1,&b,1); write(1,&c,1);
						}
			
						// Print backspaces
						int i = 0;
						for (i =0; i < line_length; i++) {
						  ch = 8;
						  write(1,&ch,1);
						}

						// Print spaces on top
						for (i =0; i < line_length; i++) {
						  ch = ' ';
						  write(1,&ch,1);
						}

						// Print backspaces
						for (i =0; i < line_length; i++) {
						  ch = 8;
						  write(1,&ch,1);
						}	
			
						// echo line
						line_length--;
						write(1, line_buffer, line_length);
			
						// Print backspaces
						for (i =0; i < line_length; i++) {
						  ch = 8;
						  write(1,&ch,1);
						}	
						line_buffer[line_length] ='\0';
			
						//len--;

						for (x = 0; x < len; x++) {
							char a = 27; char b = 91; char c = 67;
							write(1,&a,1); write(1,&b,1); write(1,&c,1);
						}
					}
				}
			}

	// other stuff
	else if (ch==27) {
		char x;
		char y;
		read(0, &x, 1);
		read(0, &y, 1);

		// left arrow
		if (x==91 && y==68) {
			if (len == 0) {
				;
			} 
			else {
				// Go back one character
			 	ch = 8;
			  	write(1,&ch,1);
				len--;
			}
		}
		// right arrow
		else if (x==91 && y==67) {
			if (len == line_length) {
				;
			} 
			else {
				write(1,&ch,1);	
				write(1,&x,1);	
				write(1,&y,1);	
				len++;	
			}
		}
		else if (x==91 && y==65) {
			// Up arrow. Print next line in history.
			if (historyIndex == 0) {
			}
			//else if (historyIndex == historyCount) { historyIndex = historyCount - 1;}
			else {
			historyIndex--;
			// Erase old line
			// Print backspaces
			int i = 0;
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}

			// Print spaces on top
			for (i =0; i < line_length; i++) {
			  ch = ' ';
			  write(1,&ch,1);
			}

			// Print backspaces
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}	

			// Copy line from history
			strcpy(line_buffer, history[historyIndex]);
			line_length = strlen(line_buffer);

			len = line_length;
			
			// echo line
			write(1, line_buffer, line_length);
			}
	  	}
		else if (x==91 && y==66) {
			// Down arrow. Print next line in history.
			if (historyIndex == historyCount) {
				
			}
			//else if (historyIndex < 0) { historyIndex = 0;}
			else {
			historyIndex++;
			// Erase old line
			// Print backspaces
			int i = 0;
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}

			// Print spaces on top
			for (i =0; i < line_length; i++) {
			  ch = ' ';
			  write(1,&ch,1);
			}

			// Print backspaces
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}	

			// Copy line from history
			strcpy(line_buffer, history[historyIndex]);
			line_length = strlen(line_buffer);

			len = line_length;
			
			// echo line
			write(1, line_buffer, line_length);
			}
	  	}
		else if (x==79 && y==70) {
			// 27 91 67
			int x = 0;
			int boo = line_length - len;
			for (x = 0; x < boo; x++) {
				char a = 27; char b = 91; char c = 67;
				write(1,&a,1); write(1,&b,1); write(1,&c,1);
			}
			len = line_length;
		}
		else {
			
			char z;
			read(0, &z, 1);
			// delete character
			if (x==91 && y==51 &&  z==126) {
				if (len == -1) {
				}
				else {
					// shift all chars in buffer and add new one
					// int x = len + 1;
					char test = line_buffer[len];
					if (test == NULL) {}
					else { 
						int x = len+1;
						for (; x < line_length; x++) {
							char t = line_buffer[x];
							line_buffer[x] = line_buffer[x+1];
							line_buffer[x-1] = t;
						}

						// 27 91 67
						int boo = line_length - len;
						for (x = 0; x < boo; x++) {
							char a = 27; char b = 91; char c = 67;
							write(1,&a,1); write(1,&b,1); write(1,&c,1);
						}
			
						// Print backspaces
						int i = 0;
						for (i =0; i < line_length; i++) {
						  ch = 8;
						  write(1,&ch,1);
						}

						// Print spaces on top
						for (i =0; i < line_length; i++) {
						  ch = ' ';
						  write(1,&ch,1);
						}

						// Print backspaces
						for (i =0; i < line_length; i++) {
						  ch = 8;
						  write(1,&ch,1);
						}	
			
						// echo line
						line_length--;
						write(1, line_buffer, line_length);
			
						// Print backspaces
						for (i =0; i < line_length; i++) {
						  ch = 8;
						  write(1,&ch,1);
						}	
						line_buffer[line_length] ='\0';
			
						//len--;

						for (x = 0; x < len; x++) {
							char a = 27; char b = 91; char c = 67;
							write(1,&a,1); write(1,&b,1); write(1,&c,1);
						}
					}
				}
			}
		}
	}
    else if (ch==10) {
      // <Enter> was typed. Return line
	  //history[historyCount] = (char*) malloc (1024*sizeof(char));
	  line_buffer[line_length] = '\0';
      memcpy(history[historyCount++], line_buffer, sizeof(line_buffer)+1);
	  historyIndex = historyCount;
      // Print newline
      write(1,&ch,1);


      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 127 || ch == 8) {
      // <backspace> was typed. Remove previous character read.
	  
      // Go back one character
		if (len == 0) {
		}
		else {
			// shift all chars in buffer and add new one
			// int x = len + 1;
			int x = len;
			for (; x < line_length; x++) {
				char t = line_buffer[x];
				line_buffer[x] = line_buffer[x+1];
				line_buffer[x-1] = t;
			}

			// 27 91 67
			int boo = line_length - len;
			for (x = 0; x < boo; x++) {
				char a = 27; char b = 91; char c = 67;
				write(1,&a,1); write(1,&b,1); write(1,&c,1);
			}
			
			// Print backspaces
			int i = 0;
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}

			// Print spaces on top
			for (i =0; i < line_length; i++) {
			  ch = ' ';
			  write(1,&ch,1);
			}

			// Print backspaces
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}	
			
			// echo line
			line_length--;
			write(1, line_buffer, line_length);
			
			// Print backspaces
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}	
			line_buffer[line_length] ='\0';
			
			len--;

			for (x = 0; x < len; x++) {
				char a = 27; char b = 91; char c = 67;
				write(1,&a,1); write(1,&b,1); write(1,&c,1);
			}
		}
    }
  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  return line_buffer;
}
