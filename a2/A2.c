/**
 The time calculation could be confusing, check the exmaple of gettimeofday on tutorial for more detail.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_SIZE 1024
#define NQUEUE 2

/* Customer struct */
typedef struct Customer  {
    int id;
	int class_type;
	float service_time;
	float arrival_time;
} customer;

/* Clerk struct */
typedef struct Clerk {
    int id;
} Clerk;

/* Node struct for Queue */
typedef struct node {
    customer* customer;
    struct node* next;
} node;

/* Queue struct  */
typedef struct Queue {
    struct node* head;
    struct node* tail;
    int size;
} Queue;

/* Create the customer node */
node* createNode(customer* currCustomer) {
    node* new = (node*) malloc(sizeof(node));
    if(new == NULL) {
        perror("Error: failed on malloc while creating node\n");
        return NULL;
    }
	/* Initialise values for the new customer */
    new->customer = currCustomer;
    new->next = NULL;
    return new;
}

/* Create a Queue list */
Queue* createQueue() {
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    if(queue == NULL) {
        perror("Error: failed on malloc while creating Queue\n");
        return NULL;
    }
	/* Initialise values for the new Queue */
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}


/* Create array for business and economy queues. */
struct Queue *queue[NQUEUE];	

/* Enqueue and Dequeue functions */
/* Insert element to the queue end */
void Queue_Enqueue(Queue *queue, node* Node){	
	/* if the queue is empty */
	if(!queue->size) {
	queue->head = Node;
	queue->tail = Node;
	}
	/* add to the tail of the queue */
	else {
		queue->tail->next = Node;
		queue->tail = Node;
	}
	/* increment size by 1 */
	queue->size++;
}

/* Remove element from the queue front */
void Queue_Dequeue(Queue *queue){
	/* Check if queue is empty or not */
	if(!queue->size) {
        perror("Error: Queue_Dequeue. No elements in queue\n");
		return;
    }
	/* if there is only one element in the queue */
	if(queue->size == 1) {
        queue->head = NULL;
        queue->tail = NULL;
        queue->size--;
    }
	/* else remove the head of the queue */
	else {
        queue->head = (queue->head)->next;
		/* decrement size by 1 */
        queue->size--;
    }
}

void * Customer_entry(void * cus_info);

/* Customer and Queue variables */
customer customerArray[MAX_SIZE];
Clerk clerkArray[5];
Queue* businessQueue = NULL;
Queue* economyQueue = NULL;
struct timeval init_time; 
double businessQueue_time = 0;
double economyQueue_time = 0;
double overallQueue_time = 0;
int numBusinessCustomers = 0;
int numEconomyCustomers = 0;

/* Check length of queues: 0-> economy 1-> business */
int queue_length[NQUEUE];
int queue_status[NQUEUE] = {0, 0};

/* array for each queue to check if customer is selected or not */
int winner_selected[NQUEUE] = {0,0};

/* Customer and clerk threads */
pthread_t customerThreads[MAX_SIZE];
pthread_t clerkThreads[5];

/* Clerk mutexes and condition variables */
pthread_mutex_t Clerk1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Clerk1_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t Clerk2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Clerk2_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t Clerk3_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Clerk3_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t Clerk4_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Clerk4_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t Clerk5_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Clerk5_convar = PTHREAD_COND_INITIALIZER;

/* Mutexes for calculating wait time for economy and business queues */
pthread_mutex_t businessQTime_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t economyQTime_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Business and Economy Queue mutexes and condition variables */
pthread_mutex_t businessQueue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t businessQueue_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t economyQueue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t economyQueue_convar = PTHREAD_COND_INITIALIZER;

/* Mutex and condition variable to inform clerk that customer has arrived. */
pthread_mutex_t request_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t request_queue_convar = PTHREAD_COND_INITIALIZER;

int checkHead(customer *p_myInfo, int curr_queue);

/* 	Creates a customer struct by parsing information from the line.
	Adds customer to the customer array. */
