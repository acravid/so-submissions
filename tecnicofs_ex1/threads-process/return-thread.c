include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>


//TODO : Make this program run multiple threads

// What if we want to roll eight dices at the same time , parallelism 



pthread_mutex_t mutex;



// get return value from a thread -> pthread_join 
// the answer has to do with the second parameter of the function
// pthread_join  (pthread_t __th, void **__thread_return)

void* roll_dice() {
    

    // rand() % 6 gives us values between 0 and 5 we have to add 1 
    // in order to have values between 1 and 6 
    pthread_mutex_lock(&mutex);

    int value = rand() % 6 + 1;
    int* res = malloc(sizeof(int));
    *res = value;

    printf("%d\n", value);
    pthread_mutex_unlock(&mutex);
    // we can return  a reference
    // we can not pass the reference of a local variable 
    // since its located in the stack 

    return (void*)res;

}


int main(int argc, char *argv[]) {

    int* res; // defining a pointer to an integer
    srand(time(NULL));
    // we now have a total of eight threads
    pthread_t th[8];

    for(int i = 0; i < 8; i++) {
        if(pthread_create(&th[i] /*or th + i*/, NULL, &roll_dice, NULL) != 0) { // tests for errors
            perror("Failed to create thread");
            return 1;
        }
    }

    for(int i = 0; i < 8; i++) {
        if(pthread_join(th[i], (void**)&res) != 0) { // tests for errors
            return 2;
        }
        printf("Result of the rolldice %d\n", *res);
    }

  
    free(res);
    return 0;
}