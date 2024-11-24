#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "emufs.h"

pthread_mutex_t reader_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t readers_proceed = PTHREAD_COND_INITIALIZER;
pthread_cond_t writers_proceed = PTHREAD_COND_INITIALIZER;

int waiting_writers = 0;
int active_readers = 0;
int active_writers = 0;

int tcounter = 0;
int current_timestamp = 0;

typedef struct {
    int thread_id;
    int dir_handle;
    double timestamp;
    int type; // 0 for read, 1 for write
    char file_name[10]; // Name of the file to read/write
    char data[1024]; // Data to write or buffer to read into
    int data_size; // Size of the data to write or read
} thread_arg_t;

typedef struct {
    double timestamp;
    int type; // 0 for read, 1 for write
    thread_arg_t* arg;
} operation_t;

operation_t operations[1000];
int operation_count = 0;
int file_sizes[4] = {0}; // Array to keep track of file sizes

double get_time_in_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void add_operation(double timestamp, int type, thread_arg_t* arg) {
    operations[operation_count].timestamp = timestamp;
    operations[operation_count].type = type;
    operations[operation_count].arg = arg;
    operation_count++;
}

int compare_operations(const void* a, const void* b) {
    operation_t* op_a = (operation_t*)a;
    operation_t* op_b = (operation_t*)b;
    return (op_a->timestamp > op_b->timestamp) - (op_a->timestamp < op_b->timestamp);
}

void sort_operations() {
    qsort(operations, operation_count, sizeof(operation_t), compare_operations);
}

void start_read() {
    pthread_mutex_lock(&reader_mutex);
    while (active_writers > 0 || waiting_writers > 0) {
        pthread_cond_wait(&readers_proceed, &reader_mutex);
    }
    active_readers++;
    pthread_mutex_unlock(&reader_mutex);
}

void end_read() {
    pthread_mutex_lock(&reader_mutex);
    active_readers--;
    if (active_readers == 0) {
        pthread_cond_signal(&writers_proceed);
    }
    pthread_mutex_unlock(&reader_mutex);
}

void start_write() {
    pthread_mutex_lock(&writer_mutex);
    waiting_writers++;
    while (active_writers > 0 || active_readers > 0) {
        pthread_cond_wait(&writers_proceed, &writer_mutex);
    }
    waiting_writers--;
    active_writers++;
    pthread_mutex_unlock(&writer_mutex);
}

void end_write() {
    pthread_mutex_lock(&writer_mutex);
    active_writers--;
    if (waiting_writers > 0) {
        pthread_cond_signal(&writers_proceed);
    } else {
        pthread_cond_broadcast(&readers_proceed);
    }
    pthread_mutex_unlock(&writer_mutex);
}

void generate_random_requests(int num_requests, int dir_handle) {
    srand(time(NULL));
    const char* files[] = {"file1", "file2", "file3", "file4"};
    for (int i = 0; i < num_requests; i++) {
        thread_arg_t* arg = (thread_arg_t*)malloc(sizeof(thread_arg_t));
        arg->thread_id = i;
        arg->dir_handle = dir_handle;
        arg->timestamp = tcounter++;
        
        // Adjust the probability to make read requests 3 times more likely than write requests
        if (rand() % 4 < 3) { // 75% chance for read, 25% chance for write
            arg->type = 0; // Read operation
        } else {
            arg->type = 1; // Write operation
        }
        
        strcpy(arg->file_name, files[rand() % 4]); // Randomly choose one of the four files
        
        if (arg->type == 1) { // Write operation
            arg->data_size = 1; // Single byte data
            arg->data[0] = 'A' + (rand() % 26); // Random single character data
            arg->data[1] = '\0'; // Null-terminate the string
        } else { // Read operation
            int file_index = arg->file_name[4] - '1'; // Get the file index from the file name
            arg->data_size = file_sizes[file_index]; // Read up to the current size of the file
        }
        
        add_operation(arg->timestamp, arg->type, arg);
    }
}

