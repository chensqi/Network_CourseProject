#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include<signal.h> //handle ctrl+C
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include<errno.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/time.h>
#include<netdb.h>
#include<map>
#include<string>
#include<fstream>
#include <sstream>
#include <vector>
#include<iostream>
#include <chrono>
#include <ctime>
#define MAXBUFSIZE 10000
std::map<std::string, std::vector<std::string> > configs ;
char delim = '\n';
int partition = 4;
int backup = 2; // store a pice of file into <backup> different server

void error(char *msg)
{
        perror(msg);
        exit(0);
}
std::vector<std::string> split(const std::string &s, char delim) {
        std::vector<std::string> res;
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
                res.push_back(item);
        }
        return res;
}
void init(char *filename) {
        std::ifstream file;

        file.open(filename);
        if ( file.fail() ) {
                perror("Error opening file");
                exit(1);
        }
        std::string s;
        configs.clear();
        while ( std::getline(file,s,delim) ) {
                if ( s[0] == '#')
                        continue;
                int i ;
                for ( i = 0 ; s[i] != ' ' ; ++i ) ;
                std::vector<std::string> t = split( s.substr(i+1) , ' ' ); 
                for ( int j = 0 ; j < t.size() ; ++j )
                        configs[ s.substr(0,i) ].push_back( t[j] );
                if (file.eof())
                        break;
        }
        file.close();

        if ( configs.count("Username") == 0 ) {
                perror("No username!\n");
                exit(1);
        }
        if ( configs.count("Password") == 0 ) {
                perror("No password!\n");
                exit(1);
        }
        if ( configs.count("Server") == 0 ) {
                error("No server!\n");
        }
        partition = configs["Server"].size() / 2 ;

}
int md5(std::string file) {
        int s = 0 ;
        for ( int i = 0 ; i < file.size() ; ++i ) 
                s = s * 255 + file[i];
        return s;
}

