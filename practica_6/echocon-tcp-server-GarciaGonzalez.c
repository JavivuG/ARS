// Practica Tema 6: García González, Javier
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/ip.h>
#include <errno.h>

// Variable global
int socket_ini; // Descriptor del socket principal

void conversionCadena(char *cadena); // Funcion para convertir la cadena de mayuscula a miniscula y viceversa
void signal_handler(int signum); // Funcion que se ejecuta al recibir la señal Ctrl+C

int main(int argc, char const *argv[]){
    int socket_cliente, bytes, pid;
    unsigned short puerto=5; // Puerto en el que se ejecuta la aplicacion por defecto
    char buffer[80]; // 80 porque es el tamaño maximo permitido que se puede recibir
    struct sockaddr_in addr_server, addr_cliente;
    socklen_t addr_cliente_len=sizeof(addr_cliente);

    signal(SIGINT, signal_handler);

    // Compruebo que el formato de entrada es el correcto
    if (argc>3 || argc==2){
        fprintf(stderr, "Uso: ./echocon-udp-server [-p puerto-servidor]\n");
        exit(EXIT_FAILURE);
    }

    // Si se ha indicado el puerto por parametros
    if (argc==3){
        if (sscanf(argv[2],"%hu", &puerto)!=1) { // Puerto indicado con el parametro -p
            fprintf(stderr, "puerto invalido");
            exit(EXIT_FAILURE);
        }
    }

    // Creo el socket TCP
    if ((socket_ini = socket(AF_INET,SOCK_STREAM,0))<0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // Configuro la direccion IP, puertos y tipo de direccion del servidor
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(puerto);
    addr_server.sin_addr.s_addr = INADDR_ANY;

    // Bindeo el socket con la direccion IP y puerto
    if (bind(socket_ini, (struct sockaddr *) &addr_server, sizeof(addr_server))<0){
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    // Cambio socket a modo apertura pasiva (preparado para recibir conexiones)
    // Tamaño de cola solicitudes: 15
    if (listen(socket_ini, 15)<0){
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    printf("--- Servidor iniciado ---\n\n");

    while(true){

        // Acepto la conexion de la cola de solicitudes
        if ((socket_cliente=accept(socket_ini, (struct sockaddr *) &addr_cliente, &addr_cliente_len))<0){
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        // Creo un proceso hijo
        if ((pid=fork())==0){ // Proceso hijo
            
            // Recibo el mensaje del servidor en el buffer
            if ((bytes=recv(socket_cliente, buffer, sizeof(buffer), 0))<0){
                perror("recv()");
                close(socket_cliente);
                exit(EXIT_FAILURE);
            }

            buffer[bytes]='\0';
            conversionCadena(buffer); // Convierto la cadena

            // Devuelvo la cadena modificada al cliente
            if (send(socket_cliente, buffer, bytes, 0)<0){
                perror("send()");
                close(socket_cliente);
                exit(EXIT_FAILURE);
            } 

            // Notifico del cierre de la conexion con el cliente
            // No admito más envios ni lecturas
            if (shutdown(socket_cliente,SHUT_RDWR)<0){
                perror("shutdown()");
                close(socket_cliente);
                exit(EXIT_FAILURE);
            }

            close(socket_cliente);

            exit(0);
        }
        
    } // Fin while

    return 0;
}

// Funcion que cambia las mayusculas a minusculas en una cadena y viceversa
void conversionCadena(char* cadena){
    int caracter, i;
    
    // Convertir la cadena copiada a mayúsculas
    // Recorro la cadena
    for (i = 0; i < strlen(cadena); i++) {
        // Uso las funciones toupper y tolower ya definidas para convertir a mayuscula y/o minuscula
        caracter=toupper(cadena[i]);

        if (cadena[i]==caracter){ // El caracter ya era mayuscula, por tanto, lo hago minuscula
            cadena[i] = tolower(cadena[i]);
        }
        else {
            cadena[i]=caracter;
        }
    }
    
}

void signal_handler(int signum){
    if (signum==SIGINT){ // Compruebo que la señal capturada es la correspondiente a teclear Ctrl+C
        if (shutdown(socket_ini,SHUT_RDWR)<0){
            perror("shutdown()");
            close(socket_ini);
            exit(EXIT_FAILURE);
        }

        close(socket_ini);

        exit(0);
    }
    
}