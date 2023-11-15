// Practica tema 6, Alonso Pastor Rodrigo
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#define TAM_BUFFER 512

int main(int argc, char *argv[])
{

    if (argc > 4 || argc < 2)
    { // Comprobación de argumentos
        printf("Argumentos incorrectos. Uso: ip-servidor [-p puerto]\n");
        exit(-1);
    }

    /* Inicializo el puerto al por defecto del servicio,
    y si se ha especificado otro, lo cambio */
    int port = getservbyname("daytime", "tcp")->s_port;

    if (argc == 4)
    {
        if (strcmp(argv[2], "-p") == 0)
            port = htons(atoi(argv[3]));
        else{
            printf("Flag incorrecta. Uso: ip-servidor [-p puerto]\n ");
            exit(-1);
        }
    }

    /* Transformo la dirección a network byte order */
    struct in_addr ip_server;
    /* Para comprobar los errores en las funciones, voy a
    meterlas en una condicion if, comprobando el valor que retorna
    la funcion. De esta manera, ejecuto la funcion y lo compruebo
    con una sentencia */
    if (inet_aton(argv[1], &ip_server) == 0)
    {
        perror("Error en funcion inet_atom");
        exit(-1);
    }

    /* Creacion y bind del socket UDP local, comprobando errores */
    int localSocket;
    localSocket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = 0;

    if (bind(localSocket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    {
        perror("Error en funcion bind");
        exit(-1);
    }

    /* Relleno la estructura sockaddr para el servidor
    con la direccion ip y el puerto especificados */
    struct sockaddr_in sockaddr_server;
    sockaddr_server.sin_family = AF_INET;
    sockaddr_server.sin_addr = ip_server;
    sockaddr_server.sin_port = port;

    
    /* Peticion de conexion al servidor, no es necesario enviar ningún mensaje */
    if (connect(localSocket, (struct sockaddr *)&sockaddr_server, sizeof(sockaddr_server)) < 0)
    {
        perror("Error en funcion connect");
        exit(-1);
    }


    char buffer[TAM_BUFFER];

    if (recv(localSocket, buffer, TAM_BUFFER, 0) < 0)
    {
        perror("Error en funcion recv");
        exit(-1);
    }

    printf("Mensaje recibido: %s\n", buffer);

    /* Secuencia para el cierre del cliente
    1. shutdown(), 2. recv(), 3. close()*/

    if (shutdown(localSocket, SHUT_RDWR) < 0)
    {
        perror("Error en funcion shutdown");
        exit(-1);
    }

    if (recv(localSocket, buffer, TAM_BUFFER, 0) < 0)
    {
        perror("Error vaciando buffer");
        exit(-1);
    }

    close(localSocket);
    return 0;
}
