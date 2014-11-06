/*
 * proxy.c
 *
 * Name:- Krutika Kamilla
 * Andrew ID:- kkamilla
 * Email ID:- kkamilla@andrew.cmu.edu
 * 
 * This program implements http web proxy with multithreading 
 * capability and Caching support. It only supports get method
 * Cache is implemented in cache.c
 *
 * Caching highlights:
 * 1. Singly linked list is used to implement the cache. Eviction is
 *    done using LRU technique.
 * 2. Read write locks (with semaphores) are used to control concurrent 
 *    access to cache
 *
 * Changes to csapp.c/csapp.h:
 * 1. Disabled exit on error in error functions, as sometime during 
 *    testing proxy was stopping the process with 'Connection reset by
 *    peer'. Such errors are handled at thread level & particular 
 *    thread is stopped instead of entire process.
 * 2. Updated Rio_writen function to return error code
 *
 */

#include "csapp.h"
#include "cache.h"

/* You won't lose style points for including these long lines in your code */
static const char *user_agent = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_complete_str = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection = "Connection: close\r\n";
static const char *proxy_connection = "Proxy-Connection: close\r\n";

/* Define DEBUG to enable debugging output */
//#define DEBUG
#ifdef DEBUG
# define debug_printf(...) printf(__VA_ARGS__)
#else
# define debug_printf(...)
#endif

/* Global variables */
cache_list *cache;
sem_t mutex;

/* Helper function declarations */
void *thread(void *vargp);
int process_request(int fd);
void clienterror(int fd, char *cause, char *errnum, \
                    char *shortmsg, char *longmsg);
int read_requesthdrs(rio_t *rp, char *host_requesthdr, \
                        char *other_requesthdrs);
char *parse_uri(char *uri, char *host, char *path, char *cgiargs);
int forward_request_response(int fd, char *uri, char *host, char *port, \
        char *path, char *host_requesthdr, char *other_requesthdrs);
int open_clientfd_r(char *hostname, char *port);

/* Helper function definitions */

/* Function: thread
 * This function defines the functionality of threads for HTTP proxy
 */
void *thread(void *vargp) {
    int rc;
    int connfd = *((int *)vargp);
    Free(vargp);
    V(&mutex);
    
    /* Detach thread as each thread will serve single request */
    Pthread_detach(pthread_self());
    
    rc = process_request(connfd);
    if (rc < 0) {
        Close(connfd);
        Pthread_exit(NULL);
    }
    Close(connfd);
    return NULL;
} 

/* Function: process_request
 * This function processes the the client's request
 */
int process_request(int fd) {
    debug_printf("In process_request\n");
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host_requesthdr[MAXLINE], other_requesthdrs[MAXLINE];
    char path[MAXLINE], cgiargs[MAXLINE], host[MAXLINE];
    char uri_copy[MAXLINE];
    char *port;
    int rc;
    
    /* Initialize connection with client for reading */
    debug_printf("Initializing client buffer\n");
    Rio_readinitb(&rio, fd);
    
    /* Read request line from client */
    debug_printf("Reading request line\n");
    if (Rio_readlineb(&rio, buf, MAXLINE) < 0) {
        fprintf(stderr, "Request is null!\n");
        return -1;
    }
    
    /* Separate buffer into string format */
    sscanf(buf, "%s %s %s", method, uri, version);
    
    /* Check if method other than GET is requested */
    if (strcasecmp(method, "GET")) {
        debug_printf("Non GET method requested\n");
        clienterror(fd, method, "501", "No GET",
            "This method is not implemented in the proxy");
        return -1; 
    }
    
    debug_printf("Reading request headers\n");
    rc = read_requesthdrs(&rio,host_requesthdr,other_requesthdrs);
    if ( rc < 0 )
        return rc;
    
    strcpy(uri_copy, uri);
    debug_printf("Parsing URI\n");
    port = parse_uri(uri_copy, host, path, cgiargs); 
    
    debug_printf("Forwarding request to server\n");
    debug_printf("URI: %s\n", uri);
    rc = forward_request_response(fd, uri, host, port, path, \
                    host_requesthdr, other_requesthdrs);
    debug_printf("Returning from process_request\n");
    return rc;
} 

/* Function: clienterror
 * This function sends error to client in HTML format
 * (Adopted from book)
 */
