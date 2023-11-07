// Practica Tema 6: García González, Javier
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
    int desc_socket, bytes;
    unsigned short puerto=5; // Puerto al que se conecta el cliente por defecto
    char buffer[80]; // 80 porque es el tamaño maximo que se puede recibir
    const char* cadena;
    struct in_addr direccion_ip;
    struct sockaddr_in addr_server, addr_client;
    socklen_t addr_server_len=sizeof(addr_server);

    // Compruebo que el formato de entrada es el correcto
    if (argc<3 || argc>5 || argc==4){
        // Si el formato no es correcto muestro el error
        fprintf(stderr, "Uso: ./echocon-tcp-client ip_servidor [-p puerto-servidor] cadena\n"); 
        exit(EXIT_FAILURE);
    }

    
    if (argc==3){
        cadena=argv[2]; // Guardo la cadena para usarla posteriormente
    }
    else {
        cadena=argv[4]; // Guardo la cadena para usarla posteriormente
        if (sscanf(argv[3],"%hu", &puerto)!=1) { // Puerto indicado con el parametro -p
            fprintf(stderr, "Puerto invalido\n");
            exit(EXIT_FAILURE);
        }
    }

    // La cadena debe ser de maximo 80 caracteres
    if (strlen(cadena)>80){
        // Si es mayor de 80 caracteres muestro el error y termino la ejecucion
        fprintf(stderr, "La cadena introducida debe tener 80 caracteres como maximo\n");
        exit(EXIT_FAILURE);
    }

    // Convierto la direccion IP introducida
    if (inet_aton(argv[1], &direccion_ip) == 0) {
        fprintf(stderr, "inet_aton(): direccion IP invalida\n");
        exit(EXIT_FAILURE);
    }

    // Creo el socket TCP con direcciones IPv4
    if ((desc_socket = socket(AF_INET,SOCK_STREAM,0))<0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // Configuro la IP, puerto y tipo de direccion del cliente
    addr_client.sin_family = AF_INET;
    addr_client.sin_port = 0; // Poniendo 0 el SO elegirá un puerto automaticamente que este libre
    addr_client.sin_addr.s_addr = INADDR_ANY; 

    // Enlazo el socket a la direccion IP y puerto del cliente
    // Paso el puntero a la estructura sockaddr_in del cliente previamente configurada
    if (bind(desc_socket,(struct sockaddr *) &addr_client, sizeof(addr_client))<0){
        perror("bind()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    // Configuro la IP, puerto y tipo de IP del servidor TCP
    // El puerto es 5 (por defecto) o el introducido por parametros
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(puerto); // Convierto el numero de puerto a entero de 16 bits
    addr_server.sin_addr.s_addr = direccion_ip.s_addr;

    // Conecto el socket TCP del cliente con el socket de conexion TCP del servidor
    // Paso el puntero a la estructura sockaddr_in del servidor previamente configurada
    if (connect(desc_socket,(struct sockaddr *) &addr_server, addr_server_len)<0){
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    // Envio la cadena recibida por parametros al servidor TCP
    if (send(desc_socket, cadena, strlen(cadena), 0)<0){
        perror("send()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    // Recibo el mensaje del servidor TCP en el buffer
    if ((bytes=recv(desc_socket, buffer, strlen(cadena), 0))<0){
        perror("recv()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    buffer[bytes]='\0';
    printf("%s\n", buffer);

    // Notifico del cierre del socket de conexion con el servicor tcp para lectura y escritura
    if (shutdown(desc_socket,SHUT_RDWR)<0){
        perror("shutdown()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    // Hago un recv para asegurarme de que no queda nada por leer
    // Solo hago un if porque la llamada recv es bloqueante
    // Cuando me asguro de que no queda nada por recibir cierro el socket de conexion con el server
    if(recv(desc_socket, buffer, sizeof(buffer), 0) == 0){
        close(desc_socket);
    }

    return 0;
}
