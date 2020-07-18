#include <stdlib.h>
#include <errno.h>
#include "marketlib.h"

int tempVar = 0;



int cashierLauncher(int cashierID){


    cashierStatus[cashierID] = 1;
    changeCashier[cashierID] = false;
    cashierExit[cashierID] = -1;
    for(int i = 0; i < conf->C; i++) {
        
        IDFIFOQueue[cashierID][i] = -1;


    }
     emptyClientCashierBuffer[cashierID] = true;
    if(cashierID >= 0 && cashierID < conf->K){
    if(pthread_create(&threadCashier[cashierID], NULL,&cashier,(void*)cashierID) != 0){
        

        fprintf(stderr,"ERROR (CASHIER %d): error while launching!\n", cashierID);
        cashierStatus[cashierID] = -1;
           

        return -1;
        

    } 
       
    } else {
        cashierStatus[cashierID] = -1;
    return -1;
    }
    
     
     return 0;

}

int clientLauncher(int clientID){

    int status = 0;

    pthread_mutex_lock(&(mutexClientStatus[clientID]));
            status = clientStatus[clientID];
    pthread_mutex_unlock(&(mutexClientStatus[clientID]));

   if(status == -1){

    

    if(pthread_create(&threadClient[clientID],NULL,&client,(void*)clientID) != 0){

        fprintf(stderr,"ERROR (CLIENT %d): error while launching! with statu EAGAIN %d \n", clientID,  EAGAIN);
        clientStatus[clientID] = -1;
        return -1;
        

    } else{

            pthread_mutex_lock(&(mutexClientStatus[clientID]));
                clientStatus[clientID] = 1;
            pthread_mutex_unlock(&(mutexClientStatus[clientID]));

            int error = pthread_detach(threadClient[clientID]);
            if(error)  fprintf(stdout, "DETACH ERROR CLIENT %d\n", clientID);

            return 0;
        } 
   }

   return -1;
    

}

void* accessControl(void* args){

      struct timespec tim;
        tim.tv_sec = 100/1000;
        tim.tv_nsec = (100 % 1000) * 1000000;

    int index = 0, totalFree = 0;
    unsigned int seed = 0;
    int* IDInactiveClient = (int*)malloc(sizeof(int)*conf->C);
  
    while(conf->sighup_set == 0 && conf->sigquit_set == 0){
        
        int activeClient = 0;
         if(nanosleep(&tim , &tim) < 0 )   
            {
                printf("Nano sleep system call fail \n");
                
            }


     
        
        totalFree = 0;
        for(int i = 0; i < conf->C; i++) IDInactiveClient[i] = -1;

            for(int clientID = 0; clientID < conf->C; clientID++){

                pthread_mutex_lock(&(mutexClientStatus[clientID]));
                    if(clientStatus[clientID] != -1){
                    
                        activeClient++;

                    }  else  {
                        
                        IDInactiveClient[clientID] = clientID;
                        totalFree++;
                    }
                pthread_mutex_unlock(&(mutexClientStatus[clientID]));

            }

        
            
                   
            while(activeClient <= (conf->C) - (conf->E) && totalFree > 0 && (conf->sighup_set == 0 && conf->sigquit_set == 0)){
                index = 0;
               
                
                
                    index = rand_r(&seed)%(conf->C);
                    if(IDInactiveClient[index] != -1) {
                        
                    
                    if(clientLauncher(index) == -1) exit(EXIT_FAILURE);
                        else {
                        IDInactiveClient[index] = -1;
                        activeClient++;
                        totalFree--;
                        
                        }
                        
                        
                  }

            }
}

free(IDInactiveClient);
pthread_exit(0);

}

