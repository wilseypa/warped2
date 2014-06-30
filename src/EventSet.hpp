#ifndef EVENT_SET_HPP
#define EVENT_SET_HPP

/* This class provides an interface for a specific Event Set
 */

namespace warped {

class EventSet {
public:

    virtual bool insert(const Event* event) = 0;

    virtual bool handleAntiMessage(SimulationObject* reclaimer, const NegativeEvent* cancelEvent);

    virtual const Event* getEvent(SimulationObject*, unsigned int);

    virtual const Event* peekEvent(SimulationObject*, unsigned int);

    virtual void fossilCollect(SimulationObject*, unsigned int);

    virtual void fossilCollect(const Event*);

    virtual void rollback(SimulationObject* object, unsigned int rollbackTime);

protected:
    EventSet(){};
};

} // warped namespace

#endif
