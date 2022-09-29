#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>


typedef struct thread_arg {
    int split_num;
    int *arr;
    int arr_size;
    int num;
} thread_arg;

typedef struct answer {
    int ans;
    int exec_time;
} answer;

int thread_result = 0;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;  // Mutex 1 for calculating the result
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;  // Mutex 2 for assigning data

/* This is the task */
int find_element(int *arr, int arr_size, int num) {
    int count = 0;
    for (int i = 0; i < arr_size; ++i) {
        if (*(arr + i) == num) {
            ++count;
        }
    }
    return count;
}

void *child(void *arg) {
    // Receive argument
    thread_arg *parameter =(thread_arg *)arg;
    
    int arr_size = parameter->arr_size;
    int num = parameter->num;
    int thread_num = parameter->split_num;
    int *arr_ptr = parameter->arr;
    pthread_mutex_unlock( &mutex2 );
    //pthread_mutex_lock( &mutex1 );
    int count = find_element(arr_ptr, arr_size, num);
    pthread_mutex_lock( &mutex1 );
    thread_result += count;
    pthread_mutex_unlock( &mutex1 );
    //printf("Thread number %d and the count is %d\n", thread_num, count);
    pthread_exit(NULL);
}


/* Test multi-process */
answer multi_process(int *arr, int arr_size, int split_num, int num) {
    struct timeval tv_start, tv_end;
    struct timezone tz_start, tz_end;
    int count, count_tmp;
    int result = 0;
    answer process_ans;


    printf("##### Multi Process #####\n");
    printf("##### There are %d processes #####\n", split_num);
    gettimeofday(&tv_start, &tz_start);
    /* Parent process */
    // Split into subarrays
    int split_size = arr_size / split_num;
    pid_t pid[split_num];
    
    /* Child process */
    for (int i=0; i<split_num; ++i) {
        if ((pid[i] = fork()) == 0) {
            count_tmp = find_element((arr+i*split_size), split_size, num);
            exit(count_tmp);
        }
    }
    
    /* Parent process */
    // Using waitpid() and printing exit status (count) of children.
    for (int i=0; i<split_num; ++i) {
        pid_t cpid = waitpid(pid[i], &count, 0);
        if (WIFEXITED(count)) {
            //printf("Child %d terminated with status: %d\n",
            //cpid, WEXITSTATUS(count));
            count_tmp = WEXITSTATUS(count);
        }
        result += count_tmp;  // Sum the value returned from child process
    }
    
    // Print the result
    gettimeofday (&tv_end, &tz_end);
    printf("Integer %d occurs %d times in the array\n", num, result);
    printf("execution time: %ld us\n\n", tv_end.tv_usec - tv_start.tv_usec);
    process_ans.ans = result;
    process_ans.exec_time = tv_end.tv_usec - tv_start.tv_usec;
    
    
    return process_ans;
}


answer multi_thread(int *arr, int arr_size, int split_num, int num) {
    struct timeval tv_start, tv_end;
    struct timezone tz_start, tz_end;
    int *count;
    answer thread_ans;

    /* main thread */
    printf("##### Multi Thread #####\n");
    printf("##### There are %d threads #####\n", split_num);
    
    gettimeofday(&tv_start, &tz_start);
    int split_size = arr_size / split_num;
    pthread_t tid[split_num];
    thread_arg arg;
    arg.arr = arr;
    arg.arr_size = split_size;
    arg.num = num;

    /* create multi thread */
    for (int i=0; i<split_num; ++i) {
        pthread_mutex_lock( &mutex2 );
        arg.arr = arr + i * split_size;
        arg.split_num = i;
        pthread_create(&(tid[i]), NULL, child, (void*) &arg);
    }

    /* main thread */
    for (int i=0; i<split_num; ++i) {
        pthread_join(tid[i], NULL);
    }

    gettimeofday (&tv_end, &tz_end);
    printf("Integer %d occurs %d times in the array\n", num, thread_result);
    printf("execution time: %ld us\n\n", tv_end.tv_usec - tv_start.tv_usec);
    thread_ans.ans = thread_result;
    thread_ans.exec_time = tv_end.tv_usec - tv_start.tv_usec;
    
    return thread_ans;
}


