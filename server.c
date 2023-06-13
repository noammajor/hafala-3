#include "segel.h"
#include "request.h"

typedef struct Task {
    int confd;
} Task;

enum schedalg {block,dt,dh,bf,dynamic, randout };
typedef struct QueueTasks
{
    Task* QueueWaiting;
    Task* QueueUsed;
    int sizeWaiting;
    int sizeUsed;
    int maxTasks;
    enum schedalg typeOfOperation;
}QueueTasks;
pthread_t* ThreadPool;

pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;
void Add_Queue(struct QueueTasks* queue)
{
    queue->QueueWaiting[queue.sizeWaiting];
    queue->sizeWaiting++;
}
void remove_Queue(struct QueueTasks* queue,int pos)
{
    for (i = pos; i < queue->sizeWaiting; i++) {
        queue->QueueWaiting[i] = queue->QueueWaiting[i + 1];
    }
    queue->QueueWaiting[queue->sizeWaiting-1]=NULL;
    queue->sizeWaiting--;
}


void submitTask(Task task,struct QueueTasks Queue) {
    pthread_mutex_lock(&mutexQueue);
    if(Queue.sizeUsed+Queue.sizeWaiting>Queue.maxTasks)
    {
        if(Queue.typeOfOperation==dh)
        {
            remove_Queue(Queue,0);
            Add_Queue(Queue,task);
        }
        else if(Queue.typeOfOperation==dynamic || Queue.typeOfOperation==dt)
        {
            close(task.confd);
        }
        else if(Queue.typeOfOperation==randout)
        {
            srand(time(NULL));
            int num = Queue.sizeWaiting/2;
            for (int i = 0; i < num; ++i)
            {
                int randomNumber = rand() % Queue.maxTasks;
                remove_Queue(Queue,randomNumber);
            }
            Add_Queue(Queue,task);
        }
    }
    taskQueue[taskCount] = task;
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
}
void* startThread(void* args) {
    while (1) {
        Task task;
        pthread_mutex_lock(&mutexQueue);
        while (taskCount == 0) {
            pthread_cond_wait(&condQueue, &mutexQueue);
        }

        task = taskQueue[0];
        int i;
        for (i = 0; i < taskCount - 1; i++) {
            taskQueue[i] = taskQueue[i + 1];
        }
        taskCount--;
        pthread_mutex_unlock(&mutexQueue);
        executeTask(&task);
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
void getargs(int *port, int argc, char *argv[],struct QueueTasks* TasksQueue,pthread_t* pool)
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    int size = atoi(argv[2]);
    pool = malloc(sizeof(pthread_t)*size);
    for (int i = 0; i < size; ++i)
    {
        if (pthread_create(&pool[i], NULL, &startThread, NULL) != 0) {
            perror("Failed to create the thread");
        }
    }
    TasksQueue->maxTasks = atoi(argv[3]);
    TasksQueue->QueueUsed = malloc(sizeof (Task)*TasksQueue->maxTasks);
    TasksQueue->QueueWaiting = malloc(sizeof (Task)*TasksQueue->maxTasks);
    switch (argv[4])
    {
        case "block":
            TasksQueue->typeOfOperation=block;
            break;
        case "dt":
            TasksQueue->typeOfOperation=dt;
            break;
        case "dh":
            TasksQueue->typeOfOperation=dh;
            break;
        case "bf":
            TasksQueue->typeOfOperation=bf;
            break;
        case "dynamic":
            TasksQueue->typeOfOperation=dynamic;
            break;
        case "random":
            TasksQueue->typeOfOperation=randout;
            break;
        default:
            fprintf(stderr,"error - wrong arg");
            exit(0);
    }
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    getargs(&port, argc, argv);

    // 
    // HW3: Create some threads...
    //

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 
	requestHandle(connfd);

	Close(connfd);
    }

}


    


 
