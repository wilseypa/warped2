#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <list>
#include <random>

#include "RandomNumberGenerator.hpp"
#include "utility/memory.hpp"

TEST_CASE("Save and restore random number generator states") {
    std::list<std::shared_ptr<warped::RandomNumberGenerator>> rng_list;

    auto model_rng1 = std::make_shared<std::minstd_rand0>();
    auto warped_rng1 = std::make_shared<warped::RNGDerived<std::minstd_rand0>>(model_rng1);
    auto model_rng2 = std::make_shared<std::mt19937>();
    auto warped_rng2 = std::make_shared<warped::RNGDerived<std::mt19937>>(model_rng2);

    rng_list.push_back(warped_rng1);
    rng_list.push_back(warped_rng2);

    std::list<std::shared_ptr<std::stringstream>> saved_list;

    SECTION("With uniform distribution") {
        auto num1_1 = (*model_rng1)();
        auto num2_1 = (*model_rng1)();
        auto num1_2 = (*model_rng2)();
        auto num2_2 = (*model_rng2)();

        for (auto rng = rng_list.begin(); rng != rng_list.end(); rng++) {
            auto ss = std::make_shared<std::stringstream>();
            (*rng)->getState(*ss);
            saved_list.push_back(ss);
        }

        auto num3_1 = (*model_rng1)();
        auto num4_1 = (*model_rng1)();
        auto num3_2 = (*model_rng2)();
        auto num4_2 = (*model_rng2)();

        for (auto rng = rng_list.rbegin(); rng != rng_list.rend(); rng++) {
            auto ss = saved_list.back();
            saved_list.pop_back();

            (*rng)->restoreState(*ss);
        }

        for (auto rng = rng_list.begin(); rng != rng_list.end(); rng++) {
            auto ss = std::make_shared<std::stringstream>();
            (*rng)->getState(*ss);
            saved_list.push_back(ss);
        }

        CHECK((*model_rng1)() == num3_1);
        CHECK((*model_rng1)() == num4_1);
        CHECK((*model_rng2)() == num3_2);
        CHECK((*model_rng2)() == num4_2);

        (*model_rng1)();
        (*model_rng1)();
        (*model_rng2)();
        (*model_rng2)();

        for (auto rng = rng_list.rbegin(); rng != rng_list.rend(); rng++) {
            auto ss = saved_list.back();
            saved_list.pop_back();

            (*rng)->restoreState(*ss);
        }

        CHECK((*model_rng1)() == num3_1);
        CHECK((*model_rng1)() == num4_1);
        CHECK((*model_rng2)() == num3_2);
        CHECK((*model_rng2)() == num4_2);
    }

    SECTION("With other distribution") {
        unsigned int average = 20;
        std::exponential_distribution<double> dist(1.0/20);

        auto num1_1 = (unsigned int)dist(*model_rng1);
        auto num2_1 = (unsigned int)dist(*model_rng1);
        auto num1_2 = (unsigned int)dist(*model_rng2);
        auto num2_2 = (unsigned int)dist(*model_rng2);

        for (auto rng = rng_list.begin(); rng != rng_list.end(); rng++) {
            auto ss = std::make_shared<std::stringstream>();
            (*rng)->getState(*ss);
            saved_list.push_back(ss);
        }

        auto num3_1 = (unsigned int)dist(*model_rng1);
        auto num4_1 = (unsigned int)dist(*model_rng1);
        auto num3_2 = (unsigned int)dist(*model_rng2);
        auto num4_2 = (unsigned int)dist(*model_rng2);

        for (auto rng = rng_list.rbegin(); rng != rng_list.rend(); rng++) {
            auto ss = saved_list.back();
            saved_list.pop_back();

            (*rng)->restoreState(*ss);
        }

        for (auto rng = rng_list.begin(); rng != rng_list.end(); rng++) {
            auto ss = std::make_shared<std::stringstream>();
            (*rng)->getState(*ss);
            saved_list.push_back(ss);
        }

        CHECK((unsigned int)dist(*model_rng1) == num3_1);
        CHECK((unsigned int)dist(*model_rng1) == num4_1);
        CHECK((unsigned int)dist(*model_rng2) == num3_2);
        CHECK((unsigned int)dist(*model_rng2) == num4_2);

        dist(*model_rng1);
        dist(*model_rng1);
        dist(*model_rng2);
        dist(*model_rng2);

        for (auto rng = rng_list.rbegin(); rng != rng_list.rend(); rng++) {
            auto ss = saved_list.back();
            saved_list.pop_back();

            (*rng)->restoreState(*ss);
        }

        CHECK((unsigned int)dist(*model_rng1) == num3_1);
        CHECK((unsigned int)dist(*model_rng1) == num4_1);
        CHECK((unsigned int)dist(*model_rng2) == num3_2);
        CHECK((unsigned int)dist(*model_rng2) == num4_2);
    }
}


