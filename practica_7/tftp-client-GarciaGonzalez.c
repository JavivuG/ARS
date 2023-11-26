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

// Funcion que envia una solicitud al servidor TFTP
void enviaPeticion(bool modo, const char* archivo, int desc_socket, struct sockaddr_in addr_server);
// Funcion que comprueba que los bloques se reciben en el orden correcto
void comprobarOrdenPaquetes(char* ack, int *numeroBloque,int contadorBloques, FILE* fichero, int desc_socket);
// Funcion que comprueba si el datagrama recibido es un datagrama ERROR
void comprobarError(char *datagrama, int desc_socket);


int main(int argc, char const *argv[]){
    int desc_socket, bytes, tamanyoData, numeroBloque, contadorBloques, bytes_leidos;
    bool verbose=false; // Variable que indica si se ha introducido el modo detallado de ejecucion
    char data[516]; // Tipo de datos que representa un datagrama Data
    char ack[100]; // Tipo de datos que representa un datagrama ACK, establezco tamanyo 100 por si se recibe un datagrama ERROR
    struct in_addr direccion_ip;
    struct sockaddr_in addr_server, addr_client;
    struct servent *servicio; // Variable que almacena la informacion asociada al servicio TFTP
    socklen_t addr_server_len=sizeof(addr_server);
    FILE *fichero; // Fichero en el que se guardaran los datos recibidos desde el servidor TFTP o que se enviaran al servidor TFTP

    // Compruebo que el formato de entrada es el correcto
    if (argc>5 || argc<4){
        fprintf(stderr, "Uso: ./tftp-client ip_servidor {-r|-w} archivo [-v]\n");
        exit(EXIT_FAILURE);
    }

    // Convierto la direccion IP introducida por parametros a entero de 32 bits
    if (inet_aton(argv[1], &direccion_ip) == 0) {
        fprintf(stderr, "inet_aton(): direccion IP invalida\n");
        exit(EXIT_FAILURE);
    }

    // Compruebo que el parametro introducido como modo de operacion del cliente es el correcto
    // No sera correcto si es distinto de "-r"(Modo lectura) o "-w" (Modo escritura)
    if ((strcmp(argv[2],"-r")!=0) && (strcmp(argv[2],"-w")!=0)){
        fprintf(stderr, "Uso: ./tftp-client ip_servidor {-r|-w} archivo [-v]\n");
        exit(EXIT_FAILURE);
    }

    // Si tiene 5 parametros entonces es posible que se haya especificado el modo verbose o detallado
    if (argc==5){
        // Compruebo si se ha introducido el modo detallado de ejecucion
        // Modo con comentarios a medida que se ejecuta
        if (strcmp(argv[4],"-v")==0){
            verbose=true; // Activo el modo detallado de ejecucion
        }
        else { // Si no era -v entonces error porque es el modo de ejecucion especificado
            fprintf(stderr, "Uso: ./tftp-client ip_servidor {-r|-w} archivo [-v]\n");
            exit(EXIT_FAILURE);
        }
    }


    // Creo el socket UDP y asigno el descriptor del socket a la varible desc_socket
    if ((desc_socket = socket(AF_INET,SOCK_DGRAM,0))<0){
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

    // Obtengo la informacion asociada al servicio TFTP 
    // Esto se realiza para obtener posteriormente el puerto que se usa normalmente en TFTP
    if ((servicio=getservbyname("tftp","udp"))==NULL){
        fprintf(stderr, "getserbyname(): No se pudo encontrar el servicio");
        close(desc_socket);
        exit(EXIT_FAILURE);
    }

    // Configuro la IP, puerto y tipo de IP del servidor
    // Puerto es normalmente con TFTP el 69
    addr_server.sin_family = AF_INET; // Tipo IPv4
    addr_server.sin_port = servicio->s_port; // Asigno el puerto que recibi por el servicio TFTP a la direccion del servicor
    addr_server.sin_addr.s_addr = direccion_ip.s_addr; // La que recibo por parametros en la ejecucion


    // Inicializo las cabeceras para el datagrama Data
    data[0]=0;
    data[1]=3;

    // Inicializo las cabeceras para el datagrama ACK
    ack[0]=0;
    ack[1]=4;


    // Compruebo si el modo introducido por teclado es el de LECTURA o ESCRITURA
    if (strcmp(argv[2],"-r")==0){ // MODO LECTURA

        // Envio peticion de lectura al servidor TFTP
        // Con true indico que el modo de operacion es lectura
        // argv[3] es el archivo que quiero leer
        enviaPeticion(true,argv[3], desc_socket,addr_server);
        
        // Muestro por pantalla que he enviado la solicitud de lectura al servidor TFTP (Modo detallado)
        if (verbose){
            printf("Enviada solicitud de lectura de \"%s\" a servidor tftp (%s)\n", argv[3], argv[1]);
        }

        // Abro el fichero en el que voy a recoger los bytes del archivo que se va a leer del servidor TFTP
        // Abro en modo escritura
        if ((fichero=fopen(argv[3],"w"))==NULL){
            perror("fopen()");
            close(desc_socket);
            exit(EXIT_FAILURE);
        }

        // Contador para comprobar que los numeros de bloque que se reciben son los correctos
        // Comienza en 1 porque el primer datagrama que se reciba del servidor TFTP debera tener numero de bloque 1
        contadorBloques=1;


        do{
            // Recibo el mensaje del servidor TFTP en el array Data
            // La variable bytes indica los bytes recibidos desde el servidor
            if ((bytes=recvfrom(desc_socket, data, sizeof(data), 0, (struct sockaddr *) &addr_server, &addr_server_len))<0){
                perror("recvfrom()");
                fclose(fichero);
                close(desc_socket);
                exit(EXIT_FAILURE);
            }
            
            // Muestro por pantalla que he recibido un bloque desde el servidor (Modo detallado)
            if (verbose){
                printf("Recibido bloque del servidor tftp\n");
            }

            // Extraigo el tamaño del campo data recibido desde el servidor
            // Resto 4 porque 2 bytes son del campo opcode y otros 2 bytes del numero de bloque
            tamanyoData=bytes-4;

            // Compruebo que el numero de bloque recibido es el esperado
            comprobarOrdenPaquetes(data, &numeroBloque, contadorBloques, fichero, desc_socket);

            // Una vez comprobado que el orden es el correcto
            // Incremento el numero de bloque esperado para siguientes datagramas
            contadorBloques++;

            // Compruebo si es un datagrama ERROR
            comprobarError(data, desc_socket);

            // Muestro el numero de bloque recibido (Modo detallado de ejecucion)
            if (verbose){
                if (numeroBloque==1){
                    printf("Es el primer bloque (numero de bloque %d)\n", numeroBloque);
                }
                else {
                    printf("Es el bloque numero %d\n", numeroBloque);
                }
            }

            // Escribo el campo datos del datagrama Data recibido desde el servidor TFTP en el fichero local
            fwrite(&data[4], 1, tamanyoData, fichero);

            // Formo el ACK para mandar al servidor
            // Incluyo el numero de bloque que extraigo del datagrama Data recibido del servidor TFTP
            ack[2]=data[2];
            ack[3]=data[3];

            // Respondo al servidor TFTP mandandole un ACK para hacerle saber que he recibido el datagrama 
            if (sendto(desc_socket, ack, sizeof(ack), 0, (struct sockaddr *) &addr_server, addr_server_len)<0){
                perror("sendto()");
                fclose(fichero);
                close(desc_socket);
                exit(EXIT_FAILURE);
            }
            
            // Muestro por pantalla que he enviado el ACK correspondiente al bloque recibido (Modo detallado)
            if (verbose){
                printf("Enviamos el ACK del bloque %d\n", numeroBloque);
            }

        }while(tamanyoData==512); // Si el tamaño del campo datos del datagrama Data es 512 bytes continuo recibiendo paquetes del servidor TFTP

        // Muestro por pantalla que el bloque recibido era el ultimo (Modo detallado)
        if (verbose){
            printf("El bloque %d era el ultimo: cerramos el fichero\n", numeroBloque);
        }

        // Cierro el fichero local debido a que ya se ha formado con los datos recibidos desde el servidor TFTP
        fclose(fichero); 

        // Cierro el socket UDP
        close(desc_socket);

        // Se finaliza la ejecucion despues de cerrar el socket
    }
    else if (strcmp(argv[2],"-w")==0){ // MODO ESCRITURA
    
        // Inicializo el contador de bloques a 0 porque el ack que recibe del servidor TFTP es con numero de bloque 0
        contadorBloques=0;

        // Envio peticion de escritura al servidor TFTP
        // Con false indico que el modo de operacion es escritura
        enviaPeticion(false,argv[3], desc_socket,addr_server);

        // Muestro por pantalla que he enviado la solicitud de escritura al servidor TFTP (Modo detallado)
        if (verbose){
            printf("Enviada solicitud de escritura de \"%s\" a servidor tftp (%s)\n", argv[3], argv[1]);
        }

        // Abro el fichero que voy a enviar al servidor TFTP
        if ((fichero=fopen(argv[3],"r"))==NULL){
            perror("fopen()");
            close(desc_socket);
            exit(EXIT_FAILURE);
        }

        do{
            // Recibo el ACK del servidor TFTP
            if ((bytes=recvfrom(desc_socket, ack, sizeof(ack), 0, (struct sockaddr *) &addr_server, &addr_server_len))<0){
                perror("recvfrom()");
                fclose(fichero);
                close(desc_socket);
                exit(EXIT_FAILURE);
            }

            // Compruebo que el numero de bloque recibido es el esperado
            comprobarOrdenPaquetes(ack, &numeroBloque, contadorBloques, fichero, desc_socket);

            // Compruebo si es un datagrama ERROR
            comprobarError(ack, desc_socket);

            // Muestro por pantalla que he recibido el ACK del servidor TFTP (Modo detallado)
            if (verbose){
                if (numeroBloque==0){
                    printf("Recibido primer ACK\n");
                }
                else {
                    printf("Recibido ACK del bloque %d del servidor tftp\n", numeroBloque);
                }
            }

            // NumeroBloque variable con el numero de bloque que se va a enviar en el datagrama Data al servidor
            numeroBloque++;

            // contadorBloques variable con el numero de bloque que se espera recibir en el ACK del servidor
            contadorBloques++;

            // Formo el datagrama Data que voy a enviar al servidor TFTP
            // Incluyo el numero de bloque que se va a enviar en el datagrama Data
            data[2]=numeroBloque / 256;
            data[3]=numeroBloque % 256;

            // Leo los datos del fichero local y los guardo en el campo datos del datagrama Data
            bytes_leidos = fread(&data[4],1,sizeof(data)-4,fichero);
            
            // Envio el datagrama Data al servidor TFTP
            // Tamaño del datagrama Data es el numero de bytes leidos del fichero local + 4 bytes de las cabeceras
            if (sendto(desc_socket, data, bytes_leidos+4, 0, (struct sockaddr *) &addr_server, addr_server_len)<0){
                perror("sendto()");
                fclose(fichero);
                close(desc_socket);
                exit(EXIT_FAILURE);
            }

            // Muestro por pantalla que he enviado el datagrama Data al servidor TFTP (Modo detallado)
            if (verbose){
                printf("Enviamos el datagrama Data con numero de bloque %d\n", numeroBloque);
            }
            
        }while(bytes_leidos==512); // Si el numero de bytes leidos del fichero local es 512 continuo enviando datagramas Data al servidor TFTP

        // Muestro por pantalla que el datagrama Data enviado era el ultimo (Modo detallado)
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

        // Compruebo que el numero de bloque recibido es el esperado
        comprobarOrdenPaquetes(ack, &numeroBloque, contadorBloques, fichero, desc_socket);

        // Compruebo si es un datagrama ERROR
        comprobarError(ack, desc_socket);

        // Muetro por pantalla que he recibido el ultimo ACK del servidor TFTP (Modo detallado)
        if (verbose){
            printf("Recibido ACK del bloque %d del servidor tftp\n", numeroBloque);
        }

        // Cierro el fichero local
        fclose(fichero);

        // Cierro el socket UDP
        close(desc_socket);
        
        // Finalizo la ejecucion despues de cerrar el socket
    } 

    exit(EXIT_SUCCESS);
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

// Funcion que comprueba que los datagramas se reciben en el orden correcto
// Se usa tanto para comprobar el orden de los datagramas Data como de los ACK
void comprobarOrdenPaquetes(char* datagrama, int *numeroBloque, int contadorBloques, FILE* fichero, int desc_socket){
    // Extraigo el numero de bloque del datagrama ACK
    *numeroBloque = (unsigned char)datagrama[2] * 256 + (unsigned char)datagrama[3];

    // Compruebo que el numero de bloque del ACK es el esperado
    if ((*numeroBloque)!=contadorBloques){
        fprintf(stderr, "No se han recibido los datagramas en el orden correcto\n");
        fclose(fichero);
        close(desc_socket);
        exit(EXIT_FAILURE);
    }
}

// Funcion que comprueba si el datagrama recibido es un datagrama ERROR
void comprobarError(char *datagrama, int desc_socket){
    // Compruebo si es un datagrama ERROR
    // Si es un datagrama de error muestro sus caracteristicas y cierro socket y el fichero
    if (datagrama[1]==5){
        fprintf(stderr, "Codigo de error: %d\n", datagrama[3]);
        fprintf(stderr,"Descripcion del error: %s\n", &datagrama[4]);
        close(desc_socket);
        exit(EXIT_FAILURE);
    }
}