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
//IMprimir mensajes de error de despcriotn

void mensajeEnvioVerbose(EchoRequest request);
void mensajeRecepcionVerbose(EchoReply reply);
void imprimirMensajesError(EchoReply reply);

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
        mensajeEnvioVerbose(request);
    }

    // Creo el socket UDP y asigno el descriptor del socket a la varible desc_socket
    if ((desc_socket = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP))<0){
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

    if((servicio=getservbyname("echo", "tcp"))==NULL){
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
        mensajeRecepcionVerbose(reply);
    }

    if (reply.icmpMsg.icmpHdr.type==0){
        printf("Respuesta correcta: (Type %u, Code %u)\n", reply.icmpMsg.icmpHdr.type, reply.icmpMsg.icmpHdr.code);
    }
    else {
        imprimirMensajesError(reply);
    }

    exit(EXIT_SUCCESS);
}

void mensajeEnvioVerbose(EchoRequest request){
    printf("-> Generando cabecera ICMP...\n");
    printf("-> Type: %u\n", request.icmpHdr.type);
    printf("-> Code: %u\n", request.icmpHdr.code);
    printf("-> PID: %u\n", request.pid);
    printf("-> Sequence Number: %u\n", request.sequence);
    printf("-> Cadena a enviar: %s\n", request.payload);
    printf("-> Checksum: 0x%x\n", request.icmpHdr.checksum);
    printf("-> Tamaño total del datagrama: %lu\n", sizeof(request));
}

void mensajeRecepcionVerbose(EchoReply reply){
    printf("-> Tamaño de la respuesta: %lu\n", sizeof(reply));
    printf("-> Cadena recibida: %s\n", reply.icmpMsg.payload);
    printf("-> PID: %u\n", reply.icmpMsg.pid);
    printf("-> TTL: %u\n", reply.ipHdr.TTL);
}

void imprimirMensajesError(EchoReply reply){
    unsigned char tipo=reply.icmpMsg.icmpHdr.type;
    unsigned char codigo=reply.icmpMsg.icmpHdr.code;

    switch (tipo){
        case 3:
            printf("Destination Unreachable: ");
            switch (codigo){
                case 0:
                    printf("Net Unreachable");
                    break;
                case 1:
                    printf("Host Unreachable");
                    break;
                case 2:
                    printf("Protocol Unreachable");
                    break;
                case 3:
                    printf("Port Unreachable");
                    break;
                case 4:
                    printf("Fragmentation Needed");
                    break;
                case 5:
                    printf("Source Route Failed");
                    break;
                case 6:
                    printf("Destination Network Unknown");
                    break;
                case 7:
                    printf("Destination Host Unknown");
                    break;
                case 8:
                    printf("Source Host Isolated");
                    break;
                case 11:
                    printf("Destination Network Unreachable for Type of Service");
                    break;
                case 12:
                    printf("Destination Host Unreachable for Type of Service");
                    break;
                case 13:
                    printf("Communication Administratively Prohibited");
                    break;  
                case 14:
                    printf("Host Precedence Violation");
                    break; 
                case 15:
                    printf("Precedence Cutoff in Effect");
                    break;         
            }
            break;
        case 5:
            printf("Redirect: ");
            switch (codigo){
                case 1:
                    printf("Redirect for Destination Host");
                    break;
                case 3:
                    printf("Redirect for Destination Host Based on Type-of-Service");
                    break;
            }
            break;
        case 11:
            printf("Time exceeded: ");
            switch (codigo){
                case 0:
                    printf("Time-to-Live Exceeded in Transit");
                    break;
                case 1:
                    printf("Fragment Reassembly Time Exceeded");
                    break;
            }
            break;
        case 12:
            printf("Parameter problem: ");
            switch (codigo){
                case 0:
                    printf("Pointer indicates the error");
                    break;
                case 1:
                    printf("Missing a Required Option");
                    break;
                case 2:
                    printf("Bad Length");
                    break;
            }
            break;
    }

    printf(" (Type %u, Code %u)\n", tipo, codigo);
}
