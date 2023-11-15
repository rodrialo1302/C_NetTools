// Practica tema 5, Alonso Pastor Rodrigo
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#define TAM_BUFFER 512
#define CADENA_BUFF 250
#define CADENA_LEN 501

//Variable global para la funcion signal_handler
int localSocket;

void signal_handler(int signal);

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
        else
        {
            printf("Flag incorrecta. Uso: ip-servidor [-p puerto]\n ");
            exit(-1);
        }
    }

    /* Activo la señal para cerrar el servidor correctamente */
    signal(SIGINT, &signal_handler);

    /* Creacion y bind del socket UDP local, comprobando errores */
    localSocket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = port;

    if (bind(localSocket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    {
        perror("Error en funcion bind");
        exit(-1);
    }

    /* Creacion del socket y sockaddr hijo */
    int socket_conex;

    struct sockaddr_in sockaddr_conex;
    socklen_t socket_conex_size = sizeof(socket_conex);

    /* Creacion de variables para la informacion
    del servicio daytime*/
    char hostname[CADENA_BUFF];
    char date_buffer[CADENA_BUFF];
    FILE *fich;
    char cadena[CADENA_LEN];

    /* Preparo el socket padre para recibir conexiones */
    listen(localSocket, 50);

    printf("Servidor iniciado correctamente en el puerto %d\n\n", ntohs(port));
    fflush(stdout);
    char recv_buffer[TAM_BUFFER];
    /* Bucle infinito para el funcionamiento del servidor */
    while (1)
    {
        socket_conex = accept(localSocket, (struct sockaddr *)&sockaddr_conex, &socket_conex_size);
        if (socket_conex < 0)
        {
            perror("Error en funcion accept");
            exit(-1);
        }
        printf("Conexion recibida\n\n");


        int pid = fork();
        if (pid == 0)
        {
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

            if (send(socket_conex, cadena, CADENA_LEN, 0) < 0)
            {
                perror("Error en funcion send");
                exit(-1);
            }
            

            /* Secuencia para el cierre del socket hijo
            1. recv(), 2. shutdown(), 3. close()*/

            if (recv(socket_conex, recv_buffer, TAM_BUFFER, 0) < 0){
                perror("Error en funcion recv");
                exit(-1);
            }

            if (shutdown(socket_conex, SHUT_RDWR) < 0)
            {
                perror("Error en funcion shutdown");
                exit(-1);
            }
            
            close(socket_conex);

            printf("Respuesta enviada\n\n");
            exit(0);
        }
    }
    return 0;
}

void signal_handler(int signal)
{

    if (close(localSocket) < 0)
    {
        perror("\nError cerrando socket");
        exit(-1);
    }

    printf("\nServidor cerrado correctamente\n");
    exit(EXIT_SUCCESS);
}