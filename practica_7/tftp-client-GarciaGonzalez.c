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

#define PORT 69

int main(int argc, char const *argv[]){
    int desc_socket, bytes;
    bool verbose=false;
    char buffer[80]; // 80 debido a que es el tamaño maximo permitido
    const char *archivo, *modo;
    struct in_addr direccion_ip;
    struct sockaddr_in addr_server, addr_client;
    socklen_t addr_server_len=sizeof(addr_server);

    // Compruebo que el formato de entrada es el correcto
    if (argc>5 || argc<4){
        fprintf(stderr, "Uso: ./tftp-client ip_servidor {-r|-w} archivo [-v]\n");
        exit(EXIT_FAILURE);
    }

    // Convierto la direccion IP introducida a entero de 32 bits
    if (inet_aton(argv[1], &direccion_ip) == 0) {
        fprintf(stderr, "inet_aton(): direccion IP invalida\n");
        exit(EXIT_FAILURE);
    }

    if ((strcmp(argv[2],"-r")!=0) && (strcmp(argv[2],"-w")!=0)){
        fprintf(stderr, "Uso: ./tftp-client ip_servidor {-r|-w} archivo [-v]\n");
        exit(EXIT_FAILURE);
    }
    
    modo=argv[2];
    archivo=argv[3];

    if (argc==5){
        if (strcmp(argv[4],"-v")==0){
            verbose=true;
        }
        else {
            fprintf(stderr, "Uso: ./tftp-client ip_servidor {-r|-w} archivo [-v]\n");
            exit(EXIT_FAILURE);
        }
    }

    // Creo el socket UDP
    if ((desc_socket = socket(AF_INET,SOCK_DGRAM,0))<0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    
    // Configuro la IP, puerto y tipo de direccion del cliente
    addr_client.sin_family = AF_INET;
    addr_client.sin_port = 0; // Poniendo 0 el SO elegirá un puerto automaticamente que este libre
    addr_client.sin_addr.s_addr = INADDR_ANY; 

    // Conecto el socket a la direccion IP y puerto del cliente
    if (bind(desc_socket,(struct sockaddr *) &addr_client, sizeof(addr_client))<0){
        perror("bind()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    // Configuro la IP, puerto y tipo de IP del servidor
    addr_server.sin_family = AF_INET; // Tipo IPv4
    addr_server.sin_port = htons(PORT); // Convierto el numero de puerto a entero de 16 bits
    addr_server.sin_addr.s_addr = direccion_ip.s_addr;



    return 0;
}
