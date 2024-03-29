#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#define TRUE 1
#define MAX_CARS 5
#define NUM_ROUNDS 5

void *racerAction(void *arg);
void *boxesActions(void *arg);
void *judgeActions(void *arg);

void racerCreation();
void boxesCreation();
void judgeCreation();
void endRace();
void assignRacers();
void writeLogMessage(char *id, char *msg);

//DEFINED BY ARGUMENTS
int maxCars;
int maxBoxes;

int openBoxes;

int nRacer;
int racerNumber;
FILE *logFile;

//flags
int sanctionReceived;


//int boxesWaitList[maxCars] = {0,0,0,0,0};//Guarda la posición que ocupa el corredor en el array 'arrayCars' 
int *boxesWaitList;
//int **boxesWaitList;

pthread_cond_t sanctionNoticed = PTHREAD_COND_INITIALIZER;
pthread_cond_t sanctionEnded = PTHREAD_COND_INITIALIZER;
pthread_cond_t condRepared=PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutexRacers;//Para acceder a arrayCars
pthread_mutex_t mutexBoxes;//Para comunicarse ellos
pthread_mutex_t mutexBoxesList;
pthread_mutex_t mutexJudge;
pthread_mutex_t mutexVictory;//Para acceder a winnerRacer
pthread_mutex_t semaphore;


typedef struct boxParameters{
	int boxID;
	int racerPos;
	int attendedCars;
	//int *otherIsClosed;
	int isClosed;
}BoxParameters;

typedef struct racerParameters{
	int IDNumber;
	int posInArray;
	int sanctioned;
	int repared;//-1 no necesita ser reparado; 0 lo necesita; 1 ha sido reparado; 2 no ha sido posible repararlo
	int rounds;
	time_t initialT;
	time_t finalT;
	time_t totalT;
}RacerParameters;

//RacerParameters arrayCars[maxCars];
RacerParameters *arrayCars;
BoxParameters *arrayBoxes;
//RacerParameters **arrayCars;
//BoxParameters **arrayBoxes;
RacerParameters winnerRacer = {.totalT = 500};

int main(int argc, char *argv[]){
	struct sigaction sig;
	struct sigaction dinamicRacers;
	struct sigaction endSignal;
	
	pthread_mutex_init(&mutexRacers, NULL);
	pthread_mutex_init(&mutexBoxes, NULL);
	pthread_mutex_init(&mutexBoxesList,NULL);
	pthread_mutex_init(&mutexJudge, NULL);
	pthread_mutex_init(&mutexVictory,NULL);
	pthread_mutex_init(&semaphore,NULL);

	int *initialList;
	RacerParameters *initialRacers;
	BoxParameters *initialBoxes;
	openBoxes=0;
	nRacer=1;
	racerNumber=0;
	sanctionReceived=0;
	
	sig.sa_handler = racerCreation;
	if(sigaction(SIGUSR1,&sig,NULL)==-1){
		printf("Error: %s\n", strerror(errno));
	}

	dinamicRacers.sa_handler = assignRacers;
	if(sigaction(SIGUSR2,&dinamicRacers,NULL)==-1){
		printf("Error: %s\n", strerror(errno));
	}

	endSignal.sa_handler = endRace;
	if(sigaction(SIGINT,&endSignal,NULL)==-1){
		printf("Error: %s\n", strerror(errno));
	}	
	
	//boxesCreation();
	judgeCreation();	

	if (argc>=4){
		printf("To many params\n");

	}else if(argc==1){
		maxCars=5;
		maxBoxes = 2;
		boxesWaitList = (int*)malloc(maxCars*sizeof(int));
		arrayCars = (RacerParameters*)malloc(maxCars*sizeof(RacerParameters));
		arrayBoxes = (BoxParameters*)malloc(maxBoxes*sizeof(BoxParameters));
		/*initialList = (int*)malloc(maxCars*sizeof(int));
		initialRacers = (RacerParameters*)malloc(maxCars*sizeof(RacerParameters));
		initialBoxes = (BoxParameters*)malloc(maxBoxes*sizeof(BoxParameters));
		*boxesWaitList = initialList;
		*arrayCars = initialRacers;
		*arrayBoxes = initialBoxes;*/
	}else if(argc==2){//Pass max car 
		maxCars=atoi(argv[1]);
		maxBoxes=2;
		boxesWaitList = (int*)malloc(maxCars*sizeof(int));
		arrayCars = (RacerParameters*)malloc(maxCars*sizeof(RacerParameters));
		arrayBoxes = (BoxParameters*)malloc(maxBoxes*sizeof(BoxParameters));
		/*initialList = (int*)malloc(maxCars*sizeof(int));
		initialRacers = (RacerParameters*)malloc(maxCars*sizeof(RacerParameters));
		initialBoxes = (BoxParameters*)malloc(maxBoxes*sizeof(BoxParameters));
		*boxesWaitList = initialList;
		*arrayCars = initialRacers;
		*arrayBoxes = initialBoxes;*/
	}else if(argc==3){//PASS MAX CARS AND BOXES
		maxCars=atoi(argv[1]);
		maxBoxes=atoi(argv[2]);
		boxesWaitList = (int*)malloc(maxCars*sizeof(int));
		arrayCars = (RacerParameters*)malloc(maxCars*sizeof(RacerParameters));
		arrayBoxes = (BoxParameters*)malloc(maxBoxes*sizeof(BoxParameters));
		/*initialList = (int*)malloc(maxCars*sizeof(int));
		initialRacers = (RacerParameters*)malloc(maxCars*sizeof(RacerParameters));
		initialBoxes = (BoxParameters*)malloc(maxBoxes*sizeof(BoxParameters));
		*boxesWaitList = initialList;
		*arrayCars = initialRacers;
		*arrayBoxes = initialBoxes;*/
	}
	//usar el while para que el programa no acabe y poder mandar la señal(Carlos)
	while(TRUE){

	}
	free(arrayCars);
	free(arrayBoxes);
	return 0;
}

