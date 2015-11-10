
const char * usage =
"                                                               \n"
"daytime-server:                                                \n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   daytime-server <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"In another window type:                                       \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  \n"
"is running. <port> is the port number you used when you run   \n"
"daytime-server.                                               \n"
"                                                               \n"
"Then type your name and return. You will get a greeting and   \n"
"the time of the day.                                          \n"
"                                                               \n";
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <signal.h>

int _GET = 0;
int _POST = 0;
int _ERROR = 0;
int _isText = 1;
int QueueLength = 5;
pthread_mutex_t mutex;
const char * protocal = "HTTP/1.1 ";
const char * Server_Type = "CS 252 Lab 5";
const char * sp = "\040";
const char * crlf = "\015\012";
const char * notFound =
"<html>\n"
"    <body>\n"
"        <h1>File Not Found</h1>\n"
"        <p>Please check your URL</p>\n"
"    </body>"
"</html>\n";

// Processes time request
void processRequest( int socket );
char * setDocPath(int socket);
char * setFilePath(char * docPath);
void checkFilePath(char * filePath, char * cwd);
void poolSlave(int socket);
void processRequestThread(int socket);

extern "C" void killZombie(int sig) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int
main( int argc, char ** argv )
{
  struct sigaction signalAction;
  signalAction.sa_handler = &killZombie;
  sigemptyset(&signalAction.sa_mask);
  signalAction.sa_flags=SA_RESTART;
  int err = sigaction(SIGCHLD, &signalAction, NULL);
  if(err) {
    perror("sigaction");
    exit(-1);
  }

  int port;
  int mode = 0;


  if ( argc == 1) {
    port = 9413;
  }
  else if ( argc == 2) {
    // Get the port from the arguments
    port = atoi( argv[1] );
  }
  else if ( argc == 3) {
    port = atoi( argv[2] );

    if(!strcmp(argv[1], "-f")) {
        mode = 1;
    }
    else if(!strcmp(argv[1], "-t")) {
        mode = 2;
    }
    else if(!strcmp(argv[1], "-p")) {
        // poolslave mode
        mode = 3;
    }
    else {
        fprintf(stderr, "%s", usage);
        exit( -1 );
    }
  }
  else {
    fprintf(stderr, "%s", usage);
    exit( -1 );
  }

  if ( port < 1024 || port > 65536) {
    fprintf(stderr, "%s", usage);
    exit( -1 );
  }
  // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress;
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);
  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }

  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1;
  err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR,
		       (char *) &optval, sizeof( int ) );

  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
		    (struct sockaddr *)&serverIPAddress,
		    sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }

  // Put socket in listening mode and set the
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }

  if (mode == 3) {
    pthread_t tid[5];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    pthread_mutex_init(&mutex, NULL);

    for(int i = 0; i< 5; i++) {
        pthread_create(&tid[i], &attr, (void *(*)(void *)) poolSlave, (void *) masterSocket);
    }

    //pthread_create(&t1, &attr, (void *(*)(void *))poolSlave, (void *) masterSocket);
    //pthread_create(&t2, &attr, (void *(*)(void *))poolSlave, (void *) masterSocket);
    //pthread_create(&t3, &attr, (void *(*)(void *))poolSlave, (void *) masterSocket);
    //pthread_create(&t4, &attr, (void *(*)(void *))poolSlave, (void *) masterSocket);
    //pthread_create(&t5, &attr, (void *(*)(void *))poolSlave, (void *) masterSocket);

    pthread_join(tid[0], NULL);
  }

  else {
    while ( 1 ) {

        // Accept incoming connections
        struct sockaddr_in clientIPAddress;
        int alen = sizeof( clientIPAddress );
        int slaveSocket = accept( masterSocket,
                      (struct sockaddr *)&clientIPAddress,
                      (socklen_t*)&alen);

        if ( slaveSocket < 0 ) {
          perror( "accept" );
          //exit( -1 );
          continue;
        }

        if (mode == 0) {
            if (slaveSocket < 0) {
                perror("accept");
                exit(-1);
            }
            // Process request.
            processRequest( slaveSocket );
            // Close socket
            close( slaveSocket );
        }

        else if(mode == 1) {
            int pid = fork();
            if(pid == 0) {
                // Process request.
                processRequest( slaveSocket );
                // Close socket
                close( slaveSocket );
                exit(1);
            }

            close(slaveSocket);
        }

        else if(mode == 2) {
            pthread_t tid;
            pthread_attr_t attr;

            pthread_attr_init(&attr);
            pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

            pthread_create(&tid, &attr, (void *(*)(void *)) processRequestThread, (void *)slaveSocket);
        }
    }
  }

}

void
processRequestThread(int socket) {
    processRequest(socket);
    close(socket);
}

void
poolSlave(int socket) {
    while(1) {
        struct sockaddr_in clientIPAddress;
        int alen = sizeof(clientIPAddress);

        pthread_mutex_lock(&mutex);
        int slaveSocket = accept(socket, (struct sockaddr *)&clientIPAddress, (socklen_t*)&alen);
        pthread_mutex_unlock(&mutex);

        processRequest(slaveSocket);
        close(slaveSocket);
    }
}

