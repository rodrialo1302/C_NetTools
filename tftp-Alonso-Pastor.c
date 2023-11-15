// Practica tema 7, Alonso Pastor Rodrigo
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#define TAM_BUFFER 516

int main(int argc, char *argv[])
{

    /* Comprobacion de argumentos */
    if (argc != 4 && argc != 5)
    {
        printf("Argumentos incorrectos. Uso: ip-servidor {-r|-w} archivo [-v]\n");
        exit(-1);
    }

    int verbose = 0;

    if (argc == 5)
    {
        if (strcmp("-v", argv[4]) == 0)
        {
            verbose = 1;
        }
        else
        {
            printf("Flag incorrecta. Uso: ip-servidor {-r|-w} archivo [-v]\n ");
            exit(-1);
        }
    }

    /* Voy a usar un entero para el modo,
    0 será lectura y 1 será escritura */
    int mode;

    if (strcmp("-r", argv[2]) == 0)
    {
        mode = 0;
    }
    else
    {
        if (strcmp("-w", argv[2]) == 0)
        {
            mode = 1;
        }
        else
        {
            printf("Argumentos incorrectos. Uso: ip-servidor {-r|-w} archivo [-v]\n");
            exit(-1);
        }
    }

    /* Inicializo el puerto al por defecto del servicio */
    int port = getservbyname("tftp", "udp")->s_port;

    /* Transformo la dirección dada a network byte order */
    struct in_addr ip_server;

    if (inet_aton(argv[1], &ip_server) == 0)
    {
        perror("Error en funcion inet_atom");
        exit(-1);
    }

    /* Creacion y bind del socket UDP local, comprobando errores */
    int localSocket;
    localSocket = socket(AF_INET, SOCK_DGRAM, 0);

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
    con la direccion ip especificada */
    struct sockaddr_in sockaddr_server;
    sockaddr_server.sin_family = AF_INET;
    sockaddr_server.sin_addr = ip_server;
    sockaddr_server.sin_port = port;

    /* Archivo a enviar y recibir*/
    FILE *archivo;
    char *nombre_archivo = argv[3];

    socklen_t size = sizeof(sockaddr_server);

    unsigned char buffer_envio[514];
    unsigned char buffer_server[514];

    /* Modo de lectura */
    if (mode == 0)
    {
        archivo = fopen(nombre_archivo, "w");
        if (archivo == NULL)
        {
            printf("Error abriendo archivo");
            exit(-1);
        }

        /* Datagrama RRQ */
        int offset = 0;
        buffer_envio[0] = 0;
        buffer_envio[1] = 1;
        offset += 2;

        /* Nombre del fichero + EOS */
        strcpy((char *)&buffer_envio[offset], nombre_archivo);
        offset += strlen(nombre_archivo) + 1;

        /* Modo + EOS */
        strcpy((char *)&buffer_envio[offset], "octet");
        offset += strlen("octet") + 1;

        if (sendto(localSocket, buffer_envio, offset, 0, (struct sockaddr *)&sockaddr_server, sizeof(sockaddr_server)) < 0)
        {
            perror("Error en funcion sendto");
            exit(-1);
        }

        if (verbose == 1)
            printf("Enviada solicitud de lectura de %s a servidor tftp en %s\n", nombre_archivo, argv[1]);

        int bytes_recibidos = 0; // Tamaño de bloque recibido
        int num_block; // Numero de bloque recibido
        int num_block_esperado = 0; // Numero de bloque esperado, usado para ver si se reciben en orden
        do
        {
            bytes_recibidos = recvfrom(localSocket, buffer_server, TAM_BUFFER, 0, (struct sockaddr *)&sockaddr_server, &size);
            if (bytes_recibidos < 0)
            {
                perror("Error en funcion recvfrom");
                exit(-1);
            }

            /* Elimino el opcode y el block del tamaño del bloque */
            bytes_recibidos -= 4;

            if (verbose == 1)
                printf("Recibido bloque del servidor tftp\n");

            if (buffer_server[1] == 5)
            {
                printf("Error en servidor TFTP: %d\n", buffer_server[3]);
                exit(-1);
            }
            /* Construyo el numero de bloque recibido y compruebo que sea el correcto */
            num_block = (buffer_server[2] << 8) + buffer_server[3];
            num_block_esperado++;

            if (num_block != num_block_esperado){
                printf("Error en servidor TFTP: Recibido bloque inválido");
                printf("Bloque esperado: %d", num_block_esperado);
                printf("Bloque recibido: %d", num_block);
                exit(-1);
            }


            if (verbose == 1)
                printf("Numero de bloque %d\n", num_block);

            if (fwrite(&buffer_server[4], sizeof(char), bytes_recibidos, archivo) < bytes_recibidos)
            {
                perror("Error en funcion fwrite");
                exit(-1);
            }

            /* Construyo el ACK*/
            buffer_envio[0] = 0;
            buffer_envio[1] = 4;
            buffer_envio[2] = buffer_server[2];
            buffer_envio[3] = buffer_server[3];

            if (verbose == 1)
                printf("Enviamos el ACK del bloque %d\n", num_block);

            if (sendto(localSocket, buffer_envio, 4, 0, (struct sockaddr *)&sockaddr_server, sizeof(sockaddr_server)) < 0)
            {
                perror("Error en funcion sendto");
                exit(-1);
            }
        } while (bytes_recibidos == 512);

        if (verbose)
            printf("El bloque %d era el ultimo: cerramos el fichero \n", num_block);
    }
    /* Modo de escritura */
    else
    {

        archivo = fopen(nombre_archivo, "r");
        if (archivo == NULL)
        {
            printf("Error abriendo archivo");
            exit(-1);
        }

        /* Datagrama WRQ */
        int offset = 0;
        buffer_envio[0] = 0;
        buffer_envio[1] = 2;
        offset += 2;

        /* Nombre del fichero + EOS */
        strcpy((char *)&buffer_envio[offset], nombre_archivo);
        offset += strlen(nombre_archivo) + 1;

        /* Modo + EOS */
        strcpy((char *)&buffer_envio[offset], "octet");
        offset += strlen("octet") + 1;

        if (sendto(localSocket, buffer_envio, offset, 0, (struct sockaddr *)&sockaddr_server, sizeof(sockaddr_server)) < 0)
        {
            perror("Error en funcion sendto");
            exit(-1);
        }

        if (verbose == 1)
            printf("Enviada solicitud de escritura de %s a servidor tftp en %s\n", nombre_archivo, argv[1]);

        struct stat stats_archivo;
        stat(nombre_archivo, &stats_archivo);

        long size_archivo = stats_archivo.st_size;

        if (verbose == 1)
            printf("Tamaño del archivo: %ld", size_archivo);

        int bloques = (size_archivo / 512) + 1;
        int num_block = 0;

        do
        {

            if (recvfrom(localSocket, buffer_server, TAM_BUFFER, 0, (struct sockaddr *)&sockaddr_server, &size) < 0)
            {
                perror("Error en funcion recvfrom");
                exit(-1);
            }

            if (verbose)
                printf("Recibido mensaje del servidor tftp\n");

            if (buffer_server[1] == 5)
            {
                printf("Error en servidor TFTP: %d\n", buffer_server[3]);
                exit(-1);
            }

            /* Construyo el ACK y compruebo que sea el correcto */
            int num_ack = (buffer_server[2] << 8) + buffer_server[3];

            if (num_ack != num_block)
            {
                printf("Error en servidor TFTP: ACK inválido");
                exit(-1);
            }

            if (verbose == 1)
                printf("Recibido ACK del bloque %d\n", num_ack);

            if (bloques == num_block)
            {

                if (verbose == 1)
                    printf("El bloque %d era el ultimo: cerramos el fichero\n", num_ack);

                /* Si el ultimo bloque ha sido recibido, salgo del bucle directamente */
                break;
            }

            num_block++;

            buffer_envio[0] = 0;
            buffer_envio[1] = 3;
            buffer_envio[2] = num_block >> 8;
            buffer_envio[3] = 0x00FF & num_block;
            offset = 4;

            /* Calculo el tamaño de los datos a enviar, de tal manera
            que si es el ultimo bloque, calcule los bytes, y si no,
            envíe 512 */
            int buffer_size = (num_block == bloques) ? (size_archivo % 512) : 512;

            /* Leo el fichero para enviarlo */
            if (fread(&buffer_envio[offset], 1, buffer_size, archivo) == EOF)
            {
                printf("Error leyendo archivo");
                exit(-1);
            }

            if (verbose == 1)
                printf("Enviamos el bloque %d\n", num_block);

            if (sendto(localSocket, buffer_envio, offset + buffer_size, 0, (struct sockaddr *)&sockaddr_server, sizeof(sockaddr_server)) < 0)
            {
                printf("Error en funcion sendto");
                exit(-1);
            }

        } while (num_block <= bloques);
    }

    if (fclose(archivo) == EOF)
    {
        printf("Error cerrando archivo");
        exit(-1);
    }

    if (close(localSocket) < 0)
    {
        printf("Error cerrando socket");
        exit(-1);
    }

    return 0;
}