void lineParser(char* line){

    int i = 0;
    
   
        if(line[i] == 'P' && atoi(line+i+2) > 0) 
            conf->P = atoi(line+i+2);

        else  if(line[i] == 'K' && atoi(line+i+2) > 0) 
            conf->K = atoi(line+i+2);

        else  if(line[i] == 'C' && atoi(line+i+2) > 0) 
            conf->C = atoi(line+i+2);

        else  if(line[i] == 'T' && atoi(line+i+2) > 0) 
            conf->T = atoi(line+i+2);

        else  if(line[i] == 'E' && atoi(line+i+2) > 0) 
            conf->E = atoi(line+i+2);

         else  if(line[i] == 'W' && atoi(line+i+2) > 0) 
            conf->W = atoi(line+i+2);

        else if(line[i] == 'S'){

            if(line[i+1] == ' ' && atoi(line+i+2) > 0){ 
                conf->S = atoi(line+i+2);
            } else  if(line[i+1] == '1' && atoi(line+i+2) > 0){ 
                conf->S1 = atoi(line+i+2);
            }  if(line[i+1] == '2' && atoi(line+i+2) > 0){ 
                conf->S2 = atoi(line+i+2);
            }

        }


        else if(line[i] == 'N'){
            
            if(line[i+1] == ' ' && atoi(line+i+2) > 0){ 
                conf->Nproducts = atoi(line+i+2);
                conf->products = (int*)malloc(sizeof(int)*conf->Nproducts);
                for(int j = 0; j < conf->Nproducts; j++) conf->products[j] = 1;
                tempVar = 0;
            } else if(conf->products != NULL && atoi(line+i+2) > 0){ 
                conf->products[tempVar] = atoi(line+i+2);
                tempVar++;

            } 

        }


    

}
bool readConfiguration(void){



    FILE* file = fopen("config.txt", "r"); 
    char line[256];

    if(file){

        while (fgets(line, sizeof(line), file)) 
            lineParser(line);
        

        fclose(file);
        conf->sighup_set = 0;
        conf->sigquit_set = 0;
         fprintf(stdout, "K: %d\n", conf->K);
        fprintf(stdout, "C: %d\n", conf->C);
        fprintf(stdout, "T: %d\n", conf->T);
        fprintf(stdout, "E: %d\n", conf->E);
        fprintf(stdout, "P: %d\n", conf->P);
        fprintf(stdout, "S: %d\n", conf->S);
        fprintf(stdout, "S1: %d\n", conf->S1);
        fprintf(stdout, "S2: %d\n", conf->S2);
        for(int i = 0; i < conf->Nproducts; i++) fprintf(stdout, "Products %d of %d:    %d\n", i, conf->Nproducts, conf->products[i]);
        fprintf(stdout, "SIGHUP: %d\n", conf->sighup_set);
        fprintf(stdout, "SIGQUIT: %d\n", conf->sigquit_set);

        if(conf->E > 0 && conf->K > 0 && conf->P > 0 && conf->Nproducts > 0 && conf->S1 > 0 && conf->S2 > 0 && conf->S > 0 &&
        conf->T > 0 && conf->W > 0) return true;
        else return false;
        
    }

    return false;
   
}

bool logFile(){


    //Creazione file log a partire da
    //  logDataClient //C DIM
    //   cashierLogData //K Dim

    for(int i = 0; i < conf->K; i++){
              if(cashierLogData[i] != NULL)  cashierLogData[i]->averageServiceTime = cashierLogData[i]->openTotalTime / cashierLogData[i]->clientNumber;

    }

    FILE *fp;
    if((fp = fopen ("data.txt", "w"))){

 
        for(int i = 0; i < conf->C; i++)
            if(logDataClient[i] != NULL) fprintf(fp, "Client ID: %d | Prodotti acquistati: %d | Tempo totale: %.3lf | Tempo in coda: %.3lf\n",logDataClient[i]->clientID, logDataClient[i]->NbuyedProduct, logDataClient[i]->marketTotalTime, logDataClient[i]->queueTotalTime);

        for(int i = 0; i < conf->K; i++)
            if(cashierLogData[i] != NULL) fprintf(fp, "Cashier ID: %d | Prodotti elaborati: %d | Numero clienti: %d | Tempo apertura: %.3lf | Tempo medio servizio: %.3lf | Numero chiusure: %d \n",cashierLogData[i]->cashierID, cashierLogData[i]->NelaboratedProducts, cashierLogData[i]->clientNumber, cashierLogData[i]->openTotalTime, cashierLogData[i]->averageServiceTime, cashierLogData[i]->closureTime+1);
        
        fclose(fp);

        return true;
    }

    return false;
    
}

static void gestore(int signum) {
    
    if (signum == 1) conf->sighup_set = 1; //ID Sighup = 1
    
    if (signum == 3) conf->sigquit_set = 1; //ID Sigquit = 3

   
    fprintf(stdout, "Segnale ricevuto: %d\n",signum); 
}

