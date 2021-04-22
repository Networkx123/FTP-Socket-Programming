#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <string.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace std;

int create_socket(int,char *);

#ifdef WINDOWS
 #include <direct.h>
 #define GetCurrentDir _getcwd
#else
 #include <unistd.h>
 #define GetCurrentDir getcwd
#endif

#define MAXLINE 4096 /*max text line length*/

void getfile(int serverfd, char const *filename) {
  char buffer[MAXLINE];
  char tmpname[512];

  cout << "get " << filename << endl;

  // format and send request
  memset(buffer,0,MAXLINE);
  snprintf(buffer, MAXLINE, "get %s", filename);
  send(serverfd, buffer, strlen(buffer), 0);

  // get file length, exactly 8 chars
  memset(buffer,0,MAXLINE);
  recv(serverfd, buffer, 8, 0);
  size_t nominal_file_size = atoi(buffer);
  cout << "Nominal file size " << buffer << "=" << nominal_file_size << endl;

  // create temporary file to hold data
  memset(tmpname,0,512);
  pid_t pid = getpid(); // process ID
  snprintf(tmpname, 512, "download-%8lu.tmp", (unsigned long)pid);
  FILE *tmp = fopen(tmpname,"w");

  // fetch and save the data
  size_t bytes_read = 0;
  memset(buffer, 0, MAXLINE);
  size_t bytes_received = recv (serverfd, buffer, MAXLINE, 0);
  bytes_read += bytes_received;
  while (bytes_received == MAXLINE) {
    fwrite(buffer, 1, MAXLINE, tmp);
    bytes_received = recv (serverfd, buffer, MAXLINE, 0);
    bytes_read += bytes_received;
  }
  fwrite(buffer, 1, bytes_received, tmp);
  fclose(tmp);

  // verify file length
  assert (bytes_read == nominal_file_size);

  // update the target file atomically
  rename(tmpname, filename);
}

void client (char *inet_addr_as_char, int portno, char const *filename, int repeatcount, int delay) {
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr =  inet_addr(inet_addr_as_char);
  servaddr.sin_port = htons(portno);

  int serverfd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverfd <0) {
    cerr<<"Problem in creating socket"<<endl;
    exit(2);
  }

  //Connection of the client to the socket
  int connectres = connect(serverfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
  if (connectres<0) {
    cerr<<"Problem in connecting to the server"<<endl;
    exit(3);
  }

  // fetch the file repeatedly with delay
  while(repeatcount != 0) {
    if (repeatcount > 0) --repeatcount; 
    getfile(serverfd, filename);
    std::this_thread::sleep_for(std::chrono::seconds(delay));
  }

  // shutdown connection
  close(serverfd);
}


int main (int argc, char**argv) {
  if(argc != 6) {
    cerr << "Usage: client ipaddress port filename repeatcount delay" << endl;
    exit(2);
  }

  char *inet_addr = argv[1];

  int portno = atoi(argv[2]);
  if(portno<1024) {
    cerr << "Portno must be greater than 1023" << endl;
    exit(2);
  }
  char *filename = argv[3];
  int repeatcount = atoi(argv[4]);
  int delay = atoi(argv[5]); 

  cout << "Fetch " << filename << 
    " from " << inet_addr << ":" << portno << " " << 
    repeatcount << " times, evey " << delay << " seconds" << endl;

  client (inet_addr, portno, filename, repeatcount, delay); 
  return 0;
}

/*
int
main(int argc, char **argv)
{
 int sockfd;
 struct sockaddr_in servaddr;
 char sendline[MAXLINE], recvline[MAXLINE];

 //basic check of the arguments
 //additional checks can be inserted
 if (argc !=3) {
  cerr<<"Usage: ./a.out <IP address of the server> <port number>"<<endl;
  exit(1);
 }

 //Create a socket for the client
 //If sockfd<0 there was an error in the creation of the socket
 if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
  cerr<<"Problem in creating the socket"<<endl;
  exit(2);
 }

 //Creation of the socket
 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr= inet_addr(argv[1]);
 servaddr.sin_port =  htons(atoi(argv[2])); //convert to big-endian order

 //Connection of the client to the socket
 if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
  cerr<<"Problem in connecting to the server"<<endl;
  exit(3);
 }

 cout<<"ftp>";

 while (fgets(sendline, MAXLINE, stdin) != NULL) {
  send(sockfd, sendline, MAXLINE, 0);
  char *token,*dummy;
  dummy=sendline;
  token=strtok(dummy," ");
  if (strcmp("quit\n",sendline)==0)  {
   	return 0;
   }
  else if (strcmp("get",token)==0)  {
   char port[MAXLINE], buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE],message[MAXLINE];
	 int data_port,datasock,lSize,num_blks,num_last_blk,i;
	 FILE *fp;
	 recv(sockfd, port, MAXLINE,0);
	 data_port=atoi(port);
	 datasock=create_socket(data_port,argv[1]);
	 token=strtok(NULL," \n");
	 recv(sockfd,message,MAXLINE,0);
	 int j;
	 for(j=0;j<365;)	{
	  if(strcmp("1",message)==0){
		 if((fp=fopen(token,"w"))==NULL)
		  cout<<"Error in creating file\n";
		 else{
		  recv(sockfd, char_num_blks, MAXLINE,0);
			num_blks=atoi(char_num_blks);
			for(i= 0; i < num_blks; i++) { 
			 recv(datasock, buffer, MAXLINE,0);
			 fwrite(buffer,sizeof(char),MAXLINE,fp);
			 fflush(fp);
			 }
			recv(sockfd, char_num_last_blk, MAXLINE,0);
			num_last_blk=atoi(char_num_last_blk);
			if (num_last_blk > 0) { 
			 recv(datasock, buffer, MAXLINE,0);
			 fwrite(buffer,sizeof(char),num_last_blk,fp);
			 fflush(fp);
			 }
			fclose(fp);
			cout<<"File download done.\n";
		  }
		}
   else{
	 cerr<<"Error in opening file. Check filename\nUsage: put filename"<<endl;
 	}
	j++;
 sleep(60);
 }
 }
 else{
  cerr<<"Error in command. Check Command"<<endl;
  }
  cout<<"ftp>";
 }
 exit(0);
}


int create_socket(int port,char *addr)
{
 int sockfd;
 struct sockaddr_in servaddr;
 if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
  cerr<<"Problem in creating the socket"<<endl;
  exit(2);
 }

 //Creation of the socket
 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr= inet_addr(addr);
 servaddr.sin_port =  htons(port); //convert to big-endian order

 //Connection of the client to the socket
 if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
  cerr<<"Problem in creating data channel"<<endl;
  exit(3);
 }

return(sockfd);
}
*/

