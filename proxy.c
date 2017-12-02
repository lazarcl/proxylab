/*
 * This is a proxy written by Cooper Lazar and Ricardo Vivanco
 * with the help of TinyServer.
 */

#include <stdio.h>
#include <signal.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
// static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void hostTest(int fd);
void hostTest2(int fd);
char* read_requesthdrs(rio_t *rp);
int writeResponseHeaders(rio_t *rp, int fd);
int parse_uri(char *uri, char *filename, char *cgiargs);

int main(int argc, char **argv)
{

    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    signal(SIGPIPE, SIG_IGN);
    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }

    listenfd = open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                     port, MAXLINE, 0);
    	printf("Accepted connection from (%s, %s)\n", hostname, port);
    	hostTest(connfd);
	}
}

void hostTest(int clientfd) 
{
    char buf[MAXLINE];
    char uri[MAXLINE], version[MAXLINE];
    char hostAndPort[MAXLINE];
    char host[MAXLINE];
    char port[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, clientfd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) {
        Close(clientfd);
        return;
    }
    printf("%s", buf);
    if (sscanf(buf, "GET http://%[^/]%s %s", hostAndPort, uri, version) != 3) {
    	printf("cannot parse request using 'GET http://' sscan: %s\n", buf);
        Close(clientfd);
        return;
    }   

    char* foundHostHeader = read_requesthdrs(&rio);
    int connfd;
    if (sscanf(hostAndPort, "%[^:]%s", host, port) == 2) {
    	//increment port by one to move past ':'
	   	connfd = open_clientfd(host, port+1);
    } else {
	   	connfd = open_clientfd(host, "80");
    }
    if (connfd < 0) {
    	printf("connection to: %s failed\n", host);
    	Close(clientfd);
    	return;
    }

    //send request to host
   	rio_writen(connfd, "GET ", strlen("GET "));
   	rio_writen(connfd, uri, strlen(uri));
   	rio_writen(connfd, " ", 1);
   	rio_writen(connfd, "HTTP/1.0", strlen("HTTP/1.0"));
   	rio_writen(connfd, "\r\n", strlen("\r\n"));

   	rio_writen(connfd, "Host: ", strlen("Host: "));
    if (foundHostHeader != NULL) {
        rio_writen(connfd, foundHostHeader, strlen(foundHostHeader));
    } else {
       	rio_writen(connfd, host, strlen(host));
    }
   	rio_writen(connfd, "\r\n", strlen("\r\n"));

   	rio_writen(connfd, "Connection: close", strlen("Connection: close"));
   	rio_writen(connfd, "\r\n", strlen("\r\n"));

   	rio_writen(connfd, "Proxy-Connection: close", strlen("Proxy-Connection: close"));
   	rio_writen(connfd, "\r\n", strlen("\r\n"));

   	rio_writen(connfd, "\r\n", strlen("\r\n"));

   	//read response from host
   	rio_t rioConn;
   	Rio_readinitb(&rioConn, connfd);

    int n = 0, i = 0;
    while(( n = rio_readlineb(&rioConn, buf, MAXLINE)) > 0) {
        rio_writen(clientfd, buf, n);
        i += 1;
    }
    Close(connfd);
    Close(clientfd);
}

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
char* read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];
    char host[MAXLINE];
    // int foundHost = 0;

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {
       if (sscanf(buf, "Host: %s", host) == 1 ) {
         // foundHost = 1;
       }
       Rio_readlineb(rp, buf, MAXLINE);
       printf("%s", buf);
    }
    // if (foundHost == 1) { return host; }
    // else { return NULL; }
    return NULL;
}
/* $end read_requesthdrs */


/*
 * writeResponseHeaders - write the responses to clientfd
 */
int writeResponseHeaders(rio_t *rp, int fd) 
{
    char buf[MAXLINE];
    int bodyLen = -1;

    Rio_readlineb(rp, buf, MAXLINE);
    rio_writen(fd, buf, strlen(buf));
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {
       Rio_readlineb(rp, buf, MAXLINE);
       rio_writen(fd, buf, strlen(buf));
       sscanf(buf, "Content-length: %d", &bodyLen);
       printf("%s", buf);
    }
    return bodyLen;
}