#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fstream>
#include <unistd.h>
#include <thread>

using namespace std;

int create_socket(int);
int accept_conn(int);

#ifdef WINDOWS
 #include <direct.h>
 #define GetCurrentDir _getcwd
#else
 #include <unistd.h>
 #define GetCurrentDir getcwd
#endif

#define MAXLINE 4096 /*max text line length*/
#define LISTENQ 8 /*maximum number of client connections*/

size_t filesize(FILE *fd) {
  fseek (fd , 0 , SEEK_END);
  size_t lSize = ftell (fd);
  rewind (fd);
  return lSize;
}

void sendfile(int clientfd, char const *filename) {
  // transmission buffer
  char buffer[MAXLINE];
  memset(buffer,0,MAXLINE);

  // try to open the file
  FILE *file = fopen(filename,"r"); 

  // file doesn't exist
  if(file==nullptr) {
    snprintf(buffer, 8, "%08lu", 0ul);
    return;
  }

  // get length of file and send to client in ASCII
  // send exactly 8 characters
  size_t filelen = filesize(file);
  snprintf(buffer, 9, "%08lu", (unsigned long)filelen); // count includes trailing nul
  cout << "Sending file " << filename << " of " << filelen << "=" << buffer << "  bytes" << endl;
  send(clientfd, buffer, 8, 0);

  // now send the file
  size_t nread = fread(buffer, 1, MAXLINE, file);
  while(nread == MAXLINE) {
    send(clientfd, buffer, MAXLINE, 0);
    nread = fread(buffer,1,MAXLINE, file);
  }
  send(clientfd, buffer, nread, 0);
  fclose(file);
}

void session(int clientfd) {
  cout << "Session started" << endl;
  char buffer[MAXLINE];
  
  while (true) {
    // get command from client
    memset(buffer, 0, MAXLINE); 
    int nmsg = recv(clientfd, buffer, MAXLINE, 0);
    if(nmsg == 0) break; // client closed connection

    // parse command
    if(strncmp(buffer, "quit",4)==0) break; // client request termination
    if(strncmp(buffer, "get ",4)!=0) break; // client sent bad message
    sendfile(clientfd, buffer+4);
  }
  // close connection
  close(clientfd);
}

void server(int portno) {
  // create listening socket
  int listenfd = socket (AF_INET, SOCK_STREAM, 0);
  if (listenfd <= 0) {
    cerr<<"Problem in creating the socket"<<endl;
    exit(2);
  }

 //preparation of the socket address
 struct sockaddr_in listener;
 listener.sin_family = AF_INET;
 listener.sin_addr.s_addr = htonl(INADDR_ANY);
 listener.sin_port = htons(portno);

 //bind the address to the socket 
 int bindres = ::bind (listenfd, (struct sockaddr *) &listener, sizeof(listener));
 if(bindres != 0) {
   cerr << "Error binding address to socket, port " << portno<< " may be in use" << endl;
   exit(2);
 }
 
 // listenm for connections
 int listenres = listen(listenfd, LISTENQ);
 if(listenres != 0) {
   cerr << "Error listening on port " << portno << endl;
   exit(2);
 }
 

 // receive buffer for client address, ignored
 struct sockaddr_in clientaddr;
 socklen_t clientaddr_length = sizeof(sockaddr_in);

 while(true) { // forever
   // accept a connection
   int clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientaddr_length);

   // spawn a thread to handle the connection
   ::std::thread(session,clientfd).detach();
 } 

}

int main(int argc, char **argv) {

  if (argc !=2) {						//validating the input
    cerr<<"Usage: <port number>"<<endl;
    exit(1);
  }
  int portno = atoi(argv[1]);
  if(portno < 1024) {
	  cerr<<"Port number must be greater than 1024"<<endl;
	  exit(2);
  }
  server(portno);
  return 0;
}

