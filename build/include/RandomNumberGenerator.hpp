#ifndef RANDOM_NUMBER_GENERATOR_HPP
#define RANDOM_NUMBER_GENERATOR_HPP

#include <memory>

namespace warped {

struct RandomNumberGenerator {
    virtual ~RandomNumberGenerator() {}
    virtual void getState(std::ostream&) = 0;
    virtual void restoreState(std::istream&) = 0;
};

template<class Derived>
struct RNGMixin : public RandomNumberGenerator {
    virtual void getState(std::ostream& os) {
        os << *static_cast<const Derived&>(*this).gen_;
    }

    virtual void restoreState(std::istream& is) {
        is >> *static_cast<const Derived&>(*this).gen_;
    }
};

template<class RNGType>
struct RNGDerived : public RNGMixin<RNGDerived<RNGType>> {
    RNGDerived(std::shared_ptr<RNGType> gen) : gen_(gen) {}
    std::shared_ptr<RNGType> gen_;
};

} // namespace warped

#endif
