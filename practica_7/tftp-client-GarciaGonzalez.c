// Practica Tema 7: García González, Javier
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

void enviaPeticion(bool modo, const char* archivo, int desc_socket, struct sockaddr_in addr_server);


int main(int argc, char const *argv[]){
    int desc_socket, bytes, tamanyoData, numeroBloque, contadorBloques, bytes_leidos;
    bool verbose=false;
    char data[516], ack[4], error[128];
    struct in_addr direccion_ip;
    struct sockaddr_in addr_server, addr_client;
    struct servent *servicio;
    socklen_t addr_server_len=sizeof(addr_server);
    FILE *fichero;

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

    if (argc==5){
        // Compruebo si se ha introducido el modo detallado de ejecucion
        // Modo con comentarios a medida que se ejecuta
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

    if ((servicio=getservbyname("tftp","udp"))==NULL){
        fprintf(stderr, "getserbyname(): No se pudo encontrar el servicio");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    // Configuro la IP, puerto y tipo de IP del servidor
    addr_server.sin_family = AF_INET; // Tipo IPv4
    addr_server.sin_port = servicio->s_port; // Convierto el numero de puerto a entero de 16 bits
    addr_server.sin_addr.s_addr = direccion_ip.s_addr;


    // Inicializo las cabeceras para los tipos de mensaje
    // Para el datagrama Data
    data[0]=0;
    data[1]=3;

    // Para el datagrama ACK
    ack[0]=0;
    ack[1]=4;


    // Compruebo si el modo introducido por teclado es el de LECTURA o ESCRITURA
    if (strcmp(argv[2],"-r")==0){ // MODO LECTURA

        enviaPeticion(true,argv[3], desc_socket,addr_server);
        
        if (verbose){
            printf("Enviada solicitud de lectura de \"%s\" a servidor tftp (%s)\n", argv[3], argv[1]);
        }

        // Abro el fichero en el que voy a recoger los bytes del archivo del servidor TFTP
        if ((fichero=fopen(argv[3],"w"))==NULL){
            perror("fopen()");
            close(desc_socket);
            exit(EXIT_FAILURE);
        }

        contadorBloques=1;

        do{
            // Recibo el mensaje del servidor en el buffer
            // La variable bytes indica los bytes recibidos desde el servidor
            if ((bytes=recvfrom(desc_socket, data, sizeof(data), 0, (struct sockaddr *) &addr_server, &addr_server_len))<0){
                perror("recvfrom()");
                fclose(fichero);
                close(desc_socket);
                exit(EXIT_FAILURE);
            }
            
            if (verbose){
                printf("Recibido bloque del servidor tftp\n");
            }

            // Extraigo el tamaño del campo data recibido desde el servidor
            // Resto 4 porque 2 bytes son del campo opcode y otros 2 bytes del numero de bloque
            tamanyoData=bytes-4;

            // Extraigo el numero de bloque del datagrama Data
            numeroBloque = (unsigned char)data[2] * 256 + (unsigned char)data[3];

            // Si el numero de bloque recibido es distinto al esperado entonces se han recibido en orden incorrecto los datagramas
            if (numeroBloque!=contadorBloques){
                fprintf(stderr, "No se han recibido los datagramas en el orden correcto\n");
                fclose(fichero);
                close(desc_socket);
                exit(EXIT_FAILURE);
            }

            // Una vez comprobado que el orden es el correcto
            // Incremento el numero de bloque esperado para siguientes datagramas
            contadorBloques++;

            // Compruebo si lo que se ha recibido es un datagrama Error
            // Si es un datagrama de error muestro sus caracteristicas y cierro socket y el fichero
            if (data[1]==5){
                fprintf(stderr, "Codigo de error: %d\n", data[3]);

                
                fprintf(stderr,"Descripcion del error: %s\n", &data[4]);
                fclose(fichero);
                close(desc_socket);
                exit(EXIT_FAILURE);
            }

            if (verbose){
                if (numeroBloque==1){
                    printf("Es el primer bloque (numero de bloque %d)\n", numeroBloque);
                }
                else {
                    printf("Es el bloque numero %d\n", numeroBloque);
                }
            }

            // Escribo el campo datos del datagrama Data recibido desde el servidor TFTP en el fichero local
            fwrite(&data[4], 1, tamanyoData,fichero);

            // Formo el ACK para mandar al servidor
            // Codigo de operacion: 04
            // Incluyo el numero de bloque que extraigo del datagrama Data recibido del servidor
            ack[2]=data[2];
            ack[3]=data[3];

            // Mando el datagrama ACK al servidor TFTP
            if (sendto(desc_socket, ack, sizeof(ack), 0, (struct sockaddr *) &addr_server, addr_server_len)<0){
                perror("sendto()");
                fclose(fichero);
                close(desc_socket);
                exit(EXIT_FAILURE);
            }
            
            if (verbose){
                printf("Enviamos el ACK del bloque %d\n", numeroBloque);
            }

        }while(tamanyoData==512); // Si el tamaño del campo datos del datagrama Data es 512 bytes continuo recibiendo paquetes del servidor TFTP

        if (verbose){
            printf("El bloque %d era el ultimo: cerramos el fichero\n", numeroBloque);
        }

        // Cierro el fichero local debido a que ya se ha formado con los datos recibidos desde el servidor TFTP
        fclose(fichero); 

        // Cierro el descriptor del socket UDP
        close(desc_socket);

    }
    else if (strcmp(argv[2],"-w")==0){ // MODO ESCRITURA

        contadorBloques=0;

        enviaPeticion(false,argv[3], desc_socket,addr_server);

        if (verbose){
            printf("Enviada solicitud de escritura de \"%s\" a servidor tftp (%s)\n", argv[3], argv[1]);
        }

        if ((fichero=fopen(argv[3],"r"))==NULL){
            perror("fopen()");
            close(desc_socket);
            exit(EXIT_FAILURE);
        }

        do{
            // Recibo el mensaje del servidor en el buffer
            // La variable bytes indica los bytes recibidos desde el servidor
            if ((bytes=recvfrom(desc_socket, ack, sizeof(ack), 0, (struct sockaddr *) &addr_server, &addr_server_len))<0){
                perror("recvfrom()");
                fclose(fichero);
                close(desc_socket);
                exit(EXIT_FAILURE);
            }

            numeroBloque = (unsigned char)ack[2] * 256 + (unsigned char)ack[3];

            if (contadorBloques!=numeroBloque){
                fprintf(stderr, "No se han recibido los datagramas en el orden correcto\n");
                fclose(fichero);
                close(desc_socket);
                exit(EXIT_FAILURE);
            }
            
            numeroBloque++;
            contadorBloques++;

            if (verbose){
                printf("Recibido ACK del bloque %d del servidor tftp\n", numeroBloque);
            }

            data[2]=numeroBloque / 256;
            data[3]=numeroBloque % 256;

            bytes_leidos = fread(&data[4],1,sizeof(data)-4,fichero);
            
            if (sendto(desc_socket, data, bytes_leidos+4, 0, (struct sockaddr *) &addr_server, addr_server_len)<0){
                perror("sendto()");
                fclose(fichero);
                close(desc_socket);
                exit(EXIT_FAILURE);
            }

            if (verbose){
                printf("Enviamos el datagrama Data con numero de bloque %d\n", numeroBloque);
            }
            
        }while(bytes_leidos==512);

        if (verbose){
            printf("El datagrama Data con numero de bloque %d era el ultimo\n", numeroBloque);
        }

        // Recibo el ultimo ACK del servidor
        // La variable bytes indica los bytes recibidos desde el servidor
        if ((bytes=recvfrom(desc_socket, ack, sizeof(ack), 0, (struct sockaddr *) &addr_server, &addr_server_len))<0){
            perror("recvfrom()");
            fclose(fichero);
            close(desc_socket);
            exit(EXIT_FAILURE);
        }

        numeroBloque = (unsigned char)ack[2] * 256 + (unsigned char)ack[3];

        if (verbose){
            printf("Recibido ACK del bloque %d del servidor tftp\n", numeroBloque);
        }

        if (contadorBloques!=numeroBloque){
            fprintf(stderr, "No se han recibido los datagramas en el orden correcto\n");
            fclose(fichero);
            close(desc_socket);
            exit(EXIT_FAILURE);
        }

        fclose(fichero);
        close(desc_socket);
    }

    return 0;
}

// Funcion que envia peticion al servidor
// Si modo==true entonces MODO LECTURA
// Si modo==false entonces MODO ESCRITURA
void enviaPeticion(bool modo, const char* archivo, int desc_socket, struct sockaddr_in addr_server){
    char request[128];
    
    request[0] = 0;
    request[1] = modo ? 1 : 2;
    strcpy(&request[2], archivo);                       // Ponemos ahora el nombre del fichero
    strcpy(&request[2 + strlen(archivo) + 1], "octet"); // Y por ultimo ponemos el modo


    if (sendto(desc_socket, request, sizeof(request), 0, (struct sockaddr *) &addr_server, sizeof(addr_server))<0){
        perror("sendto()");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }
}
