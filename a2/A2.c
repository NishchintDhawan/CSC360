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
/* 0-FALSE, 1-TRUE */

#define MAX_SIZE 1024
#define NQUEUE 2

typedef struct Customer  { /// use this struct to record the Customer information read from Customers.txt
    int id;
	int class_type;
	float service_time;
	float arrival_time;
} customer;

//use to store the clerk id
typedef struct clerk {
    int id;
} clerk;

//node for queue
typedef struct node {
    customer* customer;
    struct node* next;
} node;

//Queue type include the head and tail of the queue list.
typedef struct Queue {
    struct node* head;
    struct node* tail;
    int size;
} Queue;

//create the customer node
node* createNode(customer* currCustomer) {
    node* new = (node*) malloc(sizeof(node));
    if(new == NULL) {
        perror("Error: failed on malloc while creating node\n");
        return NULL;
    }
    new->customer = currCustomer;	//assign pointer to the currCustomer pointer.
    new->next = NULL;				// next is null.
    return new;
}

//create a Queue list
Queue* createQueue() {
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    if(queue == NULL) {
        perror("Error: failed on malloc while creating Queue\n");
        return NULL;
    }
    queue->head = NULL;		// initial values are NULL.
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}

struct Queue *queue[NQUEUE];
// business and economy queue. 

/* queue functions */
void enqueue(Queue *queue, node* newNode){
	if(queue->size == 0) {
	queue->head = newNode;
	queue->tail = newNode;
	}else {
		queue->tail->next = newNode;
		queue->tail = newNode;
	}
	queue->size++;
}

void dequeue(Queue *queue){
	 if(queue->size == 0) {
        exit(-1);
    }else if(queue->size == 1) {
        queue->head = NULL;
        queue->tail = NULL;
        queue->size--;
    }else {
        queue->head = (queue->head)->next;
        queue->size--;
    }
}

int peek(Queue* queue, customer* cust) {
    if(cust == queue->head->customer){
      return 1;
    }else{
      return 0;
    }
}

void * Customer_entry(void * cus_info);

int totalCustomers = 0;
int countBusiness = 0; //count the total of business customers
int countEconomy = 0;	//count the total of economy customers
customer customerArray[MAX_SIZE];
clerk clerkArray[5];
Queue* businessQueue = NULL;
Queue* economyQueue = NULL;
struct timeval init_time; //init of the time use to count the customer wait time
float businessQueue_time = 0;
float economyQueue_time = 0;
float overallQueue_time = 0;
int numBusinessCustomers = 0;
int numEconomyCustomers = 0;

//use to record the clerk current serving customer id
int clerk1_selected_customer = 0;
int clerk2_selected_customer = 0;
int clerk3_selected_customer = 0;
int clerk4_selected_customer = 0;
int clerk5_selected_customer = 0;

//record the current clerk id
//save the current clerk
int this_clerk = 0;

int queue_length[NQUEUE];
int queue_status[NQUEUE] = {0, 0};

int winner_selected[NQUEUE] = {0,0};
//initial the pthread and mutex and condition varibal.
pthread_t customerThreads[MAX_SIZE];
pthread_t clerkThreads[5];

//use for clerk lock and unlock and convar wait and signal
pthread_mutex_t C1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t C1_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t C2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t C2_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t C3_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t C3_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t C4_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t C4_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t C5_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t C5_convar = PTHREAD_COND_INITIALIZER;

pthread_mutex_t businessQTime_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t economyQTime_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t businessQueue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t businessQueue_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t economyQueue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t economyQueue_convar = PTHREAD_COND_INITIALIZER;

pthread_mutex_t request_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t request_queue_convar = PTHREAD_COND_INITIALIZER;

pthread_mutex_t overAll_mutex = PTHREAD_MUTEX_INITIALIZER;

int checkHead(customer *p_myInfo, int curr_queue);

/* global variables */
//create clerk id's. 
// set clerk id's and store the corresponding customer id.
/* Other global variable may include: 
 1. condition_variables (and the corresponding mutex_lock) to represent each queue; 
 2. condition_variables to represent clerks
 3. others.. depend on your design
 */

