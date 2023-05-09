#include <sys/types.h>

// CLASES y PATHS
#define CLASE_TELEFONO "TELEFONO"
#define RUTA_TELEFONO "./exec/telefono"
#define CLASE_LINEA "LINEA"
#define RUTA_LINEA "./exec/linea"

// Dispositivos
#define NUMLINEAS 10    // /proc/sys/fs/mqueue/msg_max
#define NUMTELEFONOS 3

// Colas Mensajes
#define BUZON_LLAMADAS "/buzon_llamadas"
#define BUZON_LINEAS   "/buzon_linea_"
#define TAMANO_MENSAJES 64

// Lineas
#define FIN_CONVERSACION "Finalizado"

// Procesos
struct TProcess_t
{
  pid_t pid;
  char *clase;
};
