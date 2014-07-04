#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <thread>

#include "MPICommunicationManager.hpp"
#include "utility/memory.hpp"
#include "MatternGVTManager.hpp"

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::MatternGVTToken)

TEST_CASE("MPI Communication Manager functions correctly","[mpi]"){
    auto mpi_manager = warped::make_unique<warped::MPICommunicationManager>();

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
            }
        }
        mpi_manager->finalize();
    }
}