void printList(){
	int i = 0;

	for(i=0;i<totalCustomers;i++){
		printf("%d %d %f %f\n", customerArray[i].id, customerArray[i].class_type, customerArray[i].arrival_time, customerArray[i].service_time);
	}
}

void printQueue(Queue *queue){
	node *curr = queue->head;
	while(curr!=NULL){
		printf("%d %d %f %f\n", curr->customer->id, curr->customer->class_type, curr->customer->arrival_time, curr->customer->service_time);
		curr = curr->next;
	}
}

void addCustomertoList(char *line, int index){

	char *token;	
	//parse input from line. 

	token = strtok(line, ",");
	char *values[strlen(line)];
	int i=0;

	while(token!=NULL){
		values[i] = token;
		i++;
		token = strtok(NULL, ",");
	}

	char *temp[2];

	token = strtok(values[0], ":");
	i=0;
	while(token!=NULL){
		temp[i] = token;
		i++;
		token=strtok(NULL, ":");
	}

	customer* n_cust = (customer *)malloc(sizeof(customer));
	n_cust->id = strtol(temp[0], NULL, 10);
	n_cust->class_type = strtol(temp[1], NULL, 10);
	n_cust->arrival_time = atof(values[1]);
	n_cust->service_time= atof(values[2]);

	if(n_cust->class_type == 1){
		countBusiness++;
	}

	if(n_cust->class_type == 0){
		countEconomy++;
	}

	customerArray[index] = *n_cust;

	//printf("ID:%d  \n", customerArray[index].id);
}

// // function entry for Customer threads
// void get_current_customer(int curr_clerk){
// 	if(curr_clerk==1){
// 		return clerk1_selected_customer;
// 	}

// }

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

// //use to get the time of the curr time
// double get_simulation_time() {
//     struct timeval curr_time;
//     double curr_seconds;
//     double init_seconds;

//     init_seconds = (init_time.tv_sec*1000000) +  init_time.tv_usec ;
//     gettimeofday(&curr_time, NULL);
//     curr_seconds = (curr_time.tv_sec*100000) +  curr_time.tv_usec;

//     return (curr_seconds - init_seconds)/100000;
// }

float get_simulation_time(){
	struct timeval curr_time;
	gettimeofday(&curr_time, NULL);
	return(curr_time.tv_sec*1000000 + curr_time.tv_usec - init_time.tv_sec*1000000 - init_time.tv_usec)/100000.0;
}

