#include <gtest/gtest.h>
#include <core/type_info.hpp>

TEST(TypeInfo, Construction) {
    auto constructor = [](void* dest, void* src) {
        new (dest) int(*static_cast<int*>(src));
    };

    auto destructor = [](void* obj) {};

    auto swapper = [](void* a, void* b) {
        std::swap(*static_cast<int*>(a), *static_cast<int*>(b));
    };

    ::myth::core::type_info info(sizeof(int), alignof(int), constructor, destructor, swapper);

    ASSERT_EQ(info._size, sizeof(int));
    ASSERT_EQ(info._align, alignof(int));
    ASSERT_EQ(info._constructor, constructor);
    ASSERT_EQ(info._destructor, destructor);
    ASSERT_EQ(info._swapper, swapper);
}

TEST(TypeInfo, Copy) {
    auto constructor = [](void* dest, void* src) {
        new (dest) int(*static_cast<int*>(src));
    };

    auto destructor = [](void* obj) {};

    auto swapper = [](void* a, void* b) {
        std::swap(*static_cast<int*>(a), *static_cast<int*>(b));
    };

    ::myth::core::type_info info1(sizeof(int), alignof(int), constructor, destructor, swapper);
    ::myth::core::type_info info2 { info1 };

    ASSERT_EQ(info1._size, sizeof(int));
    ASSERT_EQ(info1._align, alignof(int));
    ASSERT_EQ(info1._constructor, constructor);
    ASSERT_EQ(info1._destructor, destructor);
    ASSERT_EQ(info1._swapper, swapper);

    ASSERT_EQ(info2._size, sizeof(int));
    ASSERT_EQ(info2._align, alignof(int));
    ASSERT_EQ(info2._constructor, constructor);
    ASSERT_EQ(info2._destructor, destructor);
    ASSERT_EQ(info2._swapper, swapper);

    ::myth::core::type_info info3 = info1;

    ASSERT_EQ(info3._size, sizeof(int));
    ASSERT_EQ(info3._align, alignof(int));
    ASSERT_EQ(info3._constructor, constructor);
    ASSERT_EQ(info3._destructor, destructor);
    ASSERT_EQ(info3._swapper, swapper);
}

TEST(TypeInfo, Move) {
    auto constructor = [](void* dest, void* src) {
        new (dest) int(*static_cast<int*>(src));
    };

    auto destructor = [](void* obj) {};

    auto swapper = [](void* a, void* b) {
        std::swap(*static_cast<int*>(a), *static_cast<int*>(b));
    };

    ::myth::core::type_info info1(sizeof(int), alignof(int), constructor, destructor, swapper);
    ::myth::core::type_info info2 { std::move(info1) };

    ASSERT_EQ(info1._size, sizeof(int));
    ASSERT_EQ(info1._align, alignof(int));
    ASSERT_EQ(info1._constructor, constructor);
    ASSERT_EQ(info1._destructor, destructor);
    ASSERT_EQ(info1._swapper, swapper);

    ASSERT_EQ(info2._size, sizeof(int));
    ASSERT_EQ(info2._align, alignof(int));
    ASSERT_EQ(info2._constructor, constructor);
    ASSERT_EQ(info2._destructor, destructor);
    ASSERT_EQ(info2._swapper, swapper);

    ::myth::core::type_info info3 = std::move(info2);

    ASSERT_EQ(info3._size, sizeof(int));
    ASSERT_EQ(info3._align, alignof(int));
    ASSERT_EQ(info3._constructor, constructor);
    ASSERT_EQ(info3._destructor, destructor);
    ASSERT_EQ(info3._swapper, swapper);
}

TEST(TypeInfoGenerator, GenForTrivialType) {
    ASSERT_TRUE(std::is_trivially_copyable_v<int>);
    ASSERT_TRUE(std::is_trivially_destructible_v<int>);

    auto info = ::myth::core::type_info_generator::gen<int>();

    ASSERT_EQ(info._size, sizeof(int));
    ASSERT_EQ(info._align, alignof(int));

    int x = 42;
    void* dest1 = operator new(sizeof(int), std::align_val_t(alignof(int)));
    info._constructor(dest1, &x);
    ASSERT_EQ(*static_cast<int*>(dest1), 42);

    int y = 10;
    void* dest2 = operator new(sizeof(int), std::align_val_t(alignof(int)));
    info._constructor(dest2, &y);
    ASSERT_EQ(*static_cast<int*>(dest2), 10);

    info._swapper(dest1, dest2);
    ASSERT_EQ(*static_cast<int*>(dest1), 10);
    ASSERT_EQ(*static_cast<int*>(dest2), 42);

    info._destructor(dest1);
    operator delete(dest1, std::align_val_t(alignof(int)));
    info._destructor(dest2);
    operator delete(dest2, std::align_val_t(alignof(int)));
}

