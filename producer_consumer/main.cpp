//
// Example from: http://www.amparo.net/ce155/sem-ex.c
//
// Adapted using some code from Downey's book on semaphores
//
// Compilation:
//
//       g++ main.cpp -lpthread -o main -lm
// or 
//      make
//

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore */
#include <iostream>
using namespace std;

/*
 This wrapper class for semaphore.h functions is from:
 http://stackoverflow.com/questions/2899604/using-sem-t-in-a-qt-project
 */
class Semaphore {
public:
    // Constructor
    Semaphore(int initialValue)
    {
        sem_init(&mSemaphore, 0, initialValue);
    }
    // Destructor
    ~Semaphore()
    {
        sem_destroy(&mSemaphore); /* destroy semaphore */
    }
    
    // wait
    void wait()
    {
        sem_wait(&mSemaphore);
    }
    // signal
    void signal()
    {
        sem_post(&mSemaphore);
    }
    
    
private:
    sem_t mSemaphore;
};




/* global vars */
const int bufferSize = 5;
const int numConsumers = 3; 
const int numProducers = 3; 

/* semaphores are declared global so they can be accessed
 in main() and in thread routine. */
Semaphore Mutex(1);
Semaphore Spaces(bufferSize);
Semaphore Items(0);             

// Lightswitch class from Downey's book
class Lightswitch {
public:
    Lightswitch() : counter(0), mutex(1) {}

    void lock(Semaphore &sem) {
        mutex.wait();
        counter++;
        if (counter == 1) {
            sem.wait();
        }
        mutex.signal();
    }

    void unlock(Semaphore &sem) {
        mutex.wait();
        counter--;
        if (counter == 0) {
            sem.signal();
        }
        mutex.signal();
    }

private:
    int counter;
    Semaphore mutex;
};

// Problem 1: No-starve readers-writers
const int NS_NUM_READERS = 5;
const int NS_NUM_WRITERS = 5;
const int NS_NUM_LOOPS = 5;

Lightswitch ns_readSwitch;
Semaphore ns_roomEmpty(1);
Semaphore ns_turnstile(1);
Semaphore ns_printMutex(1);

// Problem 2: Writer-priority readers-writers
const int WP_NUM_READERS = 5;
const int WP_NUM_WRITERS = 5;
const int WP_NUM_LOOPS = 5;

Lightswitch wp_readSwitch;
Lightswitch wp_writeSwitch;
Semaphore wp_noReaders(1);
Semaphore wp_noWriters(1);
Semaphore wp_printMutex(1);

// Problem 3: Dining Philosophers solution #1
const int DP_NUM_PHILOSOPHERS = 5;
const int DP_NUM_LOOPS = 5;

inline int dp_left(int i) {
    return i;
}
inline int dp_right(int i) {
    return (i+1) % DP_NUM_PHILOSOPHERS;
}

// limits philosophers at the table to 4
Semaphore dp_footman(4);
// 1 semaphore per philosopher
Semaphore dp_forks[DP_NUM_PHILOSOPHERS] = {
    Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1)
};

Semaphore dp_printMutex(1);

// Problem 4: Dining Philosophers Solution #2
const int DP2_NUM_PHILOSOPHERS = 5;
const int DP2_NUM_LOOPS = 5;

inline int dp2_left(int i) {
    return i;
}
inline int dp2_right(int i) {
    return (i+1) % DP2_NUM_PHILOSOPHERS;
}

// one semaphore per philosopher
Semaphore dp2_forks[DP2_NUM_PHILOSOPHERS] = {
    Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1)
};

Semaphore dp2_printMutex(1);

/*
    Producer function 
*/
void *Producer ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;

    while( 1 )
    {
        sleep(3); // Slow the thread down a bit so we can see what is going on
        Spaces.wait();
        Mutex.wait();
            printf("Producer %d adding item to buffer \n", x);
            fflush(stdout);
        Mutex.signal();
        Items.signal();
    }

}

/*
    Consumer function 
*/
void *Consumer ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;
    
    while( 1 )
    {
        Items.wait();
        Mutex.wait();
            printf("Consumer %d removing item from buffer \n", x);
            fflush(stdout);
        Mutex.signal();
        Spaces.signal();
        sleep(5);   // Slow the thread down a bit so we can see what is going on
    }

}

