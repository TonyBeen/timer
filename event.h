#ifndef __EVENT_H__
#define __EVENT_H__

#include <thread>

#include "timer.h"

#define DEFAULT_WAIT_TIME   50     // ms

#ifdef __linux__
typedef int32_t event_t; // for eventfd
#elif defined(_WIN32) || defined(_WIN64)
typedef void*   event_t;
#endif

class CEvent : public CTimerManager
{
public:
    typedef std::shared_ptr<CEvent> SP;

    CEvent();
    ~CEvent();

    bool start();
    void stop();

protected:
    void    _ThreadEntryWrapper();
    void    _ThreadEntry();
    void    _Tickle();
    void    _OnTimerInsertedAtFront() override;

    enum ThreadState {
        Waiting = 0,    // 等待事件状态
        Processing = 1, // 处理事件状态
        Exited = 2,     // 已退出
    };

private:
    event_t                 m_eventFd;      // eventfd/Windows Event
#ifdef __linux__
    int32_t                 m_epollFd;      // epoll
#endif
    std::thread             m_evnetThread;  // 线程句柄
    std::atomic<uint8_t>    m_state;        // 线程状态
    std::atomic<bool>       m_keepAlive;    // 保持线程存在
};

#endif
