#include "LadderQueue.hpp"
#include <cassert>

namespace warped {

LadderQueue::LadderQueue() {

    /* Initialize the variables */
    std::fill_n(bucket_width_, MAX_RUNG_CNT, 0);
    std::fill_n(last_nonempty_bucket_, MAX_RUNG_CNT, 0);
    std::fill_n(r_start_, MAX_RUNG_CNT, 0);
    std::fill_n(r_current_, MAX_RUNG_CNT, 0);

    /* Create buckets for 2nd rung onwards */
    for (unsigned int rung_index = 1; rung_index < MAX_RUNG_CNT; rung_index++) {
        for (unsigned int bucket_index = 0; bucket_index < THRESHOLD; bucket_index++) {
            rung_[rung_index].push_back(std::make_shared<std::vector<std::shared_ptr<Event>>>());
            rung_[rung_index][bucket_index]->reserve(THRESHOLD);
        }
    }
}

std::shared_ptr<Event> LadderQueue::pop() {

    /* Remove from bottom if not empty */
    if (auto event = bottom_.pop()) { return event; }

    lock_.lock();

    /* If rungs exist, remove from rungs */
    unsigned int bucket_index = 0;
    if (!recurseRung(&bucket_index)) {
        /* Check whether rungs still exist */
        if (n_rung_) assert(0);
    }

    /* Check required because rung recursion can affect n_rung_ value */
    if (n_rung_) {

        /* NOTE: Bottom should be empty */
        for (auto e : *rung_[n_rung_-1][bucket_index]) { bottom_.push(e); }
        bottom_start_ = r_current_[n_rung_-1];
        rung_[n_rung_-1][bucket_index]->clear();

        /* If bucket returned is the last valid bucket of the rung */
        if (last_nonempty_bucket_[n_rung_-1] == bucket_index+1) {
            do {
                last_nonempty_bucket_[n_rung_-1] = 0;
                r_start_[n_rung_-1]         = 0;
                r_current_[n_rung_-1]       = 0;
                bucket_width_[n_rung_-1]    = 0;
                n_rung_--;
            } while(n_rung_ && !last_nonempty_bucket_[n_rung_-1]);

        } else {
            while((++bucket_index < last_nonempty_bucket_[n_rung_-1]) && 
                                    rung_[n_rung_-1][bucket_index]->empty());
            if (bucket_index < last_nonempty_bucket_[n_rung_-1]) {
                r_current_[n_rung_-1] = 
                    r_start_[n_rung_-1] + bucket_index*bucket_width_[n_rung_-1];
            } else {
                assert(0);
            }
        }

        lock_.unlock();
        return bottom_.pop();
    }

    /* Check if top has any events before proceeding further*/
    if (top_.empty()) {
        lock_.unlock();
        return nullptr;
    }

    /* Move from top to top of empty ladder */
    /* Check if failed to create the first rung */
    bool is_bucket_width_static = false;
    if (!createNewRung(top_.size(), min_ts_, &is_bucket_width_static)) assert(0);

    /* Transfer events from Top to 1st rung of Ladder */
    /* Note: No need to update rCur[0] since it will be equal to rStart[0] initially. */
    for (auto event : top_) {
        assert(event->timestamp() >= r_start_[0]);
        bucket_index = std::min(
                (unsigned int)(event->timestamp()-r_start_[0]) / bucket_width_[0],
                                                                        RUNG_BUCKET_CNT(0)-1);
        rung_[0][bucket_index]->push_back(event);

        /* Update the numBucket parameter */
        if (last_nonempty_bucket_[0] < bucket_index+1) {
            last_nonempty_bucket_[0] = bucket_index+1;
        }
    }
    top_.clear();

    /* Copy events from bucket_k into Bottom */
    if (!recurseRung(&bucket_index)) assert(0);

    for (auto e : *rung_[n_rung_-1][bucket_index]) { bottom_.push(e); }
    bottom_start_ = r_current_[n_rung_-1];
    rung_[n_rung_-1][bucket_index]->clear();

    /* If bucket returned is the last valid rung of the bucket */
    if (last_nonempty_bucket_[n_rung_-1] == bucket_index+1) {
        last_nonempty_bucket_[n_rung_-1] = 0;
        r_start_[n_rung_-1]         = 0;
        r_current_[n_rung_-1]       = 0;
        bucket_width_[n_rung_-1]    = 0;
        n_rung_--;

    } else {
        while((++bucket_index < last_nonempty_bucket_[n_rung_-1]) && 
                (rung_[n_rung_-1][bucket_index]->empty()));
        if (bucket_index < last_nonempty_bucket_[n_rung_-1]) {
            r_current_[n_rung_-1] = 
                r_start_[n_rung_-1] + bucket_index*bucket_width_[n_rung_-1];
        } else {
            last_nonempty_bucket_[n_rung_-1] = 0;
            r_start_[n_rung_-1]         = 0;
            r_current_[n_rung_-1]       = 0;
            bucket_width_[n_rung_-1]    = 0;
            n_rung_--;
        }
    }

    lock_.unlock();
    return bottom_.pop();
}

void LadderQueue::insert(std::shared_ptr<Event> event) {

    assert(event != nullptr);
    auto timestamp = event->timestamp();

    lock_.lock();

    /* Insert into top, if valid */
    if (timestamp > top_start_) {  //deviation from APPENDIX of ladderq
        if(top_.empty()) {
            max_ts_ = min_ts_ = timestamp;
        } else {
            if (min_ts_ > timestamp) min_ts_ = timestamp;
            if (max_ts_ < timestamp) max_ts_ = timestamp;
        }
        top_.push_back(event);
        lock_.unlock();
        return;
    }

    /* Step through rungs */
    unsigned int rung_index = 0;
    while ((rung_index < n_rung_) && (timestamp < r_current_[rung_index])) {
        rung_index++;
    }

    if (rung_index < n_rung_) {  /* found a rung */
        assert(timestamp >= r_start_[rung_index]);
        unsigned int bucket_index = std::min(
            (unsigned int)(timestamp - r_start_[rung_index]) / bucket_width_[rung_index],
                                                            RUNG_BUCKET_CNT(rung_index)-1);

        /* Adjust rung parameters */
        if (last_nonempty_bucket_[rung_index] < bucket_index+1) {
            last_nonempty_bucket_[rung_index] = bucket_index+1;
        }
        unsigned int temp_ts = r_start_[rung_index] + bucket_index*bucket_width_[rung_index];
        if (r_current_[rung_index] > temp_ts) {
            r_current_[rung_index] = temp_ts;
        }

        rung_[rung_index][bucket_index]->push_back(event);
        lock_.unlock();
        return;
    }

    bottom_start_ = std::min(bottom_start_, timestamp);
    lock_.unlock();

    /* NOTE : Bottom is allowed to overflow (i.e. > THRESHOLD), if needed.
     *        This has been done to keep lock-free stack APIs simple and efficient.
     */
    bottom_.push(event);
}

void LadderQueue::setLowestTimestamp(unsigned int ts) {

    lock_.lock();
    bottom_start_ = std::min(bottom_start_, ts);
    lock_.unlock();
}

unsigned int LadderQueue::getLowestTimestamp() {

    return bottom_start_;
}

bool LadderQueue::createNewRung(unsigned int num_events, 
                                unsigned int init_start_and_cur_val, 
                                bool *is_bucket_width_static) {

    assert(is_bucket_width_static);
    assert(num_events);

    *is_bucket_width_static = false;

    /* Check if this is the first rung creation */
    if (!n_rung_) {
        assert(max_ts_ >= min_ts_);
        bucket_width_[0] = (min_ts_ == max_ts_) ? MIN_BUCKET_WIDTH : 
                                    (max_ts_ - min_ts_ + num_events -1) / num_events;
        top_start_          = max_ts_;
        r_start_[0]         = min_ts_;
        r_current_[0]       = min_ts_;
        last_nonempty_bucket_[0] = 0;
        n_rung_++;

        /* Create the actual rungs */
        //create double of required no of buckets. ref sec 2.4 of ladderq
        unsigned int bucket_index = 0;
        for (bucket_index = rung_0_length_; bucket_index < 2*num_events; bucket_index++) {
            rung_[0].push_back(std::make_shared<std::vector<std::shared_ptr<Event>>>());
            rung_[0][bucket_index]->reserve(THRESHOLD);
        }
        rung_0_length_ = bucket_index;

    } else { // When rungs already exist
        /* Check if bucket width has reached the min limit */
        if (bucket_width_[n_rung_-1] <= MIN_BUCKET_WIDTH) {
            *is_bucket_width_static = true;
            return false;
        }
        /* Check whether new rungs can be created */
        assert(n_rung_ < MAX_RUNG_CNT);
        n_rung_++;
        bucket_width_[n_rung_-1] = (bucket_width_[n_rung_-2] + num_events - 1) / num_events;
        r_start_[n_rung_-1] = r_current_[n_rung_-1] = init_start_and_cur_val;
        last_nonempty_bucket_[n_rung_-1] = 0;
    }
    return true;
}

bool LadderQueue::recurseRung(unsigned int *index) {

    bool status = false;
    *index = 0;

    /* Find_bucket label */
    while(1) {
        if (!n_rung_) break;
        if (n_rung_ > MAX_RUNG_CNT) assert(0);

        unsigned int bucket_index = 0;
        r_current_[n_rung_-1] = r_start_[n_rung_-1];
        while ((bucket_index < RUNG_BUCKET_CNT(n_rung_-1)) && 
                            rung_[n_rung_-1][bucket_index]->empty()) {
            bucket_index++;
        }

        // If the last rung is empty
        if (bucket_index == RUNG_BUCKET_CNT(n_rung_-1)) {
            r_start_[n_rung_-1]         = 0;
            r_current_[n_rung_-1]       = 0;
            bucket_width_[n_rung_-1]    = 0;
            last_nonempty_bucket_[n_rung_-1] = 0;
            n_rung_--;

        } else {
            r_current_[n_rung_-1] += bucket_index*bucket_width_[n_rung_-1];

            // If the bucket does not have any bucket overflow
            if (rung_[n_rung_-1][bucket_index]->size() <= THRESHOLD) {
                *index = bucket_index;
                status = true;
                break;
            }

            /* When there is a bucket overflow */
            bool is_bucket_width_static = false;

            // Try to create a new rung
            if (!createNewRung( rung_[n_rung_-1][bucket_index]->size(),
                                r_current_[n_rung_-1], 
                                &is_bucket_width_static )) {
                if (is_bucket_width_static) {
                    *index = bucket_index;
                    status = true;
                }
                break;
            }

            // Move events from the bucket in penultimate rung to the new rung
            for (auto iter = rung_[n_rung_-2][bucket_index]->begin(); 
                        iter != rung_[n_rung_-2][bucket_index]->end(); iter++) {
                assert((*iter)->timestamp() >= r_start_[n_rung_-1] );
                unsigned int new_bucket_index = std::min( 
                    (unsigned int) ((*iter)->timestamp() - r_start_[n_rung_-1]) / 
                                    bucket_width_[n_rung_-1], RUNG_BUCKET_CNT(n_rung_-1)-1);
                rung_[n_rung_-1][new_bucket_index]->push_back(*iter);

                /* Calculate bucket count for new rung */
                if (last_nonempty_bucket_[n_rung_-1] < new_bucket_index+1) {
                    last_nonempty_bucket_[n_rung_-1] = new_bucket_index+1;
                }
            }
            rung_[n_rung_-2][bucket_index]->clear();

            /* Re-calculate r_current and last_nonempty_bucket_ of old rung */
            bool bucket_found = false;
            for (unsigned int index = bucket_index+1; 
                            index < RUNG_BUCKET_CNT(n_rung_-2); index++) {
                if (!rung_[n_rung_-2][index]->empty()) {
                    if (!bucket_found) {
                        bucket_found = true;
                        r_current_[n_rung_-2] = 
                            r_start_[n_rung_-2] + index*bucket_width_[n_rung_-2];
                    }
                    last_nonempty_bucket_[n_rung_-2] = index+1;
                }
            }
            if (!bucket_found) {
                last_nonempty_bucket_[n_rung_-2] = 0;
                r_current_[n_rung_-2] = r_start_[n_rung_-2];
            }
        }
    }
    return status;
}

} // namespace warped
