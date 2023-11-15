// ^ractica tema 8, Alonso Pastor Rodrigo
#include "ip-icmp-ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>


unsigned short calc_checksum(unsigned short *datos, int tamRequest);
void get_error(unsigned char type, unsigned char code, char *result);


int main(int argc, char *argv[])
{
    /* Comprobacion de argumentos */
    if (argc < 2 || argc > 3)
    {
        printf("Argumentos incorrectos. Uso: ip-servidor [-v]\n");
        exit(-1);
    }

    int verbose = 0;

    if (argc == 3)
    {
        if (strcmp("-v", argv[2]) == 0)
        {
            verbose = 1;
        }
        else
        {
            printf("Flag incorrecta. Uso: ip-servidor [-v]\n ");
            exit(-1);
        }
    }

     /* Transformo la dirección dada a network byte order */
    struct in_addr ip_server;

    if (inet_aton(argv[1], &ip_server) == 0)
    {
        perror("Error en funcion inet_aton");
        exit(-1);
    }   

    /* Creacion y bind del socket UDP local, comprobando errores */
    int localSocket;
    localSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

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
    sockaddr_server.sin_port = 0;

    socklen_t size = sizeof(sockaddr_server);

    /* Creo los datagramas ICMP y relleno la solicitud */
    ECHORequest echoRequest;
    ECHOResponse echoResponse;

    if (verbose == 1)
        printf("-> Generando cabecera ICMP.\n");

    echoRequest.icmpHeader.Type = 8;
    echoRequest.icmpHeader.Code = 0;
    echoRequest.icmpHeader.Checksum = 0;


    if (verbose == 1){
        printf("-> Type: %d\n", echoRequest.icmpHeader.Type);
        printf("-> Code: %d\n", echoRequest.icmpHeader.Code);
    }

    echoRequest.ID = getpid();
    echoRequest.SeqNumber = 0;
    *echoRequest.payload = 'C';
    *(echoRequest.payload + 1) = 0;

    if (verbose == 1){
        printf("-> Identifier (pid): %d\n", echoRequest.ID);
        printf("-> Seq. number: %d\n", echoRequest.SeqNumber);
        printf("-> Cadena a enviar: %s\n", echoRequest.payload);
    
    }

    /* Calculo del checksum */
    echoRequest.icmpHeader.Checksum = calc_checksum((unsigned short *)&echoRequest, sizeof(echoRequest));

    if (verbose == 1)
    {
        printf("-> Checksum: 0x%x\n", echoRequest.icmpHeader.Checksum);
        printf("-> Tamaño total del paquete ICMP: %ld\n", sizeof(echoRequest));
    }


    /* Envio el datagrama al servidor especificando el socket local,
    cadena, bytes, dirección del socket remoto y su tamaño */
    if (sendto(localSocket, &echoRequest, sizeof(echoRequest), 0, (struct sockaddr *)&sockaddr_server, sizeof(sockaddr_server)) < 0)
    {
        perror("Error en funcion sendto");
        exit(-1);
    }

    printf("Paquete ICMP enviado a %s\n", argv[1]);



    if (recvfrom(localSocket, &echoResponse, sizeof(echoResponse), 0, (struct sockaddr *)&sockaddr_server, &size) < 0)
    {
        perror("Error en funcion recvfrom");
        exit(-1);
    }


    printf("Respuesta recibida desde %s\n", inet_ntoa(echoResponse.ipHeader.iaSrc));

    if (verbose == 1){
        printf("-> Tamaño de la respuesta: %ld\n", sizeof(echoResponse));
        printf("-> Cadena recibida: %s\n", echoResponse.payload);
        printf("-> Identifier (pid): %d\n", echoResponse.ID);
        printf("-> TTL: %d\n", echoResponse.ipHeader.TTL);        
    }

    char errorMsg[500];

    /* Compruebo la respuesta */
    get_error(echoResponse.icmpHeader.Type, echoResponse.icmpHeader.Code, errorMsg);

    printf("Descripcion de la respuesta: %s.\n", errorMsg);

    return 0;
     
}