void * Customer_entry(void * cus_info){
	
	struct Customer * p_myInfo = (struct Customer *) cus_info;
	//printf("Customer ID: %d  \n", p_myInfo->id);
	long arrival_time = p_myInfo->arrival_time;
	int class = p_myInfo->class_type;
	int curr_queue = class;	//0 -> class 0/economy , 1 -> class 1/business.
	int curr_clerk ; 
    // sleep for the arrival time of the customer
	usleep(arrival_time * 100000); 
	printf("Customer %d arriving in %.2f seconds\n", p_myInfo->id, get_simulation_time());

	//Look for the right queue and insert the customer in that queue. 

	/* Enqueue operation: get into either business queue or economy queue by using p_myInfo->class_type*/

	if(class == 1){

		// business queue
		if(pthread_mutex_lock(&businessQueue_mutex)) {
            perror("Error: failed on mutex lock.\n");
        }

		// pick the queue and enter queue.
		enqueue(businessQueue, createNode(p_myInfo));
		queue_length[curr_queue]++ ;
		//printf(Customer enters the queue of length );
		float businessQueue_time_init = get_simulation_time();
		
	 	// send signal to clerk that customer is arrived.
		pthread_cond_signal(&request_queue_convar);
		//wait for the clerk to signal when ready.
		
        //record the id from the global.
       

		//check the customer id picked by the clerk.
		//int current_customer = get_current_customer(curr_clerk);

		//wait for clerk to signal that clerk is ready
		// pthread signal by the clerk returns the wait for all the threads waiting for the condvar of that specific queue.
		// We use broadcast and check if our current customer thread is the head of the queue or not. 
		// the head of that specific queue is the next customer. 
		while(1){
			pthread_cond_wait(&businessQueue_convar, &businessQueue_mutex);
			//if i am the head of the queue and the flag for that queue is raised (queue is signaled by the clerk).
			if(checkHead(p_myInfo, curr_queue) && !winner_selected[curr_queue]){	
				//dequeue.
				dequeue(businessQueue); 
				float businessQueue_time_end = get_simulation_time();
				pthread_mutex_lock(&businessQTime_mutex);
				businessQueue_time = businessQueue_time + (businessQueue_time_end - businessQueue_time_init);
				numBusinessCustomers++;
				pthread_mutex_unlock(&businessQTime_mutex);
				curr_clerk = queue_status[curr_queue];	
				queue_length[curr_queue] = queue_length[curr_queue]-1;
				winner_selected[curr_queue] = 1;	//set flag to false.
				break;
			}
		} 
		//if this customer is picked by the clerk, dequeue and start self serve.

		if(pthread_mutex_unlock(&businessQueue_mutex)) {
            perror("Error: failed on mutex unlock.\n");
		}

		usleep(10);
		queue_status[curr_queue] = 0 ;
		printf("Clerk %1d starts serving the customer ID %2d at time: %.2f,  \n", curr_clerk, p_myInfo->id, get_simulation_time());

        usleep(p_myInfo->service_time * 100000);

		printf("Clerk %1d finishes serving Customer %d at time: %.2f \n", curr_clerk, p_myInfo->id, get_simulation_time());	
	}

	if(class==0) {

		// economy queue
		if(pthread_mutex_lock(&economyQueue_mutex)) {
            perror("Error: failed on mutex lock.\n");
        }

		// pick the queue and enter queue.
		enqueue(economyQueue, createNode(p_myInfo));
		queue_length[curr_queue]++ ;
		float economyQueue_time_init = get_simulation_time();
	// 	// send signal to clerk that customer is arrived.
		pthread_cond_signal(&request_queue_convar);

		//wait for the clerk to signal when ready.
        // if(pthread_cond_wait(&economyQueue_convar, &economyQueue_mutex)) {
        //     perror("Error: failed on condition wait.\n");
        // }
        //record the id from the global.
        

		//check the customer id picked by the clerk.
		//int current_customer = get_current_customer(curr_clerk);

		//wait for clerk to signal that clerk is ready
		// pthread signal by the clerk returns the wait for all the threads waiting for the condvar of that specific queue.
		// We use broadcast and check if our current customer thread is the head of the queue or not. 
		// the head of that specific queue is the next customer. 
		while(1){
			pthread_cond_wait(&economyQueue_convar, &economyQueue_mutex);
			//if i am the head of the queue and the flag for that queue is raised (queue is signaled by the clerk).
			if(checkHead(p_myInfo, curr_queue) && !winner_selected[curr_queue]){	
				//dequeue.
				dequeue(economyQueue); 
				float economyQueue_time_end = get_simulation_time();
				pthread_mutex_lock(&economyQTime_mutex);
				economyQueue_time = economyQueue_time + (economyQueue_time_end - economyQueue_time_init);
				numEconomyCustomers++;
				pthread_mutex_unlock(&economyQTime_mutex);
				curr_clerk = queue_status[curr_queue];	
				queue_length[curr_queue] = queue_length[curr_queue]-1;
				winner_selected[curr_queue] = 1;	//set flag to false.
				break;
			}
		} 
		//if this customer is picked by the clerk, dequeue and start self serve.

		if(pthread_mutex_unlock(&economyQueue_mutex)) {
            perror("Error: failed on mutex unlock.\n");
		}

		usleep(10);
		queue_status[curr_queue] = 0 ;
		printf("Clerk %1d starts serving a the customer ID %2d time %.2f,  \n", curr_clerk, p_myInfo->id, get_simulation_time() );

        usleep(p_myInfo->service_time * 100000);

		printf("Clerk %1d finishes serving Customer %d at time: %.2f \n", curr_clerk, p_myInfo->id, get_simulation_time());

	}

	if(curr_clerk == 1){
		pthread_mutex_lock(&C1_mutex);
		pthread_cond_signal(&C1_convar);
        pthread_mutex_unlock(&C1_mutex);
	}
	else if(curr_clerk == 2){
		pthread_mutex_lock(&C2_mutex);
        pthread_cond_signal(&C2_convar);
        pthread_mutex_unlock(&C2_mutex);
	}
	else if(curr_clerk == 3){
		pthread_mutex_lock(&C3_mutex);
        pthread_cond_signal(&C3_convar);
        pthread_mutex_unlock(&C3_mutex);
	}
	else if(curr_clerk == 4){
		pthread_mutex_lock(&C4_mutex);
        pthread_cond_signal(&C4_convar);
        pthread_mutex_unlock(&C4_mutex);
	}
	else{
		pthread_mutex_lock(&C5_mutex);
        pthread_cond_signal(&C5_convar);
        pthread_mutex_unlock(&C5_mutex);
	}
	/* Try to figure out which clerk awoken me, because you need to print the clerk Id information */
	// usleep(10); // Add a usleep here to make sure that all the other waiting threads have already got back to call pthread_cond_wait. 10 us will not harm your simulation time.
	// clerk_woke_me_up = queue_status[cur_queue];
	// queue_status[cur_queue] = IDLE;
	
	/* get the current machine time; updates the overall_waiting_time*/
	

	
	/* get the current machine time; */
//	fprintf(stdout, "A clerk finishes serving a Customer: end time %.2f, the Customer ID %2d, the clerk ID %1d. \n", /* ... */);
	
//	pthread_cond_signal(/* convar of the clerk signaled me */); // Notify the clerk that service is finished, it can serve another Customer 
	
	pthread_exit(NULL);
	
	return NULL;
}