/*

int main (int argc, char **argv)
{
 int listenfd, connfd, n;
 pid_t childpid;
 socklen_t clilen;
 char buf[MAXLINE];
 struct sockaddr_in cliaddr, servaddr;

 if (argc !=2) {						//validating the input
  cerr<<"Usage: <port number>"<<endl;
  exit(1);
 }

 //Create a socket for the soclet
 //If sockfd<0 there was an error in the creation of the socket
 if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
  cerr<<"Problem in creating the socket"<<endl;
  exit(2);
 }


 //preparation of the socket address
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 if(atoi(argv[1])<=1024){
	cerr<<"Port number must be greater than 1024"<<endl;
	exit(2);
 }
 servaddr.sin_port = htons(atoi(argv[1]));

 //bind the socket
 bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

 //listen to the socket by creating a connection queue, then wait for clients
 listen (listenfd, LISTENQ);

 cout<<"Server running...waiting for connections."<<endl;

 for ( ; ; ) {

  clilen = sizeof(cliaddr);
  //accept a connection
  connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);

  cout<<"Received request..."<<endl;

  if ( (childpid = fork ()) == 0 ) {//if it???s 0, it???s child process

  cout<<"Child created for dealing with client requests"<<endl;

  //close listening socket
  close (listenfd);
  int data_port=1024;						//for data connection
  while ( (n = recv(connfd, buf, MAXLINE,0)) > 0)  {
   cout<<"String received from client: "<<buf;
   char *token,*dummy;
   dummy=buf;
   token=strtok(dummy," ");

   if (strcmp("quit\n",buf)==0)  {
   	cout<<"The client has quit\n";
   }

  
   if (strcmp("get",token)==0)  {
	  char port[MAXLINE],buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE];
	  int datasock,lSize,num_blks,num_last_blk,i;
	  FILE *fp;
	  token=strtok(NULL," \n");
	  cout<<"Filename given is: "<<token<<endl;
	  data_port=data_port+1;
	  if(data_port==atoi(argv[1])){
  		data_port=data_port+1;
	  }
	  sprintf(port,"%d",data_port);
	  datasock=create_socket(data_port);				//creating socket for data connection
	  send(connfd, port,MAXLINE,0);					//sending port no. to client
	  datasock=accept_conn(datasock);	
	  int j;
		for(j=0;j<365;) {	
		 if ((fp=fopen(token,"r"))!=NULL) {
		//size of file
		  send(connfd,"1",MAXLINE,0);
			fseek (fp , 0 , SEEK_END);
			lSize = ftell (fp);
			rewind (fp);
			num_blks = lSize/MAXLINE;
			num_last_blk = lSize%MAXLINE; 
			sprintf(char_num_blks,"%d",num_blks);
			send(connfd, char_num_blks, MAXLINE, 0);
			//cout<<num_blks<<"	"<<num_last_blk<<endl;

			for(i= 0; i < num_blks; i++) { 
				fread (buffer,sizeof(char),MAXLINE,fp);
				send(datasock, buffer, MAXLINE, 0);
			//cout<<buffer<<"	"<<i<<endl;
			}
			sprintf(char_num_last_blk,"%d",num_last_blk);
			send(connfd, char_num_last_blk, MAXLINE, 0);
			if (num_last_blk > 0) { 
				fread (buffer,sizeof(char),num_last_blk,fp);
				send(datasock, buffer, MAXLINE, 0);
				//cout<<buffer<<endl;
			}
			fclose(fp);
			cout<<"File upload done.\n";
	  	}	
   	 else{
		  send(connfd,"0",MAXLINE,0);
			}
		 j++;
		 sleep(60);
   	}
   	}
   }
  if (n < 0)
   cout<<"Read error"<<endl;
   exit(0);
  }
 close(connfd);
 }
}

int create_socket(int port)
{
int listenfd;
struct sockaddr_in dataservaddr;


//Create a socket for the soclet
//If sockfd<0 there was an error in the creation of the socket
if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
cerr<<"Problem in creating the data socket"<<endl;
exit(2);
}


//preparation of the socket address
dataservaddr.sin_family = AF_INET;
dataservaddr.sin_addr.s_addr = htonl(INADDR_ANY);
dataservaddr.sin_port = htons(port);

if ((bind (listenfd, (struct sockaddr *) &dataservaddr, sizeof(dataservaddr))) <0) {
cerr<<"Problem in binding the data socket"<<endl;
exit(2);
}

 //listen to the socket by creating a connection queue, then wait for clients
 listen (listenfd, 1);

return(listenfd);
}

int accept_conn(int sock)
{
int dataconnfd;
socklen_t dataclilen;
struct sockaddr_in datacliaddr;

dataclilen = sizeof(datacliaddr);
  //accept a connection
if ((dataconnfd = accept (sock, (struct sockaddr *) &datacliaddr, &dataclilen)) <0) {
cerr<<"Problem in accepting the data socket"<<endl;
exit(2);
}

return(dataconnfd);
}
*/