int main() {
    int arr_size_[] = {192, 1920, 19200, 192000, 1920000};
    int multi_num_[] = {2, 4, 6, 8, 12, 24, 50, 96, 192};
    int test_times = 100;
    
    answer process_ans, thread_ans;
    /* This set used to check the answer */ 
    //int arr_size_[] = {19200, 192000, 1920000};
    //int multi_num_[] = {4, 8, 12};

    float execution_time_process[sizeof(multi_num_)/sizeof(int)][sizeof(arr_size_)/sizeof(int)];    
    float execution_time_thread[sizeof(multi_num_)/sizeof(int)][sizeof(arr_size_)/sizeof(int)];
    float execution_time_no[sizeof(multi_num_)/sizeof(int)][sizeof(arr_size_)/sizeof(int)];

    struct timeval tv_start, tv_end;
    struct timezone tz_start, tz_end;

    for (int m=0; m<sizeof(arr_size_)/sizeof(int); ++m) {
        for (int l=0; l<sizeof(multi_num_)/sizeof(int); ++l) {
            execution_time_process[l][m] = 0;
            execution_time_thread[l][m] = 0;
            execution_time_no[l][m] = 0;
            int count_for_result_process = 0;
            int count_for_result_thread = 0;
            int count_for_no = 0;
            for (int p=0; p<test_times; ++p) {
                int arr_size = arr_size_[m];  // I change this line to compute with different data size
                int arr[arr_size];
                int find_num = 5;  // The element want to find the times in array
                int multi_num = multi_num_[l];  // I change this line to compute with different process/thread number


                
                /* Generate random array and the correct output*/
                srand(5);  // To make the output the same
                for (int i = 0; i < arr_size; ++i) {
                    arr[i] = rand() % 4196;
                    //printf( "arr[%d] = %d\n", i, arr[i]);
                }

                gettimeofday(&tv_start, &tz_start);
                int ans = find_element(arr, arr_size, find_num);
                gettimeofday(&tv_end, &tz_end);
                printf("#############################");
                printf("\n#          Result           #\n");
                printf("#############################\n\n");
                printf("The array size is %d\n\n", arr_size);
                printf("##### The correct answer #####\n");
                printf("Integer 5 occurs %d times in the array\n", ans);
                printf("execution time: %ld us\n\n", tv_end.tv_usec - tv_start.tv_usec);
                
                if (tv_end.tv_usec - tv_start.tv_usec > 0) {
                    execution_time_no[l][m] += tv_end.tv_usec - tv_start.tv_usec;
                    ++count_for_no;
                }

                /* Check the answer */
                process_ans = multi_process(arr, arr_size, multi_num, find_num);
                if (process_ans.ans != ans) {
                    printf("Test %d\n", p+1);
                    printf("Check the answer...\n");
                    printf("The answer is wrong!!\n");
                    printf("Suspend the process.\n");
                    return 0;
                } else {
                    printf("Test %d\n", p+1);
                    printf("Check the answer...\n");
                    printf("Pass!\n\n");
                }
                thread_ans = multi_thread(arr, arr_size, multi_num, find_num);
                if (thread_ans.ans != ans) {
                    printf("Test %d\n", p+1);
                    printf("Check the answer...\n");
                    printf("The answer is wrong!!\n");
                    printf("Suspend the process.\n");
                    return 0;
                } else {
                    printf("Test %d\n", p+1);
                    printf("Check the answer...\n");
                    printf("Pass!\n\n");
                }

                /* Calculate the average execution time of multi-process */
                if (process_ans.exec_time>0) {
                    ++count_for_result_process;
                    execution_time_process[l][m] += process_ans.exec_time;
                }
                
                /* Calculate the average execution time of multi-thread */
                if (thread_ans.exec_time>0) {
                    ++count_for_result_thread;
                    execution_time_thread[l][m] += thread_ans.exec_time;
                }
                thread_result = 0;
            }
            execution_time_process[l][m] = execution_time_process[l][m] / count_for_result_process;
            execution_time_thread[l][m] = execution_time_thread[l][m] / count_for_result_thread;
            execution_time_no[l][m] = execution_time_no[l][m] / count_for_no;
        }
    }


    /* Print Multi Process Experimental Result */
    printf ("##### Multi Process #####\n");
    for (int m=0; m<sizeof(arr_size_)/sizeof(int); ++m) {
        printf("##### Data size: %d #####\n", arr_size_[m]);
        for (int l=0; l<sizeof(multi_num_)/sizeof(int); ++l) {
            printf("There are %d processes, Avg execution time = %.3f us, No multi-process/thread time = %.3f us\n", \
                    multi_num_[l], execution_time_process[l][m], execution_time_no[l][m]);
        }
    }

    /* Print Multi Thread Experimental Result */
    printf("\n");
    printf ("##### Multi Thread #####\n");
    for (int m=0; m<sizeof(arr_size_)/sizeof(int); ++m) {
        printf("##### Data size: %d #####\n", arr_size_[m]);
        for (int l=0; l<sizeof(multi_num_)/sizeof(int); ++l) {
            printf("There are %d threads, Avg execution time = %.3f us, No multi-process/thread time = %.3f us\n", \
                    multi_num_[l], execution_time_thread[l][m], execution_time_no[l][m]);
        }
    }
    
    return 0;
}
