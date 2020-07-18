#include "marketlib.h"
#include <time.h>
#define BILLION  1E9



bool pushFIFOQueue(int clientID, int cashierID){

    //essendo in recinto di M.E. inserisco il
    //clientID nella riga cashierID colonna clientID
    int pos = 0;


    if(!changeCashier[cashierID]){

        while(IDFIFOQueue[cashierID][pos] != -1 && pos < conf->C) pos++;
        IDFIFOQueue[cashierID][pos] = clientID;
        if(pos == 0) seenFIFOHead[cashierID] = false;
         
        return false;
    } 

    return true;
  



}

bool writeCashierBuffer(clientCashierMessage* data, int cashierID){

  
  bool change = false;
    
        if(data != NULL && bufferClientCashier[cashierID] == NULL){
                    if(bufferClientCashier[cashierID] == NULL){

                        bufferClientCashier[cashierID] = data;
                    } 
                
        } 
        return change;
   
}


int myTurn(int clientID, int cashierID){

    int code = -1;
   

    pthread_mutex_lock(&(mutexCashierClientFIFO[cashierID]));


    if(IDFIFOQueue[cashierID][0] == clientID && !seenFIFOHead[cashierID]){
        
        code = 1;
        seenFIFOHead[cashierID] = true;
        
    } else code = -1;

    if(changeCashier[cashierID]){

        code = 0;

    } 

        pthread_mutex_unlock(&(mutexCashierClientFIFO[cashierID]));

    return code;


}

int randomCasherSelect(int clientID){
    long times = time(NULL);

    int cashierID = -1;
    unsigned int seedCashier = times+clientID;

//############## Generazione randomica cassa aperta a cui accodarsi
        bool open = false;

            while(!open && conf->sighup_set == 0 && conf->sigquit_set == 0){

                cashierID = rand_r(&seedCashier)%(conf->K)+cashierID;

                   if(cashierID >= conf->K) cashierID = 0;

                if(cashierID >= 0 && cashierID < conf->K){
                pthread_mutex_lock(&(mutexCashierStatus[cashierID]));
                      
                    if(cashierStatus[cashierID] == 1){
                        open = true;
                    } 
                  
                    
                pthread_mutex_unlock(&(mutexCashierStatus[cashierID]));
            }


        }

    if(cashierID < 0 || cashierID >= conf->K) cashierID = -1;

return cashierID;

}


bool authExit(int clientID){

    
    if(managerEmptyINBuffer != NULL){
         if(managerEmptyINBuffer->clientID == clientID && managerEmptyINBuffer->authorizedToExit) return true;
         else return false;
    } 

    return false;
}


