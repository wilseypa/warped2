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

#include "HouseKeeping.hpp"
#include "LogicalProcess.hpp"

namespace warped {
    class fCollect() {
    public:
        fCollect::fCollect(std::unique_ptr<TimeWarpGVTManager> gvt_manager);
        
        // what does fCollect need to function?
        // need the queue stuff, did Jack add the old functionality back?
        
    private:
        const std::unique_ptr<TimeWarpGVTManager> gvt_manager_;
    }
}