void memFree(void){
        
//##### FREE CASSIERE
       free(threadCashier);
       free(cashierStatus);

        for (int i = 0; i < conf->K; i++){
                int* t = IDFIFOQueue[i];
                free(t);
            }
        free(IDFIFOQueue);

        free(changeCashier);
        free(mutexCashierStatus);
        free(waitingCashierStatus);
        free(mutexCashierClientFIFO);
        free(waitingClientCashierFIFO);
        free(mutexCashierClientBuffer);
        free(waitingClientCashierBuffer);
        free(mutexCashierManager);
        free(waitingCashierManager);

           for (int i = 0; i < conf->K; i++){
                clientCashierMessage* t =  bufferClientCashier[i];
               if(t != NULL) free(t);
            }

        free(bufferClientCashier);

        free(emptyClientCashierBuffer);
        free(seenFIFOHead);


           for (int i = 0; i < conf->K; i++){
                cashierManagerMessage* t =  bufferCashierManager[i];
                free(t);
            }
        free(bufferCashierManager);

        
//#### FREE CLIENTE

    //Array TID Clienti
    free(threadClient);
    free(mutexClientStatus);
    free(clientStatus);

    
//FREE LOGFILE

     for (int i = 0; i < conf->C; i++){
                clientCashierMessage* t =  logDataClient[i];
               if(t != NULL) free(t);
            }

    free(logDataClient);

     for (int i = 0; i < conf->K; i++){
                cashierDataLog* t = cashierLogData[i];
                if(t != NULL) free(t);
            }
    free(cashierLogData);

//####FREE MANAGER

    free(queueClient);
    free(cashierExit);


//#### FREE CONFIG
  free(conf->products);
  free(conf);


}



int main(int argc, char const *argv[])
{

//#### CONFIGURAZIONE
    conf = (config*)malloc(sizeof(config));

    //Lettura dal file di configurazione
    if(readConfiguration()){
    
//########################################################################## CASHIER
//ALLOCAZIONI

    //Array TID Cassieri
    threadCashier = (pthread_t*)malloc(sizeof(pthread_t)*conf->K);

    //Array status cassieri
    cashierStatus = (int*)malloc(sizeof(int)*conf->K);

   
    //Code FIFO (K Cassieri)
    IDFIFOQueue = (int **)malloc(sizeof(int*)*(conf->K));
     
    for(int i = 0; i < conf->K; i++) IDFIFOQueue[i] = (int*)malloc(sizeof(int)*(conf->C));

    //Status cambio cassa (K cassieri)
    changeCashier = (bool *)malloc(conf->K*sizeof(bool));

    mutexCashierStatus = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*conf->K);
    waitingCashierStatus = (pthread_cond_t*)malloc(sizeof(pthread_cond_t)*conf->K);

    mutexCashierClientFIFO = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*conf->K);
    waitingClientCashierFIFO = (pthread_cond_t*)malloc(sizeof(pthread_cond_t)*conf->K);//K Dimension

    mutexCashierClientBuffer = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*conf->K);
    waitingClientCashierBuffer = (pthread_cond_t*)malloc(sizeof(pthread_cond_t)*conf->K);

    mutexCashierManager = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*conf->K);
    waitingCashierManager = (pthread_cond_t*)malloc(sizeof(pthread_cond_t)*conf->K);

    //Buffer cassiere-cliente
    bufferClientCashier = (clientCashierMessage **)malloc(conf->K * sizeof(clientCashierMessage*));

    //Status buffer cassiere-cliente
    emptyClientCashierBuffer = (bool*)malloc(sizeof(bool)*conf->K);

    //Status CODA FIFO
    seenFIFOHead = (bool*)malloc(sizeof(bool)*conf->K);

    // Buffer cassiere-manager
    bufferCashierManager = (cashierManagerMessage **)malloc(conf->K * sizeof(cashierManagerMessage*));

