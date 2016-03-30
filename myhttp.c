#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFFSIZE 5000
typedef struct 
{
    char method[10];
    char url[20];
    char version[10];
    char host[20];
}HDR;


void prase_hdr(char *origin_buf, HDR *header)
{
    char *p = strtok(origin_buf, " ");

    strcpy(header->method, p);
    p = strtok(NULL, " ");
    strcpy(header->url, p);
    p = strtok(NULL, "\r");
    strcpy(header->version, p);
    while (*p != '\n')
        p++;
    p+=6;
    char *tmp = strchr(p, '\r');
    *tmp = '\0';
    strcpy(header->host, p);
    printf("host: %s, method: %s, url: %s\n", header->host, header->method, header->url);
}

void send_get_success(int clifd, HDR *header, struct sockaddr_in *cliaddr)
{
    char sendbuf[BUFFSIZE];

    bzero(sendbuf, sizeof(sendbuf));
    strcpy(sendbuf, header->version);
    strcat(sendbuf, " 200");
    strcat(sendbuf, " OK\r\n");
    strcat(sendbuf, "\r\n");
    FILE *fp = fopen("index.html", "r");
    char tmpbuf[BUFFSIZE];
    bzero(tmpbuf, sizeof(tmpbuf));
    fread(tmpbuf, sizeof(tmpbuf), 1, fp);
    strcat(sendbuf, tmpbuf);
    send(clifd, sendbuf, sizeof(sendbuf), 0);
    close(clifd);
    printf("-- %s : %d disconnected\n", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port));
}

void send_get_error(int clifd, HDR *header, struct sockaddr_in *cliaddr)
{
    char sendbuf[BUFFSIZE];
    bzero(sendbuf, sizeof(sendbuf));
    strcpy(sendbuf, header->version);
    strcat(sendbuf, " 404");
    strcat(sendbuf, " not found\r\n");
    strcat(sendbuf, "\r\n");
    send(clifd, sendbuf, sizeof(sendbuf), 0);
    close(clifd); 
    printf("-- %s : %d disconnected\n", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port));
}

void start_process(int clifd, struct sockaddr_in *cliaddr)
{
    HDR header; 
    char request_buf[BUFFSIZE];

    printf("-- %s : %d has connected --\n", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port)); 

    if (recv(clifd, request_buf, sizeof(request_buf), 0) == -1)
    {
        fprintf(stderr, "read request from clifd error.\n"); 
        exit(1);
    }

    prase_hdr(request_buf, &header);
    if (!strcmp(header.method, "GET") && !strcmp(header.url, "/"))
        send_get_success(clifd, &header, cliaddr);
    else
        send_get_error(clifd, &header, cliaddr);
}

int main()
{
    int listen_fd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("socket create");
        exit(1);
    }
    int on = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }
    
    if (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("bind"); 
        exit(1);
    }
    
    if (listen(listen_fd, 5) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf("server start, address: %s, listening: %d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
    while (1)
    {
        int newfd = accept(listen_fd, (struct sockaddr *)&cliaddr, &clilen);
        if (newfd == -1)
        {
            perror("accept");
            exit(1);
        }
        static int child_number;
        int pid = fork();
        child_number++;
        if (pid < 0)
        {
            perror("fork");
            exit(1);
        }
        else if (pid == 0)
        {
            printf("child process %d\n", child_number);
            close(listen_fd);
            start_process(newfd, &cliaddr);     
            exit(0);
        }
        close(newfd); //fixed.
        printf("main process continue listening\n");        
    }
    return 0;
}