void assignRacers(){
	/*int *newList;
	RacerParameters *newRacers;
	BoxParameters *newBoxes;
	int newMaxRacers;

	scanf("%d",&newMaxRacers);
	maxCars = newMaxRacers;
	newList = (int*)malloc(newMaxRacers*sizeof(int));
	newRacers = (RacerParameters*)malloc(newMaxRacers*sizeof(RacerParameters));
	//newBoxes = (BoxParameters*)malloc(maxBoxes*sizeof(BoxParameters));
	*boxesWaitList = newList;
	*arrayCars = newRacers;
	//*arrayBoxes = initialBoxes;*/
	int newMaxRacers;
	int *listTmp;
	RacerParameters *carsTmp;

	scanf("%d",&newMaxRacers);
	pthread_mutex_lock(&mutexRacers);
	pthread_mutex_lock(&mutexBoxesList);
	pthread_mutex_lock(&mutexJudge);
	pthread_mutex_lock(&semaphore);
	maxCars = newMaxRacers;
	listTmp = (int *)realloc(boxesWaitList,newMaxRacers);
	carsTmp = (RacerParameters *)realloc(arrayCars,newMaxRacers);
	free(boxesWaitList);
	free(arrayCars);
	if(listTmp==NULL||carsTmp==NULL){
		printf("Error en la asignacion\n");
	}
	else{
		boxesWaitList = listTmp;
		arrayCars = carsTmp;
	}
	pthread_mutex_unlock(&mutexRacers);
	pthread_mutex_unlock(&mutexBoxesList);
	pthread_mutex_unlock(&mutexJudge);
	pthread_mutex_unlock(&semaphore);
}

void endRace(){
	char msg[256], racerNum[256];

	sprintf(msg,"Es el ganador de la carrera con un tiempo de %lu segundos",winnerRacer.totalT);
	sprintf(racerNum,"Corredor %d",winnerRacer.IDNumber);
	writeLogMessage(racerNum, msg);
	exit(0);
}

void boxesCreation(){
	int i,k;
	pthread_t box;
	for(i=0;i<maxBoxes;i++){
		if(arrayBoxes[i].boxID==0/*(*arrayBoxes)[i].boxID==0*/){
			openBoxes++;
			arrayBoxes[i].boxID = i+1;
			arrayBoxes[i].attendedCars=0;
			arrayBoxes[i].isClosed = 0;
			//paramsBox[i].otherIsClosed= (int*)malloc((maxBoxes-1)*sizeof(int*));
	
			pthread_create(&box, NULL, boxesActions, (void*)&arrayBoxes[i]);
		}
	}
}

