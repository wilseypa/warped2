#ifndef EVENT_SET_HPP
#define EVENT_SET_HPP

/* This class provides an interface for a specific Event Set
 */

namespace warped {

class EventSet {
public:

    bool insert(const Event* event) = 0;

    bool handleAntiMessage(SimulationObject* reclaimer, const NegativeEvent* cancelEvent);

    const Event* getEvent(SimulationObject*, uint64_t);

    const Event* peekEvent(SimulationObject*, uint64_t);

    void fossilCollect(SimulationObject*, uint64_t);

    void fossilCollect(const Event*);

    void rollback(SimulationObject* object, uint64_t rollbackTime);

};

} // warped namespace

#endif
