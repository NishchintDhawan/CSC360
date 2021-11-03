
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/* This code is inspired from the Queue implementation here - https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/ */

struct Queue
{
    int head, tail, size;
    unsigned capacity;
    int *array;
};

struct Queue *constructCustomQueue(unsigned capacity);
int isFull(struct Queue *queue);
int isEmpty(struct Queue *queue);
int enqueue(struct Queue *queue, int item);
int dequeue(struct Queue *queue);
int getHead(struct Queue *queue);
int getTail(struct Queue *queue);

struct Queue *constructCustomQueue(unsigned capacity)
{
    struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));
    queue->capacity = capacity;
    queue->head = queue->size = 0;
    queue->tail = capacity - 1;
    queue->array = (int *)malloc(queue->capacity * sizeof(int));
    return queue;
}

int isFull(struct Queue *queue)
{
    return (queue->size == queue->capacity);
}

int isEmpty(struct Queue *queue)
{
    return (queue->size == 0);
}

int enqueue(struct Queue *queue, int item)
{
    if (isFull(queue))
        return INT_MIN;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->array[queue->tail] = item;
    queue->size = queue->size + 1;
    return 1;
}

int dequeue(struct Queue *queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    int item = queue->array[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

int getHead(struct Queue *queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->head];
}

int getTail(struct Queue *queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->tail];
}