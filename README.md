# TrabalhoComputacaoDistribuida
## Integrantes:
Francisco Losada Totaro - 10364673 <br>
Pedro Henrique Lanfredi Moreiras - 10441998

## Compilar/Executar:
### Cliente:
- Compilar: gcc -Wall -Wextra -O2 -o client client.c
- Executar: ./client 127.0.0.1 5001
### Servidor:
- Compilar: gcc -Wall -Wextra -O2 -o server server.c
- Executar: ./server 5001

### Exemplos de Comando: 

- ADD 1 2 -> Soma 1 + 2;
- SUB 2 1 -> Subtração 2 - 1;
- MUL 3 -6 -> Multiplicação 3 * -6;
- DIV 7 4.5 -> Divisão 7 / 4.5.

### Formato Protocolo e Exemplos:

Formato de Requisições: 
- OP A B\n <br>

Onde:
- OP ∈ {ADD, SUB, MUL, DIV} 
- A, B são números reais no formato decimal com ponto (ex.: 2, -3.5, 10.0). <br>

Formato da resposta do servidor:
- OK R\n
- ERR <COD> <mensagem>\n (Erro)

Requisição → Resposta

- ADD 10 2\n      ->  OK 12\n
- SUB 7  9\n      ->  OK -2\n
- MUL -3 3.5\n    ->  OK -10.5\n
- DIV 5  0\n      ->  ERR EZDV divisao_por_zero\n


### Decisões de Projeto e Limitações Conhecidas: 

- Utilizamos como base a atividade do Laboratório de Sockets (Mini Chat). A primeira mudança realizada no servidor foi separar a mensagem do cliente em três itens: a operação, primeiro argumento e segundo argumento, utilizando o 'strtok' e um " " como delimitador. Após isso, realizamos condições onde, inicialmente, pegamos o operador e comparamos com uma String, se for verdadeiro realizamos o cáclculo relativo transformando os argumentos em Float e utilizamos o 'snprintf', para transformar o resultado da operação realizada novamente no buffer. Após isso, a mensagem é enviada novamente para o cliente e mostramos o resultado com 'printf'. Agora, para sair do servidor (QUIT), utilizamos a mesma lógica de comparação anterior, e fechamos a comunicação.<br><br>
- Tivemos um problema que o programa só estáva aceitando 'QUIT' com espaço depois da palavra. Para resolver esse problema, buscamos, em um primeiro momento, que havia uma quebra de linha na mensgem do cliente, e o trasnformamos em uma '/0'.<br><br>
- Sobre as limitações, apenas transformamos em token os três primeiros argumentos da mensagem do cliente, ou seja, caso colocarmos: ADD 4 5 3, ele simplemente ignora o número 3. Além disso, não conseguimos fazer com que mais de um cliente trabalhe ao mesmo tempo. 


