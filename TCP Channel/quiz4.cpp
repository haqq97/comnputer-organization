#include<stdio.h> 
#include<unistd.h> 
#include<sys/wait.h> 
#include<sys/types.h> 
#include <ctime>
void signal_handler (int signo){
 	printf ("Got SIGUSR1\n");
 }
int main (){
	clock_t timer;
	timer = clock();
 	signal (SIGUSR1, signal_handler); //comment out for b)
 	int pid = fork ();
 	if (pid == 0){// chilld process
 		for (int i=0; i<5; i++){
 		 	kill(getppid(), SIGUSR1);
			sleep (1);
		}
	}else{	// parent process
		wait(0);
	}
	timer = clock() - timer;
	cout << "It took " << (float)timer/CLOCKS_PER_SEC << " seconds" << endl;
}
