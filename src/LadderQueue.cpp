#include "TimeWarpEventSet.hpp"

namespace warped {

LadderQueue::LadderQueue() {

    /* Initialize the variables */
    std::fill_n(bucket_width_, MAX_RUNG_CNT, 0);
    std::fill_n(rung_bucket_cnt_, MAX_RUNG_CNT, 0);
    std::fill_n(r_start_, MAX_RUNG_CNT, 0);
    std::fill_n(r_current_, MAX_RUNG_CNT, 0);

    /* Create buckets for 2nd rung onwards */
    for (unsigned int rung_index = 1; rung_index < MAX_RUNG_CNT; rung_index++) {
        for (unsigned int bucket_index = 0; bucket_index < MAX_BUCKET_CNT; bucket_index++) {
            auto bucket = make_unique<std::list<std::shared_ptr<Event>>>();
            assert(bucket != nullptr);
            rung_[rung_index].push_back(bucket);
        }
    }
}

std::shared_ptr<Event> begin() {

    /* Remove from bottom if not empty */
    if (!bottom_.empty()) {
        return *bottom_.begin();
    }

    /* If rungs exist, remove from rungs */
    unsigned int bucket_index = 0;
    if (n_rung_ && !recurseRung(&bucket_index)) {
        /* Check whether rungs still exist */
        if (n_rung_) assert(0);
    }

    /* Check required because rung recursion can affect n_rung_ value */
    if (n_rung_) { 
        for (auto event : *rung_[n_rung_-1][bucket_index]) {
            bottom_.insert(event);
        }
        rung_[n_rung_-1][bucket_index]->clear();

        /* If bucket returned is the last valid rung of the bucket */
        if (rung_bucket_cnt_[n_rung_-1] == bucket_index+1) {
            do {
                rung_bucket_cnt_[n_rung_-1] = 0;
                r_start_[n_rung_-1]         = 0;
                r_current_[n_rung_-1]       = 0;
                bucket_width_[n_rung_-1]    = 0;
                n_rung_--;
            } while(n_rung_ && !rung_bucket_cnt_[n_rung_-1]);

        } else {
            while((++bucket_index < rung_bucket_cnt_[n_rung_-1]) && 
                                    rung_[n_rung_-1][bucket_index]->empty());
            if (bucket_index < rung_bucket_cnt_[n_rung_-1]) {
                r_current_[n_rung_-1] = 
                    r_start_[n_rung_-1] + bucket_index*bucket_width_[n_rung_-1];
            } else {
                assert(0);
            }
        }

        if (bottom_.empty()) return nullptr;
        return *bottom_.begin();
    }

    /* Check if top has any events before proceeding further*/
    if (top_.empty()) return nullptr;

    /* Move from top to top of empty ladder */
    /* Check if failed to create the first rung */
    bool is_bucket_width_static = false;
    if (!createNewRung(top_.size(), min_ts_, &is_bucket_width_static)) {
        assert(0);
    }

    /* Transfer events from Top to 1st rung of Ladder */
    /* Note: No need to update rCur[0] since it will be equal to rStart[0] initially. */
    for (auto iter = top_.begin(); iter != top_.end();) {
        assert((*iter)->timestamp() >= r_start_[0]);
        bucket_index = std::min(
                (unsigned int)((*iter)->timestamp()-r_start_[0]) / bucket_width_[0], 
                                                                        RUNG_BUCKET_CNT(0)-1);
        rung_[0][bucket_index]->push_front(*iter);
        iter = top_.erase(iter);

        /* Update the numBucket parameter */
        if (rung_bucket_cnt_[0] < bucket_index+1) {
            rung_bucket_cnt_[0] = bucket_index+1;
        }
    }

    /* Copy events from bucket_k into Bottom */
    if (!recurseRung(&bucket_index)) {
        assert(0);
    }

    for (auto event : *rung_[n_rung_-1][bucket_index]) {
        bottom_.insert(event);
    }
    rung_[n_rung_-1][bucket_index]->clear();

    /* If bucket returned is the last valid rung of the bucket */
    if (rung_bucket_cnt_[n_rung_-1] == bucket_index+1) {
        rung_bucket_cnt_[n_rung_-1] = 0;
        r_start_[n_rung_-1]         = 0;
        r_current_[n_rung_-1]       = 0;
        bucket_width_[n_rung_-1]    = 0;
        n_rung_--;

    } else {
        while((++bucket_index < rung_bucket_cnt_[n_rung_-1]) && 
                (rung_[n_rung_-1][bucket_index]->empty()));
        if (bucket_index < rung_bucket_cnt_[n_rung_-1]) {
            r_current_[n_rung_-1] = 
                r_start_[n_rung_-1] + bucket_index*bucket_width_[n_rung_-1];
        } else {
            rung_bucket_cnt_[n_rung_-1] = 0;
            r_start_[n_rung_-1]         = 0;
            r_current_[n_rung_-1]       = 0;
            bucket_width_[n_rung_-1]    = 0;
            n_rung_--;
        }
    }

    if (bottom_.empty()) return nullptr;
    return *bottom_.begin();
}

} // namespace warped
