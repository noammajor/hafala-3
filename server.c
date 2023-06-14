#include "segel.h"
#include "request.h"

typedef struct Task {
    int taskFd;
} Task;

enum schedalg {block,dt,dh,bf,dynamic, randout };

typedef struct QueueTasks
{
    Task* QueueWaiting;
    Task* QueueRunning;
    int sizeWaiting;
    int sizeRunning;
    int maxTasks;
    int dynamicMax; // current max, the maxTasks is the final max
    enum schedalg typeOfOperation;
} QueueTasks;

QueueTasks queueTasks;
pthread_t* ThreadPool;
pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;   // if the first task arrived
pthread_cond_t condListen;      // if receiving new tasks is allowed

void Add_Task(Task task)
{
    queueTasks->QueueWaiting[queueTasks.sizeWaiting] = task;
    queueTasks->sizeWaiting++;
}

void remove_Queue(int pos)
{
    for ( ; pos < queueTasks->sizeWaiting-1 ; pos++) {
        queueTasks->QueueWaiting[pos] = queueTasks->QueueWaiting[pos + 1];
    }
    queueTasks->QueueWaiting[queueTasks->sizeWaiting - 1] = NULL;
    queue->sizeWaiting--;
}


void submitTask(Task task) {
    pthread_mutex_lock(&mutexQueue);
    if(queueTasks.sizeRunning + queueTasks.sizeWaiting > queueTasks.maxTasks)
    {
        if(queueTasks.typeOfOperation == dh)
        {
            remove_Queue(0);
            Add_Task(task);
        }
        else if(queueTasks.typeOfOperation == randout)
        {
            srand(time(NULL));
            int num = queueTasks.sizeWaiting/2;
            for (int i = 0 ; i < num; ++i)
            {
                int randomNumber = rand() % (queueTasks.maxTasks - i);
                remove_Queue(randomNumber);
            }
            Add_Task(task);
        }
        else // dynamic or dt
        {
            close(task.taskFd);
        }
    }
    queueTasks.QueueWaiting[queueTasks.sizeWaiting] = task;
    Queue.sizeWaiting++;
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
}

void* startThread(void* args) {
    while (1) {
        Task task;
        pthread_mutex_lock(&mutexQueue);
        while (queueTasks.sizeWaiting == 0) {
            pthread_cond_wait(&condQueue, &mutexQueue);
        }

        task = queueTasks.QueueWaiting[0];
        for (int i = 0 ; i < queueTasks.sizeWaiting - 1 ; i++) {
            queueTasks.QueueWaiting[i] = queueTasks.QueueWaiting[i + 1];
        }
        queueTasks.QueueRunning[queueTasks.sizeRunning] = task;
        queueTasks.sizeWaiting--;
        queueTasks.sizeRunning++;
        if (queueTasks.sizeWaiting + queueTasks.sizeRunning < queueTasks.maxTasks) {
            if (queueTasks.typeOfOperation == block ||
                (queueTasks.typeOfOperation == bf && queueTasks.sizeWaiting == 0 && queueTasks.sizeRunning == 0)
                    pthread_cond_signal(&condListen);
        }
        pthread_mutex_unlock(&mutexQueue);
        requestHandle(task.taskFd);
        Close(task.taskFd);
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
    int size = atoi(argv[2]);
    ThreadPool = malloc(sizeof(pthread_t)*size);
    for (int i = 0; i < size; ++i)
    {
        if (pthread_create(&pool[i], NULL, &startThread, NULL) != 0) {
            perror("Failed to create the thread");
        }
    }
    queueTasks.maxTasks = atoi(argv[3]);
    queueTasks.QueueRunning = malloc(sizeof (Task)*queueTasks.maxTasks);
    queueTasks.QueueWaiting = malloc(sizeof (Task)*queueTasks.maxTasks);
    switch (argv[4])
    {
        case "block":
            queueTasks.typeOfOperation = block;
            break;
        case "dt":
            queueTasks.typeOfOperation = dt;
            break;
        case "dh":
            queueTasks.typeOfOperation = dh;
            break;
        case "bf":
            queueTasks.typeOfOperation = bf;
            break;
        case "dynamic":
            queueTasks.typeOfOperation = dynamic;
            break;
        case "random":
            queueTasks.typeOfOperation = randout;
            break;
        default:
            fprintf(stderr,"error - wrong arg");
            exit(0);
    }
    queueTasks.dynamicMax = 0;
    if (argc > 5) {
        queueTasks.dynamicMax = queueTasks.maxTasks;
        queueTasks.maxTasks = atoi(argv[5]);;
    }
    queueTasks.sizeWaiting = 0;
    queueTasks.sizeRunning = 0;
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, numRunning, numWaiting, max;
    struct sockaddr_in clientaddr;

    getargs(&port, argc, argv);

    pthread_mutex_lock(&mutexQueue);
    numRunning = queueTasks.sizeRunning;
    numWaiting = queueTasks.sizeWaiting;
    max = queueTasks.maxTasks;


    while (1) {
        listenfd = Open_listenfd(port);
        if (queueTasks.typeOfOperation == dynamic && numWaiting + numRunning < max && numWaiting + numRunning == queueTasks.dynamicMax)
        {    // more than original count but less than allowed dynamically
            queueTasks.dynamicMax++;
            listenfd = Open_listenfd(port);
        }
        else  if (queueTasks.typeOfOperation == block || queueTasks.typeOfOperation == bf) {
            if (numWaiting + numRunning == max)
                pthread_cond_wait(&condListen, &mutexQueue);
            if (queueTasks.typeOfOperation == bf)
                listenfd = Open_listenfd(port);
        }

        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t * ) & clientlen);
        Task task (connfd);
        submitTask(task);
            //
            // HW3: In general, don't handle the request in the main thread.
            // Save the relevant info in a buffer and have one of the worker threads
            // do the work.
            //

            //requestHandle(connfd);
            //Close(connfd);
        }
    }

}


    


 
