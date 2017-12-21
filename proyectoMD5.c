//Juan Navas v25573433
//Daniel Cavadia v26122903
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>

# define NumSem 6

sem_t *semaforo[NumSem];
char *opcion;
int countDuplicados = 0, flag = 0;
char *miDirectorio;
pthread_mutex_t lock;
pthread_mutex_t lock2;
pthread_mutex_t lock3;


typedef struct directorio{
	char *nombre;
	char *ruta;
	int check;  // 0 si no ha sido procesado, 1 si ya se proceso 
}Directorio;

typedef struct aVisitar{
	Directorio dir;
	struct aVisitar *next;
}aVisitar;

aVisitar *firstA,*lastA;

typedef struct Visitados{
	char *nombre;
	char *hash;
	struct Visitados *next;
}Visitados;

Visitados *firstV,*lastV;

typedef struct Duplicados
{
	char *nombre1;
	char *nombre2;
	struct Duplicados *next;
}Duplicados;

Duplicados *firstD, *lastD;

void setAVisitarFirst(Directorio dir){

	aVisitar *Auxiliar=malloc(sizeof(struct aVisitar));

	Auxiliar->dir=dir;
	Auxiliar->next=NULL;

	if( firstA == NULL) {
		firstA=Auxiliar;
		lastA=Auxiliar;
	}

}

void llenarVisitados(char *nombre, char *hash){    // Se guarda el archivo con su hash en Visitados
	sem_wait(semaforo[4]);

	struct Visitados *aux;
			
	aux = malloc(sizeof(struct Visitados));
	aux->nombre = nombre;
	aux->hash = hash;
			
	if ( flag == 0){	
		aux->next = NULL;
		firstV = aux;
		lastV = firstV;
		flag++;
	}
	else{
		lastV->next = aux;
		lastV = aux;
		lastV->next=NULL;		
	}

	sem_post(semaforo[4]);
}

void llenarDuplicados(char *nombre1, char *nombre2){    // Se guarda el archivo con su hash en Visitados

	sem_wait(semaforo[5]);

	Duplicados *Auxiliar=malloc(sizeof(struct Duplicados));
	Auxiliar->nombre1=nombre1;
	Auxiliar->nombre2=nombre2;
	Auxiliar->next=NULL;

	if( firstD == NULL) {
		firstD=Auxiliar;
		lastD=Auxiliar;
	}else{
		lastD->next=Auxiliar;
		lastD=Auxiliar;
	}
	sem_post(semaforo[5]);
}
void error(const char *s)
{
  /* perror() devuelve la cadena S y el error (en cadena de caracteres) que tenga errno */
  perror (s);
  exit(EXIT_FAILURE);
}


void checkDuplicados(){
	Visitados *check1, *check2;
	
	check1=firstV;	

	while(check1->next != NULL){

		check2=check1->next;		
		while(check2 != NULL){
				
			if( strcmp(check1->hash, check2->hash) == 0){
					llenarDuplicados(check1->nombre, check2->nombre);
					countDuplicados++;
			}
			check2=check2->next;	
		
		}
		check1=check1->next;
	}
	
}

void procesarArchivo(char *ruta, char *nombre){
	char *nombreCopy = (char*)malloc(sizeof(char)*500);
	memcpy(nombreCopy, nombre, sizeof(char)*500);  

	if( strcmp(opcion, "e") == 0){ // Modo ejecutable
		
		char hashValueE[33];
		char *argv[3]={"./md5", ruta};
		int f, retval, estado;
		char *hashAux;
		hashAux = (char *) calloc(33, sizeof(char));
		sem_wait(semaforo[1]);		
		pid_t p;
		p = fork();
		if (p == 0){ //hijo
						
			execv(argv[0],argv);
			
			kill(p, SIGTERM);
			
		}else if(p > 0){//padre
			
			f=open("myfifo",O_RDONLY);
			retval=read(f, hashAux, sizeof(hashValueE));
			close(f);
			//printf("%s\n", hashAux);
			
			llenarVisitados(nombreCopy, hashAux);
				
		}
		wait(0);
	sem_post(semaforo[1]);
	}else if( strcmp(opcion, "l") == 0){   // Modo libreria

		char *hashValue;
		hashValue = (char *) calloc(33, sizeof(char));	
		
		int h = MDFile(ruta, hashValue);
	
		if (h == 1){   // si MDFile() es exitoso
			sem_wait(semaforo[2]);
			
			llenarVisitados(nombreCopy, hashValue);
			sem_post(semaforo[2]);
		}
	}
}

void procesarDirectorioAVisitar(char *nombre, char *ruta){    // Se guardan los directorios a visitar en la structura aVisitar

	if( strcmp(nombre,".") != 0 && strcmp(nombre,"..") != 0 ) // No se visitaran los directorios '.' y '..'
	{

		struct aVisitar *Auxiliar=malloc(sizeof(struct aVisitar));
		Auxiliar->dir.nombre=nombre;
		Auxiliar->dir.ruta=ruta;
		Auxiliar->dir.check=0;
		Auxiliar->next=NULL;

		lastA->next=Auxiliar;
		lastA=Auxiliar;
	}	
}

