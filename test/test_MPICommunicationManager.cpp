#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <thread>

#include "MPICommunicationManager.hpp"
#include "utility/memory.hpp"
#include "MatternGVTManager.hpp"

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::GVTUpdateMessage)

TEST_CASE("Messages can be enqueued, serialized, sent, received, and deserialized","[mpi]"){
    auto mpi_manager = std::make_shared<warped::MPICommunicationManager>();

    SECTION("Messages can be enqueued from multiple threads", "[mpi][enqueue][threads]") {
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < 4; ++i) {
            threads.push_back(std::thread( [&](){
                mpi_manager->enqueueMessage(warped::make_unique<warped::MatternGVTToken>
                    (0, 0, 1, 6, 9));
            }));
        }

        for (auto& t: threads) {
            t.join();
        }

        for (unsigned int i = 0; i < 4; i++) {
            auto msg = mpi_manager->dequeueMessage();
                REQUIRE(msg.get() != nullptr);
        }
    }

    SECTION("Basic management routines return correct values", "[mpi]") {
        mpi_manager->initialize();

        int flag;
        MPI_Initialized(&flag);
        REQUIRE(flag == true);

        int num_processes = mpi_manager->getNumProcesses();
        REQUIRE(num_processes == 1);

        int process_id = mpi_manager->getID();
        REQUIRE(process_id == 0);

        // Sends an MPI message to itself
        SECTION("Can serialize and send a message", "[mpi][serialize][send]") {
            mpi_manager->sendMessage(warped::make_unique<warped::MatternGVTToken>
                (0, 0, 1, 10, 12));

            SECTION("Can receive message, deserialize, and determine type and data correctly",
                    "[mpi][deserialize][receive]") {
                auto msg = mpi_manager->recvMessage();
                REQUIRE(msg.get() != nullptr);
                REQUIRE(msg->get_type() == warped::MessageType::MatternGVTToken);

                auto mattern_msg = warped::unique_cast<warped::KernelMessage,
                    warped::MatternGVTToken>(std::move(msg));

                REQUIRE(mattern_msg != nullptr);
                CHECK(mattern_msg->sender_id == 0);
                CHECK(mattern_msg->receiver_id == 0);
                CHECK(mattern_msg->m_clock == 1);
                CHECK(mattern_msg->m_send == 10);
                CHECK(mattern_msg->count == 12);


                SECTION("Recieve handlers can be added and msgs are dispatched functions correctly",
                    "[receive][dispatch][handlers]") {
                    auto mattern_gvt_manager = warped::make_unique<warped::MatternGVTManager>
                        (mpi_manager, 1000);

                    // This registers the message handlers for mattern class
                    mattern_gvt_manager->initialize();

                    mpi_manager->sendMessage(warped::make_unique<warped::MatternGVTToken>
                        (0, 0, 1, 10, 12));

                    mpi_manager->sendMessage(warped::make_unique<warped::GVTUpdateMessage>
                        (0, 0, 1234));

                    bool flag = mpi_manager->dispatchMessages();

                    // Ensure gvt has been updated
                    CHECK(mattern_gvt_manager->getGVT() == 1234);

                    // true means that a mattern token has been received
                    CHECK(flag == true);
                }
            }
        }

        mpi_manager->finalize();
    }
}

