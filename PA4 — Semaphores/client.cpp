/*
    Oneal Abdulrahim
    client.cpp

 */

#include "bounded_buffer.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>

#include <sys/time.h>
#include <cassert>
#include <assert.h>

#include <cmath>
#include <numeric>
#include <algorithm>

#include <list>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <ctime>
#include <chrono>

#include "reqchannel.h"

using namespace std;
using namespace std::chrono;

/*
    This next file will need to be written from scratch, along with
    semaphore.h and (if you choose) their corresponding .cpp files.
 */

//#include "bounded_buffer.h"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/*
    All *_params structs are optional,
    but they might help.
 */
struct PARAMS_REQUEST {
	bounded_buffer* buffer;
	int request_count;
	string request_message;
};

struct PARAMS_WORKER {
	bounded_buffer* buffer;
	RequestChannel* channel;
	bounded_buffer* john_buffer;
	bounded_buffer* jane_buffer;
	bounded_buffer* joe_buffer;
};

struct PARAMS_STAT {
	bounded_buffer* request_buffer;
	vector<int>* count;
	int value;
};

class atomic_standard_output {
    pthread_mutex_t console_lock;
public:
    atomic_standard_output() { pthread_mutex_init(&console_lock, NULL); }
    ~atomic_standard_output() { pthread_mutex_destroy(&console_lock); }
    void print(std::string s){
        pthread_mutex_lock(&console_lock);
        std::cout << s << std::endl;
        pthread_mutex_unlock(&console_lock);
    }
};

atomic_standard_output threadsafe_standard_output;

// from PA3
std::string make_histogram_table(std::string name1, std::string name2, std::string name3, std::vector<int> *data1, std::vector<int> *data2, std::vector<int> *data3) {
    std::stringstream tablebuilder;
	tablebuilder << std::setw(25) << std::right << name1;
	tablebuilder << std::setw(15) << std::right << name2;
	tablebuilder << std::setw(15) << std::right << name3 << std::endl;
	for (int i = 0; i < data1->size(); ++i) {
		tablebuilder << std::setw(10) << std::left
		        << std::string(
		                std::to_string(i * 10) + "-"
		                        + std::to_string((i * 10) + 9));
		tablebuilder << std::setw(15) << std::right
		        << std::to_string(data1->at(i));
		tablebuilder << std::setw(15) << std::right
		        << std::to_string(data2->at(i));
		tablebuilder << std::setw(15) << std::right
		        << std::to_string(data3->at(i)) << std::endl;
	}
	tablebuilder << std::setw(10) << std::left << "Total";
	tablebuilder << std::setw(15) << std::right
	        << accumulate(data1->begin(), data1->end(), 0);
	tablebuilder << std::setw(15) << std::right
	        << accumulate(data2->begin(), data2->end(), 0);
	tablebuilder << std::setw(15) << std::right
	        << accumulate(data3->begin(), data3->end(), 0) << std::endl;

	return tablebuilder.str();
}

void* request_thread_function(void* arg) {
	struct PARAMS_REQUEST* arguments = static_cast<struct PARAMS_REQUEST*>(arg);

	for (int i = 0; i < arguments -> request_count; ++i) {
		arguments -> buffer -> push_back(arguments -> request_message);
	}
	return NULL;
}

void* worker_thread_function(void* arg) {
	struct PARAMS_WORKER* arguments = static_cast<struct PARAMS_WORKER*>(arg);	
	
	RequestChannel *worker_channel = new RequestChannel( // create new worker channel "copy"
		arguments -> channel -> send_request("newthread"), // default string
		RequestChannel::CLIENT_SIDE // connect on client side
	);

	while(true) {
            std::string req = arguments->buffer->retrieve_front();
            std::string req_r = worker_channel->send_request(req);
	  
            if (req == "data John Smith") {
                arguments -> john_buffer -> push_back(req_r);
            }
            else if (req == "data Jane Smith") {
                arguments -> jane_buffer -> push_back(req_r);
            }
            else if (req == "data Joe Smith") {
                arguments -> joe_buffer -> push_back(req_r);
            }
            else if (req == "quit") {
                delete worker_channel;
                break;
            }
	}
	pthread_exit(NULL);
	return NULL;
}

