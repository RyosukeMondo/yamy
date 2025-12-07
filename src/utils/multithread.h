#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// multithread.h


#ifndef _MULTITHREAD_H
#  define _MULTITHREAD_H

#  include <mutex>


///
class SyncObject
{
public:
    ///
    virtual void acquire() = 0;
    ///
    virtual void acquire(int ) {
        acquire();
    }
    ///
    virtual void release() = 0;
};


///
class CriticalSection : public SyncObject
{
    std::recursive_mutex m_mutex;            ///

public:
    ///
    CriticalSection() {
    }
    ///
    ~CriticalSection() {
    }
    ///
    void acquire() override {
        m_mutex.lock();
    }
    ///
    void release() override {
        m_mutex.unlock();
    }
};


///
class Acquire
{
    SyncObject *m_so;    ///

public:
    ///
    Acquire(SyncObject *i_so) : m_so(i_so) {
        m_so->acquire();
    }
    ///
    Acquire(SyncObject *i_so, int i_n) : m_so(i_so) {
        m_so->acquire(i_n);
    }
    ///
    ~Acquire() {
        m_so->release();
    }
};


#endif // !_MULTITHREAD_H
