// Practica Tema 5: García González, Javier
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>


int main(int argc, char const *argv[]){
    int desc_socket;
    unsigned short puerto=5;
    struct sockaddr_in addr_server;

    // Compruebo que el formato de entrada es el correcto
    if (argc>4 || argc==2){
        fprintf(stderr, "Uso: ./echocon-udp-server [-p puerto-servidor]\n");
        exit(EXIT_FAILURE);
    }

    if (argc==4){
        if (sscanf(argv[3],"%hu", &puerto)!=1) { // Puerto indicado con el parametro -p
            fprintf(stderr, "puerto invalido");
            exit(EXIT_FAILURE);
        }
    }

    // Creo el socket
    if ((desc_socket = socket(AF_INET,SOCK_DGRAM,0))<0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(puerto);
    addr_server.sin_addr.s_addr = INADDR_ANY;

    // Bindeo el socket
    if (bind(desc_socket, (struct sockaddr *) &addr_server, sizeof(addr_server))<0){
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    while(true){
        // recvfrom
        // Procesar los datos 
        // Convertir cadena minuscula a mayuscula...
        // sendto
    }

    return 0;
}