//      return -1 if can not connect
int getConnect(std::string host, std::string port) {
        int sockfd, portno, n;
        struct sockaddr_in serveraddr;
        struct hostent *server;
        char buf[5+MAXBUFSIZE];

        portno = atoi(port.c_str());

        /* socket: create the socket */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
                perror("ERROR opening socket");
                return -1;
        }

        /* gethostbyname: get the server's DNS entry */
        server = gethostbyname(host.c_str());
        if (server == NULL) {
                fprintf(stderr,"ERROR, no such host as %s\n", host.c_str());
                close(sockfd);
                return -1;
        }

        /* build the server's Internet address */
        bzero((char *) &serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, 
                        (char *)&serveraddr.sin_addr.s_addr, server->h_length);
        serveraddr.sin_port = htons(portno);

        /* connect: create a connection with the server */
        if (connect(sockfd, (sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
                //perror("ERROR connecting");
                close(sockfd);
                return -1;
        }
        return sockfd;
}
//      return -1 : connect fail
//      return -2 : no such server, more than server list 
int getConnect(int i) {
        if ( 2*i+1 >= configs["Server"].size() )
                return -2;
        std::string addr = configs["Server"][2*i+1];
        std::string host, port;
        for ( int j = 0 ; j < addr.size() ; ++j )
                if ( addr[j] == ':' ) {
                        host = addr.substr(0,j); 
                        port = addr.substr(j+1);
                }
        return getConnect(host,port);
}
void decode(char *buf int len) {
        //for ( int i = len-2 ; i >= 0 ; --i )
        //        buf[i] ^= buf[i+1];
}
void encode(char *buf, int len) {
        //for ( int i = 0 ; i < len-1 ; ++i )
        //        buf[i] ^= buf[i+1];
}
int recvTimeout(int sock, char *buf, int len, int timeoutSec, int timeoutUsec=0)
{
        fd_set fds;
        int n;
        struct timeval tv;

        // set up the file descriptor set
        FD_ZERO(&fds);
        FD_SET(sock, &fds);

        // set up the struct timeval for the timeout
        tv.tv_sec = timeoutSec;
        tv.tv_usec = timeoutUsec;

        // wait until timeout or data received
        n = select(sock+1, &fds, NULL, NULL, &tv);
        if (n == 0) return -2; // timeout!
        if (n == -1) return -1; // error

        int t = recv(sock, buf, len, 0);
        decode(buf,t);
        buf[t] = 0;
        return t;
}
int sendMessage(int sock, const char *buf, int len) {
        char message[5+MAXBUFSIZE];
        for ( int i = 0 ; i < len ; ++i )
                message[i] = buf[i];
        message[len] = 0;
        encode(message,len);
        return write(sock,message,len);
}

//      send username and password to server
//      return 1 if server response "Ok"
//      return 0 if "No"
//      return -1 if timeout
int shakeHand(int sock) {
        std::string username = configs["Username"][0];
        std::string password = configs["Password"][0];
        std::string msg = username + " " + password; 
        int n = sendMessage(sock, msg.c_str(), msg.size());
        if (n < 0) { 
                perror("ERROR writing to socket");
                return -1;
        }
        char buf[5+MAXBUFSIZE];
        int t;

        if ( (t = recvTimeout(sock, buf, MAXBUFSIZE, 1, 0)) > 0 ) {
                buf[t] = 0;

                if ( strcmp(buf,"Ok") == 0 ) {
                        return 1;
                }
                else {
                        puts("Invalid Username/Password. Please try again\n");
                        return 0;       //      response is "No"
                }
        }
        else {
                return -1;       //      TimeOut
        }
}
std::map<std::string,int> completeList;
//      directory is "xxx/xxx/"
void handleList(std::string directory) {
        
        int sock ;
        char buf[5+MAXBUFSIZE];
        std::string filename;
        std::vector<std::string> filenameList;
        std::vector<std::string> tempList;

        filenameList.clear();

        if ( directory == "" ) directory = "list";
        else directory = std::string("list ") + directory;

        for ( int i = 0 ; (sock=getConnect(i)) != -2 ; ++i ) {
                if ( sock > 0 && shakeHand(sock) == 1 ) {        //      correct username and password
                        //printf("Server %d says OK!\n",i); 
                        sendMessage(sock, directory.c_str(), directory.size());
                        int t = recvTimeout(sock,buf,MAXBUFSIZE,1,0);
                        tempList = split( std::string(buf), ',' );
                        for ( int j = 0 ; j < tempList.size() ; ++j )
                                if ( tempList[j] != "ls!" && tempList[j] != "." && tempList[j] != "..")
                                        filenameList.push_back( tempList[j]);
                }
                close(sock);
        }
        completeList.clear();

        for ( int i = 0 ; i < filenameList.size() ; ++i ) {
                filename = filenameList[i];
                int part = -1 ;
                for ( int j = filename.size() - 1 ; j >= 0 ; --j )
                        if ( filename[j] == '.' ) {
                                part = atoi( filename.substr(j+1).c_str() );
                                filename = filename.substr(0,j);
                                completeList[filename] |= (1<<part);
                                break;
                        }
                 if ( part == -1 ) {
                        completeList[filename] = -1 ;
                 }
        }

        puts("----------- File(s): -------------\n");
        if ( completeList.size() == 0 ) puts("(empty)\n");
        for ( std::map<std::string, int>::iterator it = completeList.begin() ; it != completeList.end() ; ++it ) {
                printf"%s",(it->first).c_str());
                if ( it->second != (1<<partition)-1 ) {
                        if ( it->second == -1 ) puts("/");
                        else
                                puts("[incomplete]"); 
                }
                puts("\n");
        }
        puts("----------- List end -------------\n");
}
//      directory is "xxx/xxx/"
void handleMkdir(std::string directory) {
        
        int sock ;
        char buf[5+MAXBUFSIZE];

        if ( directory == "" ) return ;
        else directory = std::string("mkdir ") + directory;

        for ( int i = 0 ; (sock=getConnect(i)) != -2 ; ++i ) {
                if ( sock > 0 && shakeHand(sock) == 1 ) {        //      correct username and password
                        sendMessage(sock, directory.c_str(), directory.size());
                }
        }
} 
//      return 0 if no such file
//      directory must be "xxx/xxx/"
int handlePut(std::string directory, std::string filename) {
        FILE *file;
        file = fopen( filename.c_str(), "r");
        int hash = md5(filename) % partition;
        char buf[5+MAXBUFSIZE];
        if ( file == NULL )
        {
                printf("Open file fail!\n");    //      return 1 to wait for the next command
                return 0;
        }
        else {
                printf(" ---------------- \t Start sending file %s to server! \t --------------- \n",filename.c_str());

                fseek(file, 0, SEEK_END);
                int fileSize = ftell(file);
                rewind(file);

                // sending i-th partition to 2 server
                for ( int i = 0 ; i < partition ; ++i ) {
                
                        int j = (hash+i)%partition ;
                        int cnt = 0 ;
                        int loopCnt = 0 ;
                        while ( cnt < backup && loopCnt < partition ) {

                                int sock = getConnect(j) ;
                                if ( sock == -2 ) break;
                                if ( sock > 0 && shakeHand(sock) ) {
                                        // transfer
                                        ++cnt;

                                        // calc length
                                        int len = fileSize - i*(fileSize/partition);
                                        if ( i != partition-1 )
                                                len = fileSize/partition;

                                        //      send "put <directory/filename.parnum> directory"
                                        std::string cmd = std::string("put ") + directory + filename + std::string(".") + std::to_string(i);
                                        sendMessage(sock,cmd.c_str(),cmd.size()); 
                                        //      receive "Ok" from server
                                        recvTimeout(sock,buf,MAXBUFSIZE,1,0);

                                        //      send file by chunk
                                        int chunk = 0 ;
                                        fseek(file, i*(fileSize/partition), SEEK_SET);
                                        while ( chunk < len ) {
                                                int result = fread( buf, sizeof(char), std::min(MAXBUFSIZE-5,len-chunk), file ); 
                                                sendMessage(sock, buf, result);
                                                //TODO check send response
                                                chunk += result;
                                        }
                                        close(sock);
                                }
                                ++j;
                                ++loopCnt;
                                j %= partition;
                        }
                }


                printf(" ---------------- \t sending file finished! \t ---------------------\n");
        }
}

