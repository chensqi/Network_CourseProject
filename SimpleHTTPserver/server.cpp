/*
   C socket server example, handles multiple clients using threads
 */

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

#define CR ((char)13)
#define LF ((char)10)
#define MAXBUFSIZE 2000

//the thread function
void *connection_handler(void *);
int socket_desc ;
int timeout = 0 ;
char delim = '\n';
std::map<std::string, std::vector<std::string> > configs ;

std::string getCurrentTime() {
        std::chrono::time_point<std::chrono::system_clock> end;
        end = std::chrono::system_clock::now();
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
        return std::ctime(&end_time);
}

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

        file.open("ws.conf");
        if ( file.fail() ) {
                perror("Error opening file");
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

void init() {
        configs = parseConfiguratioin();
        if ( configs.count("Listen") == 0 ) {
                perror("No port configuration!");
                exit(1);
        }
        if ( configs.count("DocumentRoot") == 0 ) {
                perror("No DocumentRoot!");
                exit(1);
        }
        if ( configs.count("KeepaliveTime") != 0 ) {
                timeout = atoi(configs["KeepaliveTime"][0].c_str());
        }

}

int main(int argc , char *argv[])
{
        init();
        if ( atoi(configs["Listen"][0].c_str()) < 1024 ) {
                perror("invalid port");
                exit(1);
        }

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
        server.sin_port = htons( atoi(configs["Listen"][0].c_str()) );

        //Bind
        if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
        {
                //print the error message
                perror("bind failed. Error");
                return 1;
        }
        puts("bind done");

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
                // puts("Handler assigned");

        }

        if (client_sock < 0)
        {
                perror("accept failed");
                return 1;
        }
        close(socket_desc);

        return 0;
}
std::string getDirectoryByURL( std::string url ) {
        std::string directory = configs["DocumentRoot"][0] + url;

        if ( directory[ directory.size()-1 ] == '/' ) {
                
                for ( int i = 0 ; i < configs["DirectoryIndex"].size() ; ++i ) {
                        std::ifstream file;

                        std::string res = directory + configs["DirectoryIndex"][i];

                        file.open(res);
                        if ( !file.fail() ) {
                                file.close();
                                return res;
                        }
                        file.close();
                }
                return "File Not Exists";
        }
        else {
                std::ifstream file(directory);
                if ( !file.fail() ) {
                        file.close();
                        return directory;
                }
                else
                        return "File Not Exists";
        }
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

        return recv(sock, buf, len, 0);
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
std::vector< std::pair< std::string, std::string> > parseParaFromPost( std::string content ) {
        std::vector< std::pair< std::string, std::string> > ret ;
        ret.clear();
        
        std::string name;
        std::string value;
        std::string *p ;

        name.clear();
        value.clear();
        p = &name ;

        for ( int i = 0 ; i < content.size() ; ++i ) {
                if ( content[i] == '&' ) {
                        ret.push_back( make_pair(name,value) );
                        name.clear();
                        value.clear();
                        p = &name;
                        continue;
                }
                if ( content[i] == '=' ) {
                        p = &value ;
                        continue ;
                }

                if ( content[i] == '%' ) {
                        *p += strtoul(content.substr(i+1,2).c_str(), NULL, 16);;
                        i += 2 ;
                }
                else if ( content[i] == '+' ) {
                        *p += ' ';
                }
                else 
                        *p += content[i];
        }
        if ( name.size() > 0 && value.size() > 0 )
                ret.push_back( make_pair(name,value) );

        return ret;
}

//      return the value and reason
std::pair<int, std::string> semanticCheck( std::string init, std::string header, std::string body , int HTTP11Flag ) {

        std::vector<std::string> s;
        std::pair<int, std::string> ret ;

        ret.first = 200;

        s = split(init,' ');
        //   ---------------------- check init part ----------------------      //

        if ( s[0] != "GET" && s[0] != "POST" ) {      //      
                perror("method wrong");
                ret.first = 500;
                ret.second = "Invalid Method : " + s[0];
                return ret;
        }
        if ( s[2] != "HTTP/1.1" && s[2] != "HTTP/1.0" ) {       //      support 1.0 and 1.1 only
                perror("HTTP version wrong");
                ret.first = 500;
                ret.second = "Invalid HTTP-Version: " + s[2] ;
                return ret ;
        }
        if ( s[1].find("!") != std::string::npos ) {
                perror("Invalid URL");
                ret.first = 500;
                ret.second = "Invalid URL: " + s[1] ;
                return ret ;
        }
        if ( getDirectoryByURL(s[1]) == "File Not Exists" ) {
                perror("No such file");
                ret.first = 404;
                ret.second = "URL does not exist : " + s[1] ;
                return ret ;
        }
        return ret;
        //      ---------------------- check header ----------------------      //

        s.clear();
        s = split(header, delim);
        for ( int i = 0 ; i < s.size() ; ++i ) {
                if ( s[i][0] != '\t' && s[i][0] != ' ' && s[i].find(":") == std::string::npos ) {
                        perror("bad header");

                        ret.first = 500;
                        ret.second = "Erro header format";
                        return ret;
                }
        }

        if ( HTTP11Flag ) {
                bool hostFlag = false;
                for ( int i = 0 ; i < s.size() ; ++i ) {
                        if ( containsHeader(s[i],"host") != "" ) {
                                hostFlag = true;
                        }
                }
                if ( hostFlag == false ) {
                        ret.first = 400;
                        ret.second = "No Host: header received";
                        return ret;
                }
        }
        //      ---------------------   check body      -----------------       //
        // TODO

        s.clear();
        
        return ret;
}

void giveResponse( int sock, std::pair<int,std::string> valueAndReason, int HTTP11Flag, int keepAliveFlag, std::string init, std::string header, std::string body) {
        std::string headerRes ;
        std::string initRes ;
        std::string bodyRes ;
        std::string type = ".html"; 

        initRes = HTTP11Flag ? "HTTP/1.1" : "HTTP/1.0";
        headerRes = "Date: " + getCurrentTime();

        if ( valueAndReason.first == 200 ) {
                std::string directory = getDirectoryByURL( split(init,' ')[1] ); 
                if ( directory == "File Not Exists" ) {
                        goto NotFound;
                }

                initRes += " 200 OK";
                
                FILE *file = fopen(directory.c_str(), "r");
                type = directory.substr( directory.find_last_of(".") );
                if ( file == NULL )
                {   
                        perror("Open file fail!\n");    
                        exit(1);
                }   
                fseek(file, 0, SEEK_END);
                int fileSize = ftell(file);
                headerRes += "Content-Type: " + configs[type][0] + "\n";
                headerRes += "Content-Length: " + std::to_string(fileSize) + "\n";   //
                if ( keepAliveFlag )
                        headerRes += "Connection: keep-alive\n";

                std::string response = initRes + std::string(1,CR) + std::string(1,LF)
                        + headerRes + std::string(1,CR) + std::string(1,LF) ;//+ std::string(1,CR) + std::string(1,LF);

                write( sock, response.c_str(), strlen(response.c_str()));

                //puts(response.c_str());//debug

                if ( init.find("POST") != std::string::npos ) {
                        body = shrink(body); 
                        write(sock, "<pre><h1>\n", 10 );
                        std::vector<std::pair<std::string, std::string> > paras = parseParaFromPost( body ); 
                        for ( int i = 0 ; i < paras.size() ; ++i ) {
                                std::string tt = "(key)" + paras[i].first + ",(value)" + paras[i].second + '\n' ;
                                write(sock, tt.c_str(), tt.size() );
                        }
                        write(sock, "</h1></pre>\n", 12 );
                }

                rewind(file);
                
                int chunk = 0 ; 
                std::string ret;
                ret.clear();
                char recBuf[MAXBUFSIZE];
                while ( chunk < fileSize ) {        

                        int result = fread( recBuf, sizeof(char), MAXBUFSIZE-5, file );  
                        if ( result == 0 ) 
                                break;
                        chunk += result ;
                        write( sock, recBuf, result);
                }   
                return ;
        }
        else if ( valueAndReason.first == 400 ) {
                initRes += " 400 Bad Request" ;
                bodyRes += "<html><body>";
                bodyRes += "400 Bad Request Reason: " + valueAndReason.second ;
                bodyRes += "</body></html>";
        }
        else if ( valueAndReason.first == 404 ) {
                NotFound:
                initRes += " 404 Not Found" ;
                bodyRes += "<html><body>";
                bodyRes += "404 Not Found: " + valueAndReason.second ;
                bodyRes += "</body></html>";
        
        }
        else if ( valueAndReason.first == 501 ) {
                initRes += " 501 Not Implemented" ;
                bodyRes += "<html><body>";
                bodyRes += "501 Not Implemented: " + valueAndReason.second ;
                bodyRes += "</body></html>";
                
        }
        else if ( valueAndReason.first == 500 ) {
                initRes += " 500 Server Error: ";
                bodyRes += "<html><body>";
                bodyRes += "500 Server Error: " + valueAndReason.second ;
                bodyRes += "</body></html>";
        }
        else {
                exit(1);
        }
        headerRes += "Content-Type: " + configs[type][0] + "\n";
        headerRes += "Content-Length: " + std::to_string(bodyRes.size()) + "\n"; //
        if ( keepAliveFlag )
                        headerRes += "Connection: keep-alive\n";
        
        std::string response = initRes + std::string(1,CR) + std::string(1,LF)
                + headerRes + std::string(1,CR) + std::string(1,LF) //+ std::string(1,CR) + std::string(1,LF) 
                + bodyRes;
        write( sock, response.c_str(), strlen(response.c_str()));
}

//      returns true if keep-alive
//      give response for one tcp request
int handle( std::string content, int sock ) {
//                std::cout << content.size() << std::flush; // debug 
//                puts("@");
//                puts( content.c_str() ); // debug

                // TODO set delim for windows and linux

                std::string initRequest;
                std::string headerRequest;
                std::string bodyRequest;

                int i , j;

                for ( i = 0 ; i < content.size()-1 ; ++i ) if ( content[i] == CR && content[i+1] == LF ) break;
                for ( j = i+1 ; j < content.size()-1 ; ++j ) if ( content[j] == CR && content[j+1] == LF && content[j+2] == CR && content[j+3] == LF ) break;

                initRequest = content.substr(0,i);
                headerRequest = content.substr(i+2,j-(i+2));
                if ( j+4 < content.size() )
                        bodyRequest = content.substr(j+4);
                else
                        bodyRequest = "";

                headerRequest = shrink(headerRequest);
                std::vector< std::string > headers = split(headerRequest,LF);


                //std::cout << headerRequest << std::endl;//debug
                //for ( int i = 0 ; i < 2 ; ++i )
                //        std::cout << i << ' ' << headers[i] << std::endl;


                int HTTP11Flag = true;
                int keepAliveFlag = false;
                if ( content.find("HTTP/1.0") != std::string::npos ) {  //      HTTP 1.0
                        HTTP11Flag = false;
                }

                if ( HTTP11Flag )
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

                std::pair<int,std::string> t = semanticCheck( initRequest, headerRequest, bodyRequest, HTTP11Flag );
                //std::cout << "@" << initRequest << std::flush << std::endl;//debug

                giveResponse( sock, t, HTTP11Flag, keepAliveFlag, initRequest, headerRequest, bodyRequest);
                return keepAliveFlag;
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

        return ret ;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
        //Get the socket descriptor
        int sock = *(int*)socket_desc;
        int read_size;
        char *message , client_message[2000];

        //puts("haha");// debug
        //Receive a message from client
        std::string content("") ;

        while( (read_size = recvTimeout(sock , client_message , 1 , timeout, 50000)) > 0 )
        {
                //                        puts("@"); puts(client_message);   //      debug
                //                        std::cout << strlen(client_message.size() ;
                std::string s(client_message);
                content += s;
        }

        if ( content.size() == 0 ) {
                //                        perror("recv time out");
                //                        std::cout << read_size << std::flush ;//debug
                return 0;
        }
        std::vector< std::string > tcpContent = pipeline(content);
        
//        std::cout << "!" << tcpContent.size() << std::endl;//debug

        for ( int i = 0 ; i < tcpContent.size() ; ++i ) {
//        std::cout << "@" << tcpContent[i] << std::endl;//debug
                int keepAliveFlag = handle( tcpContent[i], sock );
               
                if(read_size == 0)
                {
                        puts("Client disconnected");
                        fflush(stdout);
                }
                else if(read_size == -1)
                {
                        perror("recv failed");
                }
                if ( keepAliveFlag == false ) break;
                else {
                        //puts("keep avlie\n"); //debug
                }
        }
        close(sock);

        free((int *)socket_desc);

        return 0;
}