// function entry for clerk threads
void *clerk_entry(void * currClerk){
	struct clerk* clerk_infor = (struct clerk*) currClerk;
    int clerk_ID = clerk_infor->id;

	while(1){
			
		pthread_mutex_lock(&request_queue_mutex);
		while (businessQueue->size == 0 && economyQueue->size == 0) {    //wait for customer to enter the queue.
			pthread_cond_wait(&request_queue_convar, &request_queue_mutex);
		}
		pthread_mutex_unlock(&request_queue_mutex);

		/* selected_queue_ID = Select the queue based on the priority and current Customers number */
		int selected_queue_ID ;
		
		// serve business queue first.
		if(businessQueue->size!=0){
			
			selected_queue_ID = 1;
			
			pthread_mutex_lock(&businessQueue_mutex);	/* mutexLock of the selected queue */
			
			queue_status[selected_queue_ID] = clerk_ID; // The current clerk (clerkID) is signaling this queue
			
			winner_selected[selected_queue_ID] = 0; // set the initial value as the Customer has not selected from the queue.
			
			pthread_cond_broadcast(&businessQueue_convar); // Awake the Customer (the one enter into the queue first) from the selected queue
			
			pthread_mutex_unlock(&businessQueue_mutex); /* mutexLock of the selected queue */
			
			// wait for the Customer to finish its service
			if(clerk_ID == 1){
				pthread_mutex_lock(&C1_mutex);
				pthread_cond_wait(&C1_convar, &C1_mutex);
				pthread_mutex_unlock(&C1_mutex);
			}
			else if(clerk_ID == 2){
				pthread_mutex_lock(&C2_mutex);
				pthread_cond_wait(&C2_convar, &C2_mutex);
				pthread_mutex_unlock(&C2_mutex);
			}
			else if(clerk_ID == 3){
				pthread_mutex_lock(&C3_mutex);
				pthread_cond_wait(&C3_convar, &C3_mutex);
				pthread_mutex_unlock(&C3_mutex);
			}
			else if(clerk_ID == 4){
				pthread_mutex_lock(&C4_mutex);
				pthread_cond_wait(&C4_convar, &C4_mutex);
				pthread_mutex_unlock(&C4_mutex);
			}
			else{
				pthread_mutex_lock(&C5_mutex);
				pthread_cond_wait(&C5_convar, &C5_mutex);
				pthread_mutex_unlock(&C5_mutex);
			}
		}

		// serve economy queue.
		else if(economyQueue->size!=0){
			
			selected_queue_ID = 0;

			pthread_mutex_lock(&economyQueue_mutex);	/* mutexLock of the selected queue */
			
			queue_status[selected_queue_ID] = clerk_ID; // The current clerk (clerkID) is signaling this queue
			
			pthread_cond_broadcast(&economyQueue_convar); // Awake the Customer (the one enter into the queue first) from the selected queue
			
			winner_selected[selected_queue_ID] = 0; // set the initial value as the Customer has not selected from the queue.
			
			pthread_mutex_unlock(&economyQueue_mutex); /* mutexLock of the selected queue */
			
			// wait for the Customer to finish its service
		
			if(clerk_ID == 1){
				pthread_mutex_lock(&C1_mutex);
				pthread_cond_wait(&C1_convar, &C1_mutex);
				pthread_mutex_unlock(&C1_mutex);
			}
			else if(clerk_ID == 2){
				pthread_mutex_lock(&C2_mutex);
				pthread_cond_wait(&C2_convar, &C2_mutex);
				pthread_mutex_unlock(&C2_mutex);
			}
			else if(clerk_ID == 3){
				pthread_mutex_lock(&C3_mutex);
				pthread_cond_wait(&C3_convar, &C3_mutex);
				pthread_mutex_unlock(&C3_mutex);
			}
			else if(clerk_ID == 4){
				pthread_mutex_lock(&C4_mutex);
				pthread_cond_wait(&C4_convar, &C4_mutex);
				pthread_mutex_unlock(&C4_mutex);
			}
			else{
				pthread_mutex_lock(&C5_mutex);
				pthread_cond_wait(&C5_convar, &C5_mutex);
				pthread_mutex_unlock(&C5_mutex);
			}

		}

	}
	
	pthread_exit(NULL);
	
 	return NULL;
}


