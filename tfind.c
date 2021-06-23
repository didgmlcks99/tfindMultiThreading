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
#include <pthread.h>
#include <sched.h>
#include <assert.h>

//worker process will be initiate after getting user's options
pthread_t *workers;

//default number of worker process
int numProc = 1;

//final result datas of explored information
clock_t t;
// struct timespec begin, end;
int foundLine = 0;
int expFil = 0;
int expDir = 1;
int type_path = 0;
int type_case = 0;
int numDir;
int numKey;
int countKey;
char ** args_thread;

typedef struct __node_t{
	char *value;
	struct __node_t *next;
	pthread_mutex_t node_lock;
} node_t;

typedef struct __queue_t{
	node_t *worker_queue;
	node_t *manager_queue;
} queue_t;

void Queue_Init(node_t *q);

void Queue_Enqueue(node_t *q, char *value);

char* Queue_Dequeue(node_t *q);

void print_queue(node_t *q);

//happens when signal ^c is sent by the user
void term_prog(){
	//getting time result
	t = clock() - t;

	printf("======== SUMMARY TOTAL RESULTS ========\n");
	printf("Found Line : %d\n", foundLine);
	printf("Explored Files : %d\n", expFil);
	printf("Explored Directories : %d\n", expDir);
	printf("Time (Second) : %f\n", (double)t/CLOCKS_PER_SEC);

	printf("Good bye.\n");

	exit(1);
}

void *work(void *queue);


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
	for(int i = 0; i < argc; i++){
		if(args[i][0] == '-'){
			//handles error if user enters options not existing
			if(args[i][1] != 't'){
				printf("Error : no such option as %s\n", args[i]);
				printf("!Existing options : -t <N>\n");
				printf("Terminating Program\n");
				exit(1);
			}else{ numOpt++; }

			//find user input for option of number of worker 
			if(args[i][1] == 't'){
				numProc = args[i+1][0] - '0';

				if(numProc < 17 && numProc > 0){
					numOpt++;
				}else{
					printf("Error : not by rule\n");
					printf("!In this limit : 0 < N < 16\n");
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
	numDir = numOpt + 1;
	numKey = numDir + 1;
	countKey = argc - numKey;

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

	args_thread = args;

	//initiating two different queue for message between threads and manager
	queue_t *q = malloc(sizeof(queue_t));
	q->worker_queue = malloc(sizeof(node_t));
	q->manager_queue = malloc(sizeof(node_t));
    Queue_Init(q->worker_queue);
	Queue_Init(q->manager_queue);

	//make workers in the user given amount with the use of malloc
	workers = (pthread_t*)malloc(numProc * sizeof(pthread_t));
	for(int i = 0; i < numProc; i++){
		pthread_create(&workers[i], NULL, work, q);
// #ifdef DEBUG
// 			printf("==> WORKER[%d] > AT WORK...\n", i);
// #endif
	}

	//start time to check
	t = clock();

	char task[128];
	//first task will be initiated with the directory user have chosen
	strcpy(task, args[numDir]);

	//recognize termination signal from user
	signal(SIGINT, term_prog);

	while(1){
#ifdef DEBUG
		printf("==> MANAGER SENDING TASK : %s\n", task);
#endif

		//manager send task through worker queue
		Queue_Enqueue(q->worker_queue, task);
			
		while(1){
			char *result_name = Queue_Dequeue(q->manager_queue);
			if(result_name == NULL) continue;

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
	return 0;
}

void Queue_Init(node_t *q){
	node_t *tmp = malloc(sizeof(node_t));
	tmp->next = NULL;
	q = tmp;
	pthread_mutex_init(&q->node_lock, NULL);
}

void Queue_Enqueue(node_t *q, char *value){
	node_t *tmp = malloc(sizeof(node_t));
	assert(tmp != NULL);
	tmp->value = value;
	tmp->next = NULL;

	node_t *walk = q;
	while(walk->next != 0x0){
		walk = walk->next;
	}
	
	pthread_mutex_lock(&walk->node_lock);
		walk->next = tmp;
		pthread_mutex_init(&walk->next->node_lock, NULL);
	pthread_mutex_unlock(&walk->node_lock);
}

char* Queue_Dequeue(node_t *q){
	node_t *tmp;
	pthread_mutex_lock(&q->node_lock);
		if(q->next == 0x0){
			pthread_mutex_unlock(&q->node_lock);
			return 0x0;
		}else{
			tmp = q->next;
			q->next = q->next->next;
		}
	pthread_mutex_unlock(&q->node_lock);
	return tmp->value;
}

void print_queue(node_t *q){
	pthread_mutex_lock(&q->node_lock);
		node_t *walk = q;
		while(walk->next != 0x0){
			printf("%s\n", walk->next->value);
			walk = walk->next;
		}
	pthread_mutex_unlock(&q->node_lock);
}

void *work(void *queue){
	queue_t *q = (queue_t *)queue;
	while(1){
		char *task_name = Queue_Dequeue(q->worker_queue);
		if(task_name == NULL) continue;

#ifdef DEBUG
		printf("==> WORKER[x] > RECEIVED TASK : %s...\n", task_name);		
#endif

		//open the directory to be searched
		DIR *directory = opendir(task_name);
		if(directory == NULL){
			printf("ERROR : cannot open this directory '%s'\n", task_name);
			exit(1);
		}

#ifdef DEBUG
		printf("==> WORKER[x] > DIRECTORY '%s' IS SUCCESSFULLY OPENED\n", task_name);
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
				printf("==> WORKER[x] > EXEC RESULT : %s", typed);
#endif

				//flag to identify what result to send for summary
				int flag = 0;
				if(strstr(typed, "ASCII") != NULL || strstr(typed, "text") != NULL){
					/*worker found a ACSII or text version file, 
					open file explore through each line, find for matching keyword,
					then send result regarding to 1. explored file 2. found line*/
#ifdef DEBUG
					printf("WORKER[x] > FILE TYPE : ASCII or text\n");
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
								for(unsigned int j = 0; j <= strlen(args_thread[numKey]); j++){
									args_thread[i][j] = tolower(args_thread[i][j]);
								}
							}
						}
						
						//compare each word from file with the keywords chosen by user,
						int correct;
						for(int i = numKey; i < numKey + countKey; i++){
							correct = 0;
							for(int j = 0; j < ctr; j++){
								if(strcmp(each_word[j], args_thread[i]) == 0){
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

							//send the result_info name for each 'foundLine' to the manager queue
							Queue_Enqueue(q->manager_queue, "foudLine");
						}
						line_num++;
					}
					flag = 1;
				}else if(S_ISDIR(file_mode) && strstr(typed, "directory") != NULL){
					/*worker found a directory,
					send to manager as a new taskv through named pipe, results, 
					then send result regarding to 3. explored directory*/
#ifdef DEBUG
					printf("==> WORKER[x] > FILE TYPE : directory\n");
					printf("==> WORKER[x] > SENT TASK : %s\n", path_name);
#endif
					//send the full path name for the found directory to the manager queue
					Queue_Enqueue(q->manager_queue, path_name);

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

					//send the result_info name for each 'expFil', 'expDir' to the manager queue
					Queue_Enqueue(q->manager_queue, result_info);

					flag = 0;
				}
			}
		}
	}
}