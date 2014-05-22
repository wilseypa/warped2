#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>

#include "serialization.hpp"

struct Class1 {
    Class1(int x=1, int y=2): x(x), y(y) {}
    int x;
    int y;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(x, y);
};

TEST_CASE("Objects can be serialized", "[serialization]") {
    std::stringstream ss;
    Class1 c1 {3, 4};
    Class1 c2 {1, 2};

    REQUIRE(c2.x == 1);
    REQUIRE(c2.y == 2);

    {
        cereal::BinaryOutputArchive oarchive(ss);
        oarchive(c1);
    }

    {
        cereal::BinaryInputArchive iarchive(ss);
        iarchive(c2);
    }
    REQUIRE(c2.x == 3);
    REQUIRE(c2.y == 4);
}

struct BaseClass {
    virtual int f() = 0;
};

struct DerivedClass : public BaseClass {
    DerivedClass() = default;
    DerivedClass(int x) : x(x) {}
    int f() { return 2; }
    int x;
    WARPED_REGISTER_SERIALIZABLE_MEMBERS(x);
};
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(DerivedClass);

TEST_CASE("Polymorphic Objects can be serialized", "[serialization]") {
    std::stringstream ss;
    std::unique_ptr<BaseClass> c1{new DerivedClass(1)};
    std::unique_ptr<BaseClass> c2;

    REQUIRE(c1->f() == 2);
    {
        cereal::BinaryOutputArchive oarchive(ss);
        oarchive(c1);
    }

    {
        cereal::BinaryInputArchive iarchive(ss);
        iarchive(c2);
    }
    REQUIRE(c2->f() == 2);
}