void recorrerDirectorio(char *nombre){

	DIR *dir;
    struct dirent *ent;
    char aux[255];

    if (!(dir = opendir(nombre)))
        return;

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_DIR) { // Si es un directorio

            char path[1024];
            char *pathCopy = (char*)malloc(sizeof(char)*500);

            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;

            snprintf(path, sizeof(path), "%s/%s", nombre, ent->d_name);
        
            memcpy(pathCopy, path, sizeof(char)*500);   

            procesarDirectorioAVisitar(ent->d_name, pathCopy);

            recorrerDirectorio(path);

        } 
    }
    closedir(dir);

}


void procesarDirectorios(char *ruta){

	DIR *dir;
    struct dirent *ent;
    char aux[255];

    if (!(dir = opendir(ruta))){
    	printf("Error al abrir directorio\n");
        return;
    }

    while ((ent = readdir(dir)) != NULL) {

    	if(ent->d_type == DT_REG) {     // Si es un archivo
    			memset(aux, 0, 1);
                strcat(aux, ruta);  // Para obtener la ruta relativa del archivo y guardarla en aux
                if (aux[strlen(aux)-1] != '/') strcat(aux, "/");
                strcat(aux, ent->d_name);                

                procesarArchivo(aux, ent->d_name);
    	}
    }

    closedir(dir);
}

void *administrador(void* n){

	int *m = (int*)n;

	aVisitar *auxiliar;

	auxiliar = firstA;

	 while( auxiliar != NULL ){

		sem_wait(semaforo[3]);

	 	if(auxiliar->dir.check == 0){     // Si el directorio no ha sido procesado
			//printf("hilo %d se le asigna dir: \n", m[0]);
		     	auxiliar->dir.check = 1;  // Marca de directorio procesado
			sem_post(semaforo[3]);
			procesarDirectorios(auxiliar->dir.ruta);//SE SACA DEL SEM PARA PERMITIR PARALELISMO DE HILOS	 	
		}else{
			sem_post(semaforo[3]); 	
		}
		auxiliar = auxiliar->next;
	 }
	 pthread_exit(0);
}

int main(int argc, char **argv)
{

	int numHilos,i;
	char c;
	char *tvalue = NULL, *mvalue = NULL, *dvalue = NULL;

	if(  argc == 7 ) { 

		while ((c = getopt (argc, argv, "m:t:d:")) != -1){    // Lee los argumentos en cualquier orden
		    switch (c)
		      {
		      case 'm':
		        mvalue = optarg;

		        if( *mvalue != 'e' && *mvalue != 'l') {
		          printf("Opcion de -m no valida\n");
		          return 0;
		        }        
		        break;
		      case 't':
		        tvalue = optarg;
		        break;
		      case 'd':
		        dvalue = optarg;
		        break;
		      case '?':
		        break;
		      default:
		        abort ();
		      }
		}
		numHilos= atoi(tvalue);
		if(numHilos<1){
			printf("Se necesita como minimo un hilo\n");
			return 0;
		}
		Directorio directorioIni;

		directorioIni.nombre = dvalue;
		directorioIni.ruta = dvalue;
		directorioIni.check = 0;

		setAVisitarFirst(directorioIni);   // Se inicializa aVisitar con el directorio

		opcion = mvalue;

		recorrerDirectorio(firstA->dir.nombre);  // Se buscan todos los directorios a procesar
		
		//-------- Semaforos ------------------------------------------


		char *nombre;

		for( i = 0; i < NumSem; i++){

			nombre = (char*)malloc(strlen("mutex")+1);
	    	snprintf(nombre,strlen("mutex")+1, "mutex");
			sem_unlink(nombre);
	    	semaforo[i] = sem_open(nombre, O_CREAT, S_IRWXU, 1);

		}
  		

		//------------ Hilos -------------------------------------------------

		int hiloID[1];
		pthread_t hiloSeek[numHilos];
		for(i=0;i<numHilos;i++)
		{
			hiloID[0]=i;			
			pthread_create( &hiloSeek[i], NULL, &administrador, (void*)(hiloID));
			usleep(1000);
		}
		for(i=0;i<numHilos;i++)
		{
			pthread_join(hiloSeek[i],NULL);
		}
		checkDuplicados();

		// -------------------------------------------------------------

		 //printf("Numero de hilos %d\n", numHilos);

		Duplicados *Auxiliar3;

		Auxiliar3=firstD;
	
		
		printf("Se han encontrado %d archivos duplicados\n", countDuplicados);


		while(Auxiliar3 != NULL){
			printf("%s es duplicado de %s\n", Auxiliar3->nombre2, Auxiliar3->nombre1);
			Auxiliar3=Auxiliar3->next;
		}


		for(i=0; i<NumSem; i++){
			sem_close(semaforo[i]);
		}
   	    sem_unlink(nombre);
	}
	else {
		
		printf("Argumentos incorrectos\n");
		printf("%s\n", argv[1]);
		
	}

	return 0;
}
