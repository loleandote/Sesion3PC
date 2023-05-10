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
    char cLinea[TAMANO_MENSAJES];// denominamos el buzon linea con el tamaño de los mensajes
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
        int recibir1 = mq_receive(qHandler, cLinea, TAMANO_MENSAJES, 0);
        if(recibir1== -1) {
                perror("[TELEFONO] Error recibiendo mensaje de la cola de mensajes.");
                exit(EXIT_FAILURE);
            }
        printf("[TELEFONO %d] en conversación de llamada desde la línea: %s...\n", pid, cLinea);
        // Simulamos la conversación por un tiempo aleatorio entre 10 y 20 segundos
        sleep(rand() % 11 + 10);

        // Si recibimos el mensaje de "COLGAR", es porque la conversación ha terminado
        printf("[TELEFONO %d] ha colgado la llamada. [%s]\n", pid, cLinea);
        // Notificamos que la conversación ha terminado;
        
        char texto[1];
        strncpy(texto, &cLinea[13],1);
        printf("%s",texto);
        int posicion=atoi(texto);
        qHandlerLineas[posicion]=mq_open(cLinea,O_WRONLY);
            if (mq_send(qHandlerLineas[posicion], cLinea, TAMANO_MENSAJES, 0) == -1) {
                printf/*perror*/("[TELEFONO %d] Error enviando mensaje de finalización a la línea.\n", pid);
                exit(EXIT_FAILURE);
            }
            mq_close(qHandlerLineas[posicion]);
    }

    return EXIT_SUCCESS;
}
