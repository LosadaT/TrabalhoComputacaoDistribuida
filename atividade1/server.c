#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS FD_SETSIZE
#define BUF_SIZE 1024

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Envia 'msg' para todos os clientes conectados, exceto 'sender_fd' (quem enviou).
// static void broadcast(int *clients, int max_i, int sender_fd, const char *msg, size_t len) {
//     for (int i = 0; i <= max_i; i++) {
//         int fd = clients[i];
//         if (fd >= 0 && fd != sender_fd) {
//             // send(int sockfd, const void *buf, size_t len, int flags)
//             //  - sockfd: descritor do socket de destino
//             //  - buf:    ponteiro para dados a enviar
//             //  - len:    tamanho em bytes dos dados
//             //  - flags:  normalmente 0 (sem flags especiais)
//             ssize_t n = send(fd, msg, len, 0);
//             if (n < 0) {
//                 // Ex.: cliente pode ter desconectado; apenas reportamos
//                 perror("send");
//             }
//         }
//     }
// }

int main(int argc, char **argv) {
    

    if (argc != 2) {
        fprintf(stderr, "Use %s <porta>\nEx %s 5001", argv[0], argv[0]);
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Porta inválida!\n\n");
        return EXIT_FAILURE;
    }

    // socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) die("socket");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // aceita conn de qq. interface

    //bind
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
        die("bind");

    // listen
    if (listen(listen_fd, 8) < 0) //backlog (fila de conns pendentes)
        die("listen");

    printf("\n*** Servidor ouvindo em 0.0.0.0:%d ***\n\n", port);
    
    int clients[MAX_CLIENTS];

    for(int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;

    fd_set allset, rset;
    FD_ZERO(&allset);
    FD_SET(listen_fd, &allset);
    int maxfd = listen_fd;
    int max_i = -1;

    char buf[BUF_SIZE];

    for (;;) {
        rset = allset; // cópia (select modifica o set)
        // select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
        //  - nfds:     1 + maior descritor monitorado (maxfd + 1)
        //  - readfds:  conjunto de FDs para verificação de leitura pronta
        //  - writefds: (não usado aqui) NULL
        //  - exceptfds:(não usado) NULL
        //  - timeout:  (bloqueante) NULL para esperar indefinidamente
        int nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0) {
            if (errno == EINTR) continue; // interrompido por sinal
            die("select");
        }

        // Novo cliente chegando?
        if (FD_ISSET(listen_fd, &rset)) {
            struct sockaddr_in cliaddr;
            socklen_t clilen = sizeof(cliaddr);
            // accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
            //  - sockfd: socket em listen
            //  - addr:   (saída) endereço do cliente conectado (pode ser NULL)
            //  - addrlen:(entrada/saída) tamanho do addr; ajustado pelo kernel
            int connfd = accept(listen_fd, (struct sockaddr*)&cliaddr, &clilen);
            if (connfd < 0) {
                perror("accept");
            } else {
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cliaddr.sin_addr, ip, sizeof(ip));
                printf("[SERVIDOR] Novo cliente %s:%d (fd=%d)\n", ip, ntohs(cliaddr.sin_port), connfd);

                // Adiciona na lista de clientes
                int i;
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] < 0) {
                        clients[i] = connfd;
                        break;
                    }
                }
                if (i == MAX_CLIENTS) {
                    fprintf(stderr, "Muitos clientes, recusando.\n");
                    close(connfd);
                } else {
                    FD_SET(connfd, &allset);
                    if (connfd > maxfd) maxfd = connfd;
                    if (i > max_i) max_i = i;

                    const char *welcome = "Bem-vindo ao mini chat!\n";
                    send(connfd, welcome, strlen(welcome), 0);
                }
            }
            if (--nready <= 0) continue; // nada mais pronto
        }

        // Verifica dados vindos dos clientes existentes
        for (int i = 0; i <= max_i; i++) {
            char *op;
            char *arg1;
            char *arg2;
            const char delimiter[] = " ";
            float result;
            int fd = clients[i];
            if (fd < 0) continue;
            if (FD_ISSET(fd, &rset)) {
                ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
                // recv(int sockfd, void *buf, size_t len, int flags)
                //  - sockfd: socket do qual receber
                //  - buf:    buffer destino
                //  - len:    bytes máximos a ler
                //  - flags:  0 normal (sem flags)
                if (n <= 0) {
                    if (n < 0) perror("recv");
                    printf("[SERVIDOR] Cliente fd=%d desconectou.\n", fd);
                    close(fd);
                    FD_CLR(fd, &allset);
                    clients[i] = -1;
                } else {
                    buf[n] = '\0';
                    char *newline = strchr(buf, '\n');
                    if (newline) {
                         *newline = '\0';
                    }

                    op = strtok(buf, delimiter);
                    for (int i = 0; i < 2; i++) {
                        if (i == 0) arg1 = strtok(NULL, delimiter);
                        if (i == 1) arg2 = strtok(NULL, delimiter);
                    }
                    if (strcmp(op, "ADD") == 0 && arg1 != NULL && arg2 != NULL){ // ADD
                            result = atof(arg1) + atof(arg2);
                            snprintf(buf, sizeof(buf), "[RESPOSTA] OK %.6f\n", result);     
                    }

                    else if (strcmp(op, "SUB") == 0 && arg1 != NULL && arg2 != NULL){ // SUB
                            result = atof(arg1) - atof(arg2);
                            snprintf(buf, sizeof(buf), "[RESPOSTA] OK %.6f\n", result); 
                    }

                    else if (strcmp(op, "MUL") == 0 && arg1 != NULL && arg2 != NULL){ // MUL
                            result = atof(arg1) * atof(arg2);
                            snprintf(buf, sizeof(buf), "[RESPOSTA] OK %.6f\n", result);
                    }

                    else if (strcmp(op, "DIV") == 0 && arg1 != NULL && arg2 != NULL){ // DIV
                            if (atof(arg2) != 0.0) {
                                result = atof(arg1) / atof(arg2);
                                snprintf(buf, sizeof(buf), "[RESPOSTA] OK %.6f\n", result);
                            }
                            else {
                                snprintf(buf, sizeof(buf), "[RESPOSTA] ERR EZDV divisao_por_zero\n");
                            }
                    }
                    
                    else if (strcmp(op, "QUIT") == 0){ // QUIT
                            printf("[SERVIDOR] Cliente fd=%d desconectou.\n", fd);
                            close(fd);
                            FD_CLR(fd, &allset);
                            clients[i] = -1;
                    }
                    else {
                            snprintf(buf, sizeof(buf), "[RESPOSTA] ERR EINV operacao_invalida\n");
                            send(fd, buf, strlen(buf), 0);
                            break;
                    }
                    send(fd, buf, strlen(buf), 0);
                    printf("[MSG fd=%d] %s", fd, buf);
                    
                }
                if (--nready <= 0) break; // nenhum FD restante pronto
            }
        }
    }

    close(listen_fd);
    return 0;
}