void *boxesActions(void *arg){
	/*BoxParameters * params=(BoxParameters*)arg;
	char msgBox[256];
	int prob=0;
	srand(time(NULL));
	sprintf(msgBox,"Box %d",(params->boxID));
	for(;;){
		pthread_mutex_lock(&mutexBoxes);
		if(*boxesWaitList[0]!=0){
			params->racerPos=*boxesWaitList[0];
			int i=0;
			/*Desplaza la lista para que despues de atender
			 al primero el segundo sea el primero, el tercero el segundo, etc.*/
	/*		for(i=0;i<MAX_CARS-1;i++){
				boxesWaitList[i]=boxesWaitList[i+1];
				boxesWaitList[i+1]=0;
			}
			pthread_mutex_unlock(&mutexBoxes);
			//Dormir tiempo de atención, de  1 a 3 segundos aleatorio
			
			
			sleep((rand()%3) +1);
			//Comprobar si hay problemas. 70% no tiene, 30% ;sí-> si hay abandonar carrera
			prob=(rand()%10)+1;
			//mandar señal de terminar thread al racer
			
				pthread_mutex_lock(&semaphore);
				if(prob>=7){
					(*arrayCars)[params->racerPos].repared=2;
				}else{
					(*arrayCars)[params->racerPos].repared=1;
				}
				pthread_cond_signal(&condRepared);
				pthread_mutex_unlock(&semaphore);
			
			//Comprobar si hay que cerrar el box
			params->attendedCars++;
			if(params->attendedCars>=3){
				pthread_mutex_lock(&mutexBoxes);
				if(openBoxes>1){
					openBoxes--;
					params->isClosed=1;
					params->attendedCars=0;
					
				}

				pthread_mutex_unlock(&mutexBoxes);
			}
			if(params->isClosed==1){
				writeLogMessage(msgBox,"Se cierra temporalmente");
				sleep(20);
				params->isClosed=0;
				openBoxes++;
				
				writeLogMessage(msgBox,"Se abre");
			}

		}else{
			sleep(1);
		}	
		pthread_mutex_unlock(&mutexBoxes);
	}*/	
}

void racerCreation(){

	if(racerNumber<maxCars){
		int pos=0;		
		while(arrayCars[pos].IDNumber!=0){
			pos++;
		}
		arrayCars[pos].IDNumber = nRacer++;
		arrayCars[pos].posInArray = pos;
		arrayCars[pos].repared=-1;
		printf("%d\n",pos);
		
		pthread_t racer;
		pthread_create(&racer,NULL,racerAction,(void*)&arrayCars[pos]);
		
	}

}

