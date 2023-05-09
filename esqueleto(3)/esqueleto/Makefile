DIROBJ := obj/
DIREXE := exec/
DIRHEA := include/
DIRSRC := src/

CFLAGS := -I$(DIRHEA) -c -Wall -ggdb
LDFLAGS := -lpthread -lrt
CC := gcc

#all : dirs manager barbero cliente
all : dirs manager telefono linea

dirs:
	mkdir -p $(DIROBJ) $(DIREXE)

manager: $(DIROBJ)manager.o 
	$(CC) -o $(DIREXE)$@ $^ $(LDFLAGS)

telefono: $(DIROBJ)telefono.o 
	$(CC) -o $(DIREXE)$@ $^ $(LDFLAGS)

linea: $(DIROBJ)linea.o 
	$(CC) -o $(DIREXE)$@ $^ $(LDFLAGS)


$(DIROBJ)%.o: $(DIRSRC)%.c
	$(CC) $(CFLAGS) $^ -o $@

clean : 
	rm -rf *~ core $(DIROBJ) $(DIREXE) $(DIRHEA)*~ $(DIRSRC)*~
