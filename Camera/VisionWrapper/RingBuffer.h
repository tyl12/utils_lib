#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <iostream>
#include <unistd.h>
#include <cassert>
using namespace std;

//#define DEBUG_RING_BUFFER

int self_test();

// Thread safe circular buffer
template <typename T>
class RingBuffer
{
    public:
        void push(const T& data) {
            assert((_write+1)%mLength != _read && "push() called on full queue");
#ifdef DEBUG_RIGN_BUFFER
            cout<<__FUNCTION__<<":"<<data<<endl;
#endif
            mQueue[_write] = data;
            _write = (_write+1)%mLength;
        }
        void push_force(const T& data) {
#ifdef DEBUG_RIGN_BUFFER
            cout<<__FUNCTION__<<":"<<data<<endl;
#endif
            if (isFull()){
                pop();
            }
            push(data);
        }
        const T top() {
            assert(_read != _write && "top() called on empty queue");
            return mQueue[_read];
        }
        void pop() {
            assert(_read != _write && "pop() called on empty queue");
#ifdef DEBUG_RIGN_BUFFER
            cout<<__FUNCTION__<<":"<<mQueue[_read]<<endl;
#endif
            _read = (_read+1) % mLength;
        }
        void clear(){
#ifdef DEBUG_RIGN_BUFFER
            cout<<__FUNCTION__<<endl;
#endif
            _read = _write = 0;

        }
        size_t size() {
            if (_write >= _read)
                return _write - _read;
            else{
                return mLength - (_read - _write);
            }
        }
        size_t capacity() {
            return mLength - 1;
        }

        bool isFull(){
            return size() == mLength -1;
        }
        bool isEmpty(){
            return size() == 0;
        }

        virtual ~RingBuffer(){
#ifdef DEBUG_RIGN_BUFFER
            cout<<__FUNCTION__<<endl;
#endif
cout<<"ring "<<__LINE__<<endl;
            if (mQueue != nullptr)
                delete []mQueue;
cout<<"ring "<<__LINE__<<endl;
            mQueue = nullptr;
            _write = _read = 0;
        }

        RingBuffer(){
#ifdef DEBUG_RIGN_BUFFER
            cout<<__FUNCTION__<<endl;
#endif
            mQueue = nullptr;
            mLength = 0;
            _write = _read = 0;
        }

        RingBuffer(size_t capacity){
#ifdef DEBUG_RIGN_BUFFER
            cout<<__FUNCTION__<<":"<<capacity<<endl;
#endif
            mQueue = nullptr;
            set_capacity(capacity);
        }
        void set_capacity(size_t capacity) {
            assert( capacity > 0 );
            if (mQueue != nullptr)
                delete []mQueue;
            mLength = capacity + 1;
            mQueue = new T[mLength];
            _write = _read = 0;
        }

        void show(){
            printf(">>>>>\n");
            printf("length=%ld, capacity=%ld, size=%ld, isFull=%s, isEmpty=%s, write=%ld, read=%ld\n",
                    mLength, capacity(), size(), isFull()?"true":"false", isEmpty()?"true":"false",_write, _read);
#ifdef DEBUG_RIGN_BUFFER
            if (!isEmpty()){
                printf("contents:\n");
                size_t cnt = 0;
                for (;cnt<size();cnt++){
                    int idx = (_read + cnt)%mLength;
                    std::cout<<mQueue[idx]<<std::endl;
                }
            }
#endif
            printf("<<<<<\n");
        }

    private:
        T* mQueue;
        size_t mLength;
        size_t _write;
        size_t _read;
    public:
        friend int self_test();
};

#endif
