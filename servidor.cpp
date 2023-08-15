#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <iostream>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 500

std::map<int,std::vector<std::string> > mapa_clientes; //armazena as tags de todos os clientes conectados

int server_sockaddr_init(const char *portstr, struct sockaddr_storage *storage) { 
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1;
    }
    port = htons(port); // host to network short

    memset(storage, 0, sizeof(*storage));
    struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
    addr4->sin_family = AF_INET;
    addr4->sin_addr.s_addr = INADDR_ANY;
    addr4->sin_port = port;
    return 0;
}


int ischvalid(char c){ //verifica se o caractere passado é válido
    int ch = (int)c;
    if((ch==10) || (ch==12) || (ch>=32 && ch<=59) || (ch==61) || (ch>=63 && ch<=93) || (ch>=97 && ch<=125)){
        return 1;
    } else{
        return 0;
    }
}

void usage(int argc, char **argv) {
    printf("usage: %s <server port>\n", argv[0]);
    printf("example: %s 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct client_data {
    int csock;
    struct sockaddr_storage storage;
};


void * client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);
    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    std::vector<std::string> tags_client; //armazena as tags em que o cliente declarou interesse

    while(1){ 
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        unsigned total = 0;

        //trata o recebimento de uma mensagem em múltiplos pacotes
       while(1){ //chama recv até encontrar o caractere \n, que indica o fim da mensagem
            size_t count = recv(cdata->csock, buf + total, BUFSZ - total, 0);
            if(count == 0){
                break;
            }
            total += count;
            if(buf[total]='\n'){ // a mensagem chegou por completo 
                break;
            }
        }

    if(buf[0]=='#'){ //termina a execução do servidor caso receba a mensagem "##kill" do cliente
        char kill[7]={'#','#','k','i','l','l','\n'};
        int i = 0;
        int count_kill = 0;
        for(i=0;i<sizeof(kill);i++){
            if(buf[i]!=kill[i]){
                count_kill++;
            }
        }
        if(count_kill==0){
            logexit("kill servidor");
        }

    } else if (buf[0] == '+'){

        char buf_hashtag[BUFSZ];
        memset(buf_hashtag,0,BUFSZ);
        int i = 0;
        int j = 1;

        while(1){
            if(ischvalid(buf[j])==0){ //desconecta o cliente se houver caracter invalido na mensagem
                std::map<int,std::vector<std::string> >::iterator it_map;
                for (it_map = mapa_clientes.begin(); it_map != mapa_clientes.end();it_map++) {
                    if(it_map->first == cdata->csock){
                        mapa_clientes.erase(it_map); //apaga o cliente do mapa_clientes 
                    }
                }
                close(cdata->csock);
                pthread_exit(EXIT_SUCCESS);
            } else if(buf[j]=='\n'){
                break;
            }
            buf_hashtag[i] = buf[j];
            i++;
            j++;
        }
    
        bool tageq = true;
        for(i=0;i<tags_client.size();i++){
            if(strcmp(buf_hashtag,&tags_client[i][0])==0){ 
                tageq = false; 
            }
        }
        
        if(tageq){
            std::string s = (std::string) buf_hashtag;
            tags_client.push_back(s); //adiciona a nova tag ao vector tags_client
            mapa_clientes[cdata->csock]=tags_client; //atualiza o mapa_clientes com o novo vector tags_client
            std::cout<<"tag adicionada: " << tags_client.back() << std::endl;
            sprintf(buf, "subscribed +%s\n", buf_hashtag);
        } else {
             sprintf(buf, "already subscribed +%s\n", buf_hashtag);
        }

        size_t count = send(cdata->csock, buf, strlen(buf), 0);
        if (count != strlen(buf)) {
            logexit("send");
        }
            
    } else if (buf[0] == '-') {

        char buf_hashtag[BUFSZ];
        memset(buf_hashtag,0,BUFSZ);
        int i = 0;
        int j = 1;

        while(1){
            if(ischvalid(buf[j])==0){ //desconecta o cliente se houver caracter invalido na mensagem
                std::map<int,std::vector<std::string> >::iterator it_map;
                for (it_map = mapa_clientes.begin(); it_map != mapa_clientes.end();it_map++) {
                    if(it_map->first == cdata->csock){
                        mapa_clientes.erase(it_map); //apaga o cliente do mapa_clientes 
                    }
                }
                close(cdata->csock);
                pthread_exit(EXIT_SUCCESS);
            } else if(buf[j]=='\n'){
                break;
            }
            buf_hashtag[i] = buf[j];
            i++;
            j++;
        }
        
        sprintf(buf, "not subscribed -%s\n", buf_hashtag);

        std::vector<std::string>::iterator it = tags_client.begin();
        int k = 0;
        for(k=0;k<tags_client.size();k++){
            if(strcmp(buf_hashtag,&tags_client[k][0])==0){
                std::cout<<"tag removida: "<<tags_client[k]<<std::endl;
                tags_client.erase(tags_client.begin()+k);
                mapa_clientes[cdata->csock]=tags_client; //atualiza o mapa_clientes com o novo vector tags_client
                sprintf(buf, "unsubscribed -%s\n", buf_hashtag);
                break;
            }
        }    
    
        size_t count = send(cdata->csock, buf, strlen(buf), 0);
        if (count != strlen(buf)) {
            logexit("send");
        }

    } else {
        printf("%s",buf); 
        char buf_aux[BUFSZ];
        memset(buf_aux,0,BUFSZ);
        std::vector<std::string> tags_msg;
        int i = 0;
        int j = 0;
        int k = 0;
        for(i=0;i<strlen(buf);i++){
            if(ischvalid(buf[i])==0){ //desconecta o cliente se houver caracter invalido na mensagem
                std::map<int,std::vector<std::string> >::iterator it_map;
                for (it_map = mapa_clientes.begin(); it_map != mapa_clientes.end();it_map++) {
                    if(it_map->first == cdata->csock){
                        mapa_clientes.erase(it_map); //apaga o cliente do mapa_clientes 
                    }
                }
                close(cdata->csock);
                pthread_exit(EXIT_SUCCESS);
            } else if(buf[i]=='#' && buf[i-1] == ' '){
                char buf_hashtag[BUFSZ];
                memset(buf_hashtag,0,BUFSZ);
                j = i+1;
                while(1){ 
                    if(ischvalid(buf[j])==0){ //desconecta o cliente se houver caracter invalido na mensagem
                        std::map<int,std::vector<std::string> >::iterator it_map;
                        for (it_map = mapa_clientes.begin(); it_map != mapa_clientes.end();it_map++) {
                            if(it_map->first == cdata->csock){
                                mapa_clientes.erase(it_map); //apaga o cliente do mapa_clientes 
                            }
                        }
                        close(cdata->csock);
                        pthread_exit(EXIT_SUCCESS);
                    } else if(buf[j] == ' ' || buf[j]=='\n'){ //um # ou caractere de espaço indica o fim do identificador da tag
                        break; //interrompe a operação e armaneza buf_hashtag no vector tags_msg
                    } else if(buf[j]=='#'){ //se a varredura encontra um novo caractere # antes de um espaço ou quebra de linha, a tag atual é considerada inválida 
                       memset(buf_hashtag,0,BUFSZ); //descarta a tag invalida e interrompe o processo
                       break;
                    }
                    buf_hashtag[k]=buf[j];
                    j++;
                    k++;
                } 

                j = 0;
                k = 0;

                printf("[tag_msg] %s\n", buf_hashtag); 
                std::string s = (std::string) buf_hashtag;
                tags_msg.push_back(s); //armazena a tag atual na lista de tags da mensagem 

            } else if(buf[i]=='\n'){ //fim da mensagem

                //compara as tags da mensagem com as tags de cada cliente 
                std::map<int,std::vector<std::string> >::iterator it_map;
                std::vector<std::string>::iterator it_vector;
                std::vector<std::string>::iterator it_tags_msg;

                for (it_map = mapa_clientes.begin(); it_map != mapa_clientes.end();it_map++) {
                    int aux = 0;
                    if(it_map->first != cdata->csock){ //evita que a mensagem seja enviada para o mesmo cliente que a enviou
                        for(it_vector = it_map->second.begin();it_vector != it_map->second.end(); it_vector++){
                            for(it_tags_msg=tags_msg.begin();it_tags_msg!=tags_msg.end();it_tags_msg++){
                                if(*it_vector == *it_tags_msg){
                                    sprintf(buf_aux,"%s",buf);
                                    size_t count = send(it_map->first, buf_aux, strlen(buf_aux), 0); //obtém o socket do cliente
                                    if (count != strlen(buf_aux)) {                                  //através da chave do mapa_clientes
                                        logexit("send");
                                    }
                                    memset(buf_aux,0,BUFSZ);
                                    aux++;
                                    break; //interrompe o loop para evitar que a mensagem 
                                }          //seja enviada mais de uma vez para o mesmo cliente
                            }
                            if(aux!=0){
                                break; //interrompe o loop para evitar que a mensagem 
                            }           //seja enviada mais de uma vez para o mesmo cliente
                        }
                    }
                }
                tags_msg.clear(); //no caso de recebimento de múltiplas mensagens em um único recv(),
                                 //limpa o vector tags_msg para evitar que a próxima mensagem "herde"
                                 //as tags da mensagem anterior
            }
        }


    
    }

    }

}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

	    struct client_data *cdata = (struct client_data *)malloc(sizeof(*cdata));
	    if (!cdata) {
		    logexit("malloc");
	    }

	    cdata->csock = csock;
	    memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}
