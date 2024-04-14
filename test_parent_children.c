#include "types.h"
#include "fcntl.h"
#include "user.h"

void test_parent_children(void){
       int pid1_1 = fork();
       if(pid1_1 == 0){
            int n = get_parent_id();
            sleep(10);
            printf(1, "      The current process's ID: %d, Parent's IDs: %d.\n", getpid(), n);
            exit();
        }
        int pid1_2 = fork();
        if(pid1_2 == 0){
            int n = get_parent_id();
            sleep(40);
            printf(1, "      The current process's ID: %d, Parent's IDs: %d.\n", getpid(), n);
            exit();
        }
        int pid1_3 = fork();
        if(pid1_3 == 0){
            int n = get_parent_id();
            sleep(70);
            printf(1, "      The current process's ID: %d, Parent's IDs: %d.\n", getpid(), n);
            exit();
        }
        sleep(100);
        int n = get_children(getpid());
        printf(1, "\nThe current process's ID: %d, Children's IDs: %d.\n", getpid(), n);
        wait();
        wait();
        wait();
        exit();
}

int main(int argc, char* argv[]) {
    test_parent_children();
    exit();
}

