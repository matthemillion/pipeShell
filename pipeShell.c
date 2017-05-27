/**
 *	Matthew Hunt
 *	pipeShell in C99 for Unix/Linux OS
 *	version: 5
 *
 *	Dr. Zhao
 *	Operating Systems
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>


// insert '\0' at first delimiter
void nullAtDelimiter(char *, char);

// get input from user
int getInput(char *, size_t);

// get argument sets
size_t getCommandSets(char *, char **,
					  const size_t, const size_t);

// run single command without pipe
void singleCommand(char **);

// run multiple commands with pipes
void multiCommand(size_t, char **,
				  const size_t);


/*******************************************************************/


// pipeShell
int main() {

	// input buffer
	const size_t INPUT_SIZE = 64;
	char input[INPUT_SIZE + 1];
	input[INPUT_SIZE] = '\0';

	// command sets
	const size_t MAX_SETS = 8, MAX_ARGS = 8;
	char *command_sets[MAX_SETS * MAX_ARGS];

	// number of command sets
	size_t number_of_sets;

	while(getInput(input, INPUT_SIZE)) {

		number_of_sets = getCommandSets(input, command_sets,
										MAX_SETS, MAX_ARGS);		

		if(number_of_sets == 1)
			singleCommand(command_sets);

		if(number_of_sets > 1)
			multiCommand(number_of_sets, command_sets, MAX_ARGS);
	}

	exit(0);
}


/*******************************************************************/


// insert '\0' at first delimiter
void nullAtDelimiter(char *string, char d) {
	for(int i = 0; string[i] != '\0'; ++i) {
		if(string[i] == d) {
			string[i] = '\0';
			return;
		}
	}
}

// get input from user
int getInput(char *input, const size_t INPUT_SIZE) {

	// prompt
	printf("pipeShell > ");

	// clear input buffer
	memset(input, 0, INPUT_SIZE * sizeof(char));

	// get input if available
	if(fgets(input, INPUT_SIZE, stdin) == NULL) {
		perror("input error\n");
		exit(1);
	}

	// '\0' at '\n'
	nullAtDelimiter(input, '\n');

	// check for "exit" or "quit"
	if(strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
		printf("Peace Out\n");
		return 0;
	}

	return 1;
}

// get command sets
size_t getCommandSets(char *input, char **command_sets,
					  const size_t MAX_SETS, const size_t MAX_ARGS) {

	// set each char pointer to NULL
	for(int i = 0; i < MAX_SETS; ++i)
		for(int j = 0; j < MAX_ARGS; ++j)
			command_sets[i * MAX_ARGS + j] = NULL;

	// position in input, and token
	char *set_position, *arg_position, *set_token, *arg_token;

	// set count
	size_t number_of_sets = 0;

	set_token = strtok_r(input, "|", &set_position);
	for(int i = 0; i < MAX_SETS; ++i) {

		if(set_token != NULL) {
			++number_of_sets;
			
			arg_token = strtok_r(set_token, " ", &arg_position);
			for(int j = 0; j < MAX_ARGS; ++j) {
				
				if(arg_token != NULL) {
					command_sets[i * MAX_ARGS + j] = arg_token;
					arg_token = strtok_r(NULL, " ", &arg_position);
				} else
					break;
			}

			set_token = strtok_r(NULL, "|", &set_position);

		} else
			break;
	}

	return number_of_sets;
}

// run single command without pipe
void singleCommand(char **command_sets) {

	pid_t child_pid;
	
	// child
	if((int) (child_pid = fork()) < 0) {
		perror("fork error\n");
		return;
	} else if((int) child_pid == 0) {
		execvp(*command_sets, command_sets);

		// handle execution errors
		perror("invalid arguments\n");
		_exit(1);
	}

	// parent waits for child
	wait(NULL);
}

// run multiple commands with pipes
void multiCommand(size_t number_of_sets, char **command_sets,
				  const size_t MAX_ARGS) {

	const int MAX_PIPES = 7;

	int pipes[2 * MAX_PIPES],
		n = 2 * ((int) number_of_sets - 1),
		pipe_position = 0,
		command = 0;

	pid_t child_pid; 

	// ensure argument number is not greater than pipe capacity
	if(n > 2 * MAX_PIPES) {
		perror("too many arguments\n");
		return;
	}

	// make pipes
	for(int i = 0; i < n; i += 2) {
		if(pipe(pipes + i) < 0) {
			perror("pipe error\n");
			_exit(1);
		}
	}

	// first child
	if((int) (child_pid = fork()) < 0) {
		perror("fork error\n");
		_exit(1);
	} else if((int) child_pid == 0) {
		dup2(pipes[pipe_position + 1], 1);

		for(int i = 0; i < n; ++i)
			// if(i != pipe_position + 1)
				close(pipes[i]);

		execvp(*command_sets, command_sets);

		// handle execution errors
		perror("invalid arguments\n");
		_exit(1);
	}

	++command;

	// middle children
	for(int i = 1; i < (int) number_of_sets - 1; ++i) {
		if((int) (child_pid = fork()) < 0) {
			perror("fork error\n");
			_exit(1);
		} else if((int) child_pid == 0) {
			dup2(pipes[pipe_position], 0);
			dup2(pipes[pipe_position + 3], 1);

			for(int i = 0; i < n; ++i)
				// if(i != pipe_position && i != pipe_position + 3)
					close(pipes[i]);

			execvp(*(command_sets + command * MAX_ARGS),
				   command_sets + command * MAX_ARGS);

			// handle execution errors
			perror("invalid arguments\n");
			_exit(1);
		}

		pipe_position += 2;
		++command;
	}

	// last child
	if((int) (child_pid = fork()) < 0) {
		perror("fork error\n");
		_exit(1);
	} else if((int) child_pid == 0) {
		dup2(pipes[pipe_position], 0);

		for(int i = 0; i < n; ++i)
			// if(i != pipe_position)
				close(pipes[i]);

		execvp(*(command_sets + command * MAX_ARGS),
			   command_sets + command * MAX_ARGS);

		// handle execution errors
		perror("invalid arguments\n");
		_exit(1);
	}

	for(int i = 0; i < n; ++i)
		close(pipes[i]);

	for(int i = 0; i < (int) number_of_sets; ++i)
		wait(NULL);
}

