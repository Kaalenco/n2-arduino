#include "Queue.h"

namespace fifoQueue {

    fifoQueue::Queue::Queue() 
    {
        mQueueHead = 0;
        mQueueTail = 0;
        mNumItems = 0;
        /// Initialize the queue
        memset(mQueue, 0x00, sizeof(mQueue));
    }

    boolean fifoQueue::Queue::push(unsigned char* data )
    {
        // Copy the data to the queue
        for (size_t i = 0; i < QUEUE_ITEM_SIZE; i++)
        {
            mQueue[mQueueTail + i] = data[i];
        }
        mQueueTail = (mQueueTail + QUEUE_ITEM_SIZE) % kQueueSize;

        if ( mNumItems > kQueueSize )
        {
            overFlowValue = true;
        }
        else
        {
            mNumItems++;
        }
        return true;
    }

    boolean fifoQueue::Queue::pop(unsigned char* data)
    {
        if (isEmpty())
        {
            return false;
        }
        // Copy the data from the queue
        for (size_t i = 0; i < QUEUE_ITEM_SIZE; i++)
        {
            data[i] = mQueue[mQueueHead + i];
        }
        mQueueHead = (mQueueHead + QUEUE_ITEM_SIZE) % kQueueSize;
        overFlowValue = false;
        mNumItems--;
        return true;
    }

    boolean fifoQueue::Queue::isEmpty(){
    return mNumItems == 0;
    }

    boolean fifoQueue::Queue::overFlow(){
        return overFlowValue;
    }

    int fifoQueue::Queue::count(){
        return mNumItems;
    }
}