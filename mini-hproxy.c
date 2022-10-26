// import std libs
#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// import sys libs
#ifdef _WIN32
    #include <windows.h>
#endif

#ifdef __x86_64__
    #include <sys/wait.h>
    #include <netinet/in.h> 
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <resolv.h>
    #include <sys/socket.h>
#endif

#define BUF_SIZE 8192

#define READ  0
#define WRITE 1

#define DEFAULT_LOCAL_PORT          8080  
#define DEFAULT_REMOTE_PORT         8081 
#define SERVER_SOCKET_ERROR         -1
#define SERVER_SETSOCKOPT_ERROR     -2
#define SERVER_BIND_ERROR           -3
#define SERVER_LISTEN_ERROR         -4
#define CLIENT_SOCKET_ERROR         -5
#define CLIENT_RESOLVE_ERROR        -6
#define CLIENT_CONNECT_ERROR        -7
#define CREATE_PIPE_ERROR           -8
#define BROKEN_PIPE_ERROR           -9
#define HEADER_BUFFER_FULL          -10
#define BAD_HTTP_PROTOCOL           -11

#define MAX_HEADER_SIZE 8192

#if defined(OS_ANDROID)
    #include <android/log.h>
    #define LOG(fmt...) __android_log_print(ANDROID_LOG_DEBUG,__FILE__,##fmt)
#else
    #define LOG(fmt...)  do { fprintf(stderr,"%s %s ",__DATE__,__TIME__); fprintf(stderr, ##fmt); } while(0)
#endif


char remote_host[128]; 
int remote_port; 
int local_port;

int server_sock; 
int client_sock;
int remote_sock;

char *header_buffer;

enum 
{
    FLG_NONE = 0,       /* 正常数据流不进行编解码 */
    R_C_DEC = 1,        /* 读取客户端数据仅进行解码 */
    W_S_ENC = 2         /* 发送到服务端进行编码 */
};

static int io_flag;     /* 网络io的一些标志位 */
static int m_pid;       /* 保存主进程id */

void server_loop();
void stop_server();
void handle_client(int client_sock, struct sockaddr_in client_addr);
void forward_header(int destination_sock);
void forward_data(int source_sock, int destination_sock);
void rewrite_header();
int send_data(int socket,char * buffer,int len );
int receive_data(int socket, char * buffer, int len);
void hand_mproxy_info_req(int sock,char * header_buffer) ;
void get_info(char * output);
const char * get_work_mode() ;
int create_connection() ;
int _main(int argc, char *argv[]) ;

/*=============================================================================================*/

int main(int argc, char *argv[])
{
    return _main(argc,argv);
}

int _main(int argc, char *argv[]) 
{
    local_port = DEFAULT_LOCAL_PORT;
    io_flag = FLG_NONE;
    int daemon = 0; 

    char info_buf[2048];
	
	int opt;
	char optstrs[] = ":l:h:dED";
	char *p = NULL;
	while(-1 != (opt = getopt(argc, argv, optstrs))){
		switch(opt){
			case 'l':
				local_port = atoi(optarg);
				break;
			case 'h':
				p = strchr(optarg, ':');
				if(p){
					strncpy(remote_host, optarg, p - optarg);
					remote_port = atoi(p+1);
				}
				else{
					strncpy(remote_host, optarg, strlen(remote_host));
				}
				break;
			case 'd':
				daemon = 1;
				break;
			case 'E':
				io_flag = W_S_ENC;
				break;
			case 'D':
				io_flag = R_C_DEC;
				break;
			case ':':
				printf("\nMissing argument after: -%c\n", optopt);
				usage();
			case '?':
				printf("\nInvalid argument: %c\n", optopt);
			default:
				usage();
		}
    }

    get_info(info_buf);
    LOG("%s\n",info_buf);
    start_server(daemon);
    return 0;
}


void start_server(int daemon)
{
    //初始化全局变量
    header_buffer = (char *) malloc(MAX_HEADER_SIZE);

    signal(SIGCHLD, sigchld_handler); // 防止子进程变成僵尸进程

    if ((server_sock = create_server_socket(local_port)) < 0) 
    { // start server
        LOG("Cannot run server on %d\n",local_port);
        exit(server_sock);
    }
   
    if(daemon)
    {
        pid_t pid;
        if((pid = fork()) == 0)
        {
            server_loop();
        } else if (pid > 0 ) 
        {
            m_pid = pid;
            LOG("mporxy pid is: [%d]\n",pid);
            close(server_sock);
        } else 
        {
            LOG("Cannot daemonize\n");
            exit(pid);
        }

    } else 
    {
        server_loop();
    }

}

void usage(void)
{
    printf("Usage:\n");
    printf(" -l <port number>  specifyed local listen port \n");
    printf(" -h <remote server and port> specifyed next hop server name\n");
    printf(" -d <remote server and port> run as daemon\n");
    printf(" -E encode data when forwarding data\n");
    printf(" -D decode data when receiving data\n");
    exit(8);
}