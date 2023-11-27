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
#include "ip-icmp-ping.h"
#include <errno.h>

// for (int i=0; i<(sizeof(datagrama)); i++){
//  acumulador += (unsigned int) *puntero;
//  pun
//} 
//

int main(int argc, char const *argv[]){
    int desc_socket;
    bool verbose=false;
    EchoRequest request;
    EchoReply reply;
    struct servent *servicio; // Variable que almacena la informacion asociada al servicio ICMP
    struct in_addr direccion_ip;
    struct sockaddr_in addr_client, addr_server;
    socklen_t addr_server_len=sizeof(addr_server);


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

    // Formando el paquete ICMP de tipo REQUEST
    request.icmpHdr.type=8;
    request.icmpHdr.code=0;
    request.icmpHdr.checksum=0;
    request.pid=getpid();
    request.sequence=0;
    strcpy(request.payload,"Este es el payload.");

    if (verbose){
        printf("-> Generando cabecera ICMP...\n");
        printf("-> Type: %d\n", request.icmpHdr.type);
        printf("-> Code: %d\n", request.icmpHdr.code);
        printf("-> PID: %d\n", request.pid);
        printf("-> Sequence Number: %d\n", request.sequence);
        printf("-> Cadena a enviar: %s\n", request.payload);
        printf("-> Checksum: %d\n", request.icmpHdr.checksum);
        printf("-> Tamaño total del datagrama: %d\n", sizeof(request));
    }

    // Creo el socket UDP y asigno el descriptor del socket a la varible desc_socket
    if ((desc_socket = socket(AF_INET,SOCK_RAW,0))<0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    
    // Configuro la IP, puerto y tipo de direccion del cliente
    addr_client.sin_family = AF_INET; // Tipo de direccion IPv4
    addr_client.sin_port = 0; // Poniendo 0 el SO elegirá un puerto automaticamente que este libre
    addr_client.sin_addr.s_addr = INADDR_ANY; 

    // Conecto el socket UDP, creado previamente, a la direccion IP y puerto del cliente
    if (bind(desc_socket,(struct sockaddr *) &addr_client, sizeof(addr_client))<0){
        perror("bind()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    if((servicio=getservbyname("echo", "udp"))==NULL){
        perror("getservbyname()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    addr_server.sin_family = AF_INET; // Tipo de direccion IPv4
    addr_server.sin_port = servicio->s_port; // Puerto de echo
    addr_server.sin_addr = direccion_ip; // Direccion IP del servidor

    // Envio el paquete ICMP de tipo REQUEST
    if ((sendto(desc_socket, &request, sizeof(request), 0, (struct sockaddr *) &addr_server, sizeof(addr_server)))<0){
        perror("sendto()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    printf("Mensaje ICMP enviado al host: %s\n", argv[1]);

    if ((recvfrom(desc_socket, &reply, sizeof(reply), 0, (struct sockaddr *) &addr_server, &addr_server_len))<0){
        perror("recvfrom()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    printf("Respuesta recibida desde: %s\n", inet_ntoa(reply.ipHdr.iaSrc));

    if (verbose){
        printf("-> Tamaño de la respuesta\n", sizeof(reply));
        printf("-> Cadena recibida: %s\n", reply.icmpMsg.payload);
        printf("-> PID: %d\n", reply.icmpMsg.pid);
        printf("-> TTL: %d\n", reply.ipHdr.TTL);
    }

    if (reply.icmpMsg.icmpHdr.type==0 && reply.icmpMsg.icmpHdr.code==0){
        printf("Respuesta correcta: (Type %d, Code %d)\n", reply.icmpMsg.icmpHdr.type, reply.icmpMsg.icmpHdr.code);
    }
    else {
        printf("%s (Type %d, Code %d)\n", reply.icmpMsg.payload, reply.icmpMsg.icmpHdr.type, reply.icmpMsg.icmpHdr.code);
    }
}