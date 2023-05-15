#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#include <mqueue.h>
#include <definitions.h>

void crear_buzones();
void instalar_manejador_senhal();
void manejador_senhal(int sign);
void iniciar_tabla_procesos(int n_procesos_telefono, int n_procesos_linea);
void crear_procesos(int numTelefonos, int numLineas);
void lanzar_proceso_telefono(const int indice_tabla);
void lanzar_proceso_linea(const int indice_tabla);
void esperar_procesos();
void terminar_procesos(void);
void terminar_procesos_especificos(struct TProcess_t *process_table, int process_num);
void liberar_recursos();

int g_telefonosProcesses = 0;
int g_lineasProcesses = 0;
struct TProcess_t *g_process_telefonos_table;
struct TProcess_t *g_process_lineas_table;
mqd_t qHandlerLlamadas;
mqd_t qHandlerLineas[NUMLINEAS];

int main(int argc, char *argv[])
{
    crear_buzones();

    // Manejador de Ctrl-C
    instalar_manejador_senhal();

    // Crea Tabla para almacenar los pids de los procesos
    iniciar_tabla_procesos(NUMTELEFONOS, NUMLINEAS);

    // Tenemos todo
    // Lanzamos los procesos
    crear_procesos(NUMTELEFONOS, NUMLINEAS);

    // Esperamos a que finalicen las lineas
    esperar_procesos();

    // Matamos los telefonos y cualquier otro proceso restante
    terminar_procesos();

    // Finalizamos Manager
    printf("\n[MANAGER] Terminacion del programa (todos los procesos terminados).\n");
    liberar_recursos();

    return EXIT_SUCCESS;
}
// creamos los buzones, crearemos un buzon para las llamadas y otro para las lineas
void crear_buzones()
{
    // Buzón de llamadas
    struct mq_attr atributos;               // definimos mq_atrtr el cual lo llamamos atributos para co nocer los atributos de nuestro buzon
    atributos.mq_flags = 0;                 // lo ponemos a 0 porque no tendremmos opcionesa adicionales
    atributos.mq_maxmsg = NUMLINEAS;        // el numero maximo sera el numero de lineas que puede haber en la cola
    atributos.mq_msgsize = TAMANO_MENSAJES; // el tamaño sera el que hemos establecido en definitions.h, el cual es 64
    atributos.mq_curmsgs = 0;               // establecemos que el numero actual de mensajes en el buzon es cero

    // ahora por medio del mq_open creamos una cola de mensajes con los siguintes atributos
    // BUZON_LLAMADAS: este el nombre de la cola de mensaje que definimos en definitions.h
    // O_CREAT | O_RDW: son banderas de creacion (este crea la cola de mensajes si no existe) y la de lectura y escritura
    // S_IRUSR | S_IWUSR: son los permisos que damos a la cola de mensaje, en este caso de lectura y escritura para el que lo crea
    //&atributos un puntero que contiene todos los atributos que anteriormente definimos para nuestra cola de mensajes
    qHandlerLlamadas = mq_open(BUZON_LLAMADAS, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &atributos);
    if (qHandlerLlamadas == (mqd_t)-1)
    { // en el caso de ser -1 es un error
        perror("No fue posible crear la cola de mensajes de las llamadas");
        exit(EXIT_FAILURE);
    }

    // Buzón de líneas
    char cLinea[strlen(BUZON_LINEAS)+20];
    int i;
    // ahora cremaos un bucle para iterar en el numero de lineas que existen
    for (i = 0; i < NUMLINEAS; i++)
    {
        // ahora creamos el nombre de la cola de mensaje de la lienas
        sprintf(cLinea, "%s%d", BUZON_LINEAS, i);
        // creamos el buzon de la misma manera como lo hicimos con el de las llamadas
        qHandlerLineas[i] = mq_open(cLinea, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &atributos);
        if (qHandlerLineas[i] == (mqd_t)-1)
        { // controlamos si hay un error
            perror("No fue posible crear la cola de mensajes de las lineas");
            exit(EXIT_FAILURE);
        }
    }
}

