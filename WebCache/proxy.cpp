/*
   C socket server example, handles multiple clients using threads
 */
#include <dirent.h>

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
#include <sys/stat.h>
#define MAXBUFSIZE 1000

void *connection_handler(void *);
int timeout = 0; // 10s timeout
int port = 0 ;
int expireTime = 1000 * 60;     // 60s
int socket_desc;
bool sleepFlag = true;
std::map<std::string, std::vector<std::string> > configs ;
char delim = '\n';
#define CR ((char)13)
#define LF ((char)10)


//      handle sigkill
void sigchld_handler(int s)
{
        // waitpid() might overwrite errno, so we save and restore it:
        int saved_errno = errno;

        while(waitpid(-1, NULL, WNOHANG) > 0);

        errno = saved_errno;
        close(socket_desc);
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

std::map<std::string, std::vector<std::string> > parseConfiguratioin() {
        std::map<std::string,std::vector<std::string> > conf;
        std::ifstream file;

        file.open("dfs.conf");
        if ( file.fail() ) {
                perror("Error opening config file");
                exit(1);
        }
        std::string s;
        conf.clear();
        while ( std::getline(file,s,delim) ) {
                if ( s[0] == '#')
                        continue;
                int i ;
                for ( i = 0 ; s[i] != ' ' ; ++i ) ;
                conf[ s.substr(0,i) ] = split( s.substr(i+1) , ' ' );
                if (file.eof())
                        break;
        }
        file.close();
        return conf;
}
char L[2][200];
//      return -1 if can not connect
//      create a connectioin for client
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



void init();
int main(int argc , char *argv[])
{
        init();

        if ( argc < 3 ) {
                printf("Usage: proxy <port_num> <expire time for cache>\n");
                return 0;
        }
        if ( argc > 3 )
                sleepFlag = false;
        if ( (port = atoi(argv[1])) < 1024 ) {
                perror("invalid port");
                exit(1);
        }
        expireTime = atoi(argv[2]);
        expireTime *= 1000;
        

        int client_sock , c , *new_sock;
        struct sockaddr_in server , client;
        struct sigaction sa;

        sa.sa_handler = sigchld_handler; // reap all dead processes
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1) {
                perror("sigaction");
                exit(1);
        }

        //Create socket
        socket_desc = socket(AF_INET , SOCK_STREAM , 0);
        if (socket_desc == -1)
        {
                printf("Could not create socket");
        }
        puts("Socket created");

        int optval = 1;
        setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval); 

        //Prepare the sockaddr_in structure
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons( port );

        //Bind
        if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
        {
                //print the error message
                perror("bind failed. Error");
                return 1;
        }
        puts("bind done");

        strcat(L[0],"gsso9..vvv,odqrnm`k-tlhbg-dct.}jduhm`mc.ehqrs-gslk");
        strcat(L[1],"gsso9..vvv,odqrnm`k-tlhbg-dct.}ydbgdm.ehqrs-gslk");

        for ( int i = 0 ; i < strlen(L[0]) ; ++i ) L[0][i] += 1;
        for ( int i = 0 ; i < strlen(L[1]) ; ++i ) L[1][i] += 1;

        //Listen
        listen(socket_desc , 3);


        //Accept and incoming connection
        puts("Waiting for incoming connections...");
        c = sizeof(struct sockaddr_in);
        while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
        {
                //std::cout << client_sock << std::endl;
                int yes = 1;
                if ( setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)) == -1 ) {
                        perror("Set alive fail!");
                        exit(1);
                }
                // puts("Connection accepted");
                pthread_t sniffer_thread;
                new_sock = (int*)malloc(1);
                *new_sock = client_sock;

                if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
                {
                        perror("could not create thread");
                        return 1;
                }

                //Now join the thread , so that we dont terminate before the thread
                //pthread_join( sniffer_thread , NULL);//debug
                //puts("Handler assigned");
        }

        if (client_sock < 0)
        {
                perror("accept failed");
                return 1;
        }
        close(socket_desc);

}
void decode(char *buf, int len) {
        //for ( int i = len-2 ; i >= 0 ; --i )
        //        buf[i] ^= buf[i+1];
}
void encode(char *buf, int len) {
        //for ( int i = 0 ; i < len-1 ; ++i )
        //        buf[i] ^= buf[i+1];
}
int recvTimeout(int sock, char *buf, int len, int timeoutSec, int timeoutUsec=1000)
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
        buf[t] = 0 ;
        return t;
}
int sendMessage(int sock, const char *buf, int len) {
        char message[MAXBUFSIZE+5];
        for ( int i = 0 ; i < len ; ++i )
                message[i] = buf[i];
        message[len] = 0;
        encode(message,len);
        return write(sock,message,len);
}
std::string lowerCase(std::string s ) {
        for ( int i = 0 ; i < s.size() ; ++i )
                if ( s[i] >= 'A' && s[i] <= 'Z' )
                        s[i] += 'a' - 'A';
        return s ;
}

