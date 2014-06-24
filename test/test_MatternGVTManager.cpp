#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <memory>
#include <sstream>

#include "MatternGVTManager.hpp"
#include "serialization.hpp"
#include "utility/memory.hpp"

TEST_CASE("Mattern GVT messages can be serialized", "[serialization][mattern][message]") {
    std::stringstream ss;
    std::unique_ptr<warped::MatternGVTToken> msg = warped::make_unique<warped::MatternGVTToken>
        (2, 3, 1, 5, 10);

    std::unique_ptr<warped::MatternGVTToken> msg2;

    {
        cereal::BinaryOutputArchive oarchive(ss);
        oarchive(msg);
    }

    {
        cereal::BinaryInputArchive iarchive(ss);
        iarchive(msg2);
    }

    auto m = dynamic_cast<warped::MatternGVTToken*>(msg2.get());
    REQUIRE(m != nullptr);
    REQUIRE(m->sender_id == 2);
    REQUIRE(m->receiver_id == 3);
    REQUIRE(m->m_clock == 1);
    REQUIRE(m->m_send == 5);
    REQUIRE(m->count == 10);
}

TEST_CASE("Returns correct type", "[mattern][message][type]") {
    std::unique_ptr<warped::MatternGVTToken> msg = warped::make_unique<warped::MatternGVTToken>
        (2, 3, 1, 5, 10);

    REQUIRE(msg->get_type() == warped::MessageType::MatternGVTToken);
}
