#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

int count = 0;
pthread_mutex_t mtx;
pthread_cond_t cond1;
pthread_cond_t cond2;

int arr[] = {2000, 2001, 2002, 2003, 2004};

bool is_leap (int year) {
    if ((year % 4) == 0) {
        if ((year % 100) == 0) {
            if ((year % 400) == 0) 
                return true;
            else 
                return false;
        } else {
            return true;
        }
    } else {
        return false;
    }
}

/* Print leap */
void* threadc1(void * args) {
    int num = *(int *)args;
    bool is;

    while (count < num) {
       pthread_mutex_lock(&mtx);
       while((count < num) && ((is = is_leap(arr[count])) == false)) {
           pthread_cond_wait(&cond1, &mtx);  
       }
       if (count >= num) {
           pthread_mutex_unlock(&mtx);
           break;
       }
       while (is == true) { 
           printf("thread1: %d: Leap Year\n", arr[count]);
           count++;
           if (count >= num) {
               pthread_mutex_unlock(&mtx);
               break;
           }
           is = is_leap(arr[count]);
       }
       pthread_cond_signal(&cond2);
       pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

/* Print non-leap */
void* threadc2(void * args) {
    int num = *(int *)args;
    bool is;

    while (count < num) {
       pthread_mutex_lock(&mtx);
       while((count < num) && ((is = is_leap(arr[count])) == true)) {
           pthread_cond_wait(&cond2, &mtx);  
       }
       if (count >= num) {
           pthread_mutex_unlock(&mtx);
           break;
       }
       while (is == false) {
           printf("thread2: %d: Non-Leap Year\n", arr[count]);
           count++;
           if (count >= num) {
               pthread_mutex_unlock(&mtx);
               break;
           }
           is = is_leap(arr[count]);
       }
       pthread_cond_signal(&cond1);
       pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

int main (void) {

    pthread_t thread1, thread2;
    int num_elem = sizeof(arr)/sizeof(arr[0]);
   
    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&cond1, NULL);
    pthread_cond_init(&cond2, NULL);

    pthread_create(&thread1, NULL, *threadc1, &num_elem); 

    pthread_create(&thread2, NULL, *threadc2, &num_elem); 
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
}
