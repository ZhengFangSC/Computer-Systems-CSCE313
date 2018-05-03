#ifndef SafeBuffer_h
#define SafeBuffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include <pthread.h>

class SafeBuffer {
	public:
		SafeBuffer();
		~SafeBuffer();
		int size();
		void push_back(std::string str);
		std::string retrieve_front();
		
		std::queue<std::string> buffer;
		pthread_mutex_t my_mutex;
};

#endif
