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
namespace warped {
    
        fCollect::fCollect() {
    
        }
        
        void fCollect::initialize() {
        
        }
        
        void fCollect::thread() {
            foreach lp in LPs do {
                // above
                
                while s.next.rTime < fcGvt do {
                    s <- s.next()
                }
                
                if (s != q) {
                    //clear lp.stateQ, lp.procQ, lp.outQ that are at or before s.rTime
                    //set fossilFound = true;
                }
                
                if (fossilFound) {
                    sleep(HouseKeeping::gvtCycleInterval/2);
                }
            }
        }
}
