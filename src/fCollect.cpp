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
                foreach lp in LPs do { // go through each lp in a list of active lps to check for fossils (look into creation of lps for for statement)
                    
                    s = lp.stateQ.head();
                    q = s;
                    Housekeeping::fcgvt = gvt.prevGvt;
                    
                    while s.next.rTime < fcGvt do {
                        s = s.next();
                    }
                    
                    if (s != q) {
                        //clear lp.stateQ, lp.procQ, lp.outQ that are at or before s.rTime
                        // these will be linked lists? need to clear out all the items before s.rTime
                        // while (start != end) {go through each from the most recent back? and clear out the q with .pop()?}
                        fossilFound = true;
                    }
                }
                if (fossilFound) {
                        sleep(gvt_manager_->getGVTPeriod()/2); // this will be HouseKeeping::gvtCycleInterval when changing, just a placeholder
                }
            }
        }
    
}
