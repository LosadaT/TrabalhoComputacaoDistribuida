// chat_client.c
// Cliente do "mini-chat" TCP. Usa select() para ler simultaneamente:
// - stdin (teclado) -> envia ao servidor
// - socket -> imprime mensagens vindas do servidor
//
// Compilar: gcc -Wall -Wextra -O2 -o chat_client chat_client.c
// Executar: ./chat_client 127.0.0.1 5000

#include <stdio.h>      // printf, perror, fgets
#include <stdlib.h>     // exit, atoi
#include <string.h>     // memset, strlen
#include <unistd.h>     // close, read, write
#include <errno.h>      // errno
#include <sys/types.h>  // tipos básicos
#include <sys/socket.h> // socket, connect, send, recv
#include <netinet/in.h> // sockaddr_in, htons
#include <arpa/inet.h>  // inet_pton

#define BUF_SIZE 1024

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ip-servidor> <porta>\nEx.: %s 127.0.0.1 5000\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Porta inválida.\n");
        return EXIT_FAILURE;
    }

    // 1) socket(AF_INET, SOCK_STREAM, 0)
    //  - AF_INET: IPv4
    //  - SOCK_STREAM: TCP
    //  - 0: protocolo padrão para TCP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) die("socket");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    // Preenche o endereço do servidor
    servaddr.sin_family = AF_INET;               // família IPv4
    servaddr.sin_port   = htons((uint16_t)port); // porta em ordem de rede
    // inet_pton(int af, const char *src, void *dst)
    //  - af: família do endereço (AF_INET)
    //  - src: string do IP (e.g., "127.0.0.1")
    //  - dst: ponteiro onde será armazenado em binário (servaddr.sin_addr)
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "IP inválido: %s\n", server_ip);
        close(sockfd);
        return EXIT_FAILURE;
    }

    // 2) connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
    //  - sockfd: socket criado
    //  - addr:   ponteiro p/ endereço do servidor (IP/porta)
    //  - addrlen:tamanho da struct
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        die("connect");

    printf("[CLIENTE] Conectado em %s:%d\n", server_ip, port);
    printf("[CLIENTE] Digite mensagens e pressione ENTER. Ctrl+D para sair.\n");

    fd_set rset;
    char sendbuf[BUF_SIZE];
    char recvbuf[BUF_SIZE];

    for (;;) {
        FD_ZERO(&rset);
        FD_SET(STDIN_FILENO, &rset); // monitorar teclado
        FD_SET(sockfd, &rset);       // monitorar socket
        int maxfd = (STDIN_FILENO > sockfd ? STDIN_FILENO : sockfd);

        // select espera até haver dados no teclado ou no socket
        int nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0) {
            if (errno == EINTR) continue;
            die("select");
        }

        // Entrada do teclado?
        if (FD_ISSET(STDIN_FILENO, &rset)) {
            // fgets lê até '\n' ou EOF
            if (fgets(sendbuf, sizeof(sendbuf), stdin) == NULL) {
                // EOF (Ctrl+D): encerramos a conexão ordenadamente
                printf("\n[CLIENTE] EOF do teclado. Encerrando.\n");
                break;
            }
            size_t len = strlen(sendbuf);
            if (len > 0) {
                // send(int sockfd, const void *buf, size_t len, int flags)
                //  - sockfd: nosso socket conectado ao servidor
                //  - buf:    mensagem a enviar
                //  - len:    tamanho em bytes
                //  - flags:  0 para envio "normal"
                ssize_t n = send(sockfd, sendbuf, len, 0);
                if (n < 0) {
                    perror("send");
                    break;
                }
            }
        }

        // Dados vindos do servidor?
        if (FD_ISSET(sockfd, &rset)) {
            // recv retorna 0 quando o servidor fecha a conexão
            ssize_t n = recv(sockfd, recvbuf, sizeof(recvbuf) - 1, 0);
            // recv(int sockfd, void *buf, size_t len, int flags):
            //  - sockfd: socket conectado
            //  - buf:    buffer de recepção
            //  - len:    tamanho máximo
            //  - flags:  0 (sem flags)
            if (n <= 0) {
                if (n < 0) perror("recv");
                printf("[CLIENTE] Servidor encerrou a conexão.\n");
                break;
            }
            recvbuf[n] = '\0';
            printf("%s", recvbuf); // já contém '\n' em geral
            fflush(stdout);
        }
    }

    close(sockfd); // encerra o socket
    return 0;
}

