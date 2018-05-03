/*
Oneal Abdulrahim
CSCE 313 - 511
Programming Assignment 2: UNIX Shell

Due: Sunday, February 25, 2018
*/

// Critical resource
// https://www.cs.cornell.edu/courses/cs414/2004su/homework/shell/shell.html

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <iterator>
#include <vector>
#include <chrono>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <utility>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

const char AMP = '&';
std::vector<std::string> COMMAND_LOG;
std::vector<int> BACKGROUND_PROCESSES;
char CURRENT_DIRECTORY[256];
std::string HOME_DIRECTORY;

/*
Captures a single line of user input, returns string.
Pretty print shell command line & cwd.
@return     string containing the line of user input
*/
const std::string get_next_command() {
    std::string user_input;

    std::time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	std::cout << "\n\n~["
			  << getcwd(CURRENT_DIRECTORY, 255) << "] ——— "
              << std::ctime(&current_time)
              << ":: ";
         
    getline(std::cin, user_input);

    COMMAND_LOG.push_back(user_input); // add it to the log before parsing

    return user_input;
}

/*
Builds new string by tokenizing string input by char type delimiter.
In-place access of input string.
@param  s       The source string
@param  delim   Single character delimiter to split at
*/
template<typename T>
void split(const std::string &s, char delim, T result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

/*
Using split method, returns a vector laoded with each element a delimited
result. Does not account for empty results.
@param  haystack    The source string
@param  needle      Single character delimiter to split at
@return             vector<string> containing split results
*/
std::vector<std::string> split(const std::string &haystack, char needle) {
    std::vector<std::string> elems;
	split(haystack, needle, std::back_inserter(elems));

    return elems;
}

/*
Uses the split method to tokenize the inputs via pipe (or any other hard-coded
character value). Returns a vector of strings split by pipe |.
@param s    The input string to be tokenized
@return     A vector<string> containing tokenized string elements
*/
std::vector<std::string> tokenize(const std::string &s) {
    // delimit by spaces for now
	std::vector<std::string> result = split(s, ' ');

	// clean up empty splits
	for (int i = 0; i < result.size(); ++i) {
		if (result[i].empty()) {
			result.erase(result.begin() + i);
		}
	}

	// strip quotes (nasty)
	for (int i = 0; i < result.size(); ++i) {

		// if first char of string
		if (result[i].at(0) == '\"') {
			result[i].erase(result[i].begin());
		}

		// if last char of string
		if (result[i].at(result[i].size() - 1) == '\"') {
			result[i].erase(result[i].end() - 1);
		}
	}

	return result;
}

void change_directory(std::vector<std::string> tokens) {
	std::string current_directory;
	std::string directory(CURRENT_DIRECTORY);

	if (tokens.size() > 1) { // there's more to do on cd
		if (tokens[1] == ".." || tokens[1] == "-") {
			int previous_directory = directory.find_last_of("/");
			if (previous_directory != std::string::npos) { // if we're not at the end of str (current dir)
				current_directory = std::string(directory).substr(0, previous_directory);
			}
		} else {
			current_directory = std::string(directory) + "/" + tokens[1]; // append path
		}
	} else { // only "cd" was given -- go to home
		current_directory = HOME_DIRECTORY;
	}

	chdir(current_directory.c_str());
}

void execute_shell() {
	// preliminary -- get working directories set properly
	getcwd(CURRENT_DIRECTORY, 255);
	HOME_DIRECTORY = CURRENT_DIRECTORY;

	// fetch the next command from the user & tokenize
	std::string x = get_next_command();
	std::vector<std::string> tokens = tokenize(x);
	std::vector<std::vector<std::string>> all_commands;
	std::vector<std::string> current_command;

	bool is_active_shell = true; // so far so good

	while (is_active_shell) {
		// the first token should be a command
		current_command.push_back(tokens[0]);

		// let's check for common commands first
		if (tokens[0] == "exit") {
			is_active_shell = !is_active_shell;
			std::cout << "Thanks for using Oneal's shell! ♥ :)" << std::endl;
			continue;
		} else if (tokens[0] == "cd") {
			change_directory(tokens);
		}

		bool is_background_process = false;
		int input_redirect_size = -1;
		int output_redirect_size = -1;
		std::string input_redirect = "";
		std::string output_redirect = "";

		for (int i = 1; i < tokens.size(); ++i) {
			std::string current_token = tokens[i];

			// consider if it's a regular command
			if (current_token != "&"
			 && current_token != "|"
			 && current_token != "<"
			 && current_token != ">") {
				 current_command.push_back(current_token);
			} else if (current_token == "|" && i != tokens.size() - 1) {
				all_commands.push_back(current_command);
				current_command.clear();
			} else if ((current_token == "<" || current_token == ">") // if redirects and
				     && i == tokens.size() - 2) { // there's another token following
				if (current_token == "<") {
					input_redirect_size = all_commands.size();
					input_redirect = tokens[i + 1];
				} else if (current_token == ">") {
					output_redirect_size = all_commands.size();
					output_redirect = tokens[i + 1];
				}

				all_commands.push_back(current_command);
				current_command.clear();

				++i; // don't care about what the file name is right now
			
			// if & is the last token... all are background processes
			} else if (current_token == "&" && i == tokens.size() - 1) {
				is_background_process = true;
			}
		}

		if (current_command.size() > 0) {
			all_commands.push_back(current_command);
		}

		// time to execute the commands
		int pipe_count = all_commands.size() - 1;
		int fd[pipe_count][2];

		for (int i = 0; i < pipe_count; ++i) {
			pipe(fd[i]); // pipe it up!
		}

		// create new process & map it with corresponding pipe
		for (int i = 0; i < all_commands.size(); ++i) {
			pid_t child;
			child = fork();

			if (child != 0 && is_background_process) {
				BACKGROUND_PROCESSES.push_back(child);
			} else if (child == 0) {
				if (i == 0) { // first command corresponds to the first pipe
					dup2(fd[0][1], STDOUT_FILENO);
				} else if (i == all_commands.size() - 1) { // last command corresponds to last pipe
					dup2(fd[pipe_count - 1][0], STDOUT_FILENO);
				} else { // we are somewhere in the middle of the piping
					dup2(fd[i - 1][0], STDIN_FILENO);
					dup2(fd[i][1], STDOUT_FILENO);
				}

				// now consider program redirects
				if (i == input_redirect_size) { // input redirect
					int file_d = open(input_redirect.c_str(), O_RDONLY);

					if (file_d < 0) { // if that file is not found
						std::cerr << "!!! Cannot find file or directory!!" << std::endl;
						exit(1); // quit w error
					}

					dup2(file_d, STDIN_FILENO);
				} else if (i == output_redirect_size) { // output redirect
					int file_d = open(output_redirect.c_str(), O_CREAT
													         | O_WRONLY
													         | O_TRUNC, S_IRUSR
													         | S_IWUSR
													         | S_IRGRP
													         | S_IROTH);
					dup2(file_d, STDOUT_FILENO);
				}

				// closing pipes
				for (int j = 0; j < pipe_count; ++j) {
					close(fd[j][0]);
					close(fd[j][1]);
				}
				
				int execute;

				// need to convert args from str to char array for execvp
				// this is only if the command has some arguments (deeper into 2d array)
				if (all_commands[i].size() > 1) {
					char* args[all_commands[i].size() - 1];

					for (int j = 0; j < all_commands[i].size(); ++j) {
						args[j] = (char*) (all_commands[i][j].c_str());
					}

					args[all_commands[i].size()] = NULL;

					int execute = execvp(all_commands[i][0].c_str(), args);
				} else { // otherwise, there are no arguments
					int execute = execlp(all_commands[i][0].c_str(), all_commands[i][0].c_str(), NULL);
				}

				if (execute < 0) { // we've failed to exec properly on process
					std::cerr << "!!! failed to find the command: " << all_commands[i][0] << std::endl;
					exit(1); // exit w error
				}

			}
		}

		// Need to close pipes for parent process
		for (int i = 0; i < pipe_count; ++i) {
			close(fd[i][0]);
			close(fd[i][1]);
		}

		// if child process is in the foreground, wait for execution
		if (!is_background_process) {
			for (int i = 0; i < pipe_count + 1; ++i) { 
				wait(0); // must consider all piped processes
			}
		} else { // if it is in the background... mimic bash
			std::cout << "[1 instance] " << BACKGROUND_PROCESSES[0] << std::endl;
			BACKGROUND_PROCESSES.clear();
		}

		all_commands.clear();
		current_command.clear();
		x = get_next_command();
		tokens = tokenize(x);
	}
	
}

int main(int argc, char** argv) {
	execute_shell();
    return 0;
}