#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <mqueue.h>
#include <definitions.h>

// Función para generar un número aleatorio entre min y max
int random_between(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

// Función principal de la línea
int main(int argc, char *argv[]) {

// Verificamos los parámetros que pasamos en el programa
    if (argc != 2) {
        fprintf(stderr, "Error. Usa: ./exec/linea <cola_linea_llamante>.\n");
        exit(EXIT_FAILURE);
    }
    // Define variables locales
    int pid = getpid();
    srand(pid);//generamos un numero random
    //DECLAMOS LOS BUZONES
    mqd_t qHandlerLlamadas;
    mqd_t qHandlerLinea;
     char buf[BUFSIZ];
    char cLinea[TAMANO_MENSAJES];// denominamos el buzon linea con el tamaño de los mensajes
    //char buffer[TAMANO_MENSAJES+1];//y el buffer del mismo modo pero sumandole 1
    //ahora en buzon linea colocamos 

    sprintf(cLinea,"%s",argv[1]);
    //declaramos los atributos de la cola de mensajes
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = NUMLINEAS;
    attr.mq_msgsize = TAMANO_MENSAJES;
    attr.mq_curmsgs = 0;
    //creamos los buzones 
    qHandlerLinea=mq_open(cLinea, O_RDWR, &attr);
    qHandlerLlamadas = mq_open(BUZON_LLAMADAS, O_WRONLY);
    // Inicia Random

    // Crea la cola de mensajes para notificar a los teléfonos
    //si falla la creacion de las colas de mensajes
    if (qHandlerLinea == (mqd_t)-1) {
        perror("error al crear el buzon");
        exit(EXIT_FAILURE);
    }

    // Entra en un bucle infinito para esperar llamadas
    while (1) {

        printf("Linea[%d] esperando llamada\n", pid);
        // Espera un tiempo aleatorio antes de verificar la cola de mensajes
        int tiempoEspera = random_between(1, 30);
        sleep(tiempoEspera);
    
        // Verifica si hay una llamada en la cola de mensajes
        int recibir1 =mq_receive(qHandlerLinea,buf ,sizeof(TAMANO_MENSAJES+1), NULL);
        
        //controlamos el error 
        if(recibir1 == -1) {
            perror("error al recibir");    
        }  
        printf("Linea[%d] ha recibido una llamada [%s]\n", pid, cLinea);
        printf("Linea[%d] esperando fin de conversacion[%s]\n", pid,NULL);
        // Envía un mensaje a la cola de llamadas para ser atendida
        int envio= mq_send(qHandlerLlamadas,buf, strlen(cLinea) + 1, 0); 
        if (envio== -1) {
            perror("mq_send");
            exit(EXIT_FAILURE);
        }
        // Espera la notificación de un teléfono de que la llamada ha finalizado
        int recibir2 = mq_receive(qHandlerLinea, cLinea,sizeof(TAMANO_MENSAJES+1), NULL);

        if(recibir2 == -1) {
        perror("error al recibir");    
        }
        printf("Linea[%d]conversacion finalizada[%s]\n", pid); 
            // Notifica la recepción del teléfono y vuelve al estado de espera de llamada
    }

    // Cierra la cola de mensajes de la línea
    if (mq_close(qHandlerLinea) == -1) {
        perror("mq_close");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

