/**
 * Oneal Abdulrahim
 * bounded_buffer.cpp
 * 
 */

#include "bounded_buffer.h"
#include <string>
#include <queue>

bounded_buffer::bounded_buffer(int _capacity) {
    mutex = PTHREAD_MUTEX_INITIALIZER;
    max = new semaphore(0); // thinking in semaphores
    min = new semaphore(_capacity);
    buffer = new std::queue<std::string>();
    capacity = _capacity;
}

void bounded_buffer::push_back(std::string req) {
    min -> P(); // start counting down
    pthread_mutex_lock(&mutex);
    buffer -> push(req); // load request into buffer stream
    pthread_mutex_unlock(&mutex);
    max -> V(); // unlock
}

std::string bounded_buffer::retrieve_front() {
    max -> P(); // place lock -- starting ctdwn
    pthread_mutex_lock(&mutex);

    std::string current_request = buffer -> front();
    buffer -> pop(); // remove current request from buffer

    pthread_mutex_unlock(&mutex);
    min -> V(); // unlock semaphore

    return current_request;
}

int bounded_buffer::size() {
    return capacity;
}

// require proper destruction and freeing of semaphore objects
// also, request queue must be freed, then destructor for mutex
bounded_buffer::~bounded_buffer() {
    delete min;
    delete max;
    delete buffer;
    pthread_mutex_destroy(&mutex);
}