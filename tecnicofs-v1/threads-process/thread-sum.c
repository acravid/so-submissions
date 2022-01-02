#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// Threads and summing numbers from an array 


// given the array primes we want to obtain 
// the sums of its elements,given two threads
// our goal is to: for each thread, sum exactly 
// half of the array of primes
// e.g = [2,3,5,7,11,13,17,19,23,29]
// thread 1 -> sum[2,3,5,7,11]
// thread 2 -> sum[13,17,19,23,29]
// and lastly sum each thread half sum
// sum[] = sum(thread1) + sum(thread2)

int primes[10] = {2,3,5,7,11,13,17,19,23,29};

void* routine(void* arg) {
    int index = *(int*)arg;
    int sum = 0;
    for (int j = 0; j < 5; j++) {
        sum += primes[index + j];
    }
    printf("Local sum: %d\n", sum);
    *(int*)arg = sum;
    return arg;
}

int main(int argc, char* argv[]) {

    pthread_t th[2];
    int i;
    for(i = 0; i < 2; i++) {

        int* a = malloc(sizeof(int));
        // thread 1 -> index = 0;
        // thread 2 -> index = 5
        *a = i*5;
        if(pthread_create(&th[i], NULL, &routine, a) != 0) {

            perror("Failed to create thread");
        }
    }

    int threads_sum = 0;
    for(i = 0; i < 2; i++) {
        int* res;
        if(pthread_join(th[i], (void**)&res) != 0){
            perror("Failed to join thread");
        }
        threads_sum += *res;
        free(res);
    }
    printf("thread 1 + thread 2: %d\n", threads_sum);

    return 0;
}