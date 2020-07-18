#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>


typedef struct config_t {
    int K;
    int C;
    int T;
    int E;
    int P;
    int S;
    int S1;
    int S2;
    int W;
    int Nproducts;
    int* products;
    volatile sig_atomic_t sighup_set;
    volatile sig_atomic_t sigquit_set;
} config;


    //Manager's TID
    pthread_t threadManager;

    //Thread per il controllo accessi al market (valore soglia < C-E)
    pthread_t threadAccessControl;

    //TID array of cashiers
    pthread_t* threadCashier; //K Dimension

    //TID array of clients
    pthread_t* threadClient; //C Dimension

    //Cashier status (CLOSE: -1, OPEN: 1, EXITING: 0)
    int* cashierStatus; //K Dimension

    //Client status (OFF: -1, WAITING: 1, EXITING 2)
    int*  clientStatus; //C Dimension

    //FIFO Queue of clients IDs to manage cashier scheduling thread
    //Accessed in M.E. with mutexCashierClient mutex and waitingClientCashier cond var

    typedef struct fifoq_t{
        int clientID;
        bool seen;
    } fifoq;

    int** IDFIFOQueue; //Matrix K rows C columns
    
    int* HeadFIFOQueue; //K Dimension

    pthread_mutex_t* mutexCashierStatus; //K Dimension
    pthread_cond_t* waitingCashierStatus; //K Dimension

    pthread_mutex_t* mutexClientStatus; //K Dimension

    //GESTIONE SEGNALI SIGQUIT E SIGHUP
    


    int randomCasherSelect(int use);
    void cashierAnalytics(int* data);

    //M.E. for client/cashier FIFO queue
    pthread_mutex_t* mutexCashierClientFIFO; //K Dimension
    pthread_cond_t* waitingClientCashierFIFO; //K Dimension

    //M.E. for client/cashier buffer
    pthread_mutex_t* mutexCashierClientBuffer; //K Dimension
    pthread_cond_t* waitingClientCashierBuffer; //K Dimension
    pthread_cond_t* waitingClientCashierAB; //K Dimension

    //M.E. for cashier/manager buffer
    pthread_mutex_t* mutexCashierManager; //K Dimension
    pthread_cond_t* waitingCashierManager; //K Dimension

    //M.E. for client/manager buffer (exiting from market)
    pthread_mutex_t mutexClientManager; //Client-Manager comunication (exit) 
    pthread_cond_t waitingClientManager; //Client-Manager comunication (exit)


    struct timespec diff(struct timespec start, struct timespec end);

    //Client/cashier message structure
    typedef struct clientCashierMessage_t {

                int clientID; //ID del cliente da servire
                int NbuyedProduct; //Numero prodotti acquistati
                double marketTotalTime; //Tempo passato nel supermercato
                double queueTotalTime; //Tempo passato in coda
                bool changeCashier;
                int* buyedProducts;

                struct clientCashierMessage_t* next;

        } clientCashierMessage;

    //Pointer to head of client/cashier buffer
    clientCashierMessage* *bufferClientCashier; //K Dimension

    bool* allowedClientCashierFIFO; //K Dimension
    bool* emptyClientCashierBuffer; //K Dimension
    bool* seenFIFOHead; //K Dimension


    //Cashier/manager message structure
    typedef struct cashierManagerMessage_t {

                //Numero clienti in attesa alla casa
                //Se -1 è indirizzata al cassiere che deve chiudere la cassa
                int waitingClient; 
                bool new;
                struct cashierManagerMessage_t* next;

        } cashierManagerMessage;
        
    //Pointer to head of cashier/manager buffer
    cashierManagerMessage** bufferCashierManager; //K Dimension

    //Client/manager message structure
    typedef struct clientManagerMessage_t {
                
                //Richiesta di uscita del cliente con P = 0 al manager
                int clientID; 
                bool authorizedToExit; //1 se autorizzato, 0 altrimenti
                struct clientManagerMessage_t* next;

        } clientManagerMessage;

    //Pointer to head of client/manager buffer
    clientManagerMessage* bufferClientManager;
    clientManagerMessage* managerEmptyINBuffer;

    //La prima riga indica le casse da chiudere (se true, chiudere)
    //La seconda riga indica le casse da aprire (se true, aprire)
     bool** cashierProgram;

     int* queueClient; //K dimension
     bool* changeCashier; //K dimension
    int* cashierExit; //K dimension

    clientCashierMessage** logDataClient; //C Dimension

    typedef struct cashierDataLog_t {

        int cashierID; //ID cassiere
        int NelaboratedProducts; //Numero prodotti acquistati
        int clientNumber; //Tempo passato nel supermercato
        float openTotalTime; //Tempo passato in coda
        double averageServiceTime;
        int closureTime;

                struct clientCashierMessage_t* next;

    } cashierDataLog;

    cashierDataLog** cashierLogData;



//###### Gestione mutex generali

void Pthread_mutex_lock(pthread_mutex_t *mtx);

int Pthread_mutex_unlock(pthread_mutex_t *mtx);

//######## FILE DI CONFIGURAZIONE

config* conf; //Contiene i valori globali di K,C,P,T,S1,S2, etc

bool readConfiguration();

bool logFile();

//GESTIONE MARKET

void* accessControl(void* args);

//######## GESTIONE CASSE

void* cashier(void* args);

int cashierLauncher(int cashierID);

int checkExitCashier(int cashierID);

clientCashierMessage* servingClient(int cashierID);

void managerMessageCenter(int cashierID);


//######## GESTIONE CLIENTI

void* client(void* args);

bool pushFIFOQueue(int clientID, int cashierID);

bool writeCashierBuffer(clientCashierMessage* data, int cashierID);

int myTurn(int clientID, int cashierID);

//######## GESTIONE MANAGER

void* manager(void* args);








//##################################################






