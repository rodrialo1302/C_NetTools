// Practica tema 5, Alonso Pastor Rodrigo
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#define TAM_BUFFER 512
#define CADENA_BUFF 250
#define CADENA_LEN 501

int main(int argc, char *argv[])
{
    if (argc > 3)
    { // Comprobación de argumentos
        printf("Argumentos incorrectos. Uso: [-p puerto]\n");
        exit(-1);
    }

    /* Inicializo el puerto al por defecto del servicio,
    y si se ha especificado otro, lo cambio */
    int port = getservbyname("daytime", "udp")->s_port;


    if (argc == 3)
    {
        if (strcmp(argv[1], "-p") == 0)
            port = htons(atoi(argv[2]));
        else{
            printf("Flag incorrecta. Uso: ip-servidor [-p puerto]\n ");
            exit(-1);
        }
    }

    /* Creacion y bind del socket UDP local, comprobando errores */
    int localSocket;
    localSocket = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = port;

    if (bind(localSocket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    {
        perror("Error en funcion bind");
        exit(-1);
    }

    /* Creacion de la estructura sockaddr del cliente*/
    struct sockaddr_in sockaddr_client;
    socklen_t size = sizeof(sockaddr_client);

    /* Creacion de variables para la informacion
    del servicio daytime*/
    char hostname[CADENA_BUFF];
    char date_buffer[CADENA_BUFF];
    FILE *fich;
    char cadena[CADENA_LEN];

    printf("Servidor iniciado correctamente en el puerto %d\n\n", ntohs(port));
    fflush(stdout);
    char recv_buffer[TAM_BUFFER];
    /* Bucle infinito para el funcionamiento del servidor */
    while (1)
    {
        if (recvfrom(localSocket, recv_buffer, TAM_BUFFER, 0, (struct sockaddr *)&sockaddr_client, &size) < 0)
        {
            perror("Error en funcion recvfrom");
            exit(-1);
        }

        printf("Datagrama recibido\n\n");

        gethostname(hostname, CADENA_BUFF);

        system("date > /tmp/tt.txt");
        fich = fopen("/tmp/tt.txt", "r");
        if (fgets(date_buffer, CADENA_BUFF, fich) == NULL)
        {
            perror("Error recuperando la fecha");
            exit(-1);
        }

        fclose(fich);

        /* Concateno las dos cadenas con formato*/
        sprintf(cadena, "%s: %s", hostname, date_buffer);

        if (sendto(localSocket, cadena, CADENA_LEN, 0, (struct sockaddr *)&sockaddr_client, sizeof(sockaddr_client)) < 0)
        {
            perror("Error en funcion sendto");
            exit(-1);
        }
        printf("Respuesta enviada\n\n");
    }
    close(localSocket);
    return 0;
}