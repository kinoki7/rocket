#include <sys/timerfd.h>
#include <string.h>
#include "rocket/net/timer.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"


namespace rocket {

Timer::Timer() {

    m_fd = timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK | TFD_CLOEXEC);
    DEBUGLOG("timer fd = %d", m_fd);

    listen(FdEvent::IN_EVENT, std::bind(&Timer::onTime, this));
}

Timer::~Timer() {
    char buf[8];
    while(1){
        if((read(m_fd, buf, 8) == -1) && errno == EAGAIN) {
            break;
        }
    }
}

void Timer::resetArriveTime() {
    ScopeMutex<Mutex> lock(m_mutex);
    auto tmp = m_pending_events;
    lock.unlock();

    if(tmp.size()) {
        return ;
    }

    int64_t now = getNowMs();

    auto it = tmp.begin();
    int64_t interval = 0;
    if(it->second->getArriveTime() > now) {
        interval = it->second->getArriveTime() - now;
    }else {
        interval = 100;
    }

    timespec ts;
    ts.tv_sec = interval /1000;
    ts.tv_nsec = (interval%1000) * 1000000;

    itimerspec value;
    value.it_value = ts;

    int rt = timerfd_settime(m_fd, 0, &value, NULL);
    if(rt != 0) {
        ERRORLOG("timefd_settimer error, errno=%d, error=%s", errno, strerror(errno));
    }
}

void Timer::addTimerEvent(TimerEvent::s_ptr event) {
    bool is_reset_timerfd = false;

    ScopeMutex<Mutex> lock(m_mutex);
    if(m_pending_events.empty()) {
        is_reset_timerfd = true;
    }else {
        auto it = m_pending_events.begin();
        if((*it).second->getArriveTime() > event->getArriveTime()) {
            is_reset_timerfd = true;
        }
    }
    m_pending_events.emplace(event->getArriveTime(), event);
    lock.unlock();

    if(is_reset_timerfd) {
        resetArriveTime();
    }
}

void Timer::deleteTimerEvent(TimerEvent::s_ptr event) {
    event->setCancled(true);
    ScopeMutex<Mutex> lock(m_mutex);
    
    auto begin = m_pending_events.lower_bound(event->getArriveTime());
    auto end = m_pending_events.upper_bound(event->getArriveTime());

    auto it = begin;
    for(it = begin; it != end; it++) {
        if(it->second == event) {
            break;
        }
    }
    if(it != end) {
        m_pending_events.erase(it);
    }

    lock.unlock();
    DEBUGLOG("success delete TimerEvnt at arrive time %lld", event->getArriveTime());
}

void Timer::onTime() {

}


}