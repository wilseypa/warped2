#ifndef COMMUNICATOR_HPP
#define COMMUNICATOR_HPP

#include "CommunicationManager.hpp"

/*  The Communicator class serves as a base class for any class that must send message
 *  or receive messages.
 */

namespace warped {

class Communicator {
public:
    Communicator() = default;

    static unsigned int initCommunicationManager(std::unique_ptr<CommunicationManager> cm);

    // receiveMessage must be implemented in any subclass, the receiveMessage implementation
    // should downcast the message to the appropriate message type and then do whatever with it.
    virtual void receiveMessage(std::unique_ptr<KernelMessage> msg) = 0;

protected:

    void finalizeCommunicationManager();

    void dispatchReceivedMessages();

    static std::unique_ptr<CommunicationManager> comm_manager_;
};

} // namespace warped

#endif
