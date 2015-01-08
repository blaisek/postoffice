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
#include <unistd.h>
#include <semaphore.h>

/**
 * DEFINES
 */
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
#define NUM_TRANSACTIONS 8
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
#define ANSI_RESET  "\u001B[0m"
#define ANSI_RED "\u001B[31m"
#define ANSI_GREEN "\u001B[32m"
#define ANSI_YELLOW "\u001B[33m"
#define ANSI_CYAN "\u001B[36m"

/**
 * Semaphores / mutex
 */
sem_t mutex; // Mutex on post office bank account
sem_t counters; // Post office counters

/**
 * Structures
 */

struct post_office_struct{
	int bankVal; // Money in the post office's bank.
};
typedef struct post_office_struct post_office;

struct client_struct{
	int id; 
	int balance; // Account's balance
	int outside; // Client is vulnerable to Robin
	int emprunt; // Amount loaned by the client
};
typedef struct client_struct client;

struct robin_struct{
	int balance; // Robin's account balance
	int state; // 0 -> working | 1 -> pause
};
typedef struct robin_struct robin;



// Clients array
pthread_t clients[NUM_CLIENTS];

// Clients data array
client *d[NUM_CLIENTS];

// Robin's thread
pthread_t robin_t;

// The post office
post_office *poste;

// Global variable which will be decremented everytime a client has done all his transactions
int clients_still_there;

// Global variable which will contain the amount of money distributed at the beginning
int given;

/**
 * Determines if Robin has stolen or not
 * @return [description]
 */
int steal(){
	srand( time( NULL ) );
	int factor = (int) 1/(double)STOLEN_CHANCE;
	int rnd = rand();
	int temp = rnd % factor;
	return temp;
}

/**
 * Generates a random number between min and max
 * @param  min int:minimum
 * @param  max int:maximum
 * @return     int: a random number between min and max
 */
int getRand(int min, int max){
	srand( time( NULL ) );
	int ret = ( rand() %  ( max - min ) ) + min;
	return ret;	
}

/**
 * Function that performs a transaction between 2 clients
 * @param from   int: Source client id
 * @param to     int: Destination client id
 * @param amount int: amount given
 */
void transaction(int from, int to, int amount){

	/**
	 * We cannot give money to ourselves, so in that case let's give it to the one after that
	 */
	
	if(from == to && to < (NUM_CLIENTS - 1)) to++;
	if(from == to && to >= NUM_CLIENTS) to--;

	printf("client n°%i has %i in his account\n", to, d[to]->balance);
	/**
	 * Source client (d[from]) gives `amount` to Destination client plus 2 as fees
	 * Destination client will only receive `amount`.
	 */
	d[from]->balance = d[from]->balance - (amount + 2);
	d[to]->balance = d[to]->balance + amount;

	printf("client n°%i has %i in his account after payment\n", to, d[to]->balance);

	/**
	 * Increment the bank value of the post office to take taxes into account
	 */
	sem_wait(&mutex);
	poste->bankVal = poste->bankVal + 2;
	sem_post(&mutex);

}


/**
 * Function which performs a loan from the bank to a client
 * @param  to     Destination client ID
 * @param  amount Amount loaned
 * @return        0 if loan was successful, else 1
 */
int loan(int to, int amount){

	if( d[to]->emprunt + amount >= 200 ){
		/**
		 * Client has already loaned to much to the bank
		 */
		return 1;
	} else {
		/**
		 * Client can loan
		 */
		printf("%i is left in the bank\n", poste->bankVal);
		printf( "\x1b[1;33mA client has loaned %i at the bank \x1b[0m\n", amount);

		/**
		 * Remove amount from bank
		 */

		sem_wait(&mutex);
		poste->bankVal = poste->bankVal - amount;
		sem_post(&mutex);


		printf("%i is left in the bank\n", poste->bankVal);

		/**
		 * Give it to the client and record his loan
		 */

		d[to]->balance = d[to]->balance + amount;
		d[to]->emprunt = d[to]->emprunt + amount;
		return 0;
	}
}

/**
 * When a client goes outside the post office, hence is vulnerable to Robinhood
 * @param id Client id
 */
void take_a_walk(int id){
	/**
	 * Notify that he's vulnerable
	 */
	d[id]->outside = 1;
	int wtime = getRand(TIME_WALK_MIN*1000000, TIME_WALK_MAX*1000000);

	/**
	 * Waits and goes back into the bank or home
	 */
	usleep(wtime);
	d[id]->outside = 0;
}