/*
Atividade: Calculadora Cliente–Servidor com Sockets em C
Objetivo

Implementar uma aplicação cliente–servidor em C, usando sockets TCP (IPv4), na qual o cliente envia operações matemáticas e o servidor executa e retorna o resultado. A atividade reforça conceitos de redes, protocolos baseados em texto, parsing robusto, tratamento de erros, organização do código e automação de build com Makefile.

Descrição Geral

Você deverá implementar:

Servidor (server): aceita conexões TCP; recebe mensagens de requisição, separa tokens e executa as quatro operações básicas (+, −, ×, ÷); devolve o resultado ao cliente.
Cliente (client): conecta ao servidor, envia a operação digitada pelo usuário (linha por linha) e imprime a resposta.
O servidor deve processar uma requisição por linha e responder uma linha por requisição. A conexão permanece aberta até o cliente enviar QUIT (ou ocorrer erro).

Porta padrão: 5050. Permita alterá-la via linha de comando (ex.: ./server 6000).
Especificação do Protocolo (texto simples)

Formato obrigatório (prefixo) para requisições:

OP A B\n
Onde:

OP ∈ {ADD, SUB, MUL, DIV}
A, B são números reais no formato decimal com ponto (ex.: 2, -3.5, 10.0).
Formato da resposta do servidor:

OK R\n
ou, em caso de erro:

ERR <COD> <mensagem>\n
Códigos sugeridos: EINV (entrada inválida), EZDV (divisão por zero), ESRV (erro interno).
Exemplos

Requisição → Resposta

ADD 10 2\n      ->  OK 12\n
SUB 7  9\n      ->  OK -2\n
MUL -3 3.5\n    ->  OK -10.5\n
DIV 5  0\n      ->  ERR EZDV divisao_por_zero\n
Bônus opcional: aceitar também a forma infixa (não obrigatória): A <op> B\n com <op> ∈ {+, -, *, /}. Ex.: 10 + 2\n.
Requisitos Mínimos

Servidor TCP (IPv4) funcional na porta indicada; pode ser single-process/single-thread (um cliente por vez).
Cliente de terminal que lê linhas do stdin, envia ao servidor e imprime a resposta.
Parsing robusto (validar quantidade de tokens, tipos numéricos e operação).
Tratamento de erros (entrada inválida e divisão por zero).
Formatação do resultado com ponto decimal (não depender do locale). Sugestão: printf("%.6f\n", valor) para R.
Makefile com targets all, server, client, clean.
README.md explicando execução, exemplos e decisões de projeto.
Requisitos Recomendados

Logs simples no servidor (conexões, requisições, erros).
Encerramento limpo ao receber SIGINT (Ctrl+C).
Parametrização de endereço/porta no cliente (./client 127.0.0.1 5050).
Bônus (até +10%)

Concorrência: atender múltiplos clientes (via fork, threads ou select/poll).
Testes automatizados (scripts que disparam casos de teste e comparam saídas).
Protocolo estendido (ex.: aceitar forma infixa, mensagens HELP, VERSION).
Estrutura Sugerida do Projeto

/src
  client.c
  server.c
/include
  proto.h
/Makefile
/README.md
/tests (opcional)
Separe funções de parsing, cálculo e I/O de socket.
Mantenha cabeçalhos em /include e implemente em /src.
Exemplos de Uso

Servidor (porta padrão 5050):

./server
Cliente:

./client 127.0.0.1 5050
ADD 10 2
OK 12
DIV 5 0
ERR EZDV divisao_por_zero
QUIT
Entrega

Escolha uma das opções:

PDF único contendo todo o código-fonte, Makefile e README (com trechos de código bem formatados), OU
Link de um repositório com o projeto completo (README, fontes e Makefile).
Inclua no README:

Como compilar e executar (exemplos de comandos).
Formato do protocolo e exemplos.
Decisões de projeto e limitações conhecidas.
Como rodar testes (se houver).
Critérios de Avaliação

Funcionalidade e conformidade ao protocolo – 40%
Robustez e tratamento de erros – 20%
Organização do código e Makefile – 15%
Documentação (README e clareza) – 15%
Testes e/ou Bônus – 10%
*/