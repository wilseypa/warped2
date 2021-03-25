/*
fCollect Thread File

-----------------
fCollect() {
    fossilFound <- emphfalse
    for each lp in LPs do {
        s <- lp.stateQ.head()
        q <- s
        fcgvt <- gvt.prevGvt
        
        while s.next.rTime < fcGvt do {
            s <- s.next()
        }

        if(s != q) then {
            remove all entries from lp.stateQ, lp.procQ, and lp.outQ that are at or before s.rTime
            set fossilFound <- true
        }
    }
    if fossilFound {
        sleep(gvtCycleInterval/2)
    }
}

for (unsigned int current_lp_id = 0; current_lp_id < num_local_lps_; current_lp_id++) {
        unsigned int num_committed = event_set_->fossilCollect(gvt, current_lp_id);
        tw_stats_->upCount(EVENTS_COMMITTED, thread_id, num_committed);
    }

while ((next != state_queue_[local_lp_id].end()) && (next->state_event_->timestamp() < gvt)) {
        state_queue_[local_lp_id].pop_front();
        min = state_queue_[local_lp_id].begin();
        next = std::next(min);
    }

    if (gvt == (unsigned int)-1) {
        state_queue_[local_lp_id].pop_front();
        return gvt;
    }

*/
#include "fCollect.hpp"
#include "HouseKeeping.hpp"

namespace warped {
    
        fCollect::fCollect(std::unique_ptr<TimeWarpGVTManager> gvt_manager :
                gvt_manager_(std::move(gvt_manager))) {

        }
        
        void fCollect::initialize() {
        
        }
        
        void fCollect::thread() {
            gvt = gvt_manager_->getGVT();
            
            while(!termination_manager_->terminationStatus()){
                bool fossilFound = false;
                // foreach lp in LPs do { // go through each lp in a list of active lps to check for fossils (look into creation of lps for for statement)
                for (unsigned int current_lp_id = 0; current_lp_id < num_local_lps_; current_lp_id++) {
                    
                    s = current_lp_id.stateQ.peek();
                    q = s;
                    Housekeeping::fcgvt = gvt.prevGvt;
                    
                    while s.next.rTime < fcGvt do {
                        s = s.next();
                    }
                    
                    if (s != q) { // can I use queue.peek() here? it returns .top() when the queue has a value then i step through
                        //clear lp.stateQ, lp.procQ, lp.outQ that are at or before s.rTime
                        // while (start != end) {go through each from the most recent back? and clear out the q with .pop()?} 
                        // need to clear the events within the object and then move through
                        fossilFound = true;
                    }
                }
                if (fossilFound) {
                        sleep(gvt_manager_->getGVTPeriod()/2); // this will be HouseKeeping::gvtCycleInterval when changing, just a placeholder
                }
            }
        }
    
}
