#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <memory>
#include <sstream>

#include "MatternGVTControlMessage.hpp"
#include "serialization.hpp"

TEST_CASE("Mattern GVT messages can be serialized", "[serialization]") {
    std::stringstream ss;
    std::unique_ptr<warped::MatternGVTControlMessage> msg
                        {new warped::MatternGVTControlMessage{1, 5, 10}};
    std::unique_ptr<warped::MatternGVTControlMessage> msg2;

    {
        cereal::BinaryOutputArchive oarchive(ss);
        oarchive(msg);
    }

    {
        cereal::BinaryInputArchive iarchive(ss);
        iarchive(msg2);
    }

    auto m = dynamic_cast<warped::MatternGVTControlMessage*>(msg2.get());
    REQUIRE(m != nullptr);
    REQUIRE(m->mClock() == 1);
    REQUIRE(m->mSend() == 5);
    REQUIRE(m->count() == 10);
}
