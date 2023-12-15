/*************************************************************************
    > File Name: test_timer.cc
    > Author: hsz
    > Brief:
    > Created Time: Fri 15 Dec 2023 06:34:03 PM CST
 ************************************************************************/

#include <log/log.h>
#include "event.h"

#define TIMER_SIZE 10000

#define LOG_TAG "test_timer"

int main()
{
    CEvent ev;
    uint64_t timerData[TIMER_SIZE] = {0};

    for (int32_t i = 0; i < TIMER_SIZE; ++i)
    {
        timerData[i] = CTimer::CurrentTime();
        ev.AddTimer((i + 1) * 10, [i, &timerData]() {
            // uint64_t currentTime = CTimer::CurrentTime();
            // printf("%lu ms: I am the %d timer\n", (currentTime - timerData[i]), i);
            // timerData[i] = currentTime;
            LOGI("I am the %d timer\n", i);
        });
    }

    ev.start();

    getchar();
    return 0;
}
