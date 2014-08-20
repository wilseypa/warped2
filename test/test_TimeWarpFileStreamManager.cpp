#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <cstring>
#include <fstream>

#include "TimeWarpFileStream.hpp"

TEST_CASE("Get filestreams, fossil collection, rollback operate correctly for output",
    "[filestream][output]") {

    unsigned int num_objects = 4;
    warped::TimeWarpFileStreamManager twfsm;
    twfsm.initialize(num_objects);

    SECTION("Create and get filestreams correctly", "[get][filestream]") {
        CHECK(twfsm.size(0) == 0);
        CHECK(twfsm.size(1) == 0);
        CHECK(twfsm.size(2) == 0);
        CHECK(twfsm.size(3) == 0);

        auto t1 = twfsm.getFileStream("test_out1.txt", std::ios_base::out, 1);
        REQUIRE(t1 != nullptr);
        CHECK(twfsm.size(0) == 0);
        CHECK(twfsm.size(1) == 1);
        CHECK(twfsm.size(2) == 0);
        CHECK(twfsm.size(3) == 0);

        auto t2 = twfsm.getFileStream("test_out1.txt", std::ios_base::out, 1);
        REQUIRE(t2 != nullptr);
        CHECK(twfsm.size(0) == 0);
        CHECK(twfsm.size(1) == 1);
        CHECK(twfsm.size(2) == 0);
        CHECK(twfsm.size(3) == 0);

        CHECK_THROWS(twfsm.getFileStream("test_out1.txt", std::ios_base::out, 2));

        auto t3 = twfsm.getFileStream("test_out2.txt", std::ios_base::out, 1);
        REQUIRE(t3 != nullptr);
        CHECK(twfsm.size(0) == 0);
        CHECK(twfsm.size(1) == 2);
        CHECK(twfsm.size(2) == 0);
        CHECK(twfsm.size(3) == 0);

        auto t4 = twfsm.getFileStream("test_out3.txt", std::ios_base::out, 3);
        REQUIRE(t4 != nullptr);
        CHECK(twfsm.size(0) == 0);
        CHECK(twfsm.size(1) == 2);
        CHECK(twfsm.size(2) == 0);
        CHECK(twfsm.size(3) == 1);

        // This should not work, it has already been created as output
        auto t5 = twfsm.getFileStream("test_out3.txt", std::ios_base::in, 3);

        SECTION("Output when used as intended, works with rollback and fossil collection",
            "[output]") {

            CHECK(t1->size() == 0);
            twfsm.setObjectCurrentTime(5, 1);
            *t1 << "Object " << 1 << " with timestamp " << 5 << "\n";
            CHECK(t1->size() == 5);

            twfsm.setObjectCurrentTime(10, 1);
            *t2 << "Object " << 1 << " with timestamp " << 10 << "\n";
            CHECK(t2->size() == 10);

            twfsm.setObjectCurrentTime(15, 1);
            auto t6 = twfsm.getFileStream("test_out1.txt", std::ios_base::out, 1);
            *t6 << "Object " << 1 << " with timestamp " << 15 << "\n";
            CHECK(t2->size() == 15);

            twfsm.setObjectCurrentTime(20, 1);
            auto t7 = twfsm.getFileStream("test_out1.txt", std::ios_base::out, 1);
            *t7 << "Object " << 1 << " with timestamp " << 20 << "\n";
            CHECK(t2->size() == 20);

            twfsm.setObjectCurrentTime(25, 1);
            auto t8 = twfsm.getFileStream("test_out1.txt", std::ios_base::out, 1);
            *t8 << "Object " << 1 << " with timestamp " << 25 << "\n";
            CHECK(t2->size() == 25);

            twfsm.setObjectCurrentTime(30, 1);
            auto t9 = twfsm.getFileStream("test_out1.txt", std::ios_base::out, 1);
            *t9 << "Object " << 1 << " with timestamp " << 30 << "\n";
            CHECK(t2->size() == 30);

            twfsm.setObjectCurrentTime(35, 1);
            auto t10 = twfsm.getFileStream("test_out1.txt", std::ios_base::out, 1);
            *t10 << "Object " << 1 << " with timestamp " << 35 << "\n";
            CHECK(t2->size() == 35);

            SECTION("Rollback", "[rollback][output]") {
                twfsm.rollback(38, 1);
                CHECK(t6->size() == 35);

                twfsm.rollback(28, 1);
                CHECK(t9->size() == 25);

                twfsm.rollback(22, 1);
                CHECK(t10->size() == 20);

                twfsm.rollback(8, 1);
                CHECK(t2->size() == 5);

                twfsm.rollback(2, 1);
                CHECK(t2->size() == 0);
            }

            SECTION("Fossil Collection", "[fossilCollect][output]") {
                twfsm.fossilCollect(2, 1);
                CHECK(t10->size() == 35);

                twfsm.fossilCollect(6, 1);
                CHECK(t8->size() == 30);

                twfsm.fossilCollect(27, 1);
                CHECK(t6->size() == 10);

                twfsm.fossilCollect(39, 1);
                CHECK(t7->size() == 0);
            }
        }
    }
}

TEST_CASE("Can get input streams and use them.", "[filestream][input]") {
    std::fstream file("test_in.txt", std::fstream::out | std::fstream::trunc);
    const char *string = "line 1\n10\n56.78\nline 4\nline 5";
    file.write(string, std::strlen(string));
    file.close();

    unsigned int num_objects = 2;
    warped::TimeWarpFileStreamManager twfsm;
    twfsm.initialize(num_objects);

    auto t1 = twfsm.getFileStream("test_in.txt", std::ios_base::in, 1);
    REQUIRE(t1 != nullptr);
    CHECK(twfsm.size(0) == 0);
    CHECK(twfsm.size(1) == 1);

    auto t2 = twfsm.getFileStream("test_in.txt", std::ios_base::in, 1);
    REQUIRE(t2 != nullptr);
    CHECK(twfsm.size(0) == 0);
    CHECK(twfsm.size(1) == 1);

    REQUIRE_THROWS(twfsm.getFileStream("test_in.txt", std::ios_base::in, 0));

    char *test1 = new char[16];
    int test2;
    double test3;
    char *test4 = new char[16];
    char *test5 = new char[16];

    t1->getline(test1, 16);
    CHECK(strcmp(test1, "line 1") == 0);

    *t2 >> test2;
    CHECK(test2 == 10);
    t2->ignore(1);

    auto t3 = twfsm.getFileStream("test_in.txt", std::ios_base::in, 1);
    *t3 >> test3;
    CHECK(test3 == 56.78);
    t3->ignore(1);

    auto t4 = twfsm.getFileStream("test_in.txt", std::ios_base::in, 1);
    t4->getline(test4, 16);
    CHECK(strcmp(test4, "line 4") == 0);
    CHECK(t4->gcount() == 7);

    auto t5 = twfsm.getFileStream("test_in.txt", std::ios_base::in, 1);
    t5->read(test5, 16);
    CHECK(t5->eof() == true);
}