void addCustomertoList(char *line, int index){
	
	/* parse input from line */
	char *token;
	token = strtok(line, ",");
	char *values[strlen(line)];
	int i=0;

	/* 	Tokenize on the "," character. 
		First part is the number of customers. 
		Second part contains the values of customer data.
	*/
	while(token!=NULL){
		values[i] = token;
		i++;
		token = strtok(NULL, ",");
	}	

	/* Tokenize on ":" to store the customer details */
	char *temp[2];
	token = strtok(values[0], ":");
	i=0;
	while(token!=NULL){
		temp[i] = token;
		i++;
		token=strtok(NULL, ":");
	}

	/* Allocate space for the customer struct */
	customer* n_cust = (customer *)malloc(sizeof(customer));
	n_cust->id = strtol(temp[0], NULL, 10);
	n_cust->class_type = strtol(temp[1], NULL, 10);
	n_cust->arrival_time = atof(values[1]);
	n_cust->service_time= atof(values[2]);

	/* Add struct to customerArray */
	customerArray[index] = *n_cust;

}

/* check if the passed customer is in the head of the queue or not. */
int checkHead(customer *p_myInfo, int curr_queue) {
	if(curr_queue == 1){
		if(p_myInfo->id == businessQueue->head->customer->id){
			return 1;
			}
		}
	else{
		if(p_myInfo->id == economyQueue->head->customer->id){
			return 1;
		}
	}
	return 0;
}

/* get the current time value. */
double get_current_system_time(){
	struct timeval curr_time;
	gettimeofday(&curr_time, NULL);
	double final_time = curr_time.tv_sec*1000000 + curr_time.tv_usec; 
	double initial_time = init_time.tv_sec*1000000 + init_time.tv_usec;
	return (final_time - initial_time)/100000.0;
}

