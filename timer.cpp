#include "timer.h"

std::atomic<uint64_t>   g_uniqueIdCount{0};

CTimer::CTimer() :
    m_time(0),
    m_recycleTime(0),
    m_callback(nullptr)
{
    m_uid = ++g_uniqueIdCount;
}

CTimer::CTimer(uint64_t ms, const CallBack &cb, uint32_t recycle) :
    m_time(CurrentTime() + ms),
    m_recycleTime(recycle),
    m_callback(cb)
{
    m_uid = ++g_uniqueIdCount;
}

CTimer::CTimer(const CTimer& timer) :
    m_time(timer.m_time),
    m_recycleTime(timer.m_recycleTime),
    m_callback(timer.m_callback),
    m_uid(timer.m_uid)
{

}

CTimer::~CTimer()
{

}

CTimer &CTimer::operator=(const CTimer &timer)
{
    m_time = timer.m_time;
    m_recycleTime = timer.m_recycleTime;
    m_callback = timer.m_callback;
    m_uid = timer.m_uid;

    return *this;
}

void CTimer::Cancel()
{
    m_time = 0;
    m_recycleTime = 0;
    m_callback = nullptr;
}

void CTimer::Refresh()
{
    if (m_recycleTime > 0) {
        m_time += m_recycleTime;
    }
}

void CTimer::Reset(uint64_t ms, const CallBack &cb, uint32_t recycle)
{
    if (ms < m_time)
    {
        return;
    }

    m_time = ms;
    m_callback = cb;
    m_recycleTime = recycle;
}

/**
 * @brief 获取绝对时间
 * 
 * @return uint64_t 返回获得的绝对时间
 */
uint64_t CTimer::CurrentTime()
{
    std::chrono::steady_clock::time_point tm = std::chrono::steady_clock::now();
    std::chrono::milliseconds mills = 
        std::chrono::duration_cast<std::chrono::milliseconds>(tm.time_since_epoch());

    return (uint64_t)mills.count();
}

uint64_t CTimerManager::GetNearTimeout()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_timers.empty())
    {
        return UINT32_MAX;
    }

    const CTimer::SP &spTimer = *m_timers.begin();
    uint64_t currentTimeMS = CTimer::CurrentTime();
    if (currentTimeMS >= spTimer->GetTimeout())
    {
        return 0;
    }

    return spTimer->GetTimeout() - currentTimeMS;
}

uint64_t CTimerManager::AddTimer(uint64_t ms, CTimer::CallBack cb, uint32_t recycle)
{
    if (nullptr == cb)
    {
        return INVALID_TIMER_ID;
    }

    uint64_t uid = INVALID_TIMER_ID;
    try
    {
        CTimer::SP spTimer = std::make_shared<CTimer>(ms, cb, recycle);
        uid = _AddTimer(spTimer);
    }
    catch (...)
    {
        return INVALID_TIMER_ID;
    }

    return uid;
}

void CTimerManager::DelTimer(uint64_t uniqueId)
{
    if (INVALID_TIMER_ID == uniqueId)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_timers.begin(); it != m_timers.end(); )
    {
        if ((*it)->m_uid == uniqueId)
        {
            m_timers.erase(it);
            break;
        }

        ++it;
    }
}

void CTimerManager::_ListExpireTimer(std::list<std::function<void()>>& cbs)
{
    uint64_t currentTimeMS = CTimer::CurrentTime();
    std::vector<CTimer::SP> expired;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_timers.empty())
        {
            return;
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_timers.begin();
    while ((it != m_timers.end()) && ((*it)->GetTimeout() <= currentTimeMS))
    {
        ++it;
    }

    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);

    for (const auto &timer : expired)
    {
        if (nullptr == timer->m_callback)
        {
            continue;
        }

        cbs.push_back(timer->m_callback);
        if (0 < timer->m_recycleTime)
        {
            timer->Refresh();
            m_timers.insert(timer);
        }
    }
}

uint64_t CTimerManager::_AddTimer(CTimer::SP timer)
{
    if (nullptr == timer)
    {
        return INVALID_TIMER_ID;
    }

    bool atFront = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_timers.insert(timer).first;
        atFront = (it == m_timers.begin());
    }

    // 需要更新等待时间
    if (atFront)
    {
        _OnTimerInsertedAtFront();
    }

    return timer->GetUID();
}
