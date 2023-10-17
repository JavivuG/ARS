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
#include <arpa/inet.h>
#include <errno.h>


int main(int argc, char const *argv[]){
    int desc_socket, n;
    unsigned short puerto=5;
    char buffer[80];
    const char* cadena;
    struct in_addr direccion_ip;
    struct sockaddr_in addr_server;
    socklen_t addr_server_len=sizeof(addr_server);

    // Compruebo que el formato de entrada es el correcto
    if (argc<3 || argc>5 || argc==4){
        fprintf(stderr, "Uso: ./echocon-udp-client ip_servidor [-p puerto-servidor] cadena\n");
        exit(EXIT_FAILURE);
    }

    // Guardo la cadena para usarla posteriormente
    if (argc==3){
        cadena=argv[2];
    }
    else {
        cadena=argv[4];
        if (sscanf(argv[3],"%hu", &puerto)!=1) { // Puerto indicado con el parametro -p
            fprintf(stderr, "Puerto invalido\n");
            exit(EXIT_FAILURE);
        }
    }

    if (strlen(cadena)>80){
        fprintf(stderr, "La cadena introducida debe tener 80 caracteres como maximo\n");
        exit(EXIT_FAILURE);
    }

    // Convierto la direccion IP introducida
    if (inet_aton(argv[1], &direccion_ip) == 0) {
        fprintf(stderr, "inet_aton(): direccion IP invalida\n");
        exit(EXIT_FAILURE);
    }

    // Creo el socket
    if ((desc_socket = socket(AF_INET,SOCK_DGRAM,0))<0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    

    // Configuro la IP, puerto y tipo de IP del servidor
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(puerto); // Convierto el numero de puerto a entero de 16 bits
    addr_server.sin_addr.s_addr = direccion_ip.s_addr;

    // Envio la cadena al servidor
    if (sendto(desc_socket, cadena, strlen(cadena), 0, (struct sockaddr *) &addr_server, addr_server_len)<0){
        perror("sendto()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    // Recibo el mensaje del servidor en el buffer
    if ((n=recvfrom(desc_socket, buffer, strlen(cadena), 0, (struct sockaddr *) &addr_server, &addr_server_len))<0){
        perror("recvfrom()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    buffer[n]='\0';
    printf("%s\n", buffer);

    close(desc_socket); // Cierro el socket

    return 0;
}