/* function entry for the customer threads. */
void * Customer_entry(void * cus_info){
	
	struct Customer * p_myInfo = (struct Customer *) cus_info;
	long arrival_time = p_myInfo->arrival_time;
	int class = p_myInfo->class_type;
	int curr_queue = class;	/* 0 -> class 0/economy , 1 -> class 1/business. */
	int curr_clerk ; 

    /* sleep for the arrival time of the customer */
	usleep(arrival_time * 100000); 
	
	printf("A customer arrives: customer ID  %d.\n", p_myInfo->id);

	/* Add customer to the queue by checking the class_type. */
	if(class == 1){

		/* business queue */
		if(pthread_mutex_lock(&businessQueue_mutex)) {
            perror("Error: failed on mutex lock.\n");
        }

		/* pick the queue and enter queue. */
		Queue_Enqueue(businessQueue, createNode(p_myInfo));
		queue_length[curr_queue]++ ;
		/* initialize time for calculating business queue waiting time. */
		double businessQueue_time_init = get_current_system_time();
		
	 	/* send signal to Clerk that customer is arrived. */
		pthread_cond_signal(&request_queue_convar);

		printf("A customer enters a queue: the queue ID: %d, and length of the queue  %d.\n", 1, queue_length[curr_queue]);
		
		/* 	Wait for Clerk to signal that Clerk is ready
			pthread signal by the Clerk returns the wait for all the threads waiting for the condvar of that specific queue.
			We use broadcast and check if our current customer thread is the head of the queue or not. 
			The head of that specific queue is the next customer. */

		while(1){
			pthread_cond_wait(&businessQueue_convar, &businessQueue_mutex);
			/* if i am the head of the queue and this queue is signaled by the Clerk */
			if(checkHead(p_myInfo, curr_queue) && !winner_selected[curr_queue]){	
				/* Queue_Dequeue. */
				Queue_Dequeue(businessQueue); 
				double businessQueue_time_end = get_current_system_time();
				pthread_mutex_lock(&businessQTime_mutex);
				businessQueue_time = businessQueue_time + (businessQueue_time_end - businessQueue_time_init);
				numBusinessCustomers++;
				pthread_mutex_unlock(&businessQTime_mutex);
				curr_clerk = queue_status[curr_queue];	
				queue_length[curr_queue] = queue_length[curr_queue]-1;
				/* change winner for business queue */
				winner_selected[curr_queue] = 1;
				break;
			}
		} 
		
		/* if this customer is picked by the Clerk, Queue_Dequeue and start self serve.*/
		if(pthread_mutex_unlock(&businessQueue_mutex)) {
            perror("Error: failed on mutex unlock.\n");
		}

		usleep(10);
		queue_status[curr_queue] = 0 ;

		printf("A clerk starts serving a customer: start time %.2f, the customer ID  %d, the clerk ID %d.\n", get_current_system_time()/10, p_myInfo->id, curr_clerk);

        usleep(p_myInfo->service_time * 100000);

		printf("-->>> A clerk finishes serving a customer: end time %.2f, the customer ID  %d, the clerk ID %d.\n", get_current_system_time()/10, p_myInfo->id, curr_clerk);	
	}

	if(class==0) {

		/* economy queue */
		if(pthread_mutex_lock(&economyQueue_mutex)) {
            perror("Error: failed on mutex lock.\n");
        }

		/* pick the queue and enter */
		Queue_Enqueue(economyQueue, createNode(p_myInfo));
		queue_length[curr_queue]++ ;
		double economyQueue_time_init = get_current_system_time();
		/* send signal to clerk that customer is arrived. */
		pthread_cond_signal(&request_queue_convar);

		printf("A customer enters a queue: the queue ID: %d, and length of the queue  %d.\n", 0, queue_length[curr_queue]);
		
		while(1){
			pthread_cond_wait(&economyQueue_convar, &economyQueue_mutex);
			/* if I am the head of the queue and this queue is signaled by the clerk */
			if(checkHead(p_myInfo, curr_queue) && !winner_selected[curr_queue]){	
				/* Queue_Dequeue */
				Queue_Dequeue(economyQueue); 
				/* calculate waiting time and customer count */
				double economyQueue_time_end = get_current_system_time();
				pthread_mutex_lock(&economyQTime_mutex);
				economyQueue_time = economyQueue_time + (economyQueue_time_end - economyQueue_time_init);
				numEconomyCustomers++;
				pthread_mutex_unlock(&economyQTime_mutex);
				/* update queue length and status */
				curr_clerk = queue_status[curr_queue];	
				queue_length[curr_queue] = queue_length[curr_queue]-1;
				/* change winner for economy queue */
				winner_selected[curr_queue] = 1;
				break;
			}
		} 

		if(pthread_mutex_unlock(&economyQueue_mutex)) {
            perror("Error: failed on mutex unlock.\n");
		}

		usleep(10);

		printf("A clerk starts serving a customer: start time %.2f, the customer ID  %d, the clerk ID %d.\n", get_current_system_time()/10, p_myInfo->id, curr_clerk);

        usleep(p_myInfo->service_time * 100000);

		printf("-->>> A clerk finishes serving a customer: end time %.2f, the customer ID  %d, the clerk ID %d.\n", get_current_system_time()/10, p_myInfo->id, curr_clerk);

	}
	
	/* Try to figure out which Clerk awoke me */
	if(curr_clerk == 1){
		pthread_mutex_lock(&Clerk1_mutex);
		pthread_cond_signal(&Clerk1_convar);
        pthread_mutex_unlock(&Clerk1_mutex);
	}
	else if(curr_clerk == 2){
		pthread_mutex_lock(&Clerk2_mutex);
        pthread_cond_signal(&Clerk2_convar);
        pthread_mutex_unlock(&Clerk2_mutex);
	}
	else if(curr_clerk == 3){
		pthread_mutex_lock(&Clerk3_mutex);
        pthread_cond_signal(&Clerk3_convar);
        pthread_mutex_unlock(&Clerk3_mutex);
	}
	else if(curr_clerk == 4){
		pthread_mutex_lock(&Clerk4_mutex);
        pthread_cond_signal(&Clerk4_convar);
        pthread_mutex_unlock(&Clerk4_mutex);
	}
	else{
		pthread_mutex_lock(&Clerk5_mutex);
        pthread_cond_signal(&Clerk5_convar);
        pthread_mutex_unlock(&Clerk5_mutex);
	}

	pthread_exit(NULL);
	
	return NULL;
}

