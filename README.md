# TP-Redes
## Servidor de Mensagens Publish/Subscribe

### Introdução
Neste trabalho iremos implementar um servidor e clientes para troca de mensagens de forma
similar ao serviço da plataforma Twitter. Clientes enviam mensagens para o servidor
informando em quais mensagens estão interessados. O servidor recebe mensagens de clientes
e repassa cada mensagem para todos os clientes que estão interessados naquela mensagem.
O tópico de uma mensagem é definido pelos tags que ela contém. Cada cliente envia ao
servidor em quais tags está interessado, e o servidor irá repassar ao cliente todas as
mensagens que contém pelo menos uma tag de seu interesse. Esse paradigma de
comunicação é conhecido como publish/subscribe.

### Protocolo
Servidores e clientes trocam mensagens curtas de até 500 bytes usando o protocolo TCP.
Mensagens carregam texto codificado segundo a tabela ASCII. Apenas letras, números, os
caracteres de pontuação ,.?!:;+-*/=@#$%()[]{} e espaços podem ser transmitidos.
(Caracteres acentuados não podem ser transmitidos.)
Clientes informam ao servidor que estão interessados em receber mensagens com um tag
enviando uma mensagem contendo o caractere + (mais) seguido do identificador do tag.
Clientes podem informar ao servidor que não estão mais interessados num tag enviando uma
mensagem contendo o caractere - (menos) seguido do identificador da tag. Identificadores de
tag são qualquer sequência de letras (sem números e sem pontuação). O servidor deve
confirmar mensagens de declaração de interesse “+tag” com uma mensagem de texto
contendo “subscribed +tag”; de forma similar, o servidor deve confirmar mensagens de
declaração de desinteresse “-tag” com uma mensagem de texto contendo “unsubscribed
-tag”. Por exemplo, abaixo segue um exemplo de comunicação do cliente com o servidor, onde
linhas começando com > são enviadas pelo cliente e linhas começando com < foram recebidas
do servidor:

````
> +dota
< subscribed +dota
> -overwatch
< unsubscribed -overwatch
````

