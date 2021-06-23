# pfindMultiProcessing
## Makefile : build script
### 'make' command builds pfind.c to an executable file 'pfind'
+ gcc -W -Wall -pthread -o pfind pfind.c
#### 'make pfind_debug; comand build debug mode for pfind.c to an executable file 'pfind_debug'
+ only in master branch
+ gcc pfind.c -W -Wall -pthread -DDEBUG -o pfind_debug pfind.c
### 'make clean' command automatically removes files (to be used after execution is done) 
#### removed file : pfind, pfind_debug, tasks, results
+ rm pfind pfind_debug tasks results
### step 1 : 'make'
### step 2 : ./pfind [<option>]* <dir> [<keyword>]+
+ existing opetions :
+ -p ; chooses number of worker process ; a number between 1 and 8 should come next after option ; [1 < N < 8] ; default 2
+ -c ; matches word in case-insensitive way ; default case-sensitive 
+ -a ; prints absolute path ; default relative path
+ directory in current directory no need '/'
+ directory not in current directory need absolute path name to be opened and search
+ keyword should be atleast 1 word, with space between each words.
### step 3 : 'make clean'
## video link : https://youtu.be/jeE1SS3wNNQ