void clienterror(int fd, char *cause, char *errnum, \
                    char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];
    debug_printf("In clienterror\n");
    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>Vishal's HTTP proxy</em>\r\n", body);
    
    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
    debug_printf("Returning from clienterror\n");
}

/* Function: read_requesthdrs
 * This function reads host & other request headers
 */
int read_requesthdrs(rio_t *rp, char *host_requesthdr, \
                        char *other_requesthdrs) {
    debug_printf("In read_requesthdrs\n");
    char buf[MAXLINE];
    char *host_str = "Host: ";
    char *accept_str = "Accept: ";
    char *ua_str = "User-Agent: ";
    char *ae_str = "Accept-Encoding: ";
    char *conn_str = "Connection: ";
    char *pconn_str = "Proxy-Connection: ";
    
    if ((Rio_readlineb(rp, buf, MAXLINE)) < 0) {
        fprintf(stderr, "Rio_writen error in clienterror!\n");
        return -1;
    }
    while(strcmp(buf, "\r\n")) {
        if ((Rio_readlineb(rp, buf, MAXLINE)) < 0) {
            fprintf(stderr, "Rio_writen error in clienterror!\n");
            return -1;
        }
        debug_printf("%s", buf);
        
        if (!strncmp(buf, host_str, strlen(host_str)))
            strcpy(host_requesthdr, buf + strlen(host_str));
        
        if (strncmp(buf, ua_str, strlen(ua_str)) && \
            strncmp(buf, accept_str, strlen(accept_str)) && \
            strncmp(buf, ae_str, strlen(ae_str)) && \
            strncmp(buf, conn_str, strlen(conn_str)) && \
            strncmp(buf, pconn_str, strlen(pconn_str))) {
            sprintf(other_requesthdrs, "%s%s", buf, other_requesthdrs);
        } 
    }
    debug_printf("Returning from read_requesthdrs\n");
    return 0;
}
/* Function: parse_uri
 * This function parses uri to get host, path & port
 */
char *parse_uri(char *uri, char *host, char *path, char *cgiargs) {
    debug_printf("In parse_uri\n");
    char *path_start, *port_start, *tmp;
    char *port;
    const char s[2] = ":";
    
    sscanf(uri, "http://%s", uri);
    if ((path_start = strchr(uri, '/')) == NULL) {
        strcpy(host, uri);
        strcpy(cgiargs, "");
        strcpy(path, "/");
    } else {
        strcpy(path, path_start);
        path_start[0] = '\0';
        strcpy(host, uri);
    }
    
    if ((port_start = strchr(host, ':')) != NULL) {
        port_start++;
        port = port_start;
        tmp = strtok(host,s);
        strcpy(host,tmp);
    } else {
        port = "80";
    }
    debug_printf("Returning from parse_uri\n");
    return port;
}

/* Function: forward_request_response
 * This function forwards request to server & upon receiving response
 * forwards it to client, it also makes cache decision.
 */