void* thread_func(void* arg) {
    thread_arg_t* thread_arg = (thread_arg_t*)arg;
    int thread_id = thread_arg->thread_id;
    
    // Wait until it's this thread's turn to execute
    pthread_mutex_lock(&writer_mutex);
    while (thread_arg->timestamp != current_timestamp) {
        pthread_cond_wait(&writers_proceed, &writer_mutex);
    }
    pthread_mutex_unlock(&writer_mutex);
    
    if (thread_arg->type == 1) { // Write operation
        start_write();
        int fd = open_file(thread_arg->dir_handle, thread_arg->file_name);
        // sleep(1); // Simulate CPU operation
        emufs_write(fd, thread_arg->data, thread_arg->data_size);
        printf("Thread %d wrote data to %s at %f\n", thread_id, thread_arg->file_name, thread_arg->timestamp);
        emufs_close(fd, 0);
        end_write();
    } else { // Read operation
        start_read();
        int fd = open_file(thread_arg->dir_handle, thread_arg->file_name);
        char buf[1024] = {0}; // Initialize read buffer
        // sleep(1); // Simulate CPU operation
        emufs_read(fd, buf, thread_arg->data_size);
        printf("Thread %d read data from %s at %f: %s\n", thread_id, thread_arg->file_name, thread_arg->timestamp, buf);
        emufs_close(fd, 0);
        end_read();
    }
    
    // Increment the global timestamp counter
    pthread_mutex_lock(&writer_mutex);
    current_timestamp++;
    pthread_cond_broadcast(&writers_proceed);
    pthread_mutex_unlock(&writer_mutex);
    
    return NULL;
}

void execute_single_threaded() {
    for (int i = 0; i < operation_count; i++) {
        if (operations[i].type == 1) { // Write operation
            start_write();
            int fd = open_file(operations[i].arg->dir_handle, operations[i].arg->file_name);
            // sleep(1); // Simulate CPU operation
            emufs_write(fd, operations[i].arg->data, operations[i].arg->data_size);
            printf("Single-threaded: Thread %d wrote data to %s at %f\n", operations[i].arg->thread_id, operations[i].arg->file_name, operations[i].timestamp);
            emufs_close(fd, 0);
            end_write();
        } else { // Read operation
            start_read();
            int fd = open_file(operations[i].arg->dir_handle, operations[i].arg->file_name);
            char buf[1024] = {0}; // Initialize read buffer
            // sleep(1); // Simulate CPU operation
            emufs_read(fd, buf, operations[i].arg->data_size);
            printf("Single-threaded: Thread %d read data from %s at %f: %s\n", operations[i].arg->thread_id, operations[i].arg->file_name, operations[i].timestamp, buf);
            emufs_close(fd, 0);
            end_read();
        }
    }
}

void execute_multithreaded() {
    pthread_t threads[operation_count];
    
    // Create threads in the order of their timestamps
    for (int i = 0; i < operation_count; i++) {
        pthread_create(&threads[i], NULL, thread_func, operations[i].arg);
    }

    // Join threads
    for (int i = 0; i < operation_count; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_requests>\n", argv[0]);
        return 1;
    }

    int num_requests = atoi(argv[1]);
    int mnt2 = opendevice("disk5", 60);
    if (mnt2 == -1) {
        printf("error!\n");
        return 0;
    }
    if (create_file_system(mnt2, 0) == -1) {
        printf("error!\n");
        return 0;
    }

    int dir2 = open_root(mnt2);
    emufs_create(dir2, "file1", 0);
    emufs_create(dir2, "file2", 0);
    emufs_create(dir2, "file3", 0);
    emufs_create(dir2, "file4", 0);

    // Write initial data to each file
    int fd1 = open_file(dir2, "file1");
    char str1[] = "!-----------------------64 Bytes of Data-----------------------!!-----------------------64 Bytes of Data-----------------------!!-----------------------64 Bytes of Data-----------------------!!-----------------------64 Bytes of Data-----------------------!";
    emufs_write(fd1, str1, 64*4);
    emufs_close(fd1, 0);
    file_sizes[0] = 64 * 4;

    int fd2 = open_file(dir2, "file2");
    emufs_write(fd2, str1, 64*4);
    emufs_close(fd2, 0);
    file_sizes[1] = 64 * 4;

    int fd3 = open_file(dir2, "file3");
    emufs_write(fd3, str1, 64*4);
    emufs_close(fd3, 0);
    file_sizes[2] = 64 * 4;

    int fd4 = open_file(dir2, "file4");
    emufs_write(fd4, str1, 64*4);
    emufs_close(fd4, 0);
    file_sizes[3] = 64 * 4;

    // Generate random requests
    generate_random_requests(num_requests, dir2);

    // Sort operations by timestamp
    sort_operations();

    // Execute single-threaded
    double start_time = get_time_in_seconds();
    execute_single_threaded();
    double end_time = get_time_in_seconds();
    double single_threaded_time = end_time - start_time;
    // printf("\n\nSingle-threaded execution time: %f seconds\n\n", single_threaded_time);

    sleep(3);

    // Execute multithreaded
    start_time = get_time_in_seconds();
    execute_multithreaded();
    end_time = get_time_in_seconds();
    double multithreaded_time = end_time - start_time;

    printf("\n\nSingle-threaded execution time: %f seconds\n", single_threaded_time);
    printf("Multithreaded execution time: %f seconds\n", multithreaded_time);

    fsdump(mnt2);
    return 0;
}