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
#include "ip-icmp-ping.h"
#include <errno.h>

// Funcion que imprime los datos del paquete ICMP enviado
void mensajeEnvioVerbose(EchoRequest request);
// Funcion que imprime datos de la respuesta ICMP
void mensajeRecepcionVerbose(EchoReply reply);
// Funcion que imprime los mensajes de error de la respuesta ICMP
void imprimirMensajesError(EchoReply reply);
// Funcion que calcula el checksum de un datagrama ICMP
unsigned short int calcularChecksum(unsigned short int *datagrama, int tam);

int main(int argc, char const *argv[]){
    int desc_socket;
    bool verbose=false;
    EchoRequest request;
    EchoReply reply;
    struct in_addr direccion_ip;
    struct sockaddr_in addr_client, addr_server;
    socklen_t addr_server_len=sizeof(addr_server);


    // Compruebo que el formato de entrada es el correcto
    if (argc>3 || argc<2){
        fprintf(stderr, "Uso: ./miping ip_servidor [-v]\n");
        exit(EXIT_FAILURE);
    }

    // Convierto la direccion IP introducida por parametros a un entero de 32 bits
    if (inet_aton(argv[1], &direccion_ip) == 0) {
        fprintf(stderr, "inet_aton(): direccion IP invalida\n");
        exit(EXIT_FAILURE);
    }

    // Si tiene 3 parametros entonces es posible que se haya especificado el modo verbose o detallado
    if (argc==3){
        // Compruebo si se ha introducido el modo detallado de ejecucion
        // Modo con comentarios a medida que se ejecuta
        if (strcmp(argv[2],"-v")==0){
            verbose=true; // Activo el modo detallado de ejecucion
        }
        else { // Si no era -v entonces error porque es el modo de ejecucion especificado
            fprintf(stderr, "Uso: ./miping ip_servidor [-v]\n");
            exit(EXIT_FAILURE);
        }
    }

    // Formando el paquete ICMP de tipo REQUEST
    request.icmpHdr.type=8; // Tipo 8 para REQUEST
    request.icmpHdr.code=0; // Codigo del mensaje
    request.icmpHdr.checksum=0; // Incializo el checksum a 0
    request.pid=getpid(); // Identificador del proceso que realiza el ping
    request.sequence=0; // Numero de secuencia del paquete ICMP
    strcpy(request.payload,"Este es el payload.");
    request.icmpHdr.checksum=calcularChecksum((unsigned short int *) &request, sizeof(request)); // Calculo el cheksum


    // Si el modo de ejecucion es detallado imprimo los datos del paquete ICMP
    if (verbose){
        mensajeEnvioVerbose(request);
    }

    // Creo el socket IP con el protocolo IPPROTO_ICMP
    // Uso sockets RAW para acceder directamente a la capa IP
    // Asigno el descriptor del socket a la varible desc_socket
    if ((desc_socket = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP))<0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    
    // Configuro la IP, puerto y tipo de direccion del cliente
    addr_client.sin_family = AF_INET; // Tipo de direccion IPv4
    addr_client.sin_port = 0; // Poniendo 0 el SO elegirá un puerto automaticamente que este libre
    addr_client.sin_addr.s_addr = INADDR_ANY; 

    // Conecto el socket IP, creado previamente, a la direccion IP y puerto del cliente
    if (bind(desc_socket,(struct sockaddr *) &addr_client, sizeof(addr_client))<0){
        perror("bind()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    // Configuro la IP, puerto y tipo de IP del servidor
    addr_server.sin_family = AF_INET; // Tipo de direccion IPv4
    addr_server.sin_port = 0; // Puerto del servicio
    addr_server.sin_addr = direccion_ip; // Direccion IP del servidor

    // Envio el datagrama ICMP de tipo REQUEST
    // La función sendto() añade la cabecera IP al datagrama ICMP
    if ((sendto(desc_socket, &request, sizeof(request), 0, (struct sockaddr *) &addr_server, sizeof(addr_server)))<0){
        perror("sendto()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    // Imprimo que se ha enviado el mensaje ICMP
    printf("Mensaje ICMP enviado al host: %s\n", argv[1]);

    // Recibo la respuesta del servidor
    // La respuesta contiene la cabecera IP y el datagrama ICMP
    if ((recvfrom(desc_socket, &reply, sizeof(reply), 0, (struct sockaddr *) &addr_server, &addr_server_len))<0){
        perror("recvfrom()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    // Imprimo que se ha recibido la respuesta del servidor
    printf("Respuesta recibida desde: %s\n", inet_ntoa(reply.ipHdr.iaSrc));

    // Si el modo de ejecucion es detallado imprimo los datos de la respuesta ICMP
    if (verbose){
        mensajeRecepcionVerbose(reply);
    }

    // Compruebo si la respuesta es correcta
    // Correcta si tipo de mensaje ICMP es 0
    if (reply.icmpMsg.icmpHdr.type==0){
        printf("Respuesta correcta: (Type %u, Code %u)\n", reply.icmpMsg.icmpHdr.type, reply.icmpMsg.icmpHdr.code);
    }
    else {
        // Si no es correcta imprimo los mensajes de error
        imprimirMensajesError(reply);
    }

    exit(EXIT_SUCCESS);
}

// Funcion que imprime los datos del paquete ICMP enviado
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

// Funcion que imprime datos de la respuesta ICMP
void mensajeRecepcionVerbose(EchoReply reply){
    printf("-> Tamaño de la respuesta: %lu\n", sizeof(reply));
    printf("-> Cadena recibida: %s\n", reply.icmpMsg.payload);
    printf("-> PID: %u\n", reply.icmpMsg.pid);
    printf("-> TTL: %u\n", reply.ipHdr.TTL);
}

// Funcion que imprime los mensajes de error de la respuesta ICMP
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

// Funcion que calcula el checksum de un datagrama
// Devuelve el checksum calculado (entero de 16 bits sin signo)
unsigned short int calcularChecksum(unsigned short int *datagrama, int tam){
    unsigned short int* puntero=datagrama;
    unsigned int acum=0;
    int i;

    for (i=0; i<(tam/2); i++){
        acum += (unsigned int) (*puntero);
        puntero++;
    }

    // Se realiza dos veces la operacion para asegurarse de que el acarreo se suma
    acum = (acum >> 16) + (acum & 0x0000ffff);
    acum = (acum >> 16) + (acum & 0x0000ffff);

    // Obtengo el complemento a uno
    acum = ~acum;

    return (unsigned short int) acum;

}
