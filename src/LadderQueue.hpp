#ifndef LADDER_QUEUE_HPP
#define LADDER_QUEUE_HPP

/* Ladder Queue Library */

/* Include section */
#include <iostream>
#include <algorithm>
#include <vector>
#include <mutex>

#include "TicketLock.hpp"
#include "LockFreeStack.hpp"
#include "config.h"
#include "Event.hpp"

/* Configurable Ladder Queue parameters */
#define MAX_RUNG_CNT       8    //ref. sec 2.4 of ladderq paper
#define THRESHOLD          50   //ref. sec 2.3 of ladderq paper
#define MIN_BUCKET_WIDTH   1

using bucket = std::vector<std::shared_ptr<warped::Event>>;


namespace warped {

class LadderQueue {
public:
    LadderQueue();

    std::shared_ptr<Event> dequeue();

    bool erase(std::shared_ptr<Event> event);

    void insert(std::shared_ptr<Event> event);

    unsigned int lowestTimestamp();

private:
    void createNewRung();

    void recurseRung();

    /* Lock to protect Top and Rungs */
#ifdef SCHEDULE_QUEUE_SPINLOCKS
    TicketLock lock_;
#else
    std::mutex lock_;
#endif

    /* Top variables */
    struct Top {
        bucket buffer_;
        unsigned int start_ts_  = 0;
        unsigned int max_ts_    = 0;
    } top_;

    /* Rungs */
    unsigned int rung_cnt_ = 0;

    struct Rung {
        std::vector<bucket> buffer_;
        unsigned int bucket_width_  = 0;
        unsigned int start_ts_      = 0;
        unsigned int first_nonempty_bucket_ts_  = 0;
        unsigned int last_nonempty_bucket_ts_   = 0;

    } rung_[MAX_RUNG_CNT];

    /* Bottom */
    LockFreeStack bottom_;
    unsigned int bottom_start_ = 0;
};

} // namespace warped

#endif /* LADDER_QUEUE_HPP */

