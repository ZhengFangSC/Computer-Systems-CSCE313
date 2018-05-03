// Oneal Abdulrahim
// CSCE 313 - 511
// Spring 2018
// Programming Assignment 3

// https://stackoverflow.com/questions/34524/what-is-a-mutex

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

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
#include "SafeBuffer.h"


/* DATA STRUCTURES */

struct request_arguments {
	SafeBuffer* buffer;
	int id;
	std::string argument;
};

struct worker_arguments {
	SafeBuffer* buffer;
	RequestChannel* channel;
	std::vector<int> john_frequency_count {std::vector<int>(10,0)};
	std::vector<int> jane_frequency_count {std::vector<int>(10,0)};
	std::vector<int> joe_frequency_count {std::vector<int>(10,0)};
};

/*
	This function is provided for your convenience. If you vhoose not to use it,
	your final display should still be a histogram with bin size 10.
*/
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

class atomic_standard_output {
    pthread_mutex_t console_lock;
public:
		atomic_standard_output() {
			pthread_mutex_init(&console_lock, NULL);
		}
		~atomic_standard_output() {
			pthread_mutex_destroy(&console_lock);
		}
		void println(std::string s) {
			pthread_mutex_lock(&console_lock);
			std::cout << s << std::endl;
			pthread_mutex_unlock(&console_lock);
		}
		void perror(std::string s) {
			pthread_mutex_lock(&console_lock);
			std::cerr << s << ": " << strerror(errno) << std::endl;
			pthread_mutex_unlock(&console_lock);
		}
};

atomic_standard_output threadsafe_console_output;

void* request_thread_function(void* arg) {

	// inbound arguments cast as struct for simpler organization
	// assume well-formed and appropriate parameters
	struct request_arguments* arguments = (struct request_arguments*) (arg);

	// for every argument, add argument to queue
	for(int i = 0; i < arguments -> id; ++i) {
		arguments -> buffer -> push_back(arguments -> argument);
	}

	pthread_exit(NULL); // properly exit!!

	return NULL; // method signature calls for void ptr return
}