void* stat_thread_function(void* arg) {
	struct PARAMS_STAT* arguments = static_cast<struct PARAMS_STAT*>(arg);
	int count = 0;
	while (count < arguments -> value) {
		string response = arguments->request_buffer->retrieve_front();		
		int index = stoi(response) / 10;
		arguments -> count -> at(index)+=1;
		count++;
	}
	pthread_exit(NULL);
	return NULL;
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/
int main(int argc, char * argv[]) {
    int n = 10000; //default number of requests per "patient"
    int b = 50; //default size of request_buffer
    int w = 35; //default number of worker threads
    bool USE_ALTERNATE_FILE_OUTPUT = false;
    int opt = 0;
    while ((opt = getopt(argc, argv, "n:b:w:m:h")) != -1) {
        switch (opt) {
            case 'n':
                n = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg);
                break;
            case 'm':
                if(atoi(optarg) == 2) USE_ALTERNATE_FILE_OUTPUT = true;
                break;
            case 'h':
            default:
                std::cout << "This program can be invoked with the following flags:" << std::endl;
                std::cout << "-n [int]: number of requests per patient" << std::endl;
                std::cout << "-b [int]: size of request buffer" << std::endl;
                std::cout << "-w [int]: number of worker threads" << std::endl;
                std::cout << "-m 2: use output2.txt instead of output.txt for all file output" << std::endl;
                std::cout << "-h: print this message and quit" << std::endl;
                std::cout << "Example: ./client_solution -n 10000 -b 50 -w 120 -m 2" << std::endl;
                std::cout << "If a given flag is not used, a default value will be given" << std::endl;
                std::cout << "to its corresponding variable. If an illegal option is detected," << std::endl;
                std::cout << "behavior is the same as using the -h flag." << std::endl;
                exit(0);
        }
    }

    int pid = fork();
	if (pid > 0) {
        struct timeval start_time;
        struct timeval finish_time;
        int64_t start_usecs;
        int64_t finish_usecs;
        std::ofstream ofs;
        if(USE_ALTERNATE_FILE_OUTPUT) ofs.open("output2.txt", std::ios::out | std::ios::app);
        else ofs.open("output.txt", std::ios::out | std::ios::app);

        std::cout << "n == " << n << std::endl;
        std::cout << "b == " << b << std::endl;
        std::cout << "w == " << w << std::endl;

        std::cout << "Client initialized!" << std::endl;
        std::cout << "Establishing control channel... " << std::flush;
        RequestChannel *channel = new RequestChannel("control", RequestChannel::CLIENT_SIDE);
        std::cout << "Complete!" << std::endl;
			
		//timer start 
		high_resolution_clock::time_point t1 = high_resolution_clock::now();

	    bounded_buffer request_buffer(b);
	    bounded_buffer john_buffer(b);
	    bounded_buffer jane_buffer(b);
	    bounded_buffer joe_buffer(b);

	    std::vector<int> john_frequency_count(10, 0);
	    std::vector<int> jane_frequency_count(10, 0);
	    std::vector<int> joe_frequency_count(10, 0);

        pthread_t john_request_thread;
	    struct PARAMS_REQUEST john_request_info;
        john_request_info.buffer = &request_buffer;
        john_request_info.request_count = n;
        john_request_info.request_message = "data John Smith";
        pthread_create(&john_request_thread, NULL, request_thread_function, &john_request_info);

        pthread_t jane_request_thread;
	    struct PARAMS_REQUEST jane_request_info;
        jane_request_info.buffer = &request_buffer;
        jane_request_info.request_count = n;
        jane_request_info.request_message = "data Jane Smith";
        pthread_create(&jane_request_thread, NULL, request_thread_function, &jane_request_info);

        pthread_t joe_request_thread;
        struct PARAMS_REQUEST joe_request_info;
	    joe_request_info.buffer = &request_buffer;
        joe_request_info.request_count = n;
        joe_request_info.request_message = "data Joe Smith";
        pthread_create(&joe_request_thread, NULL, request_thread_function, &joe_request_info);

	    pthread_t john_stat_thread;
        struct PARAMS_STAT john_stat_thread_info;
        john_stat_thread_info.request_buffer = &john_buffer;
        john_stat_thread_info.value = n;
        john_stat_thread_info.count = &john_frequency_count;
        pthread_create(&john_stat_thread, NULL, stat_thread_function, &john_stat_thread_info);

        pthread_t jane_stat_thread;
        struct PARAMS_STAT jane_stat_thread_info;
        jane_stat_thread_info.request_buffer = &jane_buffer;
        jane_stat_thread_info.value = n;
        jane_stat_thread_info.count = &jane_frequency_count;
        pthread_create(&jane_stat_thread, NULL, stat_thread_function, &jane_stat_thread_info);

        pthread_t joe_stat_thread;
	    struct PARAMS_STAT joe_stat_thread_info;
	    joe_stat_thread_info.request_buffer = &joe_buffer;
	    joe_stat_thread_info.value = n;
	    joe_stat_thread_info.count = &joe_frequency_count;
	    pthread_create(&joe_stat_thread, NULL, stat_thread_function, &joe_stat_thread_info);

	    pthread_t* workers = new pthread_t[w];

        for (int i = 0; i < w; ++i) {
	    	struct PARAMS_WORKER worker_parameters;
        	worker_parameters.channel = channel;
            worker_parameters.buffer = &request_buffer;
	    	worker_parameters.john_buffer = &john_buffer;
	    	worker_parameters.jane_buffer = &jane_buffer;
	    	worker_parameters.joe_buffer = &joe_buffer;

            // Worker threads
	        pthread_create(&workers[i], NULL, worker_thread_function, &worker_parameters);
	    }

	    pthread_join(john_stat_thread, NULL);
	    pthread_join(jane_stat_thread, NULL);
	    pthread_join(joe_stat_thread, NULL);
    
	    high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration <double> time_span = duration_cast <duration <double> > (t2 - t1);

        std::cout << "Finale: " << channel->send_request("quit") << std::endl;	
	    ofs.close();
        std::cout << "Sleeping..." << std::endl;
        usleep(100000);
	    cout << "TIME: " << time_span.count() << " seconds." << endl;
	    std::string histogram_table = make_histogram_table("John Smith", "Jane Smith", "Joe Smith", &john_frequency_count, &jane_frequency_count, &joe_frequency_count);
        std::cout << "Results for n == " << n << ", b == " << b << ", w == " << w << std::endl;
        std::cout << histogram_table << std::endl;
    }
    
	else if (pid == 0)
		execl("dataserver", NULL);
}