/**
 * Oneal Abdulrahim
 * bounded_buffer.h
 * 
 */

#ifndef bounded_buffer_h
#define bounded_buffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include <pthread.h>
#include "semaphore.h"

class bounded_buffer {
	semaphore* max;
    semaphore* min;
    pthread_mutex_t mutex;
    std::queue<std::string>* buffer;
    int capacity;

public:
    bounded_buffer(int _capacity);
    void push_back(std::string str);
    std::string retrieve_front();
    int size();
    ~bounded_buffer(); //need a destructor to properly destroy
};

#endif /* bounded_buffer_h */
