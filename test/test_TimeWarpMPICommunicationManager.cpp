#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <thread>

#include "TimeWarpMPICommunicationManager.hpp"
#include "utility/memory.hpp"
#include "TimeWarpMatternGVTManager.hpp"

TEST_CASE("Messages can be enqueued, serialized, sent, received, and deserialized","[mpi]"){
    auto mpi_manager = std::make_shared<warped::TimeWarpMPICommunicationManager>(100, 100, 512);

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
            mpi_manager->insertMessage(warped::make_unique<warped::MatternGVTToken>
                (0, 0, 1, 10, 12));

            mpi_manager->sendMessages();
            mpi_manager->sendMessages();
            mpi_manager->sendMessages();
            mpi_manager->sendMessages();

            SECTION("Can receive message, deserialize, and determine type and data correctly",
                    "[mpi][deserialize][receive]") {
                auto msg = mpi_manager->getMessage();
                REQUIRE(msg.get() != nullptr);
                REQUIRE(msg->get_type() == warped::MessageType::MatternGVTToken);

                auto mattern_msg = warped::unique_cast<warped::TimeWarpKernelMessage,
                    warped::MatternGVTToken>(std::move(msg));

                REQUIRE(mattern_msg != nullptr);
                CHECK(mattern_msg->sender_id == 0);
                CHECK(mattern_msg->receiver_id == 0);
                CHECK(mattern_msg->m_clock == 1);
                CHECK(mattern_msg->m_send == 10);
                CHECK(mattern_msg->count == 12);


                SECTION("Recieve handlers can be added and msgs are dispatched functions correctly",
                    "[receive][dispatch][handlers]") {
                    auto mattern_gvt_manager = warped::make_unique<warped::TimeWarpMatternGVTManager>
                        (mpi_manager, 1000);

                    // This registers the message handlers for mattern class
                    mattern_gvt_manager->initialize();

                    mpi_manager->insertMessage(warped::make_unique<warped::MatternGVTToken>
                        (0, 0, 1, 10, 12));

                    mpi_manager->insertMessage(warped::make_unique<warped::GVTUpdateMessage>
                        (0, 0, 1234));

                }
            }
        }

        mpi_manager->finalize();
    }
}

