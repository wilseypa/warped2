#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <memory>
#include <sstream>

#include "TerminationManager.hpp"
#include "serialization.hpp"

TEST_CASE("Termination messages can be serialized", "[serialization]") {
    std::stringstream ss_a;
    std::stringstream ss_p;
    std::unique_ptr<warped::TerminationControlMessage> msg_a
                        {new warped::TerminationControlMessage{warped::TerminationState::ACTIVE}};
    std::unique_ptr<warped::TerminationControlMessage> msg_p
                        {new warped::TerminationControlMessage{warped::TerminationState::PASSIVE}};
    std::unique_ptr<warped::TerminationControlMessage> msg_a2;
    std::unique_ptr<warped::TerminationControlMessage> msg_p2;

    {
        cereal::BinaryOutputArchive oarchive(ss_a);
        oarchive(msg_a);

        cereal::BinaryOutputArchive oarchive2(ss_p);
        oarchive2(msg_p);
    }

    {
        cereal::BinaryInputArchive iarchive(ss_a);
        iarchive(msg_a2);

        cereal::BinaryInputArchive iarchive2(ss_p);
        iarchive2(msg_p2);
    }

    auto m = dynamic_cast<warped::TerminationControlMessage*>(msg_a2.get());
    REQUIRE(m != nullptr);
    REQUIRE(m->state_indicator == warped::TerminationState::ACTIVE);

    auto m2 = dynamic_cast<warped::TerminationControlMessage*>(msg_p2.get());
    REQUIRE(m2 != nullptr);
    REQUIRE(m2->state_indicator == warped::TerminationState::PASSIVE);
}