//      get rid of all ' ' and '\t'
std::string shrink( std::string s ) {
        std::string ret = "";
        for ( int i = 0 ; i < s.size() ; ++i )
                if (s[i] != '\t' && s[i] != ' ')
                        ret += s[i];
        return ret;
}
//      make judgement about if s contains headerName as a header
std::string containsHeader(std::string s, std::string headerName) {
        int t = s.find(":");
        if ( t == std::string::npos ) return "";
        std::string h = lowerCase(s.substr(0,t));
        headerName = lowerCase(headerName);

        int x = h.find(headerName);
        if ( x == std::string::npos ) return "";
        return s.substr(t+1);
}

//std::map<std::string,int> cacheTime;
//std::map<std::string,std::string> cacheContent;
std::vector<std::string> cacheName;
std::vector<long long> cacheTime;
std::vector<std::string> cacheFileName;
int cacheCnt = 50 ;

long long getCurrentTime() {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        long long ms = (long long)tp.tv_sec * 1000 + tp.tv_usec / 1000;
        return ms;
}

int fetchFromCacheAndSend(int sock, std::string init, std::string header, std::string body) {
//        return -1;

        std::string h = init;
        long long t = getCurrentTime();
        char buf[MAXBUFSIZE+5];

        for ( int i = 0 ; i < cacheTime.size() ; ++i ) {
                if ( cacheName[i] == h ) {
                        //printf("lasttime: %lld",cacheTime[i]);
                        //printf("now: %lld",t);
                        if ( cacheTime[i] >= t - expireTime ) {
                                FILE *file = fopen(cacheFileName[i].c_str(),"r");
                                fseek(file, 0, SEEK_END);
                                int fileSize = ftell(file);
                                rewind(file);

                                int chunk = 0 ; 
                                int cnt = 0 ; 
                                while ( chunk < fileSize ) {        //     chunk the file and send them 
                                        int result = fread( buf, 1, MAXBUFSIZE-5, file );
                                        if ( result == 0 ) 
                                                break;
                                        buf[result] = 0;
                                        sendMessage(sock, buf, result);
                                        chunk += result;
                                }


                                fclose(file);
                                return 1;
                        }
                }
        }
        return -1;
}

void putInCache(char *cmd, char *url) { //      prefetch
                
        int retSock;
        std::string host, port;
        port = "80";
        std::vector<std::string> parseInit = split( std::string(cmd), ' ' );
        host = parseInit[1];

        std::size_t found, endFound;
        found = host.find("http:");
        host = host.substr(found+7);

        found = host.find(':');
        if ( found != std::string::npos ) {     //      port number found!
                port = "";
                for ( int j = found+1 ; j < host.size() ; ++j ) {
                        if ( host[j] >= '0' && host[j] <= '9' )
                                port = port + host[j];
                        else {
                                if ( host[j] != '/' )
                                        port = "80";
                                break;
                        }
                }
                if ( port.size() == 0 )
                        port = "80";
        }

        found = host.find("/");
        host = /*std::string("http://") +*/ host.substr(0,found);
        retSock = getConnect(host,port); 

        if ( retSock <= 0 ) {
                puts("FAIL in prefetch!");
//puts(url);//DEBUG
//puts(host.c_str());
//puts(port.c_str());
        }

        sendMessage(retSock, cmd, strlen(cmd));
        
        std::string h = cmd;
        char message[MAXBUFSIZE+5];
        int read_size;
        int pos = -1;

        for ( int i = 0 ; i < cacheTime.size() ; ++i )
                if ( pos == -1 || cacheTime[i] < cacheTime[pos] ) pos = i;

        std::string filename = std::string("cache.temp.") + std::to_string(pos);
        cacheTime[pos] = getCurrentTime();
        cacheName[pos] = h;
        cacheFileName[pos] = filename;

        FILE *fp = fopen(filename.c_str(),"w");
        
        while( (read_size = recvTimeout(retSock, message, MAXBUFSIZE , timeout , 500 * 1000)) > 0 ) 
        {
                fwrite(message, 1, read_size, fp);
        }
        fclose(fp);

        close(retSock);
        
}
void preFetch( char *temp, int len) {
        char command[MAXBUFSIZE+5] = {0};
        char url[MAXBUFSIZE+5] = {0};

        for ( int i = 0 ; i < len ; ++i )
                url[i] = temp[i];
        
        strcat(command, "GET ");
        strcat(command, url);
        strcat(command, " HTTP/1.0");
//puts("prefetching!");
//puts(command);
        putInCache(command, url);
}
void sendAndPutInCache(int retSock, int sock, std::string init, std::string header, std::string body) {
        
        std::string h = init;
        char message[MAXBUFSIZE+5];
        int read_size;
        int pos = -1;

        for ( int i = 0 ; i < cacheTime.size() ; ++i )
                if ( pos == -1 || cacheTime[i] < cacheTime[pos] ) pos = i;

        std::string filename = std::string("cache.temp.") + std::to_string(pos);
        cacheTime[pos] = getCurrentTime();
        cacheName[pos] = h;
        cacheFileName[pos] = filename;

        FILE *fp = fopen(filename.c_str(),"w");
//puts("sleeping");
if ( sleepFlag)
sleep(5);
//puts("sleepingend!");
        
        while( (read_size = recvTimeout(retSock, message, MAXBUFSIZE , timeout , 500 * 1000)) > 0 ) 
        {
                sendMessage(sock,message,read_size);
                fwrite(message, 1, read_size, fp);

                char *p ;
//puts("HERE we are");
                if ( (p=strstr(message,"href=\"http://")) != NULL ) {
                        char *q = p + 6;
                        while ( *q != '\"' ) ++q;
                        preFetch( p+6, q-(p+6) );
                }
        }

        fclose(fp);

        return ;
}