void* client(void* args){


   

    // Calculate time taken by a request
    struct timespec marketStart, marketEnd; //Tempo totale market
    struct timespec queueStart, queueEnd; //Tempo totale in coda

    clock_gettime(CLOCK_REALTIME, &marketStart);
    
   

    time_t start;
    time(&start);
  
    //Client status (ACTIVE 1, WAITING: 0, EXIT: -1)
    int clientID = ((int)args);
    unsigned int seedBuyTime = clientID+start;
    unsigned int seedCashier = clientID+start;
    int randomTime = 0;
    int cause = -2; 
    bool changeCashier = true;
    
    while((randomTime = rand_r(&seedBuyTime)%conf->T < 10));
    
    //Client passa < T Nanosecondi nel market
    struct timespec tim = {(randomTime/1000),((randomTime%1000)*1000000)};
    
   if(nanosleep(&tim , NULL) < 0 )   
   {
      printf("CLIENT %d: Nano sleep call failed \n", clientID);
      pthread_exit((void*)1);
   } 

    //Generazione casuale prodotti acquistati
    int buyedObj = rand_r(&seedCashier)%(conf->Nproducts);
   
    
  
 
   

    //Cliente con almeno un prodotto si mette in coda presso una cassa
    if(buyedObj > 0 && conf->sighup_set == 0 && conf->sigquit_set == 0){


            int* timeBuyedProducts = (int*)malloc(sizeof(int)*buyedObj); 
            for(int i = 0; i < buyedObj; i++)
            timeBuyedProducts[i] += conf->products[rand_r(&seedBuyTime)%(conf->Nproducts)];

       

         
            int cashierID = -1;

            clientCashierMessage* data = (clientCashierMessage*)malloc(sizeof(clientCashierMessage));
                    data->NbuyedProduct = buyedObj;
                    data->buyedProducts = timeBuyedProducts;
                    data->clientID = clientID;
                    data->marketTotalTime = 0; //#TIME DALL'INIZIO DEL THREAD
                    data->queueTotalTime = 0; //#TIME DA QUANDO È IN CODA A QUANDO È USCITO
                    data->changeCashier = true;
                    data->next = NULL;


    while(changeCashier == true && conf->sigquit_set == 0 && conf->sighup_set == 0 ){





     
       while((cashierID = randomCasherSelect(clientID)) == -1 && conf->sigquit_set == 0)


        pthread_mutex_lock(&(mutexCashierClientFIFO[cashierID]));
                 clock_gettime(CLOCK_REALTIME, &queueStart);
                //Iscrizione cliente clientID presso la coda FIFO della cassa cashierID
                if(conf->sighup_set == 0 && conf->sigquit_set == 0) {
                    if(!pushFIFOQueue(clientID, cashierID)) changeCashier = false;
                } else changeCashier = false;
                
        
           


        pthread_cond_broadcast(&(waitingClientCashierFIFO[cashierID]));        
        pthread_mutex_unlock(&(mutexCashierClientFIFO[cashierID]));




        pthread_mutex_lock(&(mutexCashierClientBuffer[cashierID]));
     
            if(!changeCashier && conf->sighup_set == 0){
            

                //Cliente attende finché non è in testa alla FIFO o finché non deve cambiare cassa
                while((conf->sigquit_set == 0 && (cause = myTurn(clientID, cashierID)) == -1))
                    pthread_cond_wait(&(waitingClientCashierBuffer[cashierID]), &(mutexCashierClientBuffer[cashierID]));
                
                clock_gettime(CLOCK_REALTIME, &queueEnd);

                if(cause == 1 && conf->sigquit_set == 0){


                             //Comunico al cassiere tempo totale in coda, tempo nel market e lista acquisti
                            writeCashierBuffer(data, cashierID);
                        
                            data->changeCashier = false;
                            emptyClientCashierBuffer[cashierID] = false;
                } 
                else if(cause == 0) {
                    changeCashier = true;
                    cause = 0;
              
                }

            } else if(changeCashier && conf->sighup_set == 0){
               
                cause = 0;
            } 
            
         
                    
   

            pthread_cond_broadcast(&(waitingClientCashierBuffer[cashierID]));        
            pthread_mutex_unlock(&(mutexCashierClientBuffer[cashierID]));
    }


       
        clock_gettime(CLOCK_REALTIME, &marketEnd);
        data->marketTotalTime = (marketEnd.tv_sec - marketStart.tv_sec )+(marketEnd.tv_nsec - marketStart.tv_nsec ) / BILLION;
       if(conf->sigquit_set == 0 && conf->sighup_set == 0) data->queueTotalTime = (queueEnd.tv_sec - queueStart.tv_sec )+(queueEnd.tv_nsec - queueStart.tv_nsec ) / BILLION;

                           
       

        //Aggiornamento dati cassa prima della chiusura
        clientCashierMessage* t = NULL;
        if(logDataClient[clientID] == NULL){
                 logDataClient[clientID] = data;
                 t =  logDataClient[clientID];
               

        }
        else {
            t =  logDataClient[clientID];
            t->NbuyedProduct += data->NbuyedProduct;
            t->queueTotalTime += data->queueTotalTime;
            t->marketTotalTime += data->marketTotalTime;
        }

        
           
         pthread_mutex_lock(&(mutexClientStatus[clientID]));

            clientStatus[clientID] = -1;
            
    pthread_mutex_unlock(&(mutexClientStatus[clientID]));
    
    fprintf(stdout, "Client %d  EXIT (CASHIER ID %d)\n",clientID, cashierID);
    pthread_exit(0);

       
    } else {

            //Se cliente con acquisti = 0

        pthread_mutex_lock(&(mutexClientManager));

            clientManagerMessage* outMessage = (clientManagerMessage*)malloc(sizeof(clientManagerMessage));

            outMessage->authorizedToExit = false;
            outMessage->clientID = clientID;
            outMessage->next = NULL;

            clientManagerMessage* temp = managerEmptyINBuffer;
            if(temp != NULL){  

                    //Scrivo sul buffer del Manager per autorizzazione uscita
                     while(temp->next != NULL) temp = temp->next;
                     temp->next = outMessage;


            } else managerEmptyINBuffer = outMessage;

            pthread_cond_broadcast(&(waitingClientManager));     
            pthread_mutex_unlock(&(mutexClientManager));

             pthread_mutex_lock(&(mutexClientManager));

            //Attendo che il Manager autorizzi uscita  
            while(!authExit(clientID) && conf->sigquit_set == 0 ) pthread_cond_wait(&(waitingClientManager), &(mutexClientManager));

            temp = managerEmptyINBuffer;
          
            managerEmptyINBuffer = managerEmptyINBuffer->next;
              free(temp);
          
            


            pthread_cond_broadcast(&(waitingClientManager));     
            pthread_mutex_unlock(&(mutexClientManager));

    

             pthread_mutex_lock(&(mutexClientStatus[clientID]));

            clientStatus[clientID] = -1;
            
    pthread_mutex_unlock(&(mutexClientStatus[clientID]));
    

    pthread_exit(0);

            




    }
    
    

    

}