/**
 * A client at the counter
 * @param cdata The client's data
 */
void go_to_counter(void *cdata){
	client *data = cdata;

	/**
	 * Client is no longer outside, so no longer vulnerable
	 */

	data->outside = 0;

	/**
	 * Start of critical section, only NUM_COUNTERS can run this code
	 */

	sem_wait(&counters); // Decrement semaphore

	printf("I am client n°%i and I am at the counter\n", data->id);

	/**
	 * Generate time, amount and client values
	 */

	int sltime = getRand(TIME_TRANS_MIN*1000000, TIME_TRANS_MAX*1000000);
	int amount = getRand(5,50);
	int other = getRand(0, NUM_CLIENTS);


	if(data->balance >= (amount + 2)){
		/**
		 * Client has enough money to make the transaction
		 */
		printf("I have enough money, I will give %i to client n°%i\n", amount, other );
		transaction(data->id, other, amount); // Makes the transaction
	
	} else {
		/**
		 * Client does not have enough, 
		 * he should lend some money
		 */
		if(loan(data->id, (amount+2)) != 0){
			/**
			 * Client could not loan money, he leaves
			 */
			sem_post(&counters);
			return;
		} else {
			/**
			 * Client has loaned some money,
			 * he makes the transaction
			 */
			transaction(data->id, other, amount);
		}
	}

	usleep(sltime);
	sem_post(&counters);
}

/**
 * Client's routine
 * @param arg client's data
 */
void* client_routine(void *arg){
	client *data = arg;

	printf( "I (client n°%i) have started with %i$ in my pocket\n", data->id, data->balance );

	/**
	 * Each will alternate between walks and transaction NUM_TRANSACTIONS times
	 */

	for(int i = 0; i<NUM_TRANSACTIONS; i++){
		go_to_counter(data);
		take_a_walk(data->id);
	}

	/**
	 * Notify Robinhood that a client has finished
	 */
	clients_still_there--;

	return NULL;
}

/**
 * Robin's routine
 * @param arg robin's data
 */
void* robin_routine(void *arg){
	robin *rob = arg;

	printf("\x1b[1;32mHi, I'm robinhood \x1b[0m\n");

	/**
	 * Robinhood will keep going until there are no clients left to steal from
	 */
	while(1){
		
		if (rob->state == 0 ){
			/**
			 * That means robin is working
			 */
			printf("\x1b[1;32mI'm working \x1b[0m\n");
			
			/**
			 * Robin chooses a client to steal from, quits if no clients left
			 */
			int gotClient = 0; // flag
			int client_id; // The ID of the client Robin will choose

			while(gotClient != 1){
				if( clients_still_there == 0 ) return NULL;
				client_id = getRand(0, NUM_CLIENTS);
				if(d[client_id]->outside == 1) gotClient = 1;
			}

			printf("\x1b[1;32mI caught client n°%i ! \x1b[0m\n", client_id);

			/**
			 * See how long this will take, and calculate the chance Robin will actually steal from the guy
			 */
			int temps_travail = (rand() %(TIME_ROBIN_WORK_MAX - TIME_ROBIN_WORK_MIN )) + TIME_ROBIN_WORK_MIN;
			int stolen = steal();

			if(stolen != 0){
				/**
				 * Robin has decided to steal
				 */
				
				if( d[client_id]->balance > 300 ){
					/**
					 * The client is rich enough to be stolen from
					 */
					printf("\x1b[1;32mHi, %d i'm robin you \x1b[0m\n",d[client_id]->id);

					/**
					 * Adjust balances accordingly
					 */

					d[client_id]->balance = d[client_id]->balance-100; 
					rob->balance = rob->balance+100;

				} else if ( d[client_id]->balance < 50 && rob->balance >= 100 ){
					/**
					 * Client is too poor, and Robin has enough to give to him
					 */
					printf("\x1b[1;32mHi, %d i give you 100\x1b[0m\n", d[client_id]->id);

					d[client_id]->balance = d[client_id]->balance + 100;
					rob->balance = rob->balance - 100; 

				} else {
					/**
					 * Nothing happens.
					 */
					printf("\x1b[1;32mNext time %d ! \x1b[0m\n", d[client_id]->id);

				}

			} else {
				/**
				 * Got lucky, Robin won't steal from him
				 */
				printf("\x1b[1;32mNext time %d ! \x1b[0m\n", d[client_id]->id);
			}

			usleep(temps_travail*1000000);

			/**
			 * Let's go to the bar
			 */
			rob->state = 1;

			/**
			 * If there are no more clients, go home.
			 */
			if( clients_still_there == 0 ) return NULL;

		} else {
			/**
			 * Robinhood is at the bar, let's see how long he stays there 
			 */
			int temps_repos = (rand()%(TIME_ROBIN_PAUSE_MAX-TIME_ROBIN_PAUSE_MIN))+TIME_ROBIN_PAUSE_MIN;
			
			/**
			 * If he is rich enough, he'll have a pint
			 */
			if( rob->balance > 5 ){
				printf("\x1b[1;32mRobin drinks a beer \x1b[0m\n");
				rob->balance = rob->balance - 5;
				given -= 5;
				printf("\x1b[1;32mRobin has %i$ left \x1b[0m\n", rob->balance);
			} else {
				printf("\x1b[1;32mRobin is broke, needs to work\x1b[0m\n");
			}

			usleep(temps_repos*1000000);

			/**
			 * Back to work, if there is any more.
			 */
			rob->state = 0;
			if(clients_still_there == 0) return NULL;

		}
	}

	return NULL;
}

