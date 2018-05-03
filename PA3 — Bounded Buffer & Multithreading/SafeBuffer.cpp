#include "SafeBuffer.h"
#include <string>
#include <queue>

// https://linux.die.net/man/3/pthread_mutex_init

SafeBuffer::SafeBuffer() {
	pthread_mutex_init(&my_mutex, NULL);   // initialize mutex object
}

SafeBuffer::~SafeBuffer() {
	pthread_mutex_destroy(&my_mutex); // destructor
}

int SafeBuffer::size() {
    pthread_mutex_lock(&my_mutex); // place lock!!
	int size = buffer.size(); // now check the size
	pthread_mutex_unlock(&my_mutex); // be sure to unlock

	return size;
}

void SafeBuffer::push_back(std::string str) {
	pthread_mutex_lock(&my_mutex); // place lock!!
	buffer.push(str); // now perform insertion
	pthread_mutex_unlock(&my_mutex); // be sure to unlock
}

std::string SafeBuffer::retrieve_front() {
	pthread_mutex_lock(&my_mutex); // place lock!!

	std::string front = buffer.front(); // get head of queue
	buffer.pop(); // remove top

	pthread_mutex_unlock(&my_mutex); // be sure to unlock

	return front;
}