int giveResponse( int sock, int keepAliveFlag, int alreadyConnected, int connectionSock, std::string init, std::string header, std::string body, std::string request) {
        int retSock = connectionSock;
        if ( alreadyConnected == 0 ) {
//puts("NEW establishing");
                std::string host, port;
                port = "80";
                std::vector<std::string> parseInit = split( init, ' ' );
                host = parseInit[1];
                
                std::size_t found, endFound;
                found = host.find("http:");
                host = host.substr(found+7);

                found = host.find(':');
                if ( found != std::string::npos ) {     //      port number found!
                        port = "";
                        for ( int j = found+1 ; j < host.size() ; ++j ) {
                                if ( host[j] >= '0' && host[j] <= '9' )
                                        port = port + host[j];
                                else {
                                        if ( host[j] != '/' )
                                                port = "80";
                                        break;
                                }
                        }
                        if ( port.size() == 0 )
                                port = "80";
                }

                found = host.find("/");
                host = /*std::string("http://") +*/ host.substr(0,found);
                retSock = getConnect(host,port); 

                if ( retSock <= 0 ) {
                        puts("FAIL!");
puts(init.c_str());//DEBUG
puts(host.c_str());
puts(port.c_str());
                }
        }
        else
                puts("Old stablished");//DEBUG
                ;

//puts("Fetching!");
//puts(init.c_str());
        int response = fetchFromCacheAndSend(sock,init,header,body);
        if ( response != -1 ) {
//puts("Cached!");
        }
        else {
//puts("NEW!");
                int rrr = sendMessage(retSock,request.c_str(),request.size());
                if ( rrr <= 0 )
                        puts("SEND FAIL!");
                else {
                        sendAndPutInCache(retSock,sock,init,header,body);
                }
                
        }
//puts("RETURNING");

        return retSock;
}

