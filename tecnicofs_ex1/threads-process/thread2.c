#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int mails = 0;
pthread_mutex_t mutex;



void* routine() {


    for(int i = 0; i < 1000000; i++) {
        pthread_mutex_lock(&mutex);
        mails++;
        pthread_mutex_unlock(&mutex);
    }
}


int main() {

    pthread_t th[5]; // we're creating 5 threads

    pthread_mutex_init(&mutex,NULL);

    // NOT GOOD

    // sequencial execution, not parallel

    //for(int i = 0; i < 5; i++) {

     //   if(pthread_create( &th[i],NULL,&routine,NULL) != 0) { // testing for errors
     //       perror("Failed to create thread\n");
     //        return 1;
     //   }
     //   printf("Thread %d has started its execution\n", i);
     //   if(pthread_join(th[i],NULL) != 0) {
            
     //       return 2;
     //   }

     //  printf("Thread %d has finished its execution\n", i);
     // }


    // Our goal is to create parallel threads using for loop // parallelism 

    // how can we do it 

    for(int i = 0; i < 5; i++) {

        if(pthread_create( &th[i],NULL,&routine,NULL) != 0) { // testing for errors
            perror("Failed to create thread\n");
            return 1;
        }
        printf("Thread %d has started its execution\n", i);

    }


    for(int i = 0; i < 5 ; i++) {

        if(pthread_join(th[i], NULL) != 0) { // testing for errors
            return 2;
        }
        printf("Thread %d has finished its execution\n", i);
    }

    pthread_mutex_destroy(&mutex);

    return 0;
}