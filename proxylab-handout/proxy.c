/*
 * File Name: Proxy.c
 * Name:      Sajjan Gundapuneedi
 * Andrew ID: sgundapu
 *
 * Description: A proxy server with cacheing.
 *
 */

#include "csapp.h" 
#include "cache.h"

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0\
 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,\
 application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";

/*
 * Helper Functions
 */
void get_request_from_client(int client_fd);
char *parse_uri(char *uri, char *host, char *path, char *cgiargs);
void prepare_string(char *buf2, char *path, char *host);

/*
 * Thread function prototype
 */
void *thread(void *vargp);

cache_list *cache;
sem_t mutex;

/*
 * Main
 *
 */
int main(int argc, char** argv)
{
    printf("%s%s%s", user_agent_hdr, accept_hdr, accept_encoding_hdr);

    /* Copied from the echo server code of the text book */
    int listenfd, *connfdp, port;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    pthread_t tid;

    /* Initialize the cache */
    cache = (cache_list*) Malloc(sizeof(cache_list));
    cache->head = NULL;
    cache->size = 0;
    init_cache();

    clientlen = sizeof(clientaddr);

    /* Check the command line arguments */
    if (argc != 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(0);
    }

    /* Handler for the sigpipe, to ignore it */
    Signal(SIGPIPE, SIG_IGN);

    /* Get the port number */
    port = atoi(argv[1]);

    Sem_init(&mutex, 0, 1);
    /* Here it listens for connections until there is a connection */
    listenfd = Open_listenfd(port);
    while(1) {
	if ((connfdp = Malloc(sizeof(int))) == NULL) {
	    printf("Malloc error !!\n");
	}
	P(&mutex);
	/* The connfd accepts the connection from the client and get its address*/
	*connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
	Pthread_create(&tid, NULL, thread, connfdp);
    }

    /* The program should not reach here */
    exit(0);

    return 0;
}

/*
 * Thread function
 */
void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Free(vargp);
    V(&mutex);
    Pthread_detach(pthread_self());
    get_request_from_client(connfd);
    Close(connfd);
    return NULL;
} 

/*
 * Basic job after accepting the connection through the connfd
 *
 */
void get_request_from_client(int client_fd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char buf2[MAX_OBJECT_SIZE];
    rio_t rio_c, rio_s;
    int port, proxyfd;
    size_t rec_count;
    char host[MAXLINE], path[MAXLINE], cgiargs[MAXLINE]; 

    char temp_cache[MAX_OBJECT_SIZE];
    unsigned int temp_size = 0;

    Rio_readinitb(&rio_c, client_fd);
    Rio_readlineb(&rio_c, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);

    port = atoi(parse_uri(uri, host, path, cgiargs));
    strcat(uri, path);
    printf("uri: %s", uri);

    cache_node *match_node = search(cache, uri);

    /* if data in the cache, send as a response*/
    if (match_node != NULL) {
        printf("Reading from the cache\n");
        if (rio_writen(client_fd, match_node->content, match_node->size) < 0) {
            printf("Error while sending the response to client\n");
        }
    }
    else {

    prepare_string(buf2, path, host);
    printf("the string sent to the server is %s \n", buf2);
    printf("string length is %lu\n", strlen(buf2));
    /* Reentrant open client fd function */
    proxyfd = Open_clientfd_r(host, port);
       if (proxyfd < 0)
    {
        if (proxyfd != -1) {
return;        
}
        return;
    }

    Rio_readinitb(&rio_s, proxyfd);

    Rio_writen(proxyfd, buf2, strlen(buf2));
    
    memset(buf2, 0, MAX_OBJECT_SIZE); 
    memset(temp_cache, 0, MAX_OBJECT_SIZE); 
    while ((rec_count = Rio_readnb(&rio_s, buf2, MAX_OBJECT_SIZE)) >= 0) {
	temp_size = temp_size + rec_count;
	if (temp_size < MAX_OBJECT_SIZE) {
	    //strcat(temp_cache, buf2);
	    memcpy(temp_cache, buf2, rec_count);
	}
	Rio_writen(client_fd, buf2, rec_count);	
	//temp_size = temp_size + rec_count;
	memset(buf2, 0, MAX_OBJECT_SIZE); 
	printf("%s\n", buf2);
    }
    if (temp_size < MAX_OBJECT_SIZE) {
        printf("Adding data to the cache\n");
        add_node(cache, uri, temp_cache, temp_size);
    }
    }
    //Close(proxyfd);
    return;
}

/* 
 * String to be sent to the server
 */
void prepare_string(char *buf2, char *path, char *host)
{ 
    strcpy(buf2, "GET ");
    strcat(buf2, path);
    strcat(buf2, " HTTP/1.0\r\n");
    strcat(buf2, "Host: ");
    strcat(buf2, host);
    strcat(buf2, "\r\n");
    strcat(buf2, user_agent_hdr);
    strcat(buf2, accept_hdr);
    strcat(buf2, accept_encoding_hdr);
    strcat(buf2, "Connection: close\r\n");
    strcat(buf2, "Proxy-Connection: close\r\n\r\n");

    return;
}

/*
 * parse uri function
 */
char *parse_uri(char *uri, char *host, char *path, char *cgiargs)
{
    char *path_start, *port_start, *tmp, *port;
    const char s[2] = ":";

    sscanf(uri, "http://%s", uri);
    
    if ((path_start = strchr(uri, '/')) == NULL) {
	strcpy(cgiargs, "");
	strcpy(path, "/");	
	strcpy(host, uri);
    } else {
	strcpy(path, path_start);
	path_start[0] = '\0';
	strcpy(host, uri);	
    }

    if ((port_start = strchr(host, ':')) != NULL) {
	port_start++;
	port = port_start;
	tmp = strtok(host, s);
	strcpy(host, tmp);
    } else {
	port = "80";
    }
    return port;
}