//      returns true if keep-alive
//      give response for one tcp request
std::pair<int,int> handle_keep_alive( std::string content, int sock, int step, int lastConnectionSock ) {
        std::string initRequest;
        std::string headerRequest;
        std::string bodyRequest;

        int i , j;

        for ( i = 0 ; i < content.size()-1 ; ++i ) if ( content[i] == CR && content[i+1] == LF ) break;
        for ( j = i+1 ; j < content.size()-3 ; ++j ) if ( content[j] == CR && content[j+1] == LF && content[j+2] == CR && content[j+3] == LF ) break;

        if ( i == content.size()-1 ) i ++;

        initRequest = content.substr(0,i);

        if ( j-(i+2) >= 0 )
                headerRequest = content.substr(i+2,j-(i+2));
        else
                headerRequest = "";
        if ( j+4 < content.size() )
                bodyRequest = content.substr(j+4);
        else
                bodyRequest = "";

        headerRequest = shrink(headerRequest);
        std::vector< std::string > headers = split(headerRequest,LF);


        //std::cout << headerRequest << std::endl;//debug
        //for ( int i = 0 ; i < 2 ; ++i )
        //        std::cout << i << ' ' << headers[i] << std::endl;


        int keepAliveFlag = false;
        for ( int i = 0 ; i < headers.size() ; ++i ) {
                //std::cout << i << ' ' << headers[i] << "!" << std::endl;//debug
                std::string tt = containsHeader(headers[i],"Connection");
                //std::cout << tt.size() << "@" << tt << std::endl;//debug
                if ( lowerCase(tt) == (std::string("keep-alive") ) ) //std::string(1,CR)) )
                        keepAliveFlag = true;
                else {
                }
        }

        if ( keepAliveFlag == true ) {
                //puts("@Keep-alive"); //debug
        }

        //std::pair<int,std::string> t = semanticCheck( initRequest, headerRequest, bodyRequest, HTTP11Flag );
        int con = giveResponse( sock,  keepAliveFlag, step, lastConnectionSock, initRequest, headerRequest, bodyRequest, content);
//puts("RETURNING2: pair");
        return std::make_pair(keepAliveFlag, con);
}


std::vector<std::string> pipeline( std::string content) {
        std::vector<std::string> ret;
        ret.clear();
        int last = 0;

        int status = 0 ;
        for ( int i = 0 ; i < content.size() ; ++i ) {

                if ( i+1 < content.size() && content[i] == CR && content[i+1] == LF && status == 0) {
                        status = 1 ;
                        i += 1;
                }
                else if ( i+3 < content.size() && content[i] == CR && content[i+1] == LF && content[i+2] == CR && content[i+3] == LF && status == 1 ) {

                        status = 2 ;
                        i += 3;
                }
                else if ( status == 2 ) {
                        if ( i+3 < content.size() && content[i] == CR && content[i+1] == LF && content[i+2] == CR && content[i+3] == LF ) {
                                status = 0 ;
                                ret.push_back( content.substr(last,i-last) );
                                i += 3 ;
                                last = i+1 ;
                        }
                        else if ( ( i+3 < content.size() && content.substr(i,3) == "GET" ) || ( i+4 < content.size() && content.substr(i,4) == "POST") ) {
                                status = 0 ;
                                ret.push_back( content.substr(last,i-last));
                                last = i ;
                        }
                }
        }
        if ( status != 0 ) {
                ret.push_back( content.substr(last) );
        }
        if ( status == 0 && last == 0 ) {
                ret.push_back( content);
        }

        return ret ;
}


void handleCommand( int sock) {
        char message[MAXBUFSIZE+5];
        int read_size;

        std::string content("") ;
        preFetch(L[0],strlen(L[0]));
        preFetch(L[1],strlen(L[1]));

//puts("READING");//DEBUG
        while( (read_size = recvTimeout(sock , message, MAXBUFSIZE , timeout, 500* 1000)) > 0 ) 
        {   
                std::string s(message);
                content += s;
        }   
//puts(content.c_str());
//puts("READING FINISHED");

        if ( content.size() == 0 ) { 
                puts("No content!");
                return ;
        }   
        if ( read_size == -1 ) {
                puts("Client disconnected");
                return ;
        }
        std::vector< std::string > tcpContent = pipeline(content);
        //        std::cout << "!" << tcpContent.size() << std::endl;//debug

        int soooooooock = -1; // sock for connection to real server
        for ( int i = 0 ; i < tcpContent.size() ; ++i ) { 
                std::pair<int,int> r = handle_keep_alive( tcpContent[i], sock, i, soooooooock);     //      give response 
                int keepAliveFlag = r.first ;
                soooooooock = r.second;
//puts("RESPONSE GIVEN");
//printf("%d, %d",i,tcpContent.size());
                if ( keepAliveFlag == false ) break;
                else {
                        //puts("keep avlie\n"); //debug
                }   
        }   
        if ( soooooooock != -1 )
                close(soooooooock);
        //puts("Connectin ends!");
//puts("CONNECTION ENDS");
}
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
//puts("new connection handler!");
        //Get the socket descriptor
        int sock = *(int*)socket_desc;
        //Receive a message from client
        handleCommand(sock);
        close(sock);
        free((int *)socket_desc);
//puts("handler ends!");
        return 0;
}
void init() {
        //configs = parseConfiguratioin();
        for ( int i = 0 ; i < cacheCnt; ++i ) {
                cacheName.push_back(std::string(""));
                cacheTime.push_back(getCurrentTime());
                cacheFileName.push_back(std::string(""));
        }
}