int handleGet(std::string directory, std::string filename) {
        int mask = 0 ;
        char buf[MAXBUFSIZE+5];

        /*
        if ( completeList.count(filename) == 0 || completeList[filename] != (1<<partition)-1 ) {
                puts("File is incomplete\n");
                return 0;
        }
       */
        FILE *file = fopen(filename.c_str(),"w");

        for ( int j = 0 ; j < partition ; ++j ) //      j-th part of the file
                for ( int i = 0 ; i < partition ; ++i ) {       // i-th server
                        int sock = getConnect(i);
                        if ( sock > 0 && shakeHand(sock) == 1 ) {        //      correct username and password
                                std::string cmd = std::string("exist ") + directory + filename + std::string(".") + std::to_string(j);
                                sendMessage(sock,cmd.c_str(),cmd.size());
                                recvTimeout(sock,buf,MAXBUFSIZE,1,0);
                                if ( strcmp(buf,"Ok") == 0 ) {
                                        
                                        std::string cmd = std::string("get ") + directory + filename + std::string(".") + std::to_string(j);
                                        sendMessage(sock,cmd.c_str(),cmd.size());

                                        int t ;
                                        while ( (t=recvTimeout(sock,buf,MAXBUFSIZE,1,0)) > 0 ) {
                                                fwrite(buf, sizeof(char), t, file);
                                        } 

                                        fseek(file, -1, SEEK_CUR);

                                        mask |= (1<<j);
                                        close(sock);
                                        break;
                                }
                        }
                        close(sock);
                }
        fclose(file);
        if ( mask != (1<<partition)-1) {
                system((std::string("rm ")+filename).c_str());
                //printf("Get file %s fail!\n",filename.c_str());
                puts("File is incomplete\n");
        }
        return 1;
}

int main(int argc, char * argv[]) {
        
        /* check command line arguments */
        if (argc != 2) {
                fprintf(stderr,"usage: %s <dfc.conf>\n", argv[0]);
                exit(0);
        }
        init(argv[1]);

        char buf[5+MAXBUFSIZE];

        while ( true ) {
        
                puts("Please input:\n");
                puts("1. list <directory>\n");
                puts("2. get <file> <directory>\n");
                puts("3. put <file> <directory>\n");
                puts("4. mkdir <directory>\n");
                puts("5. quit\n");

                fgets(buf, MAXBUFSIZE, stdin);
                std::stringstream ss(buf) ;
                std::string cmd, filename, directory;
                ss >> cmd;

                if ( cmd == "list" ) {
                        ss >> directory;
                        handleList(configs["Username"][0]+std::string("/")+directory);
                }
                else if ( cmd == "get" ) {
                        ss >> filename >> directory;
                        handleGet(configs["Username"][0]+std::string("/")+directory,filename);
                }
                else if ( cmd == "put" ) {
                        ss >> filename >> directory;
                        handlePut(configs["Username"][0]+std::string("/")+directory,filename);
                }
                else if ( cmd == "quit" ) {
                        puts("bye!\n");
                        break;
                }
                else if ( cmd == "mkdir" ) {
                        ss >> directory;
                        handleMkdir(configs["Username"][0]+std::string("/")+directory);
                }
                else {
                        puts("please input correct command\n");
                }
        }


        /* get message line from the user */
//        printf("Please enter msg: ");
//        bzero(buf, BUFSIZE);
//        fgets(buf, BUFSIZE, stdin);

        /* send the message line to the server */
//        n = write(sockfd, buf, strlen(buf));
//        if (n < 0) 
//                error("ERROR writing to socket");


        /* print the server's reply */
//        bzero(buf, BUFSIZE);
 //       n = read(sockfd, buf, BUFSIZE);
  //      if (n < 0) 
   //             error("ERROR reading from socket");
    //    printf("Echo from server: %s", buf);
     //   close(sockfd);

        return 0;


}