TEST(TypeInfoGenerator, GenForNonTrivialType) {
    struct NonTrivial {
        int value;

        NonTrivial(int v) noexcept : value(v) {}

        ~NonTrivial() noexcept {}
    };

    ASSERT_FALSE(std::is_trivially_copyable_v<NonTrivial>);
    ASSERT_FALSE(std::is_trivially_destructible_v<NonTrivial>);

    auto info = ::myth::core::type_info_generator::gen<NonTrivial>();

    ASSERT_EQ(info._size, sizeof(NonTrivial));
    ASSERT_EQ(info._align, alignof(NonTrivial));

    NonTrivial obj1(42);
    void* dest1 = operator new(sizeof(NonTrivial), std::align_val_t(alignof(NonTrivial)));
    info._constructor(dest1, &obj1);
    ASSERT_EQ(static_cast<NonTrivial*>(dest1)->value, 42);

    NonTrivial obj2(10);
    void* dest2 = operator new(sizeof(NonTrivial), std::align_val_t(alignof(NonTrivial)));
    info._constructor(dest2, &obj2);
    ASSERT_EQ(static_cast<NonTrivial*>(dest2)->value, 10);

    info._swapper(dest1, dest2);
    ASSERT_EQ(static_cast<NonTrivial*>(dest1)->value, 10);
    ASSERT_EQ(static_cast<NonTrivial*>(dest2)->value, 42);

    info._destructor(dest1);
    operator delete(dest1, std::align_val_t(alignof(NonTrivial)));
    info._destructor(dest2);
    operator delete(dest2, std::align_val_t(alignof(NonTrivial)));
}

TEST(TypeInfoGenerator, GenForMoveOnlyType) {
    struct MoveOnly {
        int value;

        MoveOnly(int v) noexcept : value(v) {}

        MoveOnly(MoveOnly&& other) noexcept : value(other.value) { other.value = -1; }
        MoveOnly& operator=(MoveOnly&& other) noexcept { value = other.value; other.value = -1; return *this; }

        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
    };

    ASSERT_FALSE(std::is_copy_constructible_v<MoveOnly>);
    ASSERT_TRUE(std::is_move_constructible_v<MoveOnly>);
    ASSERT_FALSE(std::is_trivially_copyable_v<MoveOnly>);

    auto info = ::myth::core::type_info_generator::gen<MoveOnly>();

    ASSERT_EQ(info._size, sizeof(MoveOnly));
    ASSERT_EQ(info._align, alignof(MoveOnly));

    MoveOnly obj1(42);
    void* dest1 = operator new(sizeof(MoveOnly), std::align_val_t(alignof(MoveOnly)));
    info._constructor(dest1, &obj1);
    ASSERT_EQ(static_cast<MoveOnly*>(dest1)->value, 42);

    MoveOnly obj2(10);
    void* dest2 = operator new(sizeof(MoveOnly), std::align_val_t(alignof(MoveOnly)));
    info._constructor(dest2, &obj2);
    ASSERT_EQ(static_cast<MoveOnly*>(dest2)->value, 10);

    info._swapper(dest1, dest2);
    ASSERT_EQ(static_cast<MoveOnly*>(dest1)->value, 10);
    ASSERT_EQ(static_cast<MoveOnly*>(dest2)->value, 42);

    info._destructor(dest1);
    operator delete(dest1, std::align_val_t(alignof(MoveOnly)));
    info._destructor(dest2);
    operator delete(dest2, std::align_val_t(alignof(MoveOnly)));
}

TEST(TypeInfoGenerator, GenIsIdempotent) {
    auto info1 = ::myth::core::type_info_generator::gen<int>();
    auto info2 = ::myth::core::type_info_generator::gen<int>();

    ASSERT_EQ(info1._size, info2._size);
    ASSERT_EQ(info1._align, info2._align);
    ASSERT_EQ(info1._constructor, info2._constructor);
    ASSERT_EQ(info1._destructor, info2._destructor);
    ASSERT_EQ(info1._swapper, info2._swapper);
}

TEST(TypeInfoGenerator, SwapperSelfSwap) {
    auto info = ::myth::core::type_info_generator::gen<int>();

    int x = 42;
    void* p = operator new(sizeof(int), std::align_val_t(alignof(int)));
    info._constructor(p, &x);
    ASSERT_EQ(*static_cast<int*>(p), 42);

    info._swapper(p, p);
    ASSERT_EQ(*static_cast<int*>(p), 42);

    info._destructor(p);
    operator delete(p, std::align_val_t(alignof(int)));
}