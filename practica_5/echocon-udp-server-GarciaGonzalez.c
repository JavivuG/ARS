// Practica Tema 5: García González, Javier
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>

void conversionCadena(char *cadena);

int main(int argc, char const *argv[]){
    int desc_socket, n;
    unsigned short puerto=5;
    char buffer[80];
    struct sockaddr_in addr_server, addr_cliente;
    socklen_t addr_cliente_len=sizeof(addr_cliente);

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

    printf("--- Servidor iniciado ---\n\n");

    while(true){

        // Recibo el mensaje del servidor en el buffer
        if ((n=recvfrom(desc_socket, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr_cliente, &addr_cliente_len))<0){
            perror("recvfrom()");
            close(desc_socket);
            exit(EXIT_FAILURE);
        }

        buffer[n]='\0';
        conversionCadena(buffer);

        // Devuelvo la cadena modificada al cliente
        if (sendto(desc_socket, buffer, n, 0,(struct sockaddr *) &addr_cliente, addr_cliente_len)<0){
            perror("sendto()");
            close(desc_socket);
            exit(EXIT_FAILURE);
        } 
    }

    return 0;
}

// Funcion que cambia las mayusculas a minusculas en una cadena y viceversa
void conversionCadena(char* cadena){
    int caracter;
    
    // Convertir la cadena copiada a mayúsculas
    for (int i = 0; i < strlen(cadena); i++) {
        caracter=toupper(cadena[i]);

        if (cadena[i]==caracter){
            cadena[i] = tolower(cadena[i]);
        }
        else {
            cadena[i]=caracter;
        }
    }
    
}