int main(int args, char *argv[]) {

	// initialize all the condition variable and thread lock will be used
	
	/** Read Customer information from txt file and store them in the structure you created 
		
		1. Allocate memory(array, link list etc.) to store the Customer information.
		2. File operation: fopen fread getline/gets/fread ..., store information in data structure you created

	*/

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
	// initialise the queues.
	economyQueue = createQueue();
	businessQueue = createQueue();
	while(fgets(line, sizeof(line), fp)){
		if(numflag==0){
			numCustomers = strtol(line, NULL, 10);
			numflag=1;
			continue;
		}
		if(index<numCustomers){
			addCustomertoList(line, index);
			totalCustomers ++;
			index++;
		}
		else{
			break;
		}
	}

	fclose(fp);
	
	int f;
    for(f = 1; f <= 5; f++){
        clerk c = {f};
        clerkArray[f-1] = c;
    }

	// create clerk threads 
	for(int i = 0; i < 5; i++){ // number of clerks
		//clerk_info: passing the clerk information (e.g., clerk ID) to clerk thread
		 if(pthread_create(&clerkThreads[i], NULL, clerk_entry, (void *)&clerkArray[i])) {
            perror("Error: failed on thread create.\n");
            exit(0);
        }
	}
	
	gettimeofday(&init_time, NULL);
	pthread_t customer_threads[numCustomers];//create Customer thread
	for(int i = 0; i < numCustomers; i++){ // number of Customers
		if(pthread_create(&customer_threads[i], NULL, Customer_entry, (void *)&customerArray[i])  !=0 ){
			printf("Error: Unable to create customer pthread.\n");
            exit(1);
		} //custom_info: passing the Customer information (e.g., Customer ID, arrival time, service time, etc.) to Customer thread
	}

	// wait for all Customer threads to terminate
	int j;
    for(j = 0; j < numCustomers; j++) {
        if(pthread_join(customer_threads[j], NULL)){
            perror("Error: failed on join thread.\n");
            exit(0);
        }
    }

	// destroy mutex & condition variable (optional)
	
	// calculate the average waiting time of all Customers
	printf("\nTotal Business Customers: %d\n", numBusinessCustomers);
	printf("Total Economy Customers: %d\n", numEconomyCustomers);
	printf("Average wait time for Business Queue: %f\n", businessQueue_time/(numBusinessCustomers*10));
	printf("Average wait time for Economy Queue: %f\n", economyQueue_time/(numEconomyCustomers*10));
	printf("Average wait time Overall: %f\n", (businessQueue_time + economyQueue_time)/((numBusinessCustomers + numEconomyCustomers)*10));

	return 0;
}
