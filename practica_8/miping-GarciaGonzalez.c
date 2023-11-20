// Practica Tema 8: García González, Javier
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
#include <netdb.h>
#include <errno.h>


int main(int argc, char const *argv[]){
    bool verbose=false;
    struct in_addr direccion_ip;

    // Compruebo que el formato de entrada es el correcto
    if (argc>3 || argc<2){
        fprintf(stderr, "Uso: ./miping ip_servidor [-v]\n");
        exit(EXIT_FAILURE);
    }

    // Convierto la direccion IP introducida a entero de 32 bits
    if (inet_aton(argv[1], &direccion_ip) == 0) {
        fprintf(stderr, "inet_aton(): direccion IP invalida\n");
        exit(EXIT_FAILURE);
    }

    if (argc==3){
        // Compruebo si se ha introducido el modo detallado de ejecucion
        // Modo con comentarios a medida que se ejecuta
        if (strcmp(argv[2],"-v")==0){
            verbose=true;
        }
        else {
            fprintf(stderr, "Uso: ./miping ip_servidor [-v]\n");
            exit(EXIT_FAILURE);
        }
    }
}