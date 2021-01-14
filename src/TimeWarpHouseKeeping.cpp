/* 
----notes----
Splitting three main tasks into seperate threads, this class will hold the stuff each task requires

*gvtCntrl will handle estimation and termination

Class houseKeeping() {
    gvt.barrier() <- # of worker threads + 2 (Coordinate the worker threads w/ gCollect and receiveEvent) 
    gvtCntrl.barrier() <- 2 (Sync gvtCntrl w/ fCollect) ==> Figure 3

    gvt.gvtEstCycle <- false
    gvt.gvtCycleInterval <- delay (delay > 0: Time between gvt estimation cycles)
    gvt.gvt <- 0
    gvt.nextGvt <- 0

    gvtCntrl **thread** (GVT Estimation Task)
    receiveEvent **thread**
    fCollect **thread**
}
*/