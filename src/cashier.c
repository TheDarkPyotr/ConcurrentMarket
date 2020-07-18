#include "marketlib.h"
#define BILLION  1E9
      bool emptyQueue = false;

     

clientCashierMessage* servingClient(int cashierID){

    
    clientCashierMessage* ret = NULL;

    if(bufferClientCashier[cashierID] != NULL){
        
    
        ret = bufferClientCashier[cashierID];

    
            if(bufferClientCashier[cashierID]->next != NULL)  bufferClientCashier[cashierID]=bufferClientCashier[cashierID]->next;
            else bufferClientCashier[cashierID] = NULL;

        }

         emptyClientCashierBuffer[cashierID] = true;

    return ret;     

    
}


void managerMessageCenter(int cashierID){


     struct timespec tim;
     //Ogni 50 msec comunica
        tim.tv_sec = 0;
        tim.tv_nsec = (conf->W % 1000) * 1000000;

    
    int queuedClients = 0;

    nanosleep(&tim , &tim);
    //NANOSLEEP


    pthread_mutex_lock(&(mutexCashierClientFIFO[cashierID]));

       
        for(int i = 0; i < conf->C; i++) {
            
            if(IDFIFOQueue[cashierID][i] != -1) queuedClients++;
        }
       

        
    pthread_cond_broadcast(&(waitingClientCashierFIFO[cashierID]));        
    pthread_mutex_unlock(&(mutexCashierClientFIFO[cashierID]));
       

    cashierManagerMessage* localData = (cashierManagerMessage*)malloc(sizeof(cashierManagerMessage));
    localData->waitingClient = queuedClients;
    localData->new = true;
    localData->next = NULL;


    
    
    pthread_mutex_lock(&(mutexCashierManager[cashierID]));
            
     
       if(bufferCashierManager[cashierID] != NULL){
       
               free(bufferCashierManager[cashierID]);
               bufferCashierManager[cashierID] = localData;
               

         
       } else bufferCashierManager[cashierID] = localData;

       pthread_cond_signal(&(waitingCashierManager[cashierID]));        
        pthread_mutex_unlock(&(mutexCashierManager[cashierID]));
    

    
}



int checkStatus(int cashierID){

   int temp = -2;

    pthread_mutex_lock(&(mutexCashierStatus[cashierID]));

        temp = cashierStatus[cashierID];
              
    pthread_mutex_unlock(&(mutexCashierStatus[cashierID]));


    return temp;

}


void clientProcess(int cashierID){

       unsigned int seed  = cashierID;
        long rtime = 0;

        cashierDataLog* cashierLocalData = NULL;
        cashierLocalData = (cashierDataLog*)malloc(sizeof(cashierDataLog));
        cashierLocalData->cashierID = cashierID;
        cashierLocalData->NelaboratedProducts = 0;
        cashierLocalData->clientNumber = 0;
        cashierLocalData->closureTime = 0;
        cashierLocalData->averageServiceTime = 0;
        cashierLocalData->next = NULL;
        cashierLocalData->openTotalTime = 0;


        struct timespec tim, tim2;

    clientCashierMessage* temp = servingClient(cashierID);

    if(temp != NULL){ 
                        
                 
                        cashierLocalData->NelaboratedProducts += temp->NbuyedProduct;
                        cashierLocalData->clientNumber++;

                        //attesa tempo tra 80 e 20 + tempo per prodotti clienti
                        int servingTime = 0;
                        for(int i = 0; i < temp->NbuyedProduct; i++)servingTime += temp->buyedProducts[i];

                        while ((rtime = rand_r(&seed) % 80) < 20);

                        tim.tv_nsec = servingTime+rtime; 

                        nanosleep(&tim , &tim2);
                       
                                

                        free(temp->buyedProducts);
                    }


                managerMessageCenter(cashierID);

        
        //Aggiornamento dati cassa prima della chiusura
        if(cashierLogData[cashierID] == NULL) cashierLogData[cashierID] = cashierLocalData;
        else {


            cashierLogData[cashierID]->clientNumber += cashierLocalData->clientNumber;
            cashierLogData[cashierID]->NelaboratedProducts += cashierLocalData->NelaboratedProducts;
            free(cashierLocalData);
        }



}


int updateFIFO(int cashierID){

       
    int index = 0;
    int queuedClients = 0;
                for(int i = 0; i < conf->C; i++) if(IDFIFOQueue[cashierID][i] != -1) queuedClients++;

                if(seenFIFOHead[cashierID]){
                    while(index < (conf->C)-1){
                                
                                
                            IDFIFOQueue[cashierID][index] = IDFIFOQueue[cashierID][index+1];
                            IDFIFOQueue[cashierID][index+1] = -1;
                                index++;
                               
                    }

                    seenFIFOHead[cashierID] = false;
                    
                   
                     for(int i = 0; i < conf->C-1; i++) 
                        for(int j = i+1; j <= conf->C-1; j++)
                        if(IDFIFOQueue[cashierID][i] == IDFIFOQueue[cashierID][j]) 
                                IDFIFOQueue[cashierID][j] = -1;

                }  



    return queuedClients;

}

bool checkManagerDecision(int cashierID){



    bool lastOne = false;
    pthread_mutex_lock(&(mutexCashierStatus[cashierID]));
        
    
        if(cashierStatus[cashierID] == 0){ 
            lastOne = true;
       
       }
        
               

    pthread_mutex_unlock(&(mutexCashierStatus[cashierID]));

 
    return lastOne;
                        
            


}

