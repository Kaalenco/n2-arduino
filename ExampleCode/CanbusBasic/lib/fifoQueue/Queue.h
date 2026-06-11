#pragma once

#include <Arduino.h>

// Define the size of the queue item
#ifndef QUEUE_ITEM_SIZE
#define QUEUE_ITEM_SIZE 8
#endif

// Define the size of the queue
#ifndef QUEUE_SIZE
#define QUEUE_SIZE 8 * QUEUE_ITEM_SIZE
#endif

namespace fifoQueue
{
    class Queue
    {
    public:
        // Constructor
        Queue();

        // Add an item to the queue
        boolean push(unsigned char* data );

        // Remove an item from the queue
        boolean pop(unsigned char* data);

        // Check if the queue is overflown
        boolean overFlow();

        // Check if the queue is empty
        boolean isEmpty();

        // Get the number of items in the queue
        int count();

    private:

        static const int kQueueSize = QUEUE_SIZE;

        struct QueueItem
        {
            char* data;
        };

        // The queue
        char mQueue[ QUEUE_SIZE ];

        // Index of event queue head
        int mQueueHead;

        bool overFlowValue;

        // Index of event queue tail
        int mQueueTail;

        // Actual number of items in queue
        int mNumItems;
    };
}
