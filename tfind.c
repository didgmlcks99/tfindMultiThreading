#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/file.h>

//worker process will be initiate after getting user's options
pid_t *workers;

//number of worker process
int numProc = 2;

//final result datas of explored information
struct timespec begin, end;
int foundLine = 0;
int expFil = 0;
int expDir = 1;

//return the size of the information that will be sent to named pipe
int write_bytes(int fd, void * a, int len){
	char * s = (char *) a;

	int i = 0;
	while(i < len){
		int b;
		b = write(fd, s+i, len-i);
		if(b == 0) break;
		i += b;
	}
	return i;
}

//return the size of the information that should be read in the named pipe
int read_bytes (int fd, void * a, int len){
	char * s = (char *) a ;
	
	int i ;
	for (i = 0 ; i < len ; ) {
		int b ;
		b = read(fd, s+i, len-i) ;
		if (b == 0)
			break ;
		i += b ;
	}
	return i ; 
}

//happens when signal ^c is sent by the user
void term_prog(){
	//getting time result
	clock_gettime(CLOCK_MONOTONIC, &end);
	long time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec);

	//killing all worker process
	for(int i = 0; i < numProc; i++){
		kill(workers[i], SIGCHLD);	
	}

	printf("======== SUMMARY TOTAL RESULTS ========\n");
	printf("Found Line : %d\n", foundLine);
	printf("Explored Files : %d\n", expFil);
	printf("Explored Directories : %d\n", expDir);
	printf("Time (Second) : %lf\n", (double)time/1000000000);

	printf("Good bye.\n");

	exit(1);
}


