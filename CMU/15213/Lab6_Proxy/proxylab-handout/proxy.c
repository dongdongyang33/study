#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
void CheckAndForwardToServer(int cfd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio;
    Rio_readinitb(&rio, cfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    if(CheckParamter(cfd, method, version) == 0) return;

    char hostname[MAXLINE], filepath[MAXLINE];
    char port[MAXLINE] = "8080";
    if(GetServerInfoFromUri(cfd, uri, hostname, port, filepath) == 0) return;

    int rfd;
    char recbuf[MAXLINE];
    rio_t r_rio;
    rfd = Open_clientfd(hostname, port);
    printf("Connect success! Begin to receive respond and send to client.\n");
    Rio_readinitb(&r_rio, rfd);
    MakeRequestBodyAndSend(rfd, method, filepath, "HTTP/1.0", hostname);
    while (Rio_readlineb(&r_rio, recbuf, MAXLINE) != 0)
    {
        Rio_writen(cfd, recbuf, strlen(recbuf));
    }
    Close(rfd);
    return;
}

int CheckParamter(int cfd, char* method, char* version)
{
    if(strcasecmp(method, "GET"))
    {
        clienterror(cfd, method, "501", "Not Implemented", "The server not implement this method");
        return 0;
    }

    if(strncmp(version, "HTTP/1.", strlen(version)-1) != 0)
    {
        clienterror(cfd, version, "403", "Wrong Version", "The version is in wrong format");
        return 0;
    }

    return 1;
}

int GetServerInfoFromUri(int cfd, char* uri, char* hostname, char* port, char* filepath)
{
    int i = 0, j = 0, k = 0, cnt = 0;
    int urilen = strlen(uri);
    for(; i < urilen; i++)
    {
        if(uri[i] == '/') cnt++;
        if(cnt == 2 && j == 0) j = i+1;
        if(cnt == 2 && uri[i] == ':') k = i+1;
        if(cnt == 3) break;
    }
    if(cnt != 3)
    {
        clienterror(cfd, "Wrong URI", "403", "Wrong URI", "The input URI is wrong");
        return 0;
    }

    if(i+1 < urilen)
    {
        strncpy(filepath, uri + i, urilen-i+1);
        filepath[urilen-i+1] = '\0';
    }
    if(k != 0)
    {
        strncpy(hostname, uri+j, k-1-j);
        hostname[k-1-j] = '\0';
        strncpy(port, uri + k, i-k);
        port[i-k] = '\0';
    }
    else
    {
        strncpy(hostname, uri + j, i-j);
        hostname[i-j] = '\0';
    }
    printf("(hostname:port) = %s:%s\nfilepath = %s", hostname, port, filepath);
    return 1;
}

void MakeRequestBodyAndSend(int fd, char* method, char* filepath, char* version, char* hostname)
{
    char buf[MAXLINE], header[MAXBUF];

    sprintf(buf, "%s %s %s\r\n", method, filepath, version);
    sprintf(header, "HOST: %s\r\n", hostname);
    sprintf(header, "User-Agent: %s\r\n", user_agent_hdr);
    sprintf(header, "Connection: close\r\n");
    sprintf(header, "Proxy-Connection: close\r\n\r\n");

    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, header, strlen(header));
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>Proxy</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

int main(int argc, char* argv[])
{
    char defaultport[10] = "8080";
    char* listenport = defaultport;
    int receivefd, connectfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if(argc > 2)
    {
        printf("input error!\n");
        exit(0);
    }
    if(argc == 2) listenport = argv[1];
    receivefd = Open_listenfd(listenport);
    while(1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connectfd = Accept(receivefd, (SA*) &clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        CheckAndForwardToServer(connectfd);
        Close(connectfd);
        printf("Connectfd %d close. Wait for next connect...\n", connectfd);
    }
    printf("%s", user_agent_hdr);
    return 0;
}
