#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "svec.h"
#include "tokenize.h"

#define MAX_SIZE 256

int
find_index(svec* str, char* want) 
{
	int found = -1;
	for (int ii = 0; ii < str->size; ++ii) {
		 if (strcmp(svec_get(str, ii), want) == 0) {
			return ii;
		}
	}
	return found;
}

void 
redirectin(char* prog, svec* cmds, int index) 
{	
	int cpid;
	if ((cpid = fork())) {
		int status;
               	waitpid(cpid, &status, 0);
		return;
	} else {
		char* args[index + 1];

		for(int ii = 0; ii < index; ++ii) {
			args[ii] = svec_get(cmds, ii);	
		}
		args[index] = 0;

		char* file_path = svec_get(cmds, index + 1);
		FILE *input = freopen(file_path, "r", stdin);

		execvp(prog, args);
		fclose(input);	
	}
}

void
redirectout(char* prog, svec* cmds, int index)
{
	int cpid;
        if ((cpid = fork())) {
                int status;
                waitpid(cpid, &status, 0);
                return;
        } else {	
		char* args[index + 1];
                for(int ii = 0; ii < index; ++ii) {
			args[ii] = svec_get(cmds, ii);
      		}
		args[index] = 0;	
		char* file_path = svec_get(cmds, index + 1);
                
		FILE *output = freopen(file_path, "w", stdout);
                execvp(prog, args);
                fclose(output);
	}
}

// sort-pipe.c : This code is from Nat's lecture notes: 09-fork-exec
void
check_rv(int rv)
{
    if (rv == -1) {
        perror("fail");
        exit(1);
    }
}

void execute(char* command);

char*
flatten(svec* lst) 
{
	char result[100] = "";
	for (int ii = 0; ii < lst->size; ++ii) {
		strcat(result, svec_get(lst, ii));
		strcat(result, " ");	
	}

	char* word = malloc(100*sizeof(char));
	memcpy(word, result, sizeof(result));	
	return word;

}

void
handle_pipe(char* prog, svec* cmds, int index) {
	int cpid;
	int pipe_fds[2];
	int rv = pipe(pipe_fds);
	check_rv(rv);
       

	char* left[index + 1];
		
	for(int ii = 0; ii < index; ++ii) {
		left[ii] = svec_get(cmds, ii);
	}		

	left[index] = 0;
		
	svec* right_side = make_svec();
				
	for(int ii = 0; ii < cmds->size - index - 1; ++ii) {
		svec_push_back(right_side, svec_get(cmds, ii + index + 1));
	}

	char* flattened_right = flatten(right_side);
   	

 
	if ((cpid = fork())) {
                int status;
                waitpid(cpid, &status, 0);
		
		close(0);
		dup(pipe_fds[0]);
		close(pipe_fds[1]);
	
		execute(flattened_right);
		free(flattened_right);
	     
	        return;
        } else {
		close(1);
		dup(pipe_fds[1]);
		close(pipe_fds[0]);
		
		execvp(prog, left);
	
	}
}

void
background(char* prog, svec* cmds, int index)
{
	int cpid;
	if((cpid = fork())) {
		return;
	} else {
		char* args[index + 1];
                for(int ii = 0; ii < index; ++ii) {
                        args[ii] = svec_get(cmds, ii);
                }
                args[index] = 0;
 
                execvp(prog, args);
        }
}

void 
handle_and_or(char* prog, svec* cmds, int index, int is_and) 
{
	int cpid;
	if ((cpid = fork())) {
		int status;
                waitpid(cpid, &status, 0);
		if ((is_and == 0 && status == 0) || (is_and == 1 && status != 0)) {	
			svec* right = make_svec();
				
			for(int ii = 0; ii < cmds->size - index - 1; ++ii) {
				svec_push_back(right, svec_get(cmds, ii + index + 1));
			}

			char* flattened_right = flatten(right);
   			execute(flattened_right);
			free(flattened_right);
		}
		return;
	} else {
	
		char* left[index + 1];

                for(int ii = 0; ii < index; ++ii) {
			left[ii] = svec_get(cmds, ii);
		}	
		
	        left[index] = 0;

		execvp(prog, left);
	}
}

