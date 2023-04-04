//Damien Olsen
//Spring Semester 2023

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <pthread.h>

#define TABLE_SIZE 2 //Max number of items.

void* producer(void* arg);

//This is a struct to hold data between the producer and consumer program!
typedef struct {
    int table[TABLE_SIZE]; // The table where items will be placed.
    sem_t empty; // Semaphore to track number of empty slots on the table.
    sem_t full; // Semaphore to track number of filled slots on the table.
    sem_t mutex; // Semaphore to protect access to the table.
} shared_data;

int main(int argc, char* argv[]) {
    const char* shared_memory = "/operating_systems";
    int shm_fd = shm_open(shared_memory, O_CREAT | O_RDWR, 0666);

    ftruncate(shm_fd, sizeof(shared_data)); //Fixes size of the shared data.
    shared_data* shared = (shared_data*)mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    //Init all the semaphores
    sem_init(&(shared->empty), 1, TABLE_SIZE); //Everything is initalized as empty.
    sem_init(&(shared->full), 1, 0); //No slots are full.
    sem_init(&(shared->mutex), 1, 1); //Allows only one thread to access the table at a time.

    pthread_t producer_thread; //Creates the producer.
    pthread_create(&producer_thread, NULL, producer, (void*)shared);

    pthread_join(producer_thread, NULL);

    //Clears all previous semaphoes.
    sem_destroy(&(shared->empty));
    sem_destroy(&(shared->full));
    sem_destroy(&(shared->mutex));

    munmap(shared, sizeof(shared_data)); //Removes allocation to the range of memory maps.
    close(shm_fd);

    //Removes the name of the shared memory object.
    shm_unlink(shared_memory);

    return 0;
}


//Producer function
void* producer(void* arg) {
    shared_data* shared = (shared_data*)arg;
    int item_count = 0;
    int table_full = 0;

    while (1) {
        sem_wait(&(shared->empty)); //Wait until there's at least one empty slot.
        sem_wait(&(shared->mutex)); //Lock the table.

        if (table_full) { //Check if the table is full, if it is then wait for the consumer.
            printf("P: Table Is Full. Waiting for the Consumer. \n");
            table_full = 0;
        } else {
            shared->table[item_count % TABLE_SIZE] = item_count; //Put item into the table.
            printf("P: Put %d. \n", item_count);
            item_count++;

            if (item_count == TABLE_SIZE) { //If the Table is full, set the flag so that the producer will pause for the consumer.
                printf("P: Table Is Full. \n");
                table_full = 1; //This means the table is full, resets at the top of the if-statements.
            }
        }

        sem_post(&(shared->mutex)); //Releases the lock.
        sem_post(&(shared->full)); //Signals that another slot has been filled.

        sleep(1); //Helps see the flags.
    }

    pthread_exit(NULL);
}

