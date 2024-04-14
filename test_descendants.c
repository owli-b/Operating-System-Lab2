#include "types.h"
#include "fcntl.h"
#include "user.h"

void test_descendants(void){
       int pid1_1 = fork();
       if(pid1_1 == 0){
            sleep(10);
            exit();
        }
        int pid1_2 = fork();
        if(pid1_2 == 0){
            int pid1_2_1 = fork();
            if(pid1_2_1 == 0){
            	sleep(10);
            	exit();
            }
            int pid1_2_2 = fork();
            if(pid1_2_2 == 0){
            	sleep(50);
            	exit();
            }
            int pid1_2_3 = fork();
            if(pid1_2_3 == 0){
            	sleep(100);
            	exit();
            }
            sleep(150);
            wait();
            wait();
            wait();
            exit();
        }
        sleep(10);
        int n = get_descendants(getpid());
        printf(1, "\nThe current process's ID: %d, Descendants's IDs: %d.\n", getpid(), n);
        wait();
        wait();
        exit();
}

int main(int argc, char* argv[]) {
    test_descendants();
    exit();
}