void
semicolon (char* prog, svec* cmds, int index)
{
	svec* left = make_svec();

	for(int ii = 0; ii < index - 1; ++ii) {
		svec_push_back(left, svec_get(cmds, ii));
	}

	char* flattened_left = flatten(left);
		
	svec* right = make_svec();
				
	for(int ii = 0; ii < cmds->size - index - 1; ++ii) {
		svec_push_back(right, svec_get(cmds, ii + index + 1));
	}

	char* flattened_right = flatten(right);
   	
	execute(flattened_left);
	free(flattened_left);
	execute(flattened_right);
	free(flattened_right);
}

void
handle_parens(svec* cmds, int index_left, int index_right)
{
	int cpid;
	if ((cpid = fork())) {
		int status;
                waitpid(cpid, &status, 0);
	
		
		return;
	} else {
		svec* inside = make_svec();
				
		for(int ii = 0; ii < index_right - index_left - 1; ++ii) {
			svec_push_back(inside, svec_get(cmds, index_left + 1 + ii));
		}

		char* flattened_inside = flatten(inside);
  		execute(flattened_inside);
		free(flattened_inside);
				
	}

}
void
execute(char* cmd)
{
	svec* cmds = tokenize(cmd);

	int cpid;

	char* com = svec_get(cmds, 0);	
	
	if (find_index(cmds, "|") != -1) {
		handle_pipe(com, cmds, find_index(cmds, "|"));
		return;
	}	
	
	if (find_index(cmds, "<") != -1) {
		redirectin(com, cmds, find_index(cmds, "<"));
		return;
	}

	if (find_index(cmds, ">") != -1) {
		redirectout(com, cmds, find_index(cmds, ">"));
		return;
	}

	if (find_index(cmds, ";") != -1) {
		semicolon(com, cmds, find_index(cmds, ";"));
		return;
	}
	
	if (find_index(cmds, "&") != -1) {	
		background(com, cmds, find_index(cmds, "&"));
		return;
	}

	if (find_index(cmds, "&&") != -1) {
		handle_and_or(com, cmds, find_index(cmds, "&&"), 0);
                return;
        }

	if (find_index(cmds, "||") != -1) {
		handle_and_or(com, cmds, find_index(cmds, "||"), 1);
		return;
	}

	if (find_index(cmds, "(") != -1) {
		handle_parens(cmds, find_index(cmds, "("), find_index(cmds, ")"));
		return;
	}

	if (find_index(cmds, "exit") != -1) {	
		exit(0);
	}	

	if (find_index(cmds, "cd") != -1) {
		
		char cwd[256];
		getcwd(cwd, sizeof(cwd));
		int index = find_index(cmds, "cd");
			
		char *arg = svec_get(cmds, index + 1);
		strcat(cwd, "/");
		strcat(cwd, arg);
		chdir(cwd);
		return;
	}
			

	if ((cpid = fork())) {	
		// parent process
		int status;
		waitpid(cpid, &status, 0);
	}
	
	else {
		// child process
		for (int ii = 0; ii < strlen(cmd); ++ii) {
			if (cmd[ii] == ' ') {
				cmd[ii] = 0;
				break;
			}
		}	
		char* args[1 + cmds->size];

		for (int ii = 0; ii < cmds->size; ++ii) {
			char* token = svec_get(cmds, ii);
			args[ii] = token;
		}
		args[cmds->size] = 0;	
		execvp(cmd, args);
	}
		free_svec(cmds);

}

int
main(int argc, char* argv[])
{
	char cmd[MAX_SIZE];

		if (argc == 1) {
			while(1) {
				printf("nush$ ");
				fflush(stdout);
				char* rv = fgets(cmd, MAX_SIZE, stdin);
				if (!rv) {
					break;
				}
				execute(cmd);
				}	
		}
		else {
			char* file_path = argv[1];
		        FILE *input = fopen(file_path, "r");
			
			while(fgets(cmd, MAX_SIZE, input)) {	
				execute(cmd);
			}
			fclose(input);
		}
	return 0;
}
