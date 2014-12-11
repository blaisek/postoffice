/**
 * Sherwood's post office
 * Charles Haarman, Blaise Tchimanga, Awa Thior
 * ITI 2 - 2014
 */


#define _GNU_SOURCE
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>


#define BANK_INIT_BALANCE 10000
#define NUM_CLIENTS 20
#define TRANS_TAXES 2
#define TRANS_MAX 50
#define TRANS_MIN 5
#define MAX_LOAN 200
#define MIN_GIFT 100
#define BEER_PRICE 5
#define STOLEN_CHANCE .5
#define NUM_COUNTERS 4
#define TIME_WALK_MIN 2
#define TIME_WALK_MAX 7
#define TIME_ROBIN_WORK_MIN 1
#define TIME_ROBIN_WORK_MAX 6
#define TIME_ROBIN_PAUSE_MIN 1
#define TIME_ROBIN_PAUSE_MAX 4
#define TIME_TRANS_MIN 1
#define TIME_TRANS_MAX 3
#define WEALTH_MIN 70
#define WEALTH_MAX 500


struct post_office_struct{
	int bankVal; // Money in the post office's bank.
	int numCounters; // Number of counters at the bank
	int numCountersTaken; // " " " that are taken
};
typedef struct post_office_struct post_office;

struct client_struct{
	int id; 
	int balance; // Account's balance
	int nb_trans; // Number of transactions made
	int ignore; // If he has made all his transactions
	post_office *post; // Shared post object
};
typedef struct client_struct client;

struct robin_struct{
	int balance; // Robin's account balance
	int keeps_going;
	int state; // 1 -> working | 2 -> pause
};
typedef struct robin_struct robin;



/**
 * Determines if Robin has stolen or not
 * @return [description]
 */
int steal(){
	srand(time(NULL));
	int factor = (int) 1/(double)STOLEN_CHANCE;
	int rnd = rand();
	int temp = rnd % factor;
	return temp;
}


/**
 * Client's routine
 * @param arg client's data
 */
void* client_routine(void *arg){
	client *data = arg;
	printf("I (client n°%i) have started with %i $ in ma pocket\n", data->id, data->balance);
	return NULL;
}

/**
 * Robin's routine
 * @param arg robin's data
 */
void* robin_routine(void *arg){

	robin *data = arg;

	printf("HEY, I AM ROBIN' YOU ! I HAVE %i$ ON ME :)\n", data->balance);

	int hasStolen = steal();

	printf("HAVE I STOLEN ? : %i\n", hasStolen);

	return NULL;

}




/**
 * Program's entry point
 * @param  argc Number of arguments
 * @param  argv Arguments array
 * @return      Exit code
 */
int main(int argc, char **argv){
	// Clients array
	pthread_t clients[NUM_CLIENTS];

	// Clients data array
	
	client *d[NUM_CLIENTS];

	// Robin's thread
	pthread_t robin_t;

	// Post office instanciation
	post_office *poste = malloc(sizeof(post_office));
	poste->bankVal = BANK_INIT_BALANCE;
	poste->numCounters = NUM_COUNTERS;
	poste->numCountersTaken = 0;

	// Robin init
	
	robin *rob = malloc(sizeof(robin));
	rob->balance = 0;
	rob->keeps_going = 1;
	rob->state = 1; // working 

	if(pthread_create(&robin_t, NULL, robin_routine, rob) != 0){
		printf("Something went wrong\n");
    	return EXIT_FAILURE;
	}
	pthread_join(robin_t, NULL);

	// We'll have to instanciate all clients and pass them to the routine. 
	
	srand(time(NULL));

	for(int i=0; i<NUM_CLIENTS; i++){
		int bal = ( rand() %  (WEALTH_MAX - WEALTH_MIN)) + WEALTH_MIN;
		d[i] = malloc(sizeof(client));
		d[i]->balance = bal;
		d[i]->id = i;
		d[i]->nb_trans = 0;
		d[i]->ignore = 0;
		if(pthread_create(&clients[i], NULL, client_routine, d[i]) != 0){
			printf("Something went wrong\n");
        	return EXIT_FAILURE;
		}
	}

	for (int j = 0; j < NUM_CLIENTS ; j++){
	    pthread_join(clients[j], NULL);
	}




	// Free clients memory
	for(int i=0;i<NUM_CLIENTS; i++){
		free(d[i]);
	}


	return EXIT_SUCCESS;


}