#include "event.h"

#include <string.h>

#ifdef __linux__
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sched.h>
#include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64)
// #include <synchapi.h>
// #include <errhandlingapi.h>
// #include <WinBase.h>
#include <windows.h>
#endif

#define EPOLL_EVENT_SIZE        8
#define INVALID_EVENT_FD        0

CEvent::CEvent() :
    m_eventFd(INVALID_EVENT_FD),
    m_state(ThreadState::Exited),
    m_keepAlive(false)
{
#ifdef __linux__
    m_epollFd = epoll_create(EPOLL_EVENT_SIZE);
    if (m_epollFd < 0)
    {
        printf("epoll_create error. [%d, %s]", errno, strerror(errno));
        throw std::exception();
    }
    m_eventFd = eventfd(0, EFD_NONBLOCK);
    if (m_eventFd < 0)
    {
        m_eventFd = INVALID_EVENT_FD;
        printf("eventfd error. [%d, %s]", errno, strerror(errno));
        throw std::exception();
    }

    epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = m_eventFd;
    int32_t nRet = epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_eventFd, &ev);
    if (nRet < 0)
    {
        printf("epoll_ctl error. [%d, %s]", errno, strerror(errno));
        throw std::exception();
    }
#elif defined(_WIN32) || defined(_WIN64)
    m_eventFd = CreateEvent(nullptr, false, false, nullptr);
    if (nullptr == m_eventFd)
    {
        printf("CreateEvent error: %u", (uint32_t)GetLastError());
        throw std::exception();
    }
    ResetEvent(m_eventFd);
#endif
}

CEvent::~CEvent()
{
    stop();
}

bool CEvent::start()
{
#ifdef __linux__
    if (m_epollFd < 0)
    {
        return false;
    }
#endif

    if (INVALID_EVENT_FD == m_eventFd)
    {
        return false;
    }

    if (m_keepAlive.load(std::memory_order_relaxed))
    {
        return true;
    }

    m_keepAlive.store(true, std::memory_order_relaxed);
    try
    {
        m_evnetThread = std::thread(std::bind(&CEvent::_ThreadEntryWrapper, this));
    }
    catch (...)
    {
        return false;
    }
    return true;
}

void CEvent::stop()
{
#ifdef __linux__
    if (m_epollFd < 0)
    {
        return;
    }
#endif

    if (INVALID_EVENT_FD == m_eventFd)
    {
        return;
    }

    m_keepAlive.store(false, std::memory_order_relaxed);
    _Tickle();

    if (m_evnetThread.joinable())
    {
        m_evnetThread.join();
    }
#ifdef __linux__
    close(m_epollFd);
    m_epollFd = -1;
    close(m_eventFd);
#elif defined(_WIN32) || defined(_WIN64)
    CloseHandle(m_eventFd);
#endif

    m_eventFd = INVALID_EVENT_FD;
}

void CEvent::_ThreadEntryWrapper()
{
    try
    {
        _ThreadEntry();
    }
    catch (...)
    {
    }
}

void CEvent::_ThreadEntry()
{
#ifdef __linux__
    epoll_event eventVec[EPOLL_EVENT_SIZE];
    epoll_event *pEvents = eventVec;
#endif

    while (m_keepAlive.load(std::memory_order_relaxed))
    {
        m_state = ThreadState::Waiting;
        uint32_t timeout = DEFAULT_WAIT_TIME;
        uint32_t nearTimeOut = (uint32_t)GetNearTimeout();
        timeout = nearTimeOut > DEFAULT_WAIT_TIME ? DEFAULT_WAIT_TIME : nearTimeOut;
#ifdef __linux__
        int32_t nEvent = 0;
        do {
            nEvent = epoll_wait(m_epollFd, pEvents, EPOLL_EVENT_SIZE, timeout);
            if (nEvent < 0 && errno == EINTR)
            {
                // Interrupted system call
                sched_yield(); // sleep 700ns
            }
            else
            {
                break;
            }

            // 系统中断需要重新计算等待时间
            nearTimeOut = (uint32_t)GetNearTimeout();
            timeout = nearTimeOut > DEFAULT_WAIT_TIME ? DEFAULT_WAIT_TIME : nearTimeOut;
        } while (true);
        if (nEvent < 0)
        {
            printf("epoll_wait error. [%d, %s]", errno, strerror(errno));
            break;
        }

        if (nEvent > 0)
        {
            eventfd_t value;
            eventfd_read(m_eventFd, &value);
        }
#elif defined(_WIN32) || defined(_WIN64)
        DWORD nErrorCode = WaitForSingleObject(m_eventFd, (DWORD)timeout);
        if (WAIT_FAILED == nErrorCode)
        {
            printf("WaitForSingleObject(%p) error: %u", m_eventFd, (uint32_t)GetLastError());
            break;
        }
        // NOTE 防止因处理过程触发_Tickle导致下次Wait直接返回问题
        ResetEvent(m_eventFd);
#endif
        m_state = ThreadState::Processing;
        std::list<std::function<void()>> callbackList;
        _ListExpireTimer(callbackList);
        for (const auto &it : callbackList)
        {
            it();
        }
    }

    m_state = ThreadState::Exited;
}

void CEvent::_Tickle()
{
    if (m_state == ThreadState::Waiting)
    {
#ifdef __linux__
        eventfd_write(m_eventFd, 1);
#elif defined(_WIN32) || defined(_WIN64)
        SetEvent(m_eventFd);
#endif
    }
}

inline void CEvent::_OnTimerInsertedAtFront()
{
    _Tickle();
}