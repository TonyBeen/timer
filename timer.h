#ifndef __TIMER_H__
#define __TIMER_H__

#include <set>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <list>
#include <vector>

#define INVALID_TIMER_ID        0

class CTimer
{
public:
    typedef std::shared_ptr<CTimer>     SP;
    typedef std::function<void (void)>  CallBack;

	CTimer();
	CTimer(uint64_t ms, const CallBack &cb, uint32_t recycle);
	CTimer(const CTimer& timer);
    ~CTimer();

    CTimer &operator=(const CTimer& timer);

    // 获取超时时间
    uint64_t GetTimeout() const { return m_time; }

    // 获取唯一ID
    uint64_t GetUID() const { return m_uid; }

    // 设置下次超时时间
    void SetNextTime(uint64_t timeMs) { m_time = timeMs; }

        // 设置回调函数
    void SetCallback(const CallBack &cb) { m_callback = cb; }

    // 设置定时器循环时间
    void SetRecycleTime(uint64_t ms) { m_recycleTime = ms; }

    // 取消定时器
    void Cancel();

    // 刷新
    void Refresh();

        // 重置
    void Reset(uint64_t ms, const CallBack &cb, uint32_t recycle);

    // 计算当前时间, 绝对时间
    static uint64_t CurrentTime();
protected:
    friend class CTimerManager;
    // 比较器
    struct Comparator
    {
        // 按从小到大排序
        bool operator() (const CTimer::SP &left, const CTimer::SP &right) const
        {
            if ((nullptr == left) && (nullptr == right))
            {
                return false;
            }
            if (nullptr == left)
            {
                return true;
            }
            if (nullptr == right)
            {
                return false;
            }
            if (left->m_time == right->m_time)
            {
                // 时间相同，比较ID
                return left->m_uid < right->m_uid;
            }
            
            return left->m_time < right->m_time;
        }
    };

protected:
    uint64_t    m_time;         // (绝对时间)下一次执行时间(ms)
    uint64_t    m_recycleTime;  // 循环时间ms
    CallBack    m_callback;     // 回调函数
    uint64_t    m_uid;          // 定时器唯一ID
};

class CTimerManager
{
    CTimerManager(const CTimerManager&) = delete;
    CTimerManager& operator=(const CTimerManager&) = delete;
public:
    typedef std::set<CTimer::SP, CTimer::Comparator>            TimerSet;
    typedef std::set<CTimer *, CTimer::Comparator>::iterator    TimerSetIterator;

    CTimerManager() = default;
    virtual ~CTimerManager() = default;

    // 获取最近一次超时时间
    uint64_t    GetNearTimeout();

    // 添加定时器
    uint64_t    AddTimer(uint64_t ms, CTimer::CallBack cb, uint32_t recycle = 0);

    // 删除定时器
    void        DelTimer(uint64_t uniqueId);

protected:
    // 列出超时的定时器
    void            _ListExpireTimer(std::list<std::function<void()>> &cbs);

    // 添加定时器
    uint64_t        _AddTimer(CTimer::SP timer);

    // 当插入的定时器在头部时
    virtual void    _OnTimerInsertedAtFront() = 0;

private:
    std::mutex      m_mutex;    // 插入删除定时器锁
    TimerSet        m_timers;   // 定时器集合
};

#endif // __TIMER_H__