// INIT

    //Init fifo queue
    for(int i = 0; i < conf->K; i++) {
        for(int j = 0; j < conf->C; j++){
            IDFIFOQueue[i][j] = -1;

        } 
    }

    for(int i = 0; i < conf->K; i++) seenFIFOHead[i] = false;

     //Init client/cashier buffer pointer
    for(int i = 0; i < conf->K; i++) bufferCashierManager[i] = NULL;
    //Init client/cashier buffer pointer
    for(int i = 0; i < conf->K; i++) emptyClientCashierBuffer[i] = true;

    for(int i = 0; i < conf->K; i++) changeCashier[i] = false;

    //Init client/cashier buffer pointer
    for(int i = 0; i < conf->K; i++) bufferClientCashier[i] = NULL;
    //Init cashier status
    for(int i = 0; i < conf->K; i++) cashierStatus[i] = -1;

    //Init cond. var for client/cashier FIFO
    for(int i = 0; i < conf->K; i++) pthread_cond_init(&waitingCashierStatus[i],NULL);

    //Init cond. var for client/cashier FIFO
    for(int i = 0; i < conf->K; i++) pthread_cond_init(&waitingClientCashierFIFO[i],NULL);

    //Init cond. var for client/cashier FIFO
    for(int i = 0; i < conf->K; i++) pthread_mutex_init(&mutexCashierClientFIFO[i],NULL);

    //Init mutex for client/manager buffer 
    for(int i = 0; i < conf->K; i++) pthread_mutex_init(&mutexCashierClientBuffer[i],NULL);

       //Init mutex on cashier status
    for(int i = 0; i < conf->K; i++) pthread_mutex_init(&mutexCashierStatus[i],NULL);

    //Init cond. var for client/manager buffer 
    for(int i = 0; i < conf->K; i++) pthread_cond_init(&waitingClientCashierBuffer[i],NULL);

        //Init mutex for client/manager buffer 
    for(int i = 0; i < conf->K; i++) pthread_mutex_init(&mutexCashierManager[i],NULL);

    //Init cond. var for client/manager buffer 
    for(int i = 0; i < conf->K; i++) pthread_cond_init(&waitingCashierManager[i],NULL);

        //Init cond. var for cashier/manager buffer
    for(int i = 0; i < conf->K; i++) pthread_cond_init(&waitingCashierManager[i],NULL);

//########################################################################## CLIENTE
//ALLOCAZIONI

    //Array TID Clienti
    threadClient = (pthread_t*)malloc(sizeof(pthread_t)*conf->C);

    //Array status clienti
    clientStatus = (int*)malloc(sizeof(int)*conf->C);

    mutexClientStatus = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*conf->C);

   
//INIT 
    //Init mutex for client/manager buffer 
    for(int i = 0; i < conf->C; i++) pthread_mutex_init(&mutexClientStatus[i],NULL);

    //Init status
    for(int i = 0; i < conf->C; i++) clientStatus[i] = -1;



//#### ALLOCAZIONI LOGFILE
    //Dati clienti
    logDataClient = (clientCashierMessage **)malloc(conf->C*sizeof(clientCashierMessage*));

    //Dati cassieri
    cashierLogData = (cashierDataLog **)malloc(conf->K*sizeof(cashierDataLog*));
//#### INIT LOGFILE
    for(int i = 0; i < conf->C; i++)  logDataClient[i] = NULL;

     for(int i = 0; i < conf->K; i++)  cashierLogData[i] = NULL;

//########################################################################## MANAGER
// ALLOCAZIONI
    queueClient = (int*)malloc(sizeof(int)*conf->K);
    cashierExit = (int*)malloc(sizeof(int)*conf->K);

// INIT
    //Init cond. var for client/manager buffer (exiting from market)
    pthread_mutex_init(&mutexClientManager,NULL);
    pthread_cond_init(&waitingClientManager,NULL);
    for(int i = 0; i < conf->K; i++) queueClient[i] = 0;
    for(int i = 0; i < conf->K; i++) cashierExit[i] = -1;
    managerEmptyINBuffer = NULL;




    struct sigaction s;

    memset(&s,0,sizeof(s));
    s.sa_handler = gestore;

    if(sigaction(SIGHUP,&s,NULL) == -1) fprintf(stderr,"#SIGHUP ERROR: Gestore fallito\n");
    
    if(sigaction(SIGQUIT,&s,NULL) ==- 1) fprintf(stderr,"#SIGQUIT ERROR: Gestore fallito\n");
    

    //Manager launcher
    int param = 1;
   if(pthread_create(&threadManager,NULL,&manager,(void*)&param) != 0){

            fprintf(stderr,"ERROR (MANAGER): error while launching manager! ");
            exit(EXIT_FAILURE);
    }
      
            //Create K thread cashier
    for(int i = 0; i < conf->K; i++){

        //Cashier launcher (and setting status)
        if(cashierLauncher(i) == -1) exit(EXIT_FAILURE);

    }  


     //Create C thread client
    for(int i = 0; i < conf->C; i++){
        
        srand(time(0)); 
        if(clientLauncher(i) == -1) exit(EXIT_FAILURE);

    }  

     

 
    

        if(pthread_create(&threadAccessControl,NULL,&accessControl,(void*)&param) != 0){

            fprintf(stderr,"ERROR (CONTROLLER): error while launching access control! ");
            exit(EXIT_FAILURE);
    }

  
    
    
    pthread_join(threadAccessControl, NULL);

    pthread_join(threadManager,NULL);

    
    fprintf(stdout, "USCITI TUTTI\n");
    
    logFile();
   
   memFree();

    return 0;

    } else return -1;
}