void *racerAction(void *arg){
	char racerNum [256], msg [256];
	int probBoxes,i,random;
	RacerParameters *params = (RacerParameters *) arg;
	sprintf(racerNum,"Corredor %d",params->IDNumber);
	writeLogMessage(racerNum,"Entra al circuito");
	racerNumber++;
	probBoxes=0;
	i=0;
	random=0;
	srand(time(NULL));
	
	for(i=0;i<NUM_ROUNDS;i++){
		params->initialT=time(0);
		random=(rand()%4)+2;
		sleep(random);
		//Acaba la vuelta y mira si tiene que entrar a boxes 
		pthread_mutex_lock(&mutexBoxesList);
		probBoxes=rand()%10 ;
		/*if(probBoxes>5){
			
			params->repared=0;
			for(i=0;i<maxCars;i++){
				if(boxesWaitList[i]==0){
					(*boxesWaitList)[i]=params->posInArray;
					writeLogMessage(racerNum,"Entra en boxes");
					break;
				}
			}
			
		}*/
		pthread_mutex_unlock(&mutexBoxesList);
		pthread_mutex_lock(&semaphore);
		while(params->repared==0){
			pthread_cond_wait(&condRepared,&semaphore);
		}
		pthread_mutex_unlock(&semaphore);
		if(params->repared==2){
			//pthread_cancel(pthread_self);
		}

		//y si tiene sanción del Juez
		
		if(params->sanctioned==1){
			pthread_mutex_lock(&mutexJudge);
			printf("%d esperó por juez\n",params->IDNumber);
			sanctionReceived=1;
			pthread_cond_signal(&sanctionNoticed);
			while(params->sanctioned==1){
				pthread_cond_wait(&sanctionEnded,&mutexJudge);
			}
			pthread_mutex_unlock(&mutexJudge);	
		}
		
		
		params->finalT=time(0);
		/*roundTime = params->finalT-params->initialT;
		params->totalT = params->totalT + roundTime;*/
		params->totalT += (params->finalT-params->initialT);
		params->rounds++;
		sprintf(msg,"Completa la vuelta número %d en %lu segundos",(params->rounds),/*(roundTime)*/(params->finalT-params->initialT));
		writeLogMessage(racerNum,msg);
	}
	params->sanctioned = -1;
	pthread_mutex_lock(&mutexVictory);
	if(params->totalT<winnerRacer.totalT){
		winnerRacer.totalT = params->totalT;
		winnerRacer.IDNumber = params->IDNumber;
	}
	pthread_mutex_unlock(&mutexVictory);
	pthread_mutex_lock(&mutexRacers);
	arrayCars[params->posInArray].IDNumber=0;
	arrayCars[params->posInArray].sanctioned = 0;
	arrayCars[params->posInArray].rounds = 0;
	arrayCars[params->posInArray].initialT = 0;
	arrayCars[params->posInArray].finalT = 0;
	arrayCars[params->posInArray].totalT = 0;
	arrayCars[params->posInArray].posInArray = 0;
	racerNumber--;
	/*arrayCars[params->posInArray].sanctioned = 0;
	arrayCars[params->posInArray].IDNumber = 0;
	arrayCars[params->posInArray].rounds = 0;*/
	pthread_mutex_unlock(&mutexRacers);
		
}


void judgeCreation(){
	pthread_t judge;
	//pthread_attr_t attrJudge;
	//pthread_attr_init(&attrJudge);
	//pthread_attr_setdetachstate(&attrJudge,PTHREAD_CREATE_JOINABLE);
	pthread_create(&judge,NULL,judgeActions,NULL);
	
}


void *judgeActions(void *arg){
	//Sancionar cada 10 seg
	char msg [256];
	int sanctionedRacer=0;
	srand(time(NULL));

	for(;;){
		sleep(10);
		
		if(racerNumber!=0){
			pthread_mutex_lock(&mutexRacers);
			do{
				sanctionedRacer = rand()% maxCars;
			}while(arrayCars[sanctionedRacer].IDNumber==0||arrayCars[sanctionedRacer].sanctioned==-1);
			pthread_mutex_unlock(&mutexRacers);
		/*while(arrayCars[sanctionedRacer].IDNumber==0){
			sanctionedRacer = rand()%racerNumber;
		}*/
			
			pthread_mutex_lock(&mutexJudge);
			arrayCars[sanctionedRacer].sanctioned=1;		
			sprintf(msg,"Sanciona al corredor %d",arrayCars[sanctionedRacer].IDNumber);
			writeLogMessage("Juez",msg);
			while(sanctionReceived==0){
				pthread_cond_wait(&sanctionNoticed,&mutexJudge);
			}
			sleep(3);
			sanctionReceived=0;
			arrayCars[sanctionedRacer].sanctioned=0;
			pthread_cond_signal(&sanctionEnded);
			pthread_mutex_unlock(&mutexJudge);
		}
	}

}


void writeLogMessage(char *id, char *msg) {
	time_t now = time (0);
	struct tm *tlocal = localtime (&now);
	char stnow [50];
	strftime(stnow , sizeof(stnow), "%Y-%m-%d %H:%M:%S ", tlocal);
	logFile = fopen("registroTiempos.log" , "a");
	fprintf(logFile , "[ %s] %s: %s\n", stnow , id, msg);
	fclose(logFile);
	//"%d/ %m/ %y %H:%M:%S"
}
