//Damien Olsen
//Spring Semester 2023

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <pthread.h>

#define TABLE_SIZE 2

void* consumer(void* arg);
//This is a struct to hold data between the producer and consumer program!
typedef struct {
    int table[TABLE_SIZE]; // The table where items will be placed
    sem_t empty; // Semaphore to track number of empty slots on the table
    sem_t full; // Semaphore to track number of filled slots on the table
    sem_t mutex; // Semaphore to protect access to the table
} shared_data;

int main(int argc, char* argv[]) {
    const char* shared_memory = "/operating_systems";
    int shm_fd = shm_open(shared_memory, O_RDWR, 0666);   //Open the object for r/w.
    shared_data* shared = (shared_data*)mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); //Map the shared memory object into the address and return pointer.

    pthread_t consumer_thread;  //Create the consumer thread.
    pthread_create(&consumer_thread, NULL, consumer, (void*)shared);  //Start the consumer thread with data passed.
    pthread_join(consumer_thread, NULL);

    munmap(shared, sizeof(shared_data));  //Unmap the shared memory object from the memory space.
    close(shm_fd);

    return 0;
}


//Consumer function
void* consumer(void* arg) {
    shared_data* shared = (shared_data*)arg;
    int item; //used to get table data

    while (1) {
        sem_wait(&(shared->full));  //Decrease semaphore.
        sem_wait(&(shared->mutex)); //Decrease the mutex.

        item = shared->table[0];  //Take the first item from the table.
        shared->table[0] = shared->table[1];  //Move the second item into first pos.
        shared->table[1] = 0;  //Set the second position to 0, showin an empty spot on the Table.

        if(item != 0){  // if the item is not 0, i.e., there was actually an item on the table
            printf("C: Picked up %d. \n", item); // print a message indicating the item was picked up
        }

        sem_post(&(shared->mutex));  // increment the mutex semaphore to release mutual exclusion
        sem_post(&(shared->empty));  // increment the empty semaphore to indicate that a spot on the table is empty

        if (item == 0) {  // if the item was 0, indicating an empty spot on the table
            printf("C: The Table is empty, pausing the consumer. \n");  //Flag that the table is empty.
            sleep(1);  //Pause the consumer to allow the producer time to catch up.
        }

        sleep(1); //Helps us see the flags.
    }

    pthread_exit(NULL);  // exit the thread
}

