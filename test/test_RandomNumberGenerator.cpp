#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <list>
#include <random>

#include "RandomNumberGenerator.hpp"
#include "utility/memory.hpp"

TEST_CASE("Save and restore random number generator states") {
    std::list<std::shared_ptr<warped::RandomNumberGenerator>> rng_list;

    auto model_rng = std::make_shared<std::default_random_engine>();
    auto warped_rng = std::make_shared<warped::RNGDerived<std::default_random_engine>>(model_rng);

    rng_list.push_back(warped_rng);

    SECTION("With uniform distribution") {
        auto num1 = (*model_rng)();
        auto num2 = (*model_rng)();

        auto ss1 = std::make_shared<std::stringstream>();

        for (auto rng = rng_list.begin(); rng != rng_list.end(); rng++) {
            (*rng)->getState(*ss1);
        }

        auto num3 = (*model_rng)();
        auto num4 = (*model_rng)();

        for (auto rng = rng_list.rbegin(); rng != rng_list.rend(); rng++) {
            (*rng)->restoreState(*ss1);
        }

        ss1 = std::make_shared<std::stringstream>();

        for (auto rng = rng_list.begin(); rng != rng_list.end(); rng++) {
            (*rng)->getState(*ss1);
        }

        CHECK((*model_rng)() == num3);
        CHECK((*model_rng)() == num4);

        (*model_rng)();
        (*model_rng)();

        for (auto rng = rng_list.rbegin(); rng != rng_list.rend(); rng++) {
            (*rng)->restoreState(*ss1);
        }

        CHECK((*model_rng)() == num3);
        CHECK((*model_rng)() == num4);
    }

    SECTION("With other distribution") {
        unsigned int average = 20;
        std::exponential_distribution<double> dist(1.0/20);

        auto num1 = (unsigned int)dist(*model_rng);
        auto num2 = (unsigned int)dist(*model_rng);

        auto ss1 = std::make_shared<std::stringstream>();

        for (auto rng = rng_list.begin(); rng != rng_list.end(); rng++) {
            (*rng)->getState(*ss1);
        }

        auto num3 = (unsigned int)dist(*model_rng);
        auto num4 = (unsigned int)dist(*model_rng);

        for (auto rng = rng_list.rbegin(); rng != rng_list.rend(); rng++) {
            (*rng)->restoreState(*ss1);
        }

        ss1 = std::make_shared<std::stringstream>();

        for (auto rng = rng_list.begin(); rng != rng_list.end(); rng++) {
            (*rng)->getState(*ss1);
        }

        CHECK((unsigned int)dist(*model_rng) == num3);
        CHECK((unsigned int)dist(*model_rng) == num4);

        dist(*model_rng);
        dist(*model_rng);

        for (auto rng = rng_list.rbegin(); rng != rng_list.rend(); rng++) {
            (*rng)->restoreState(*ss1);
        }

        CHECK((unsigned int)dist(*model_rng) == num3);
        CHECK((unsigned int)dist(*model_rng) == num4);
    }
}