// Problem 1: No Starve reader-writers threads
void *ns_reader(void *arg) {
    int id = (long)arg;

    for (int i = 0; i < NS_NUM_LOOPS; i++) {
        ns_turnstile.wait();
        ns_turnstile.signal();

        ns_readSwitch.lock(ns_roomEmpty);

        ns_printMutex.wait();
        printf("[NS] Reader %d: reading (iteration %d)\n", id, i + 1);
        fflush(stdout);
        ns_printMutex.signal();

        usleep(1000000);
        ns_readSwitch.unlock(ns_roomEmpty);
        usleep(50000);
    }
    return NULL;
}

void *ns_writer(void *arg) {
    int id = (long)arg;

    for (int i = 0; i < NS_NUM_LOOPS; i++) {
        ns_turnstile.wait();
        ns_roomEmpty.wait();

        ns_printMutex.wait();
        printf("[NS] Writer %d: writing (iteration %d)\n", id, i + 1);
        fflush(stdout);
        ns_printMutex.signal();

        usleep(150000);
        ns_roomEmpty.signal();
        ns_turnstile.signal();
        usleep(70000);
    }
    return NULL;
}

void run_no_starve_readers_writers() {
    pthread_t readers[NS_NUM_READERS];
    pthread_t writers[NS_NUM_WRITERS];

    // create readers
    for (long r = 0; r < NS_NUM_READERS; r++) {
        pthread_create(&readers[r], NULL, ns_reader, (void *)(r + 1));
    }

    // create writers
    for (long w = 0; w < NS_NUM_WRITERS; w++) {
        pthread_create(&writers[w], NULL, ns_writer, (void *)(w + 1));
    }

    // wait for all threads
    for (int r = 0; r < NS_NUM_READERS; r++) {
        pthread_join(readers[r], NULL);
    }
    for (int w = 0; w < NS_NUM_WRITERS; w++) {
        pthread_join(writers[w], NULL);
    }
}

// Problem 2: Writer-priority readers-writers threads
void *wp_reader(void *arg) {
    int id = (long)arg;

    for (int i = 0; i < WP_NUM_LOOPS; i++) {
        wp_noReaders.wait();
        wp_readSwitch.lock(wp_noWriters);
        wp_noReaders.signal();

        wp_printMutex.wait();
        printf("[WP] Reader %d: reading (iteration %d)\n", id, i + 1);
        fflush(stdout);
        wp_printMutex.signal();

        usleep(100000);
        wp_readSwitch.unlock(wp_noWriters);
        usleep(50000);
    }

    return NULL;
}

void *wp_writer(void *arg) {
    int id = (long)arg;

    for (int i = 0; i < WP_NUM_LOOPS; i++) {
        wp_writeSwitch.lock(wp_noReaders);
        wp_noWriters.wait();

        wp_printMutex.wait();
        printf("[WP] Writer %d: writing (iteration %d)\n", id, i + 1);
        fflush(stdout);
        wp_printMutex.signal();

        usleep(150000); 
        wp_noWriters.signal();
        wp_writeSwitch.unlock(wp_noReaders);
        usleep(70000);  
    }

    return NULL;
}

void run_writer_priority_readers_writers() {
    pthread_t readers[WP_NUM_READERS];
    pthread_t writers[WP_NUM_WRITERS];

    // create readers
    for (long r = 0; r < WP_NUM_READERS; r++) {
        pthread_create(&readers[r], NULL, wp_reader, (void *)(r + 1));
    }

    // create writers
    for (long w = 0; w < WP_NUM_WRITERS; w++) {
        pthread_create(&writers[w], NULL, wp_writer, (void *)(w + 1));
    }

    // join readers
    for (int r = 0; r < WP_NUM_READERS; r++) {
        pthread_join(readers[r], NULL);
    }

    // join writers
    for (int w = 0; w < WP_NUM_WRITERS; w++) {
        pthread_join(writers[w], NULL);
    }
}

// Problem 3: Dining Philosophers Solution #1 threads
void *dp_philosopher(void *arg) {
    int id = (long)arg;

    for (int iter = 0; iter < DP_NUM_LOOPS; iter++) {
        // think()
        dp_printMutex.wait();
        printf("[DP1] Philosopher %d thinking (iteration %d)\n", id, iter + 1);
        fflush(stdout);
        dp_printMutex.signal();
        usleep(80000);  // thinking

        // get_forks(i): footman.wait(); fork[right(i)].wait(); fork[left(i)].wait();
        dp_footman.wait();
        dp_forks[dp_right(id)].wait();
        dp_forks[dp_left(id)].wait();

        // eat()
        dp_printMutex.wait();
        printf("[DP1] Philosopher %d eating (iteration %d)\n", id, iter + 1);
        fflush(stdout);
        dp_printMutex.signal();
        usleep(80000);  // eating

        // put_forks(i): fork[right].signal(); fork[left].signal(); footman.signal();
        dp_forks[dp_right(id)].signal();
        dp_forks[dp_left(id)].signal();
        dp_footman.signal();
    }

    return NULL;
}

