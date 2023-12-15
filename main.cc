/*************************************************************************
    > File Name: main.cpp
    > Author: hsz
    > Brief:
    > Created Time: Fri 15 Dec 2023 07:18:30 PM CST
 ************************************************************************/

#include "event.h"

#define TIMER_SIZE 10000

int main()
{
    CEvent ev;

    for (int32_t i = 0; i < TIMER_SIZE; ++i)
    {
        ev.AddTimer((i + 1) * 10, [i]() {
            uint64_t currentTime = CTimer::CurrentTime();
            printf("%lu ms: I am the %d timer\n", currentTime, i);
        });
    }

    ev.start();

    getchar();
    return 0;
}