unsigned short calc_checksum(unsigned short *datos, int tamRequest){

    int size = tamRequest / 2;
    int i;
    unsigned int sum = 0;

    for (i = 0; i < size; i++){
        sum = sum + datos[i];
    }

    sum = (sum & 0xFFFF) + (sum >> 16);

    sum = (sum & 0xFFFF) + (sum >> 16);

    return (unsigned short)~sum;

}

void get_error(unsigned char type, unsigned char code, char *result)
{

    /* Valores maximos */
    const short max_types = 44;
    const short max_code = 16;

    /* Arrays para guardar la tabla de tipos y descripciones */
    char *error_description[max_types][max_code];
    char *error_type[max_types];

    /* Excepcion si supera valores maximos */
    if (code >= max_code || type >= max_types)
    {
        sprintf(result, "Valores desconocidos (type %d, code %d)", type, code);
        return;
    }

    int i = 0, j = 0;
    /* Inicializo los valores a NULL */
    for (i = 0; i < max_types; i++)
    {
        error_type[i] = NULL;
        for (j = 0; j < max_code; j++)
        {
            error_description[i][j] = NULL;
        }
    }

    /* Tabla de errores */
    error_type[0] = "";
    error_description[0][0] = "Respuesta Correcta";

    error_type[3] = "Destination Unreachable:";
    error_description[3][0] = "Destination network unreachable";
    error_description[3][1] = "Destination host unreachable";
    error_description[3][2] = "Destination protocol unreachable";
    error_description[3][3] = "Destination port unreachable";
    error_description[3][4] = "Fragmentation required, and DF flag set";
    error_description[3][5] = "Source route failed";
    error_description[3][6] = "Destination network unknown";
    error_description[3][7] = "Destination host unknown";
    error_description[3][8] = "Source host isolated";
    error_description[3][9] = "Network administratively prohibited";
    error_description[3][10] = "Host administratively prohibited";
    error_description[3][11] = "Network unreachable for ToS";
    error_description[3][12] = "Host unreachable for ToS";
    error_description[3][13] = "Communication administratively prohibited";
    error_description[3][14] = "Host Precedence Violation";
    error_description[3][15] = "Precedence cutoff in effect";

    error_type[5] = "Redirect Message:";
    error_description[5][0] = "Redirect Datagram for the Network";
    error_description[5][1] = "Redirect Datagram for the Host";
    error_description[5][2] = "Redirect Datagram for the ToS & network";
    error_description[5][3] = "Redirect Datagram for the ToS & host";

    error_type[8] = "Echo Request:";
    error_description[8][0] = "Echo request (used to ping)";

    error_type[9] = "Router Advertisement:";
    error_description[9][0] = "Router Advertisement";

    error_type[10] = "Router Solicitation:";
    error_description[10][0] = "Router discovery/selection/solicitation";

    error_type[11] = "Time Exceeded:";
    error_description[11][0] = "TTL expired in transit";
    error_description[11][1] = "Fragment reassembly time exceeded";

    error_type[12] = "Parameter Problem: Bad IP header:";
    error_description[12][0] = "Pointer indicates the error";
    error_description[12][1] = "Missing a required option";
    error_description[12][2] = "Bad length";

    error_type[13] = "Timestamp:";
    error_description[13][0] = "Timestamp";

    error_type[14] = "Timestamp Reply:";
    error_description[14][0] = "Timestamp reply";

    error_type[40] = "";
    error_description[40][0] = "Photuris, Security failures";

    error_type[42] = "Extended Echo Request:";
    error_description[42][0] = "No error";

    error_type[43] = "Extended Echo Reply:";
    error_description[43][0] = "No Error";
    error_description[43][1] = "Malformed Query";
    error_description[43][2] = "No Such Interface";
    error_description[43][3] = "No Such Table Entry";
    error_description[43][4] = "Multiple Interfaces Satisfy Query";

    if (error_description[type][code] == NULL)
    {
        sprintf(result, "Valores desconocidos (type %d, code %d)", type, code);
        return;
    }

    sprintf(result, "%s %s (type %d, code %d)",
            error_type[type], error_description[type][code], type, code);

    return;
}
