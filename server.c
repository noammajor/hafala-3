#include "segel.h"
#include "request.h"

typedef struct Task {
    int confd;
} Task;

enum schedalg {block,dt,dh,bf,dynamic, randout }
typedef struct QueueTasks
{
    Task* QueueWaiting;
    Task* QueueUsed;
    int sizeWaiting;
    int sizeUsed;
    int maxTasks;
    enum schedalg typeOfOperation;
}QueueTasks;

pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;

void submitTask(Task task,struct QueueTasks Queue) {
    pthread_mutex_lock(&mutexQueue);
    if(Queue.sizeUsed+Queue.sizeWaiting>Queue.maxTasks)
    {





    }
    taskQueue[taskCount] = task;
    taskCount++;
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
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
void getargs(int *port, int argc, char *argv[],struct QueueTasks* TasksQueue)
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    TasksQueue->maxTasks = atoi(argv[3]);
    TasksQueue->QueueUsed = malloc(sizeof (Task)*TasksQueue->maxTasks);
    TasksQueue->QueueWaiting = malloc(sizeof (Task)*TasksQueue->maxTasks);
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


    


 