// function entry for Clerk threads
void *clerk_entry(void * currClerk){
	struct Clerk* clerk_infor = (struct Clerk*) currClerk;
    int clerk_ID = clerk_infor->id;

	while(1){
			
		pthread_mutex_lock(&request_queue_mutex);
		
		/* wait for customer to enter the queue. */
		while (businessQueue->size == 0 && economyQueue->size == 0) {
			pthread_cond_wait(&request_queue_convar, &request_queue_mutex);
		}
		pthread_mutex_unlock(&request_queue_mutex);

		/* selected_queue_ID = Select the queue based on the priority and current Customers number */
		int selected_queue_ID ;
		
		/* serve business queue first */
		if(businessQueue->size!=0){
			
			selected_queue_ID = 1;

			/* mutexLock of the selected queue */
			pthread_mutex_lock(&businessQueue_mutex);
			
			/* The current clerk (clerkID) is signaling this queue */
			queue_status[selected_queue_ID] = clerk_ID; 
			
			/* set the initial value as the Customer has not selected from the queue */
			winner_selected[selected_queue_ID] = 0;
			
			/* Awake the Customer (the one enter into the queue first) from the selected queue */
			pthread_cond_broadcast(&businessQueue_convar); 
 			
			/* mutexLock of the selected queue */
			pthread_mutex_unlock(&businessQueue_mutex);
			
			/* wait for the Customer to finish its service */
			if(clerk_ID == 1){
				pthread_mutex_lock(&Clerk1_mutex);
				pthread_cond_wait(&Clerk1_convar, &Clerk1_mutex);
				pthread_mutex_unlock(&Clerk1_mutex);
			}
			else if(clerk_ID == 2){
				pthread_mutex_lock(&Clerk2_mutex);
				pthread_cond_wait(&Clerk2_convar, &Clerk2_mutex);
				pthread_mutex_unlock(&Clerk2_mutex);
			}
			else if(clerk_ID == 3){
				pthread_mutex_lock(&Clerk3_mutex);
				pthread_cond_wait(&Clerk3_convar, &Clerk3_mutex);
				pthread_mutex_unlock(&Clerk3_mutex);
			}
			else if(clerk_ID == 4){
				pthread_mutex_lock(&Clerk4_mutex);
				pthread_cond_wait(&Clerk4_convar, &Clerk4_mutex);
				pthread_mutex_unlock(&Clerk4_mutex);
			}
			else if(clerk_ID == 5){
				pthread_mutex_lock(&Clerk5_mutex);
				pthread_cond_wait(&Clerk5_convar, &Clerk5_mutex);
				pthread_mutex_unlock(&Clerk5_mutex);
			}
		}

		/* serve economy queue. */
		else if(economyQueue->size!=0){
			
			selected_queue_ID = 0;

			/* mutexLock of the selected queue */
			pthread_mutex_lock(&economyQueue_mutex);
			/* The current Clerk (clerkID) is signaling this queue */
			queue_status[selected_queue_ID] = clerk_ID;
			/* Awake the Customer (the one enter into the queue first) from the selected queue */
			pthread_cond_broadcast(&economyQueue_convar); 
			/* set the initial value as the Customer has not selected from the queue */
			winner_selected[selected_queue_ID] = 0;
			/* mutexLock of the selected queue */
			pthread_mutex_unlock(&economyQueue_mutex);
			
			/* wait for the Customer to finish its service */
			if(clerk_ID == 1){
				pthread_mutex_lock(&Clerk1_mutex);
				pthread_cond_wait(&Clerk1_convar, &Clerk1_mutex);
				pthread_mutex_unlock(&Clerk1_mutex);
			}
			else if(clerk_ID == 2){
				pthread_mutex_lock(&Clerk2_mutex);
				pthread_cond_wait(&Clerk2_convar, &Clerk2_mutex);
				pthread_mutex_unlock(&Clerk2_mutex);
			}
			else if(clerk_ID == 3){
				pthread_mutex_lock(&Clerk3_mutex);
				pthread_cond_wait(&Clerk3_convar, &Clerk3_mutex);
				pthread_mutex_unlock(&Clerk3_mutex);
			}
			else if(clerk_ID == 4){
				pthread_mutex_lock(&Clerk4_mutex);
				pthread_cond_wait(&Clerk4_convar, &Clerk4_mutex);
				pthread_mutex_unlock(&Clerk4_mutex);
			}
			else if(clerk_ID == 5){
				pthread_mutex_lock(&Clerk5_mutex);
				pthread_cond_wait(&Clerk5_convar, &Clerk5_mutex);
				pthread_mutex_unlock(&Clerk5_mutex);
			}

		}

	}
	
	pthread_exit(NULL);
	
 	return NULL;
}