int main(int argc, char** args){
#ifdef DEBUG
	printf("==> THIS IS DEBUG MODE <==\n");
#endif

#ifdef DEBUG
	printf("==> NUMBER OF ARGS : %d\n", argc);
	printf("==> INPUT : ");
	for(int i = 0; i < argc; i++){
		printf("%s ", args[i]);
	}printf("\n");
#endif

	//check # of option
	int numOpt = 0;
	int type_path = 0;
	int type_case = 0;
	for(int i = 0; i < argc; i++){
		if(args[i][0] == '-'){
			//handles error if user enters options not existing
			if(args[i][1] != 'p' && args[i][1] != 'c' && args[i][1] != 'a'){
				printf("Error : no such option as %s\n", args[i]);
				printf("!Existing options : -p <N>, -c, -a\n");
				printf("Terminating Program\n");
				exit(1);
			}else{ numOpt++; }

			//find user input for option of number of worker 
			if(args[i][1] == 'p'){
				numProc = args[i+1][0] - '0';

				if(numProc < 8 && numProc > 1){
					numOpt++;
				}else{
					printf("Error : not by rule\n");
					printf("!In this limit : 1 < N < 8\n");
					printf("Terminating Program\n");
					exit(1);
				}
			}

			//find user input for option -a regarding to type of path
			if(args[i][1] == 'a'){
				type_path = 1;
			}

			//find user input for option -c regarding to type of case
			if(args[i][1] == 'c'){
				type_case = 1;
			}
		}
	}

#ifdef DEBUG
	printf("==> NUMBER OF OPTIONS : %d\n", numOpt);
	printf("==> OPTIONS : ");
	for(int i = 1; i <= numOpt; i++){
		printf("%s ", args[i]);
	}printf("\n");
	printf("==> # OF WORKERS : %d\n", numProc);
	printf("==> PATH TYPE : %d\n", type_path);
	printf("==> CASE TYPE : %d\n", type_case);
#endif

	//save space # for directory and start of keyword
	int numDir = numOpt + 1;
	int numKey = numDir + 1;
	int countKey = argc - numKey;

#ifdef DEBUG
	printf("==> DIR SPACE : %d\n", numDir);
	printf("==> KEY SPACE : %d\n", numKey);
	printf("==> # OF KEYWORDS : %d\n", countKey);
	printf("==> DIRECTORY NAME : %s\n", args[numDir]);
	printf("==> KEYWORD NAMES : ");
	for(int i = numKey; i < numKey + countKey; i++){
		printf("%s ", args[i]);
	}printf("\n");
#endif

	//handle error when input is wrong
	if(args[numDir] == NULL || args[numKey] == NULL || argc < 3){
		printf("Error : invalid input\n");
		printf("!Include both : directory and keyword\n");
		printf("Terminating Program\n");
		exit(0);
	}

	//make named pipe : tasks
	if (mkfifo("tasks", 0666)) {
		if (errno != EEXIST) {
			perror("fail to open fifo: ") ;
			exit(1) ;
		}
	}

	//make named pipe : results
	if (mkfifo("results", 0666)) {
		if (errno != EEXIST) {
			perror("fail to open fifo: ") ;
			exit(1) ;
		}
	}

	//make workers in the user given amount with the use of malloc
	workers = (pid_t*)malloc(numProc * sizeof(pid_t));
	for(int i = 0; i < numProc; i++){
		workers[i] = fork();
		if(workers[i] == 0){
#ifdef DEBUG
			printf("==> WORKER[%d] > AT WORK...\n", i);
#endif
			int tasks_rec = open("tasks", O_RDONLY | O_SYNC);
			int results_send = open("results", O_WRONLY | O_SYNC);
			
			while(1){
				//look into named pipe 'tasks' for a task, (directory name)
				char task_name[128];
				size_t len, bs;

				flock(tasks_rec, LOCK_EX);

				if(read_bytes(tasks_rec, &len, sizeof(len)) != sizeof(len)){
					flock(tasks_rec, LOCK_UN);
					continue;
				}

				bs = read_bytes(tasks_rec, task_name, len);
				task_name[bs] = 0x0;

				flock(tasks_rec, LOCK_UN);

#ifdef DEBUG
				printf("==> WORKER[%d] > RECEIVED TASK : %s...\n", i, task_name);		
#endif

				//open the directory to be searched
				DIR *directory = opendir(task_name);
				if(directory == NULL){
					printf("ERROR : cannot open this directory '%s'\n", task_name);
					exit(1);
				}

#ifdef DEBUG
				printf("==> WORKER[%d] > DIRECTORY '%s' IS SUCCESSFULLY OPENED\n", i, task_name);
#endif

				//looks into each file of the given directory
				struct dirent *each_file;
				int return_stat;
				struct stat file_info;
				mode_t file_mode;
				while((each_file = readdir(directory)) != NULL){
					char path_name[50];
					if(strcmp(each_file->d_name, "..") == 0 || strcmp(each_file->d_name, ".") == 0){}
					else{
						sprintf(path_name, "%s/%s", task_name, each_file->d_name);
						
						if((return_stat = stat(path_name, &file_info)) == -1){
							perror("Error : \n");
							exit(1);
						}
						file_mode = file_info.st_mode;

						//gets command from file to check file type
						FILE *fp = NULL;
						char typed[10240] = "";

						char check_file[100];
						sprintf(check_file, "%s %s", "file", path_name);

						if((fp = popen(check_file, "r")) == NULL){
							printf("Error: cannot pipe open this file\n");
							printf("Terminating Program\n");
							exit(1);
						}

						fgets(typed, 10240, fp);

#ifdef DEBUG
						printf("==> WORKER[%d] > EXEC RESULT : %s", i, typed);
#endif

						//flag to identify what result to send for summary
						int flag = 0;
						if(strstr(typed, "ASCII") != NULL || strstr(typed, "text") != NULL){
							/*worker found a ACSII or text version file, 
							open file explore through each line, find for matching keyword,
							then send result regarding to 1. explored file 2. found line*/
#ifdef DEBUG
							printf("WORKER[%d] > FILE TYPE : ASCII or text\n", i);
#endif
							//open detected file to be examined
							FILE *text_file = fopen(path_name, "r");
							if(!text_file){
								printf("Error : cannot open this file '%s'\n", path_name);
								continue;
							}

							//examine each line
							char line[256];
							int line_num = 0;
							while(fgets(line, 256, text_file)){
								char each_word[100][20];
								int ctr = 0;
								int j = 0;
								//crop it by each word
								for(unsigned int i = 0; i <= strlen(line); i++){
									if(line[i] == ' ' || line[i] == '\n'){
										each_word[ctr][j] = '\0';
										ctr++;
										j = 0;
									}else{
										if(type_case == 1){
											each_word[ctr][j] = tolower(line[i]);
										}else{
											each_word[ctr][j] = line[i];
										}
										j++;
									}
								}

								if(type_case == 1){
									for(int i = numKey; i < numKey + countKey; i++){
										for(unsigned int j = 0; j <= strlen(args[numKey]); j++){
											args[i][j] = tolower(args[i][j]);
										}
									}
								}
								
								//compare each word from file with the keywords chosen by user,
								int correct;
								for(int i = numKey; i < numKey + countKey; i++){
									correct = 0;
									for(int j = 0; j < ctr; j++){
										if(strcmp(each_word[j], args[i]) == 0){
											correct = 1;
											break;
										}
									}
									if(correct == 0){
										break;
									}
								}

								//if and only if, all the keywords are found than the line will be printed
								if(correct == 1){
									//print result regarding to type of path
									if(type_path == 1){
										printf("%s : %d : %s", realpath(path_name, NULL), line_num, line);
									}else{
										printf("%s : %d : %s", path_name, line_num, line);
									}

									size_t cor_len = strlen("foundLine");
									flock(results_send, LOCK_EX);
									//send the line_cor name for each 'foundLine' to the named pipe, results
									if(write_bytes(results_send, &cor_len, sizeof(cor_len)) != sizeof(cor_len)){}
									if((unsigned int)write_bytes(results_send, "foundLine", cor_len) != cor_len){
										flock(results_send, LOCK_UN);
										break;
									}
									flock(results_send, LOCK_UN);
								}
								line_num++;
							}
							flag = 1;
						}else if(S_ISDIR(file_mode) && strstr(typed, "directory") != NULL){
							/*worker found a directory,
							send to manager as a new taskv through named pipe, results, 
							then send result regarding to 3. explored directory*/
#ifdef DEBUG
							printf("==> WORKER[%d] > FILE TYPE : directory\n", i);
							printf("==> WORKER[%d] > SENT TASK : %s\n", i, path_name);
#endif
							size_t len = strlen(path_name);
							flock(results_send, LOCK_EX);
							//send the full path name for the found directory to the named pipe, results
							if(write_bytes(results_send, &len, sizeof(len)) != sizeof(len)){}
							if((unsigned int)write_bytes(results_send, path_name, len) != len){
								flock(results_send, LOCK_UN);
								break;
							}
							flock(results_send, LOCK_UN);

							flag = 2;
						}else{
#ifdef DEBUG
							printf("FILE TYPE : not regular\n");
#endif
							//non regular file
							flag = 0;
						}

						if(flag != 0){
							char result_info[20];

							if(flag == 1) strcpy(result_info, "expFil");
							if(flag == 2) strcpy(result_info, "expDir");

							size_t result_len = strlen(result_info);
							flock(results_send, LOCK_EX);
							//send the result_info name for each 'expFil', 'expDir' to the named pipe, results
							if(write_bytes(results_send, &result_len, sizeof(result_len)) != sizeof(result_len)){}
							if((unsigned int)write_bytes(results_send, result_info, result_len) != result_len){
								flock(results_send, LOCK_UN);
								break;
							}
							flock(results_send, LOCK_UN);

							flag = 0;
						}
					}
				}
			}
			close(tasks_rec);
			close(results_send);
			exit(1);
		}
	}

	//start time to check
	clock_gettime(CLOCK_MONOTONIC, &begin);

	char task[128];
	//first task will be initiated with the directory user have chosen
	strcpy(task, args[numDir]);

	char result_name[128];

	int tasks_send = open("tasks", O_WRONLY | O_SYNC);
	int results_rec = open("results", O_RDONLY | O_SYNC);

	//recognize termination signal from user
	signal(SIGINT, term_prog);

	while(1){
#ifdef DEBUG
		printf("==> MANAGER SENDING TASK : %s\n", task);
#endif

		size_t len = strlen(task);
		//manager send task through named pipe tasks
		if(write_bytes(tasks_send, &len, sizeof(len)) != sizeof(len)){}
		if((unsigned int)write_bytes(tasks_send, task, len) != len){}
			
		while(1){
			size_t length, bs;
			//get the sent results from a worker through named pipe, results
			if(read_bytes(results_rec, &length, sizeof(length)) != sizeof(length)){}

			bs = read_bytes(results_rec, result_name, length);
			result_name[bs] = 0x0;

#ifdef DEBUG
			printf("==> MANAGER RECEIVED TASK : %s\n", result_name);
#endif

			if(strcmp(result_name, "expDir") == 0){
				expDir++;
				continue;
			}else if(strcmp(result_name, "expFil") == 0){
				expFil++;
				continue;
			}else if(strcmp(result_name, "foundLine") == 0){
				foundLine++;
				continue;
			}else{
				//copy the result to task for manager to send to workers
				strcpy(task, result_name);
				break;
			}
		}
	}
	close(tasks_send);
	close(results_rec);

	return 0;
}
