#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <mqueue.h>
#include "definitions.h"

// Modulo principal
int main(int argc, char *argv[]) {
    //controlamos los parametros 
    if (argc != 1) {
        fprintf(stderr, "Error. Usa: ./exec/telefono <cola_Telefono>.\n");
        exit(EXIT_FAILURE);
    }

    // Define variables locales
    int pid = getpid();
    srand(pid);// lo generamos de manera random 
    struct mq_attr attr;
    mqd_t qHandler;
    mqd_t qHandlerLineas[NUMLINEAS];
    char buf[BUFSIZ];
    char cLinea[TAMANO_MENSAJES];// denominamos el buzon linea con el tamaño de los mensajes

    sprintf(cLinea,"%s",argv[1]);
    // Configuramos los atributos de la cola de mensajes
    attr.mq_flags = 0;
    attr.mq_maxmsg = NUMLINEAS;
    attr.mq_msgsize = TAMANO_MENSAJES;
    attr.mq_curmsgs = 0;

    // Creamos la cola de mensajes para el teléfono
    qHandler = mq_open(BUZON_LLAMADAS, O_RDWR, &attr);
    if (qHandler == -1) {
        perror("[TELEFONO] Error creando la cola de mensajes.");
        exit(EXIT_FAILURE);
    }

    // Ciclo principal del teléfono
    while (1) {
        // Informamos que el teléfono está en espera
        printf("[TELEFONO %d] en espera...\n", pid);
        // Esperamos a recibir una llamada
        int recibir1 = mq_receive(qHandler, buf, TAMANO_MENSAJES, NULL);
        if(recibir1== -1) {
                perror("[TELEFONO] Error recibiendo mensaje de la cola de mensajes.");
                exit(EXIT_FAILURE);
            }
        printf("[TELEFONO %d] en conversación de llamada desde la línea: %d...\n", pid, cLinea);
        // Simulamos la conversación por un tiempo aleatorio entre 10 y 20 segundos
        sleep(rand() % 11 + 10);

        // Si recibimos el mensaje de "COLGAR", es porque la conversación ha terminado
        printf("[TELEFONO %d] ha colgado la llamada. [%s]\n", pid, cLinea);
        // Notificamos que la conversación ha terminado;
            if (mq_send(qHandlerLineas, buf, strlen(cLinea) + 1, 0) == -1) {
                perror("[TELEFONO] Error enviando mensaje de finalización a la línea.");
                exit(EXIT_FAILURE);
            }
    }

    return EXIT_SUCCESS;
}