void* worker_thread_function(void* arg) {

	// inbound arguments cast as struct for simpler organization
	// assume well-formed and appropriate parameters
	struct worker_arguments* arguments = (struct worker_arguments*) (arg);

	RequestChannel *worker_channel = new RequestChannel( // create new worker channel "copy"
		arguments -> channel -> send_request("newthread"), // default string
		RequestChannel::CLIENT_SIDE // connect on client side
	);

    while(true) {
		std::string req = arguments -> buffer -> retrieve_front(); // top of requests
		std::string req_r = worker_channel -> send_request(req); // attempt to get response

		// properly account for arguments accordingly
		// quit on "quit" call -- delete current channel
		if (req == "data John Smith") {
			int i = stoi(req_r) / 10;
			arguments -> john_frequency_count.at(i)++;
		} else if (req == "data Jane Smith") {
			int i = stoi(req_r) / 10;
			arguments -> jane_frequency_count.at(i)++;
		} else if (req == "data Joe Smith") {
			int i = stoi(req_r) / 10;
			arguments -> joe_frequency_count.at(i)++;
		} else if (req == "quit") {
			delete worker_channel; // free current copied channel
			break;
		}
    }
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {
    /*
	 	Note that the assignment calls for n = 10000.
	 	That is far too large for a single thread to accomplish quickly.
     */
    int n = 10000; //default number of requests per "patient"
    int w = 1; //default number of worker threads
    int opt = 0;
    while ((opt = getopt(argc, argv, "n:w:h")) != -1) {
        switch (opt) {
            case 'n':
                n = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg); //This won't do a whole lot until you fill in the worker thread function
                break;
			case 'h':
            default:
				std::cout << "This program can be invoked with the following flags:" << std::endl;
				std::cout << "-n [int]: number of requests per patient" << std::endl;
				std::cout << "-w [int]: number of worker threads" << std::endl;
				std::cout << "-h: print this message and quit" << std::endl;
				std::cout << "(Canonical) example: ./client_solution -n 10000 -w 128" << std::endl;
				std::cout << "If a given flag is not used, or given an invalid value," << std::endl;
				std::cout << "a default value will be given to the corresponding variable." << std::endl;
				std::cout << "If an illegal option is detected, behavior is the same as using the -h flag." << std::endl;
                exit(0);
        }
    }

    int pid = fork();
	if (pid > 0) {

        std::cout << "n == " << n << std::endl;
        std::cout << "w == " << w << std::endl;

        std::cout << "CLIENT STARTED:" << std::endl;
        std::cout << "Establishing control channel... " << std::flush;
        RequestChannel *chan = new RequestChannel("control", RequestChannel::CLIENT_SIDE);
        std::cout << "done." << std::endl;

		SafeBuffer request_buffer;
        std::vector<int> john_frequency_count(10, 0);
        std::vector<int> jane_frequency_count(10, 0);
        std::vector<int> joe_frequency_count(10, 0);

/*--------------------------------------------------------------------------*/
/*  BEGIN CRITICAL SECTION  */
/*--------------------------------------------------------------------------*/

        std::cout << "Populating request buffer... ";
        fflush(NULL);
        
		pthread_t john_requests;
		pthread_t jane_requests;
		pthread_t joe_requests;

		struct request_arguments john_request_arguments;
		struct request_arguments jane_request_arguments;
		struct request_arguments joe_request_arguments;

		john_request_arguments.buffer = &request_buffer;
		jane_request_arguments.buffer = &request_buffer;
		joe_request_arguments.buffer = &request_buffer;

		john_request_arguments.id = n;
		jane_request_arguments.id = n;
		joe_request_arguments.id = n;

		john_request_arguments.argument = "data John Smith";
		jane_request_arguments.argument = "data Jane Smith";
		joe_request_arguments.argument = "data Joe Smith";

		pthread_create(&john_requests, NULL, request_thread_function, &john_request_arguments);
		pthread_create(&jane_requests, NULL, request_thread_function, &jane_request_arguments);
		pthread_create(&joe_requests, NULL, request_thread_function, &joe_request_arguments);

		pthread_join(john_requests, NULL);
		pthread_join(jane_requests, NULL);
		pthread_join(joe_requests, NULL);

        std::cout << "done." << std::endl;

        std::cout << "Pushing quit requests... ";
        fflush(NULL);
        for(int i = 0; i < w; ++i) {
            request_buffer.push_back("quit");
        }
        std::cout << "done." << std::endl;

		/*-------------------------------------------*/
		/* START TIMER HERE */
		/*-------------------------------------------*/
		pthread_t workers[w];
		auto start = std::chrono::steady_clock::now();

		for (int i = 0; i < w; ++i) {
            struct worker_arguments worker_arguments;
			worker_arguments.buffer = &request_buffer;
			worker_arguments.channel = chan;
			worker_arguments.john_frequency_count = john_frequency_count;
			worker_arguments.jane_frequency_count = jane_frequency_count;
			worker_arguments.joe_frequency_count = joe_frequency_count;
			pthread_create(&workers[i], NULL, worker_thread_function, &worker_arguments);
        }

		for (int i = 0; i < w; ++i) {
			pthread_join(workers[i], NULL);
		}

		auto end = std::chrono::steady_clock::now();
		std::cout << "Elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()/1000.0 << std::endl;
/*--------------------------------------------------------------------------*/
/*  END CRITICAL SECTION    */
/*--------------------------------------------------------------------------*/
        /*-------------------------------------------*/
        /* END TIMER HERE   */
        /*-------------------------------------------*/

        std::cout << "Finished!" << std::endl;

        std::cout << "Results for n == " << n << ", w == " << w << std::endl;

		//std::string histogram_table = make_histogram_table("John Smith",
		//        "Jane Smith", "Joe Smith", &john_frequency_count,
		//        &jane_frequency_count, &joe_frequency_count);

		/*
		 	This is a good place to output your timing data.
		 */

        //std::cout << histogram_table << std::endl;

        std::cout << "Sleeping..." << std::endl;
        usleep(10000);

        /*
            EVERY RequestChannel must send a "quit"
            request before program termination, and
            the destructor for each RequestChannel must be
            called somehow. If your RequestChannels are allocated
			on the stack, they will be destroyed automatically.
			If on the heap, you must properly free it. Take
			this seriously, it is part of your grade.
         */
        std::string finale = chan->send_request("quit");
        delete chan;
        std::cout << "Finale: " << finale << std::endl; //This line, however, is optional.
    }
	else if (pid == 0)
		execl("dataserver", (char*) NULL);
}