int getFIFOQ(int cashierID){

    int tot = 0;
     pthread_mutex_lock(&(mutexCashierClientFIFO[cashierID]));

               

        
        for(int i = 0; i < conf->C; i++){
            pthread_mutex_lock(&(mutexClientStatus[i]));

                if(IDFIFOQueue[cashierID][i] != -1)
                    if(clientStatus[i] != -1) tot++;

            pthread_mutex_unlock(&(mutexClientStatus[i]));
        }

            pthread_cond_broadcast(&(waitingClientCashierFIFO[cashierID]));        
            pthread_mutex_unlock(&(mutexCashierClientFIFO[cashierID]));

    return tot;


}


void* cashier(void* args){

       

        //Cashier status (CLOSE: -1, OPEN: 1, EXITING: 0)
        int cashierID = (int*)args;
        struct timespec cashierStart, cashierEnd; //Tempo totale apertura cassa
        clock_gettime(CLOCK_REALTIME, &cashierStart);
    

        int localStatus = 1;
        bool lastOne = false;
     

       
        while(checkStatus(cashierID) == 1 && conf->sigquit_set == 0){
            
            
            while(!lastOne && localStatus == 1 && conf->sigquit_set == 0){
                lastOne = checkManagerDecision(cashierID);
                localStatus = checkStatus(cashierID);

            pthread_mutex_lock(&(mutexCashierClientBuffer[cashierID]));
            while((checkStatus(cashierID) == 1) && (emptyClientCashierBuffer[cashierID]) && conf->sigquit_set == 0 
            && getFIFOQ(cashierID) > 0 && conf->sighup_set == 1 && (checkStatus(cashierID) == 1) && (emptyClientCashierBuffer[cashierID]) ){

                    pthread_cond_wait(&(waitingClientCashierBuffer[cashierID]), &(mutexCashierClientBuffer[cashierID]));
                }

           
              if(!emptyClientCashierBuffer[cashierID] && conf->sigquit_set == 0){

                    clientProcess(cashierID);

              }  
              
              
            pthread_mutex_lock(&(mutexCashierClientFIFO[cashierID]));

               if(lastOne){ changeCashier[cashierID] =  true;

            
                 }
                updateFIFO(cashierID);
       
            pthread_cond_broadcast(&(waitingClientCashierFIFO[cashierID]));        
            pthread_mutex_unlock(&(mutexCashierClientFIFO[cashierID]));

          
            pthread_cond_broadcast(&(waitingClientCashierBuffer[cashierID]));        
            pthread_mutex_unlock(&(mutexCashierClientBuffer[cashierID]));

           
            if(lastOne || (getFIFOQ(cashierID) <= 0 && conf->sighup_set == 1)){
                     pthread_mutex_lock(&(mutexCashierStatus[cashierID]));
                            cashierStatus[cashierID] = -1;
                            localStatus = -1;
                            lastOne = true;
                    pthread_mutex_unlock(&(mutexCashierStatus[cashierID]));
            clock_gettime(CLOCK_REALTIME, &cashierEnd);
            if(cashierLogData[cashierID] != NULL){
            cashierLogData[cashierID]->openTotalTime += (cashierEnd.tv_sec - cashierStart.tv_sec )+(cashierEnd.tv_nsec - cashierStart.tv_nsec ) / BILLION;
             cashierLogData[cashierID]->closureTime++;
            } else {

                cashierLogData[cashierID] = (cashierDataLog*)malloc(sizeof(cashierDataLog));
                 cashierLogData[cashierID]->cashierID = cashierID;
                  cashierLogData[cashierID]->clientNumber = 0;
                   cashierLogData[cashierID]->closureTime = 0;
                   cashierLogData[cashierID]->NelaboratedProducts = 0;
                   cashierLogData[cashierID]->openTotalTime = (cashierEnd.tv_sec - cashierStart.tv_sec )+(cashierEnd.tv_nsec - cashierStart.tv_nsec ) / BILLION;

        }
                fprintf(stdout, "CASHIER %d EXIT\n", cashierID);
                pthread_exit(0);
            }
            
        }

         
    }
    
        clock_gettime(CLOCK_REALTIME, &cashierEnd);
        if(cashierLogData[cashierID] != NULL) {
     
        cashierLogData[cashierID]->openTotalTime += (cashierEnd.tv_sec - cashierStart.tv_sec )+(cashierEnd.tv_nsec - cashierStart.tv_nsec ) / BILLION;
        cashierLogData[cashierID]->closureTime++;
        } else {

                cashierLogData[cashierID] = (cashierDataLog*)malloc(sizeof(cashierDataLog));
                 cashierLogData[cashierID]->cashierID = cashierID;
                  cashierLogData[cashierID]->clientNumber = 0;
                   cashierLogData[cashierID]->closureTime = 0;
                   cashierLogData[cashierID]->NelaboratedProducts = 0;
                   cashierLogData[cashierID]->openTotalTime = (cashierEnd.tv_sec - cashierStart.tv_sec )+(cashierEnd.tv_nsec - cashierStart.tv_nsec ) / BILLION;

        }
        fprintf(stdout, "CASHIER %d EXIT\n", cashierID);
        pthread_exit(0);
    


}