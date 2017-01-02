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
int timeout = 1;
// rootPath is like "./xxx/"
std::string rootPath = "";
int port = 0 ;
int socket_desc;
std::map<std::string, std::vector<std::string> > configs ;
char delim = '\n';

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

//      check if the Folder exist
//      creat path if not
int createFolder(const char *pathname) {
        struct stat info;

        if( stat( pathname, &info ) == -1 ) {
                //      Folder doesnt exist
                mkdir(pathname, 0777);
                return 1;
        }
        else if( info.st_mode & S_IFDIR ) {
                //      Folder exist
                return 0;
        }
        else {
                //      Is a file, not a directory
                return -1;
        }
}

void init();
int main(int argc , char *argv[])
{
        init();
        if ( argc != 3 ) {
                printf("Usage: server root_path port_num\n");
                return 0;
        }
        rootPath = std::string("./") + std::string(argv[1]);
        if ( (port = atoi(argv[2])) < 1024 ) {
                perror("invalid port");
                eit(1);
        }
        createFolder(rootPath.c_str());
        

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

}
void decode(char *buf, int len) {
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
        tv.tv_sec = timeoutec;
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
int sendMessage(int sock, char *buf, int len) {
        char message[MAXBUFSIZE+5];
        for ( int i = 0 ; i < len ; ++i )
                message[i] = buf[i];
        message[len] = 0;
        encode(message,len);
        return write(sock,message,len);
}

int handleMkdir(int sock, std::string directory) {
        directory = rootPath+directory;
        createFolder(directory.c_str());
}

// return -1 if directory doesnt exist
// directory: must be "xxx/xxx/" form
int handleList(int sock, std::string directory) {
        directory = rootPath + directory;
        char response[MAXBUFSIZE+5] = {0};
        DIR *dp;
        struct dirent *ep;
        dp = opendir(directory.c_str());
        int first = 1;
        if ( dp != NULL ) {
                while ( ep = readdir(dp) ) {
                        if ( !first )
                                strcat(response,",");
                        first = 0;
                        strcat(response,ep->d_name);
                }
                closedir(dp);
        }
        else {
                printf("Couldn't open the directory\n");
                sendMessage(sock,"ls!",3);
                return -1;
                // Send empty msg to client since it's waiting for response
        }
        strcat(response,",ls!");
        sendMessage(sock, response, strlen(response));
        return 1;
}
int handlePut(int sock, std::string filename) {
        char buf[MAXBUFSIZE+5];
        FILE *file = fopen((rootPath+filename).c_str(), "w");
        int t ;
        while ( (t = recvTimeout(sock, buf, MAXBUFSIZE, timeout, 0) ) > 0 ) {
               fwrite( buf, sizeof(char), t, file); 
        }
        fclose(file);
        return 1;
}
//      filename is <relativepath/filename.1> like
int handleGet(int sock, std::string filename) {
        char buf[MAXBUFSIZE+5];
        FILE *file = fopen((rootPath+filename).c_str(), "r");
        if ( file == NULL )
        {
                printf("Open file fail!\n");    //      return 1 to wait for the next command
                return 1;
        }

        printf(" ---------------- \t Start sending file %s to client ! \t --------------- \n",filename.c_str());

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

        printf(" ----------------- \t Sending file %s finished! \t ----------------- \n",filename.c_str());

       fclose(file);
        return 1;
}
int handleExist(int sock, std::string filename) {
        FILE *file = fopen((rootPath+filename).c_str(), "r");
        if ( file == NULL ) {
                sendMessage(sock, "No", 2);
        }
        else {
                sendMessage(sock, "Ok", 2);
                fclose(file);
        }
        return 1;
}

void handleCommand( int sock) {
        char message[MAXBUFSIZE+5];
        int read_size;

        while ( true ) {
                read_size = recvTimeout(sock, message, MAXBUFSIZE, timeout, 0);
                if ( read_size > 0 ) {
                        std::stringstream ss(message);
                        std::string cmd;
                        std::string filename, directory;

                        ss >> cmd;
                        if ( cmd == "list" ) {
                                ss>>directory ;
                                handleList(sock,directory);
                                break;
                        }
                        else if ( cmd == "put") {
                                sendMessage(sock,"Ok",2);
                                ss>>filename;
                                handlePut(sock,filename);
                                break;
                        }
                        else if ( cmd == "get") {
                                ss>>filename;
                                handleGet(sock,filename);
                        }
                        else if ( cmd == "exist" ) {
                                ss>>filename;
                                handleExist(sock,filename);
                        }
                        else if ( cmd == "mkdir" ) {
                                ss>>directory;
                                handleMkdir(sock, directory);
                                break;
                        }
                }
                else
                        break;
        }

}
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
        puts("new connection!");
        //Get the socket descriptor
        int sock = *(int*)socket_desc;
        int read_size;
        char *message , client_message[MAXBUFSIZE+5];

        //Receive a message from client
        std::string content("") ;

        if( (read_size = recvTimeout(sock , client_message , MAXBUFSIZE , timeout, 50000)) > 0 )
        {
                std::stringstream ss;
                std::string username, password;
                ss << client_message;
                ss >> username >> password;
                if ( configs.count(username)== 0 || configs[username][0] != password ) {
                        sendMessage( sock, "No", 2);
                }
                else {
                        sendMessage( sock, "Ok", 2);
                        createFolder((rootPath+username).c_str());
                        handleCommand(sock);
                }
        }

        close(sock);
        free((int *)socket_desc);

        return 0;
}
void init() {
        configs = parseConfiguratioin();
}
