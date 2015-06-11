#include <atomic>

struct TicketLock {

    void lock() {
        unsigned int me = std::atomic_fetch_add(&users_, (unsigned int)1);
        while (ticket_.load() != me) {}
    }

    void unlock() {
        std::atomic_fetch_add(&ticket_, (unsigned int)1);
    }

private:

    std::atomic<unsigned int> ticket_ = ATOMIC_VAR_INIT(0);

    std::atomic<unsigned int> users_ = ATOMIC_VAR_INIT(0);

};
