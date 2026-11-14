# ARS — Arquitectura de Redes y Servicios

Clientes y servidores de red escritos en C sobre la API de sockets de POSIX: eco UDP y TCP, un cliente TFTP y una implementación propia de `ping` con sockets RAW.

> Asignatura **Arquitectura de Redes y Servicios** · 3.º curso, 1.er cuatrimestre · Curso 2023-2024 · Grado en Ingeniería Informática, Universidad de Valladolid.

## Qué hace

| Práctica | Programa | Descripción |
|----------|----------|-------------|
| [Tema 5](practica_5/) | `echocon-udp-client` / `echocon-udp-server` | Servicio de eco sobre UDP: el servidor devuelve la cadena recibida con mayúsculas y minúsculas invertidas. |
| [Tema 6](practica_6/) | `echocon-tcp-client` / `echocon-tcp-server` | El mismo servicio sobre TCP, con un servidor concurrente que atiende a cada cliente en un proceso hijo. |
| [Tema 7](practica_7/) | `tftp-client` | Cliente TFTP que descarga (`-r`) o sube (`-w`) ficheros en modo `octet`, con salida detallada opcional (`-v`). |
| [Tema 8](practica_8/) | `miping` | Envío de un ICMP *Echo Request* mediante sockets RAW e interpretación de la respuesta, incluidos los mensajes de error ICMP. |

Las carpetas `practica_5` y `practica_6` incluyen además el enunciado y la estructura básica de un programa UDP/TCP en PDF.

## Tecnologías

- C (GCC) sobre Linux
- API de sockets de POSIX: `SOCK_DGRAM`, `SOCK_STREAM`, `SOCK_RAW`
- Protocolos UDP, TCP, TFTP e ICMP

## Aspectos técnicos destacados

- **Servidor TCP concurrente**: cada conexión aceptada se atiende en un proceso creado con `fork()`. Un manejador de `SIGINT` cierra los sockets al pulsar Ctrl+C, y `SO_REUSEPORT` evita el error `bind(): Address already in use` al reiniciar el servidor.
- **Cliente TFTP completo**: construye a mano las peticiones RRQ/WRQ, transfiere bloques de 512 bytes y considera terminada la transferencia al llegar un bloque más corto. Comprueba también el orden de los bloques y los ACK y trata los datagramas `ERROR` del servidor. El puerto se obtiene con `getservbyname("tftp", "udp")`.
- **`ping` propio**: rellena la cabecera ICMP (tipo, código, PID, número de secuencia), calcula el *checksum* y traduce los tipos y códigos de error (por ejemplo, *Destination Unreachable*) a mensajes legibles. Usa el fichero de cabecera `ip-icmp-ping.h` que da el profesorado.
- Validación de los argumentos de línea de comandos, con un mensaje de uso en cada programa, y control de errores de todas las llamadas al sistema con `perror()`.

## Estructura del repositorio

```text
ARS/
├── practica_5/   # Eco UDP (cliente y servidor) + enunciado
├── practica_6/   # Eco TCP (cliente y servidor concurrente) + enunciado
├── practica_7/   # Cliente TFTP
└── practica_8/   # miping (ICMP con sockets RAW) + ip-icmp-ping.h
```

## Requisitos

- Linux (o WSL) con `gcc`
- Permisos de superusuario para `miping`, porque abre un socket RAW
- Un servidor TFTP accesible para probar `tftp-client`

## Cómo compilar y ejecutar

```bash
git clone https://github.com/JavivuG/ARS.git
cd ARS

# Compilación (los nombres de los ejecutables coinciden con los del .gitignore)
gcc -Wall -o practica_5/echocon-udp-server practica_5/echocon-udp-server-GarciaGonzalez.c
gcc -Wall -o practica_5/echocon-udp-client practica_5/echocon-udp-client-GarciaGonzalez.c
gcc -Wall -o practica_6/echocon-tcp-server practica_6/echocon-tcp-server-GarciaGonzalez.c
gcc -Wall -o practica_6/echocon-tcp-client practica_6/echocon-tcp-client-GarciaGonzalez.c
gcc -Wall -o practica_7/tftp-client       practica_7/tftp-client-GarciaGonzalez.c
gcc -Wall -o practica_8/miping            practica_8/miping-GarciaGonzalez.c
```

Uso de cada programa:

```bash
./echocon-udp-server [-p puerto-servidor]
./echocon-udp-client ip_servidor [-p puerto-servidor] cadena

./echocon-tcp-server [-p puerto-servidor]
./echocon-tcp-client ip_servidor [-p puerto-servidor] cadena

./tftp-client ip_servidor {-r|-w} archivo [-v]

sudo ./miping ip_servidor [-v]
```

Ejemplo en local:

```bash
./practica_5/echocon-udp-server -p 5555 &
./practica_5/echocon-udp-client 127.0.0.1 -p 5555 HolaMundo   # -> hOLAmUNDO
```

Los seis programas compilan sin avisos con `gcc -Wall`, y los ejemplos de eco UDP y TCP se han probado en local. El cliente TFTP y `miping` no se han probado, porque necesitan un servidor TFTP y permisos de superusuario.

## Autoría

Javier García González.

---

Forma parte de [UVa-Projects](https://github.com/JavivuG/UVa-Projects).