// Instala un manejador de señales en el programa y muestra un mensaje de error si la operación falla.
void instalar_manejador_senhal()
{
    // utilizamos la llamada al sistema signal para asociar el metodo manejador_senhal a la señal SIGNIT
    // De esta manera podamos posteriormente realizar un apagado de emergencia con el comando Ctrl-C
    if (signal(SIGINT, manejador_senhal) == SIG_ERR)
    {
        fprintf(stderr, "[MANAGER] Error al instalar el manejador se senhal: %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    } // end if
} // end instalar_manejador_senhal
//---------------------------------------------------------------------------------------

// Apagado de Emergencia (Ctrl + C)
void manejador_senhal(int sign)
{
    printf("\n[MANAGER] Apagado de emergencia (Ctrl + C).\n");
    // al apagar todo tenemos que forzar la finalizacion de todos los procesos y luego liberar los recursos por ello llamamos a esos metodos
    terminar_procesos();
    liberar_recursos();
    exit(EXIT_SUCCESS);
} // end manejador_senhal
//-------------------------------------------------------------------------------------------

// inicializamos dos tablas de procesos una para las llamadas y otra para las lineas
void iniciar_tabla_procesos(int n_procesos_telefono, int n_procesos_linea)
{
    // Lo primero que hacemos es asignar memoria dinamica a esta tabla por medio de malloc
    // lo que le asignamos a cada tabla la calculamos por el Nº de procesos de lineas que tengamos por el tamaño a estructura "TProcess_t"
    g_process_telefonos_table = malloc(n_procesos_telefono * sizeof(struct TProcess_t));
    g_process_lineas_table = malloc(n_procesos_linea * sizeof(struct TProcess_t));

    // Ahora que ya hemos asigando la memoria a las tablas que de procesos que hemos creado, recorremos dichas tablas por un for
    // tabla lineas
    for (int i = 0; i < n_procesos_linea; i++)
    {
        // inicalizamos cada entrada a la tabla con el pid a cero ya que esta esta vacia de momento porque no hemos creado ningun proceso
        g_process_lineas_table[i].pid = 0;
    } // end for
    // tablas telefonos
    for (int i = 0; i < n_procesos_telefono; i++)
    {
        // inicalizamos cada entrada a la tabla con el pid a cero ya que esta esta vacia de momento porque no hemos creado ningun proceso
        g_process_telefonos_table[i].pid = 0;
    } // end for

} // end iniciar_tabla_procesos
//----------------------------------------------------------------------------------------------

// Este metodo se encarga de crear los procesos necesarios para las lineas y los telefonos
void crear_procesos(int numTelefonos, int numLineas)
{
    // para crear estos procesos lo que hacemos es crear un bucle for para asi llamar a la funcion lanzar proceso ya sea para linea o
    //  telefonos, la creacion de estos procesos dependera de el numero de lineas y telefonos que tengamos disponibles.
    printf("[MANAGER] %d lineas creadas.\n", numLineas);
    for (int i = 0; i < numLineas; i++)
    {
        lanzar_proceso_linea(i);
    } // end for
    // realizamos una impresion con las lineas totales que han sido creados

    printf("[MANAGER] %d telefonos creados.\n", numTelefonos);
    for (int i = 0; i < numTelefonos; i++)
    {
        lanzar_proceso_telefono(i);
    } // end for
    // realizamos una impresion con los telefonos totales que han sido creados

} // end crear_procesos
//-------------------------------------------------------------------------------------------------
void lanzar_proceso_telefono(const int indice_tabla)
{
    // creamos la varaible pid de tipo pid_t para poder alamcenar el PID correspondiente a cada proceso hijo
    pid_t pid;
    // Creamos un switch para controlar los difrenets casos cuando el fork nos de difernetes valores
    // declaramos que el pid sera igual al valor devuelto de la primitiva fork(), la cual es la que usamos para crear procesos hijos
    switch (pid = fork())
    {
    // en caso que sea -1 es que ha sucedido un error
    case -1:
        fprintf(stderr, "[MANAGER] Hemos encontrado un error al lanzar el proceso telefono: %s.\n", strerror(errno));
        // ya que hemos encontrado un error necesitamso termianr los procesos y liberar los recursos
        terminar_procesos();
        liberar_recursos();
        break;
    // Si devuelve 0, significa que se está ejecutando en el proceso hijo.
    case 0:
        // realizamos la primitiva execl para que el hijo ejecute otro proceso dirferente al del padre, en el caso de que de -1 se marca un eror
        // y nos imprime lo que ha sucedido.
        if (execl(RUTA_TELEFONO, CLASE_TELEFONO, NULL) == -1)
        {
            fprintf(stderr, "[MANAGER] Hemos encontrado un error usando execl () en el proceso %s: %s. \n", CLASE_TELEFONO, strerror(errno));
            exit(EXIT_FAILURE);
        } // end if
    }     // end switch-case
    // lo que hacemos posteriormente es actualizar la tabla de procesos con el PID del nuevo proceso lanzado y su clase en este caso clase telefono.
    g_process_telefonos_table[indice_tabla].pid = pid;
    g_process_telefonos_table[indice_tabla].clase = CLASE_TELEFONO;
} // end lanzar_proceso_telefono
//--------------------------------------------------------------------------------------------------

// hacemos exactamente lo mismo que lanzar_proceso_telefono solo que en este caso con las lineas
void lanzar_proceso_linea(const int indice_tabla)
{
    pid_t pid;
    char cLinea[TAMANO_MENSAJES];

    switch (pid = fork())
    {
    case -1:
        fprintf(stderr, "[MANAGER] Hemos encontrado un error al lanzar proceso línea: %s.\n", strerror(errno));
        terminar_procesos();
        liberar_recursos();
        exit(EXIT_FAILURE);
    case 0:
        sprintf(cLinea, "%s%d", BUZON_LINEAS, indice_tabla);
        if (execl(RUTA_LINEA, CLASE_LINEA, cLinea, NULL) == -1)
        {
            fprintf(stderr, "[MANAGER] Hemos encontrado un error usando execl() en el poceso %s: %s.\n", CLASE_LINEA, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    g_process_lineas_table[indice_tabla].pid = pid;
    g_process_lineas_table[indice_tabla].clase = CLASE_LINEA;
} // end lanzar_proceso_linea
//-----------------------------------------------------------------------------------------------------

// Este metodo se utiliza para esperar que los procesos creados por el programa terminen.
void esperar_procesos()
{
    int i;
    for (i = 0; i < NUMLINEAS; i++)
    {
        waitpid(g_process_lineas_table[i].pid, 0, 0);
    }
} // end esperar_procesos
//-------------------------------------------------------------------------------------------------------

// Este metodo se encarga de terminar los procesos enviadno una señal de SIGINT y asi terminarlo de manera ordenada
void terminar_procesos(void)
{
    printf("\n--- Finalizando procesos ---\n");
    // recorremos tosos los procesos lineas con el siguinte for
    terminar_procesos_especificos(g_process_lineas_table, g_lineasProcesses);
    terminar_procesos_especificos(g_process_telefonos_table, g_telefonosProcesses);
} // end terminar_procesos
//--------------------------------------------------------------------------------------------------------

// al igual lo que temrianr procesos este envia una señal de SIGNIT para finalizar un proceso solo que este caso sera un proceso en particular.
void terminar_procesos_especificos(struct TProcess_t *process_table, int process_num)
{
    for (int i = 0; i < process_num; i++)
    {
        printf("[MANAGER] Terminando proceso %s [%d]...\n", process_table[i].clase, process_table[i].pid);
        // Utilizamos la primitiva kill para matar al proceso especifico, ya que especificamos elindice de la tabla y el numero del proceso.
        // si en el caso de que kill sea -1, se amrca como un error y se notifica
        if (kill(process_table[i].pid, SIGINT) == -1)
        {
            fprintf(stderr, "[MANAGER] Hemos encontrado un error al usar kill() en proceso %d: %s.\n", process_table[i].pid, strerror(errno));
        }
        else
        {
            process_table[i].pid = 0;
        }
    }

} // end terminar_procesos_especificos
//--------------------------------------------------------------------------------------------------

// es imporatnte siempre liberar los recursos entonces este metodo se encarga de ello
void liberar_recursos()
{
    // utilizmaos la funcion free para liberar la memoria que antes hemos asignado con malloc
    free(g_process_telefonos_table);
    free(g_process_lineas_table);
    if(mq_close(qHandlerLlamadas)==-1){
        perror("mq_close");
        exit(EXIT_FAILURE);
    }
    if(mq_unlink(BUZON_LLAMADAS)==-1){
        perror("mq_unlink");
        exit(EXIT_FAILURE);
    }
    char cLinea[strlen(BUZON_LINEAS)+20];
    
    for (int i = 0; i < g_lineasProcesses; i++)
    {
        sprintf(cLinea, "%s%d", BUZON_LINEAS, i);
        if (mq_close(qHandlerLineas[i]) == -1) {
        perror("mq_close");
        exit(EXIT_FAILURE);
        }
        if (mq_unlink(cLinea)==-1){
        perror("mq_unlink");
        exit(EXIT_FAILURE);
        }
    }
} // end liberar_recursos
//--------------------------------------------------------------------------------------------------