int forward_request_response(int fd, char *uri, char *host, char *port, \
                char *path, char *host_requesthdr, char *other_requesthdrs) {
    debug_printf("In forward_request_response\n");
    int server_connfd, read_status;
    char buf[MAXBUF], server_response[MAX_OBJECT_SIZE];
    rio_t rio;
    /* Variable for triggering the call of cache function */
    char temp_cache_data[MAX_OBJECT_SIZE];
    int temp_content_size;
    
    /* Search requested uri in cache */
    debug_printf("Searching for uri in cache\n");
    cache_node *matched_node = search_cache(cache, uri);

    /* If data in cache, send as response to client */
    if(matched_node != NULL) {
        debug_printf("Found uri in cache, sending data to client\n");
        debug_printf("URI: %s\n", uri);
        if ((Rio_writen(fd, matched_node->content, \
                matched_node->content_size)) < 0) {
            fprintf(stderr, "Rio_writen error while sending \
                                response to client!\n");
            return -1;
        }
        return 0;
    }
    
    /* Open connection with server to forward request */
    debug_printf("Opening connection with server\n");
    debug_printf("Host: %s | Port: %s\n", host, port);
    server_connfd = open_clientfd_r(host, port);
    if (server_connfd < 0)
    {
        if (server_connfd != -1) {
            clienterror(fd, host, "500", "Connection error!", \
                        "Could not resolve remote host!");
        }
        return server_connfd;
    }
    
    /* Prepare request to send to server */
    debug_printf("Preparing request to send to server\n");
    sprintf(buf, "GET %s HTTP/1.0\r\n", path);
    if (!strlen(host_requesthdr)) {
        sprintf(buf, "%sHost: %s\r\n", buf, host);
    } else {
        sprintf(buf, "%sHost: %s\r\n", buf, host_requesthdr);
    }
    sprintf(buf, "%s%s", buf, user_agent);
    sprintf(buf, "%s%s", buf, accept_complete_str);
    sprintf(buf, "%s%s", buf, accept_encoding);
    sprintf(buf, "%s%s", buf, connection);
    sprintf(buf, "%s%s", buf, proxy_connection);
    sprintf(buf, "%s%s\r\n", buf, other_requesthdrs);
    
    debug_printf("Server Request: \n%s\n", buf);
    
    /* Send request to server */
    debug_printf("Sending request to server\n");
    if((Rio_writen(server_connfd, buf, strlen(buf))) < 0) {
        fprintf(stderr, "Rio_writen error while forwarding \
                            request to server!\n");
        return -1;
    }
    
    /* Initialize connection with server for reading */
    debug_printf("Initializing connection with server for reading\n");
    Rio_readinitb(&rio, server_connfd);
    
    /* Initialize variables */
    memset(server_response, 0, sizeof(server_response));
    temp_content_size = 0;
    
    debug_printf("Reading server response\n");
    while ((read_status = Rio_readnb(&rio, server_response, \
                                    MAX_OBJECT_SIZE)) > 0) {
            /* Store returned contents to temporary array */
            memcpy(temp_cache_data, server_response, read_status);
            
            /* Send response to client */
            debug_printf("Sending server response to client\n");
            if((Rio_writen(fd, server_response, read_status)) < 0) {
                fprintf(stderr, "Rio_writen error while sending \
                            response to client!\n");
                return -1;
            }
            
            memset(server_response, 0, sizeof(server_response));  
            temp_content_size += read_status;    
    }
    
    if (temp_content_size < MAX_OBJECT_SIZE) {
        debug_printf("Adding data to cache\n");
        add_cache_node(cache, uri, temp_cache_data, temp_content_size);
    }
    debug_printf("Returning from forward_request_response\n");
    return 0;
}

/*
 * open_clientfd_r - thread-safe version of open_clientfd
 */
int open_clientfd_r(char *hostname, char *port) {
    int clientfd;
    struct addrinfo *addlist, *p;
    int rv;

    /* Create the socket descriptor */
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    /* Get a list of addrinfo structs */
    if ((rv = getaddrinfo(hostname, port, NULL, &addlist)) != 0) {
        return -1;
    }
  
    /* Walk the list, using each addrinfo to try to connect */
    for (p = addlist; p; p = p->ai_next) {
        if ((p->ai_family == AF_INET)) {
            if (connect(clientfd, p->ai_addr, p->ai_addrlen) == 0) {
                break; /* success */
            }
        }
    } 

    /* Clean up */
    freeaddrinfo(addlist);
    if (!p) { /* all connects failed */
        close(clientfd);
        return -1;
    }
    else { /* one of the connects succeeded */
        return clientfd;
    }
}

int main(int argc, char **argv)
{
    int listenfd, *connfdp, port;
    socklen_t clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;
    
    //printf("%s%s%s", user_agent, accept_complete_str, accept_encoding);
    
    /* checking argument */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    port = atoi(argv[1]);
    if (port <= 0) {
        fprintf(stderr, "Invalid port number!\n");
        exit(1);
    }
    
    listenfd = Open_listenfd(port);
    if (listenfd < 0) {
        fprintf(stderr, "Cannot open port to listen: %d\n", port);
        exit(1);
    }
    
    /* Ignoring potential SIGPIPE delivery from kernel */
    Signal(SIGPIPE, SIG_IGN);
    
    /* Initialize cache */
    cache = (cache_list*) Malloc(sizeof(cache_list));
    cache->head_node = NULL;
    cache->total_size = 0;
    init_cache();
    
    
    /* Initializing mutex for accept function */
    Sem_init(&mutex, 0, 1);
    
    while (1) {
        connfdp = Malloc(sizeof(int));
        if (connfdp) {
            P(&mutex);
            *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
            Pthread_create(&tid, NULL, thread, connfdp);
        } else {
            fprintf(stderr, "Out of memory!\n");
            fprintf(stderr, "Sleeping for a second before retry.....\n");
            sleep(1);
        }
    }
    return 0;
}
