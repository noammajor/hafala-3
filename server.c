#include "segel.h"
#include "request.h"



typedef struct QueueTasks
{
    Task* QueueWaiting;
    int sizeWaiting;
    int sizeRunning;
    int maxTasks;
    int dynamicMax; // current max, the maxTasks is the final max
    char* typeOfOperation;
} QueueTasks;


Statistics statsThreads;
QueueTasks queueTasks;
pthread_t* ThreadPool;
pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;   // if the first task arrived
pthread_cond_t condListen;      // if receiving new tasks is allowed

int listenSignal;

void Add_Task(Task task)
{
    queueTasks.QueueWaiting[queueTasks.sizeWaiting] = task;
    queueTasks.sizeWaiting++;
}

void remove_Queue(int pos)
{
    close(queueTasks.QueueWaiting[pos].taskFd);
    for ( ; pos < queueTasks.sizeWaiting-1 ; pos++) {
        queueTasks.QueueWaiting[pos] = queueTasks.QueueWaiting[pos + 1];
    }
    queueTasks.sizeWaiting--;
}


void submitTask(Task task) {
    pthread_mutex_lock(&mutexQueue);
    if(queueTasks.sizeRunning + queueTasks.sizeWaiting == queueTasks.maxTasks)
    {
        if(strcmp(queueTasks.typeOfOperation, "dh") == 0)
        {
            remove_Queue(0);
        }
        else if(strcmp(queueTasks.typeOfOperation, "random") == 0)
        {
            srand(time(NULL));
            int num = (queueTasks.sizeWaiting + 1)/2;
            for (int i = 0 ; i < num; i++)
            {
                int randomNumber = rand() % (queueTasks.sizeWaiting);
                remove_Queue(randomNumber);
            }
        }
        else // dynamic or dt
        {
            pthread_mutex_unlock(&mutexQueue);
            pthread_cond_signal(&condQueue);
            close(task.taskFd);
            return;
        }
    }
    Add_Task(task);
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
}

void* startThread(void* args) {
    int* index = (int*)args;
    Task task;
    while (1) {
        pthread_mutex_lock(&mutexQueue);
        while (queueTasks.sizeWaiting == 0) {
            pthread_cond_wait(&condQueue, &mutexQueue);
        }

        task = queueTasks.QueueWaiting[0];
        struct timeval time_of_handling;
        gettimeofday(&time_of_handling, NULL);
        timersub(&time_of_handling, &task.arrival, &task.BeginOperation);

        for (int i = 0 ; i < queueTasks.sizeWaiting - 1 ; i++) {
            queueTasks.QueueWaiting[i] = queueTasks.QueueWaiting[i + 1];
        }

        queueTasks.sizeWaiting--;
        queueTasks.sizeRunning++;

        pthread_mutex_unlock(&mutexQueue);

       /* if (listenSignal)
            pthread_cond_signal(&condListen);*/

        //pthread_cond_signal(&condQueue);

        requestHandle(task, index, &statsThreads);
        close(task.taskFd);

        pthread_mutex_lock(&mutexQueue);
        queueTasks.sizeRunning--;
        if (strcmp(queueTasks.typeOfOperation, "bf") == 0 && queueTasks.sizeWaiting == 0 && queueTasks.sizeRunning == 0)
                listenSignal = 1;

        pthread_mutex_unlock(&mutexQueue);
        if (listenSignal)
            pthread_cond_signal(&condListen);

        //pthread_cond_signal(&condQueue);
    }
}


// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// HW3: Parse the new arguments too
void getargs(int *port, int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    int size = atoi(argv[2]); // size = number of threads

    queueTasks.maxTasks = atoi(argv[3]);
    statsThreads.DynamicRequests = malloc(sizeof(int)*size);
    statsThreads.StaticRequests = malloc(sizeof(int)*size);
    statsThreads.Requests = malloc(sizeof(int)*size);
    for (int i = 0 ; i < size ; i++)
    {
        statsThreads.DynamicRequests[i] = 0;
        statsThreads.StaticRequests[i]= 0;
        statsThreads.Requests[i] = 0;
    }

    queueTasks.dynamicMax = 0;
    if (argc > 5)
    {
        queueTasks.dynamicMax = queueTasks.maxTasks;
        queueTasks.maxTasks = atoi(argv[5]);;
    }

    queueTasks.QueueWaiting = malloc(sizeof (Task)*queueTasks.maxTasks);
    queueTasks.typeOfOperation = argv[4];

    queueTasks.sizeWaiting = 0;
    queueTasks.sizeRunning = 0;

    ThreadPool = malloc(sizeof(pthread_t)*size);
    for (int i = 0 ; i < size ; i++)
    {
        int* num = malloc(sizeof(int));
        *num = i;
        if (pthread_create(&ThreadPool[i], NULL, &startThread, num) != 0) {
            perror("Failed to create the thread");
        }
    }
}


int main(int argc, char *argv[]) {
    int listenfd, port, clientlen;
    struct sockaddr_in clientaddr;
    Task task;

    listenSignal = 1;

    pthread_mutex_init(&mutexQueue, NULL);
    pthread_cond_init(&condQueue, NULL);
    pthread_cond_init(&condListen, NULL);
    getargs(&port, argc, argv);

    listenfd = Open_listenfd(port);
    clientlen = sizeof(clientaddr);

    while (1) {
        task.taskFd = Accept(listenfd, (SA *) &clientaddr, (socklen_t * ) & clientlen);
        gettimeofday(&task.arrival, NULL);

        pthread_mutex_lock(&mutexQueue);

        if (strcmp(queueTasks.typeOfOperation, "dynamic") == 0 && queueTasks.sizeWaiting + queueTasks.sizeRunning < queueTasks.maxTasks
                && queueTasks.sizeWaiting + queueTasks.sizeRunning == queueTasks.dynamicMax)
        {    // more than original count but less than allowed dynamically
            queueTasks.dynamicMax++;
            pthread_mutex_unlock(&mutexQueue);
            pthread_cond_signal(&condQueue);
            close(task.taskFd);
            task.taskFd = Accept(listenfd, (SA *) &clientaddr, (socklen_t * ) & clientlen);
        }
        else if ((strcmp(queueTasks.typeOfOperation, "block") == 0 || strcmp(queueTasks.typeOfOperation, "bf") == 0)
                && queueTasks.sizeWaiting + queueTasks.sizeRunning == queueTasks.maxTasks)
        {
            while (queueTasks.sizeWaiting + queueTasks.sizeRunning == queueTasks.maxTasks)
                pthread_cond_wait(&condListen, &mutexQueue);
            if (strcmp(queueTasks.typeOfOperation, "bf") == 0 ) {
                if (queueTasks.sizeWaiting != 0 || queueTasks.sizeRunning != 0)
                    listenSignal = 0;
                while (queueTasks.sizeWaiting != 0 || queueTasks.sizeRunning != 0)
                    pthread_cond_wait(&condListen, &mutexQueue);
                pthread_mutex_unlock(&mutexQueue);
                close(task.taskFd);
                task.taskFd = Accept(listenfd, (SA *) &clientaddr, (socklen_t * ) & clientlen);
            }
            else {
                pthread_mutex_unlock(&mutexQueue);
            }

        }
        else        //can add the task
            pthread_mutex_unlock(&mutexQueue);

        submitTask(task);
        pthread_cond_signal(&condQueue);
    }

    return 0;
}


    


 