char * setDocPath( int socket) {
  char * path;
  char currString[1024];
  unsigned char veryNewChar= ' ';
  unsigned char newChar=' ';
  unsigned char oldChar=' ';
  unsigned char veryOldChar=' ';
  int c;

  int i = 0;
  int k = 0;
  fprintf(stderr, "Start: -------------------------------------\n");
  while( (c = read( socket, &veryNewChar, sizeof(newChar) > 0) )) {
    fprintf(stderr, "%c", newChar);
    if( veryOldChar == '\015' && oldChar == '\012' &&
            newChar == '\015' && veryNewChar == '\012') {
        break;
    }
    if( veryNewChar == ' ') {
        k++;
        currString[i] = '\0';
        i = 0;
        if(!strcmp(currString, "GET")) {
            _GET = 1;
        }
        else if(!strcmp(currString, "POST")) {
            _POST = 1;
        }
        else if(k == 2){
            path = strdup(currString);
        }
    }
    else {
        veryOldChar = oldChar;
        oldChar = newChar;
        newChar = veryNewChar;
        currString[i++] = veryNewChar;
    }
  }
  return path;
}

char * setFilePath( char * docPath, char * cwd ) {
  char * filePath = (char *)malloc(sizeof(char)*1024);

  if( strstr(docPath, "/icons") || strstr(docPath, "/htdocs")) {
    strcpy(filePath, cwd);
    strcat(filePath, "/http-root-dir/");
    strcat(filePath, docPath);
  }
  else if( !strcmp(docPath, "/") ) {
    strcpy(filePath, cwd);
    strcat(filePath, "/http-root-dir/htdocs/index.html");
  }
  else {
    strcpy(filePath, cwd);
    strcat(filePath, "/http-root-dir/htdocs");
    strcat(filePath, docPath);
  }
  free(cwd);
  return filePath;
}

void
checkFilePath(char * filePath, char * cwd) {
  char * minpath = (char *)malloc(sizeof(char)*1024);
  char * buffer = (char *)malloc(sizeof(char)*1024);
  strcpy(minpath, cwd);
  strcat(minpath, "/http-root-dir");

  if(strstr(filePath, "..")) {
    char * realPath = realpath(filePath, buffer);
    filePath = strdup(realPath);
  }
  if( strlen(filePath) < strlen(minpath) ) {
    _ERROR = 1;
    fprintf(stderr, "The URL is invalid.");
  }
}

bool
endsWith(char * string, char * ending) {
  int i = strlen(ending);
  int j = strlen(string);
  while(i) {
    if( string[j-1] == ending[i-1]) {
        i--;
        j--;
        continue;
    }
    else {
        return false;
    }
  }
  return true;
}

char * determineContentType(char * filePath) {
  char * contentType;
  if (endsWith(filePath, (char *)".html") || endsWith(filePath, (char *)".html/")) {
    contentType = strdup("text/html");
  }
  else if( endsWith(filePath, (char *)".gif") || endsWith(filePath, (char *)".gif/")) {
    contentType = strdup("image/gif");
    _isText = 0;
  }
  else {
    contentType = strdup("text/plain");
  }
  return contentType;
}

void
processRequest( int socket )
{
  char * cwd = (char *)malloc(sizeof(char)*256);
  char* docPath;
  char* filePath;
  char* contentType;
  int length = 0;
  FILE * doc;

  cwd = getcwd(cwd, 256);

  docPath = setDocPath(socket);
  filePath = setFilePath(docPath, cwd);
fprintf(stderr, "FilePath before check is: %s\n", filePath);
  checkFilePath(filePath, cwd);
  contentType = determineContentType(filePath);

fprintf(stderr, "--------------------------------\n");
fprintf(stderr, "Docpath is: %s\n", docPath);
fprintf(stderr, "FilePath is: %s\n", filePath);

  if( _isText) {
    doc = fopen(filePath, "r");
  }
  else {
    doc = fopen(filePath, "rb");
  }
  if(doc == NULL) {
    perror("open");
    _ERROR = 1;
  }

  if ( _ERROR) {
    // HTTP/1.0 <sp> 404 File Not Found <crlf>
    // Server: <sp> <Server-Type> <crlf>
    // Content-type: <sp> <Document-Type> <crlf>
    // <crlf>
    // <Error Message>
    _ERROR = 0;

    write(socket, protocal, strlen(protocal));
    write(socket, "404 File Not Found", 18);
    write(socket, crlf, strlen(crlf));
    write(socket, "Server: ", 8);
    write(socket, Server_Type, strlen(Server_Type));
    write(socket, crlf, strlen(crlf));
    write(socket, "Content-type: ", 14);
    write(socket, "text/html", 9);
    write(socket, crlf, strlen(crlf));
    write(socket, crlf, strlen(crlf));
    write(socket, notFound, strlen(notFound));
  }
  else {
    int file = fileno(doc);

    if (file > 0) {
      write(socket, protocal, strlen(protocal));
      write(socket, "200 ", 4);
      write(socket, "Document ", 9);
      write(socket, "follows", 7);
      write(socket, crlf, strlen(crlf));
      write(socket, "Server: ", 8);
      write(socket, Server_Type, strlen(Server_Type));
      write(socket, crlf, strlen(crlf));
      write(socket, "Content-type: ", 14);
      write(socket, contentType, strlen(contentType));
      write(socket, crlf, strlen(crlf));
      write(socket, crlf, strlen(crlf));

      char c;
      int count = 0;

      while(count = read(file, &c, sizeof(c))) {
        if(write(socket, &c, sizeof(c)) != count) {
          perror("write");
          break;
        }
      }
      //write(socket, protocal, strlen(protocal));
      //write(socket, "404 File Not Found", 18);
      //write(socket, crlf, strlen(crlf));
      //write(socket, "Server: ", 8);
      //write(socket, Server_Type, strlen(Server_Type));
      //write(socket, crlf, strlen(crlf));
      //write(socket, "Content-type: ", 14);
      //write(socket, "text/html", 9);
      //write(socket, crlf, strlen(crlf));
      //write(socket, crlf, strlen(crlf));
      //write(socket, notFound, strlen(notFound));
      fclose(doc);
fprintf(stderr, "**********Completed***********\n");
    }
  }
}