/**
 * Adds up all the money present in the simulation for proofing
 * @param  robBal Robin's balance
 * @return        Total money present at that point in the simulation
 */
int calc_money(int robBal){

	int amount = 0;
	amount += poste->bankVal;

	for(int i=0; i<NUM_CLIENTS; i++){
		amount += d[i]->balance;
	};

	amount += robBal;

	printf("We gave %i $ and %i is still here.\n", given, amount);

	return amount;

}


/**
 * Program's entry point
 * @param  argc Number of arguments
 * @param  argv Arguments array
 * @return      Exit code
 */
int main( int argc, char **argv ){
	
	/**
	 * Variable which will be decremented as a client leaves the game
	 */
	clients_still_there = NUM_CLIENTS;

	/**
	 * Post office instanciation
	 */
	poste = malloc( sizeof( post_office ) );
	poste->bankVal = BANK_INIT_BALANCE;

	/**
	 * Initialisation of semaphores/mutex
	 */
	sem_init(&counters, 0, NUM_COUNTERS);
	sem_init(&mutex, 0, 1);

	

	/**
	 * We should store all clients data in an array of client structures, each of them
	 * will be given to their respective thread routine.
	 */
	
	srand( time( NULL ) );

	given = BANK_INIT_BALANCE;

	for( int i=0; i<NUM_CLIENTS; i++ ){
		/**
		 * Random balance
		 */
		int bal = ( rand() %  ( WEALTH_MAX - WEALTH_MIN ) ) + WEALTH_MIN;

		/**
		 * Record how much money is distributed
		 */
		given += bal;

		d[i] = malloc( sizeof (client) );

		d[i]->balance = bal;
		d[i]->id = i;
		d[i]->emprunt = 0;
		d[i]->outside = 1; // Client starts outside
	}

	for( int j=0; j<NUM_CLIENTS; j++ ){
		if( pthread_create( &clients[j], NULL, client_routine, d[j]) != 0 ){
			printf( "Something went wrong\n" );
        	return EXIT_FAILURE;
		}
	}


	/**
	 * Robinhood's initialisation
	 */
	robin *rob = malloc( sizeof( robin ) );
	rob->balance = 0;
	rob->state = 0; // working 

	if( pthread_create( &robin_t, NULL, robin_routine, rob ) != 0 ){
		printf( "Something went wrong\n" );
    	return EXIT_FAILURE;
	}

	/**
	 * All threads have been successfully created, we join them to main
	 */

	pthread_join( robin_t, NULL );

	for ( int j = 0; j < NUM_CLIENTS ; j++ ){
	    pthread_join( clients[j], NULL );
	}

	/**
	 * Verify total balance to make sure none of the money has disappeared
	 */
	calc_money(rob->balance);

	printf("\x1b[1;31mSimulation finished.\x1b[0m\n");

	/**
	 * Free memory
	 */
	for(int i=0;i<NUM_CLIENTS; i++){
		free( d[i] );
	}

	free(rob);
	free(poste);

	return EXIT_SUCCESS;


}