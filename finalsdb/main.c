#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <stdio.h>

#include "server.h"
#include "inquery.h"
#include "db.h"
#include "json.h"
#include "outquery.h"

static int _stopFlag = 0;

static
void server_onstop(int sig)
{
    _stopFlag = 1;
}

int main(int argc, char **argv) {
    const static int _listenCount = 5;
    int sockfd;
    struct sockaddr_in addr, cliaddr;
    char buf[2048];
    socklen_t cliaddr_size;
    struct sigaction sa;

    if( argc != 2 ) {
        fprintf(stdout, "Usage: %s port\n", argv[0]);
        return 0;
    }

    sa.sa_flags = 0;
    sa.sa_handler = server_onstop;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGUSR1);

    cliaddr_size = sizeof(struct sockaddr_in);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    setbuf(stdout, NULL);
    fclose(stdin);
    stdin = NULL;

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        fprintf(stderr, "Unable to create socket: %s\n", 
            strerror(errno));
        return 1;
    }
    if( bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0 ) {
        fprintf(stderr, "Unable to bind socket: %s\n", 
            strerror(errno));
        close(sockfd);
        return 1;
    }
    if( listen(sockfd, _listenCount) < 0 ) {
        fprintf(stderr, "Unable to listen socket: %s\n", 
            strerror(errno));
        close(sockfd);
        return 1;
    }

    fprintf(stdout, "Server started. Listen count: %d. Waiting client connection...\n",
	_listenCount);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    while( !_stopFlag ) {
        int fd;
        pid_t pid;

        if( (fd = accept(sockfd, (struct sockaddr *) &cliaddr, &cliaddr_size)) < 0
            && errno != EINTR ) { 
            fprintf(stderr, "Unable to accept client: %s\n", 
            strerror(errno));
        } else if( !_stopFlag ) {
            fprintf(stdout, "Accepted client from %s\n",
            inet_ntoa(cliaddr.sin_addr));

            if( (pid = fork()) < 0 ) {
                fprintf(stderr, "Unable to fork process: %s\n", 
                strerror(errno));
            } else if( pid > 0 ) {
                close(fd);
            } else {
                /* --- */
                int n = read(fd, buf, sizeof(buf));
                buf[n] = '\0';

                struct inquery query;
                int query_status = parse_query(buf, &query);
                if (query_status > 0) {
                    struct outquery out = route(&query);
                    
                    printf("res headers: %s", out.headers);
                    write(fd, out.headers, strlen(out.headers));
                    printf("res bode: %s", out.body);
                    write(fd, out.body, strlen(out.body));

                    free(out.body);

                }
                free(query.method);
                free(query.url);
                /* --- */
                _exit(0);
                

                close(fd);
                }
            }
    } 

    close(sockfd);

    fprintf(stdout, "Server stopped.\n");

    return 0;


}