void run_dining_philosophers_1() {
    pthread_t philosophers[DP_NUM_PHILOSOPHERS];

    // create philosophers
    for (long i = 0; i < DP_NUM_PHILOSOPHERS; i++) {
        pthread_create(&philosophers[i], NULL, dp_philosopher, (void *)i);
    }

    // wait for them to finish
    for (int i = 0; i < DP_NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i], NULL);
    }
}

// Problem 4: Dining Philosophers Solution #2 threads
void *dp2_philosopher(void *arg) {
    int id = (long)arg;

    for (int iter = 0; iter < DP2_NUM_LOOPS; iter++) {
        // think()
        dp2_printMutex.wait();
        printf("[DP2] Philosopher %d thinking (iteration %d)\n", id, iter + 1);
        fflush(stdout);
        dp2_printMutex.signal();
        usleep(80000);  // thinking

        // Philosopher 0 is a "leftie", others are "righties"
        if (id == 0) {
            // leftie: pick up left fork first, then right
            dp2_forks[dp2_left(id)].wait();
            dp2_forks[dp2_right(id)].wait();
        } else {
            // rightie: pick up right fork first, then left
            dp2_forks[dp2_right(id)].wait();
            dp2_forks[dp2_left(id)].wait();
        }

        // eat()
        dp2_printMutex.wait();
        printf("[DP2] Philosopher %d eating (iteration %d)\n", id, iter + 1);
        fflush(stdout);
        dp2_printMutex.signal();
        usleep(80000);  // eating

        // put_forks
        dp2_forks[dp2_right(id)].signal();
        dp2_forks[dp2_left(id)].signal();
    }

    return NULL;
}

void run_dining_philosophers_2() {
    pthread_t philosophers[DP2_NUM_PHILOSOPHERS];

    // create philosophers
    for (long i = 0; i < DP2_NUM_PHILOSOPHERS; i++) {
        pthread_create(&philosophers[i], NULL, dp2_philosopher, (void *)i);
    }

    // wait for all to finish
    for (int i = 0; i < DP2_NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i], NULL);
    }
}

int main(int argc, char **argv )
{
    // If they gave a problem number, use that
    if (argc >= 2) {
        int problem = atoi(argv[1]);

        if (problem == 1) {
            // Problem 1: no-starve readers-writers
            run_no_starve_readers_writers();
            printf("Main: Problem 1 completed. Exiting.\n");
            pthread_exit(NULL);
        } else if (problem == 2) {
            // Problem 2: writer-priority readers-writers
            run_writer_priority_readers_writers();
            printf("Main: Problem 2 completed. Exiting.\n");
            pthread_exit(NULL);
        } else if (problem == 3) {
            // Problem 3: Dining Philosophers Solution #1
            run_dining_philosophers_1();
            printf("Main: Problem 3 completed. Exiting.\n");
            pthread_exit(NULL);
        } else if (problem == 4) {
            // Problem 4: Dining Philosophers Solution #2
            run_dining_philosophers_2();
            printf("Main: Problem 4 completed. Exiting.\n");
            pthread_exit(NULL);
        }
        else {
            printf("Unknown problem number %d\n", problem);
            pthread_exit(NULL);
        }
    }


    pthread_t producerThread[ numProducers ];
    pthread_t consumerThread[ numConsumers ];

    // Create the producers 
    for( long p = 0; p < numProducers; p++ )
    {
        int rc = pthread_create ( &producerThread[ p ], NULL, 
                                  Producer, (void *) (p+1) );
        if (rc) {
            printf("ERROR creating producer thread # %ld; \
                    return code from pthread_create() is %d\n", p, rc);
            exit(-1);
        }
    }

    // Create the consumers 
    for( long c = 0; c < numConsumers; c++ )
    {
        int rc = pthread_create ( &consumerThread[ c ], NULL, 
                                  Consumer, (void *) (c+1) );
        if (rc) {
            printf("ERROR creating consumer thread # %ld; \
                    return code from pthread_create() is %d\n", c, rc);
            exit(-1);
        }
    }

    printf("Main: program completed. Exiting.\n");


    // To allow other threads to continue execution, the main thread 
    // should terminate by calling pthread_exit() rather than exit(3). 
    pthread_exit(NULL); 


} /* main() */
