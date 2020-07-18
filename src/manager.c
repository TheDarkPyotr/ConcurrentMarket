#include "marketlib.h"


bool managerExit(void){


    if(conf->sigquit_set == 1 || conf->sighup_set == 1) {
        for (int i = 0; i < conf->K; i++) { 
            
            pthread_mutex_lock(&(mutexCashierClientBuffer[i]));
            pthread_cond_broadcast(&(waitingClientCashierBuffer[i]));
            pthread_mutex_unlock(&(mutexCashierClientBuffer[i]));
        }   

            pthread_mutex_lock(&(mutexClientManager));
            pthread_cond_broadcast(&(waitingClientManager));     
            pthread_mutex_unlock(&(mutexClientManager));
    }
    
    if(conf->sigquit_set == 0 && conf->sighup_set == 0) return true;
    
    return false;

}


void* decisionManager(void* args){
     struct timespec tim;
        tim.tv_sec = 10/1000;
        tim.tv_nsec = (10 % 1000) * 1000000;


    int* data = (int*)malloc(sizeof(int)*conf->K);
    

    for(int i = 0; i < conf->K; i++) data[i] = 0;

    while(managerExit()){
        


    for(int i = 0; i < conf->K; i++){

       if(nanosleep(&tim , &tim) < 0 )   
            {
                printf("Nano sleep system call fail \n");
                
            }

        
    pthread_mutex_lock(&(mutexCashierManager[i]));


            //Prelevo statistiche dal buffer
            cashierManagerMessage* t = NULL;
            data[i] = -1;
            if(bufferCashierManager[i] != NULL ){

                        data[i] = bufferCashierManager[i]->waitingClient;
                        t = bufferCashierManager[i];
                        bufferCashierManager[i] =  bufferCashierManager[i]->next;
                        free(t);
                        
                }

                

        pthread_mutex_unlock(&(mutexCashierManager[i]));


        }

        cashierAnalytics(data);
        
    }
    free(data);
    pthread_exit(0);
  


}



void cashierAnalytics(int* data){
    
    // Memorizzo lo status generale delle casse per questo "ciclo" di decisioni
    int* generalStatus = (int*)malloc(sizeof(int)*conf->K);
    for(int i = 0; i < conf->K; i++){

        pthread_mutex_lock(&(mutexCashierStatus[i]));
        generalStatus[i] = cashierStatus[i];
   
        pthread_mutex_unlock(&(mutexCashierStatus[i]));

    }

      
    //Conta i clienti in coda in ogni cassa
    for (int i = 0; i < conf->K; i++){   

        //Per tutte le casse aperte
        
        if(data != NULL){
            
            //Conto i clienti in coda in ogni cassa
            if(data[i] != -1){
                queueClient[i] = data[i];
               
            }
            
        }   

    }


    int people = 0;
    int cashierID = -1;
    bool lastOne = false;

    // Conto casse chiuse per non sforare limite di 1
    int final = 0;
    for(int i = 0; i < conf->K; i++) if(generalStatus[i] == -1) final++;

    if(final == conf->K-1) lastOne = true;

    for (int i = 0; i < conf->K; i++){  

        //Per tutte le casse aperte
            //Decido quale chiudere secondo il valore S1
            if(data[i] != -1 && generalStatus[i] == 1 && !lastOne){

                if(data[i] <= 1 && data[i] >= 0){
                    cashierID = i;
                    people++;
                    
                } 

      
            }
                
        }

   

    
    if(people >= conf->S1 && cashierID > -1  && conf->sighup_set == 0 && conf->sigquit_set == 0){

            
            pthread_mutex_lock(&(mutexCashierStatus[cashierID]));
                cashierStatus[cashierID] = 0;
            pthread_mutex_unlock(&(mutexCashierStatus[cashierID]));

            pthread_mutex_lock(&(mutexCashierClientBuffer[cashierID]));
            pthread_cond_broadcast(&(waitingClientCashierBuffer[cashierID]));
            pthread_mutex_unlock(&(mutexCashierClientBuffer[cashierID]));

          
            generalStatus[cashierID] = -1;


    } 


    people = 0;
    cashierID = -1;
    bool overflow = false;

    for (int i = 0; cashierID == -1 && i < conf->K; i++){ 

            //Per tutte le casse aperte
            //Controllo quale cassa ha almeno S2 clienti
            if(data[i] >= conf->S2 && data[i] != -1) overflow = true;
    }


    if(overflow){
    //########### RANDOM CASHIER
     cashierID = -1;
     bool open = false;

        int opened = 0;
        for(int i = 0; i < conf->K && !open; i++){

            cashierID = i;
            pthread_mutex_lock(&(mutexCashierStatus[i]));

                if(cashierStatus[i] == -1) {
                    opened++;
                open = true;
          
                }

            pthread_mutex_unlock(&(mutexCashierStatus[i]));

        }

    if(cashierID > -1 && cashierID < conf->K && generalStatus[cashierID] == -1   && conf->sighup_set == 0 && conf->sigquit_set == 0) {
      


                 if(cashierLauncher(cashierID) == -1) exit(EXIT_FAILURE);
                

            
        

        }
    }
    
    free(generalStatus);


}

int getFIFOQM(){

    int tot = 0;
   
               

        
        for(int i = 0; i < conf->C; i++){
            pthread_mutex_lock(&(mutexClientStatus[i]));

                    if(clientStatus[i] != -1){
                      

                        tot++;
                    } 

            pthread_mutex_unlock(&(mutexClientStatus[i]));
        }

          

    return tot;


}

void* manager(void* args){


    
    pthread_t threadDecisionManager;
    
    if(pthread_create(&threadDecisionManager,NULL,&decisionManager,(void*)NULL) != 0){

            fprintf(stderr,"ERROR (MANAGER): error while launching decision manager! ");
            exit(EXIT_FAILURE);
        }

    

    //
        while(conf->sighup_set == 0 && conf->sigquit_set == 0){

            pthread_mutex_lock(&(mutexClientManager));
            
            while(managerEmptyINBuffer == NULL && conf->sighup_set == 0 && conf->sigquit_set == 0 )
                    pthread_cond_wait(&(waitingClientManager), &(mutexClientManager));

            clientManagerMessage* temp = managerEmptyINBuffer;
            while(temp != NULL){

               

                temp->authorizedToExit = true;
                temp = temp->next;
                
            }
                 
            pthread_cond_broadcast(&(waitingClientManager));     
            pthread_mutex_unlock(&(mutexClientManager));

        }   


            pthread_detach(threadDecisionManager);
            
            pthread_exit(0);    
    
    
}