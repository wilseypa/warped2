#include "TimeWarpEventSet.hpp"

namespace warped {

LadderQueue::LadderQueue() {

    /* Initialize the variables */
    std::fill_n(bucket_width_, MAX_RUNG_CNT, 0);
    std::fill_n(max_non_empty_bucket_index_, MAX_RUNG_CNT, 0);
    std::fill_n(r_start_, MAX_RUNG_CNT, 0);
    std::fill_n(r_current_, MAX_RUNG_CNT, 0);

    /* Create buckets for rungs */
    for (unsigned int rung_index = 0; rung_index < MAX_RUNG_CNT; rung_index++) {
        for (unsigned int bucket_index = 0; bucket_index < MAX_BUCKET_CNT; bucket_index++) {
            auto bucket = make_unique<std::list<std::shared_ptr<Event>>>();
            assert(bucket != nullptr);
            rung_[rung_index].push_back(bucket);
        }
    }
}

std::shared_ptr<Event> begin() {

    unsigned int bucketIndex = 0;
    std::list<const Event*>::iterator lIterate;
    bool isBucketWidthStatic = false;

    /* Remove from bottom if not empty */
    if (!bottom_.empty()) {
        auto event = *bottom_.begin();
        return event;
    }

    /* If rungs exist, remove from rungs */
    if (n_rung_ && (INVALID == (bucketIndex = recurse_rung()))) {
        /* Check whether rungs still exist */
        if (nRung > 0) {
            std::cout << "Received invalid Bucket index." << std::endl;
            return NULL;
        }
    }

    if (nRung > 0) { /* Check required because recurse_rung() can affect nRung value */
        for (lIterate = RUNG(nRung-1,bucketIndex)->begin();
            lIterate != RUNG(nRung-1,bucketIndex)->end(); lIterate++) {
            bottomInsert(*lIterate);
        }
        RUNG(nRung-1,bucketIndex)->clear();

        /* If bucket returned is the last valid rung of the bucket */
        if (numBucket[nRung-1] == bucketIndex+1) {
            do {
                numBucket[nRung-1] = rStart[nRung-1] =
                rCur[nRung-1] = bucketWidth[nRung-1] = 0;
                nRung--;
            } while(nRung && !numBucket[nRung-1]);
        } else {
            while ((++bucketIndex < numBucket[nRung-1]) && (true == RUNG(nRung-1,bucketIndex)->empty()));
            if (bucketIndex < numBucket[nRung-1]) {
                rCur[nRung-1] = rStart[nRung-1] + bucketIndex*bucketWidth[nRung-1];
            } else {
                std::cout << "numBucket handling needs improvement." << std::endl;
                numBucket[nRung-1] = rStart[nRung-1] =
                rCur[nRung-1] = bucketWidth[nRung-1] = 0;
                nRung--;
                return NULL;
            }
        }

        if (true == bottomEmpty()) {
            std::cout << "Bottom empty 1" << std::endl;
            return NULL;
        }

        const Event* event = bottomBegin();
        return event;
    }

    /* Check if top has any events before proceeding further */
    if (true == top.empty()) {
        std::cout << "LadderQ is empty." << std::endl;
        return NULL;
    }

    /* Move from top to top of empty ladder */
    /* Check if failed to create the first rung */
    if (false == create_new_rung(top.size(), minTS, &isBucketWidthStatic)) {
        std::cout << "Failed to create the required rung." << std::endl;
        return NULL;
    }

    /* Transfer events from Top to 1st rung of Ladder */
    /* Note: No need to update rCur[0] since it will be equal to rStart[0] initially. */
    for (lIterate = top.begin(); lIterate != top.end();) {
        ASSERT( (*lIterate)->getReceiveTime().getApproximateIntTime() >= rStart[0] );
        bucketIndex =
                std::min( (unsigned int)((*lIterate)->getReceiveTime().getApproximateIntTime() -
                              rStart[0]) / bucketWidth[0],
                          NUM_BUCKETS(0)-1 );

        RUNG(0,bucketIndex)->push_front(*lIterate);
        lIterate = top.erase(lIterate);

        /* Update the numBucket parameter */
        if (numBucket[0] < bucketIndex+1) {
            numBucket[0] = bucketIndex+1;
        }
    }

    /* Copy events from bucket_k into Bottom */
    if (INVALID == (bucketIndex = recurse_rung())) {
        std::cout << "Received invalid Bucket index." << std::endl;
        return NULL;
    }

    for (lIterate = RUNG(nRung-1,bucketIndex)->begin();
                    lIterate != RUNG(nRung-1,bucketIndex)->end(); lIterate++) {
        bottomInsert(*lIterate);
    }

    /* Clear that bucket */
    RUNG(nRung-1,bucketIndex)->clear();

    /* If bucket returned is the last valid rung of the bucket */
    if (numBucket[nRung-1] == bucketIndex+1) {
        numBucket[nRung-1] = rStart[nRung-1] = rCur[nRung-1] = bucketWidth[nRung-1] = 0;
        nRung--;
    } else {
        while ((++bucketIndex < numBucket[nRung-1]) && (true == RUNG(nRung-1,bucketIndex)->empty()));
        if (bucketIndex < numBucket[nRung-1]) {
            rCur[nRung-1] = rStart[nRung-1] + bucketIndex*bucketWidth[nRung-1];
        } else {
            std::cout << "rung 1 numBucket handling needs improvement." << std::endl;
            numBucket[nRung-1] = rStart[nRung-1] = rCur[nRung-1] = bucketWidth[nRung-1] = 0;
            nRung--;
        }
    }

    if (true == bottomEmpty()) {
        std::cout << "Bottom empty 2" << std::endl;
        return NULL;
    }

    const Event* event = bottomBegin();
    return event;
}

} // namespace warped
