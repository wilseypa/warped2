/*

one instance for each cluster node.
takes in all event msgs on the MPI EVENT comm

receiveEvent thread:

while true do
    m <- MPI_Recv(EVENT, MPI_ANY_SOURCE) // receive all events, set them to m

    if m.tag = "Verify_Idle" then
        MPI_Barrier(MSGs_Proc)
    else
        lp <- findLpPtr(m.lp) // find the ptr for the given lp
        lp.inQ.insert(m.e) //use that ptr to insert the event into the queue
    end if
end while

end thread loop

*/
#include "receiveEvent.hpp"

namespace warped {
    receiveEvent::receiveEvent() {}

    void receiveEvent::initialize() {}

    void receiveEvent::thread() {

        while (true) {
            m = MPI_Recv(EVENT, MPI_ANY_SOURCE)

            if (m.tag = "Verify_Idle") {
                MPI_Barrier(MSGs_Proc) //where is MSGs defined
            }
            else {
                lp = findLpPtr(m.lp) // lp needs to be instantiated from LP class
                lp.inQ.insert(m.e)
            }
        }

    }
}