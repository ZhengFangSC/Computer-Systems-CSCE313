/**
 * Oneal Abdulrahim
 * semaphore.h
 * 
 */

#ifndef _semaphore_H_
#define _semaphore_H_

#include <pthread.h>

class semaphore {
private:
    int value;
    pthread_mutex_t mutex;
    pthread_cond_t queue;

public:

    semaphore(int _val) {
        value = _val;
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&queue, NULL);
	}

    ~semaphore(){
        pthread_mutex_destroy(&mutex);
    	pthread_cond_destroy(&queue);
    }
    
    void P() {
        pthread_mutex_lock(&mutex); // hold

        while (this -> value == 0) {
            pthread_cond_wait(&queue, &mutex);
        }
        
        --value;

        pthread_mutex_unlock(&mutex);
    }

    void V() {
        pthread_mutex_lock(&mutex);

        ++value;
        pthread_cond_signal(&queue);

        pthread_mutex_unlock(&mutex);
    }
};

#endif