/* Creates the clerk structs and adds them to clerk array */
void createClerkArray(){
	
    for(int i = 1; i <= 5; i++){
        Clerk clerk = {i};
		/* Store final values in the clerk array. */
        clerkArray[i-1] = clerk;
    }
}


int main(int args, char *argv[]) {

	/* Check input file */
	if(argv[1]==NULL){
		printf("Please pass an input file as argument\n");
		return 0;
	}

	char *filename = argv[1];
	FILE *fp = fopen(filename, "r");
	char line[1024];
	int numflag = 0;
	int numCustomers;
	int index = 0;

	/* initialize the queues */
	economyQueue = createQueue();
	businessQueue = createQueue();
	
	/* Create clerk array */
	createClerkArray();

	/* Read content from input file and create customer nodes. Store data in the customer list */
	while(fgets(line, sizeof(line), fp)){
		if(numflag==0){
			numCustomers = strtol(line, NULL, 10);
			numflag=1;
			continue;
		}
		if(index<numCustomers){
			addCustomertoList(line, index);
			index++;
		}
		else{
			break;
		}
	}
	fclose(fp);
	
	int i;
	/* create clerk threads */
	for(i = 0; i < 5; i++){
		 if(pthread_create(&clerkThreads[i], NULL, clerk_entry, (void *)&clerkArray[i])) {
            perror("Error: failed on thread create.\n");
            exit(0);
        }
	}
	
	gettimeofday(&init_time, NULL);
	/* create Customer thread */
	pthread_t customer_threads[numCustomers];
	for(i = 0; i < numCustomers; i++){
		if(pthread_create(&customer_threads[i], NULL, Customer_entry, (void *)&customerArray[i])  !=0 ){
			printf("Error: Unable to create customer pthread.\n");
            exit(1);
		} /* custom_info: passing the Customer information (e.g., Customer ID, arrival time, service time, etc.) to Customer thread */
	}

	/* wait for all Customer threads to terminate */
    for(int j = 0; j < numCustomers; j++) {
        if(pthread_join(customer_threads[j], NULL)){
            perror("Error: failed on join thread.\n");
            exit(0);
        }
    }
	
	printf("All jobs done...\n\n");
	/* Calculate the average waiting time of all Customers */
	printf("The average waiting time for business class customers in system is: %.2f seconds\n", businessQueue_time/(numBusinessCustomers*10));
	printf("The average waiting time for economy class customers in system is: %.2f seconds\n", economyQueue_time/(numEconomyCustomers*10));
	printf("The average waiting time for all customers in system is: %.2f seconds\n", (businessQueue_time + economyQueue_time)/((numBusinessCustomers + numEconomyCustomers)*10));

	return 0;
}
