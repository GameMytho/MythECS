#include <gtest/gtest.h>
#include <core/type_info.hpp>

// ============================================================================
// Construction - build a type_info from raw size/align/function pointers
// ============================================================================
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

// ============================================================================
// Copy - copy ctor and copy assignment preserve all fields
// ============================================================================
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

    // Source unchanged after copy ctor.
    ASSERT_EQ(info1._size, sizeof(int));
    ASSERT_EQ(info1._align, alignof(int));
    ASSERT_EQ(info1._constructor, constructor);
    ASSERT_EQ(info1._destructor, destructor);
    ASSERT_EQ(info1._swapper, swapper);

    // Copy matches source.
    ASSERT_EQ(info2._size, sizeof(int));
    ASSERT_EQ(info2._align, alignof(int));
    ASSERT_EQ(info2._constructor, constructor);
    ASSERT_EQ(info2._destructor, destructor);
    ASSERT_EQ(info2._swapper, swapper);

    // Copy assignment.
    ::myth::core::type_info info3 = info1;

    ASSERT_EQ(info3._size, sizeof(int));
    ASSERT_EQ(info3._align, alignof(int));
    ASSERT_EQ(info3._constructor, constructor);
    ASSERT_EQ(info3._destructor, destructor);
    ASSERT_EQ(info3._swapper, swapper);
}

// ============================================================================
// Move - move ctor and move assignment; type_info is trivially copyable so
// move is equivalent to copy (source remains valid)
// ============================================================================
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

    // type_info holds plain function pointers - move is just a copy.
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

    // Move assignment.
    ::myth::core::type_info info3 = std::move(info2);

    ASSERT_EQ(info3._size, sizeof(int));
    ASSERT_EQ(info3._align, alignof(int));
    ASSERT_EQ(info3._constructor, constructor);
    ASSERT_EQ(info3._destructor, destructor);
    ASSERT_EQ(info3._swapper, swapper);
}

// ============================================================================
// Constexpr - construction, copy, and move all work at compile time
// ============================================================================
TEST(TypeInfo, Constexpr) {
    constexpr ::myth::core::type_info info(sizeof(int), alignof(int), nullptr, nullptr, nullptr);

    static_assert(info._size == sizeof(int));
    static_assert(info._align == alignof(int));
    static_assert(info._constructor == nullptr);
    static_assert(info._destructor == nullptr);
    static_assert(info._swapper == nullptr);

    // Constexpr copy ctor.
    constexpr ::myth::core::type_info info2 { info };

    static_assert(info2._size == sizeof(int));
    static_assert(info2._align == alignof(int));
    static_assert(info2._constructor == nullptr);
    static_assert(info2._destructor == nullptr);
    static_assert(info2._swapper == nullptr);

    // Constexpr copy assignment.
    constexpr ::myth::core::type_info info3 = info;

    static_assert(info3._size == sizeof(int));
    static_assert(info3._align == alignof(int));
    static_assert(info3._constructor == nullptr);
    static_assert(info3._destructor == nullptr);
    static_assert(info3._swapper == nullptr);

    // Constexpr move ctor (move = copy for trivially-copyable type_info).
    constexpr ::myth::core::type_info info4 { std::move(info) };

    static_assert(info4._size == sizeof(int));
    static_assert(info4._align == alignof(int));
    static_assert(info4._constructor == nullptr);
    static_assert(info4._destructor == nullptr);
    static_assert(info4._swapper == nullptr);

    // Constexpr move assignment.
    constexpr ::myth::core::type_info info5 = std::move(info2);

    static_assert(info5._size == sizeof(int));
    static_assert(info5._align == alignof(int));
    static_assert(info5._constructor == nullptr);
    static_assert(info5._destructor == nullptr);
    static_assert(info5._swapper == nullptr);
}

// ============================================================================
// info<T>() for trivial types - uses memcpy-based copy, no-op destruct, byte-swap
// ============================================================================
TEST(TypeInfoGenerator, InfoForTrivialType) {
    ASSERT_TRUE(std::is_trivially_copyable_v<int>);
    ASSERT_TRUE(std::is_trivially_destructible_v<int>);

    auto info = ::myth::core::type_info_generator::info<int>();

    ASSERT_EQ(info._size, sizeof(int));
    ASSERT_EQ(info._align, alignof(int));

    // Construct via placement-copy and verify.
    int x = 42;
    void* dest1 = operator new(sizeof(int), std::align_val_t(alignof(int)));
    info._constructor(dest1, &x);
    ASSERT_EQ(*static_cast<int*>(dest1), 42);

    int y = 10;
    void* dest2 = operator new(sizeof(int), std::align_val_t(alignof(int)));
    info._constructor(dest2, &y);
    ASSERT_EQ(*static_cast<int*>(dest2), 10);

    // Swap via byte-copy and verify.
    info._swapper(dest1, dest2);
    ASSERT_EQ(*static_cast<int*>(dest1), 10);
    ASSERT_EQ(*static_cast<int*>(dest2), 42);

    // Trivial destructor is a no-op (no double-free risk).
    info._destructor(dest1);
    operator delete(dest1, std::align_val_t(alignof(int)));
    info._destructor(dest2);
    operator delete(dest2, std::align_val_t(alignof(int)));
}

// ============================================================================
// info<T>() for non-trivial types - uses placement-new ctor, real dtor, std::swap
// ============================================================================
TEST(TypeInfoGenerator, InfoForNonTrivialType) {
    struct NonTrivial {
        int value;

        NonTrivial(int v) noexcept : value(v) {}

        ~NonTrivial() noexcept {}
    };

    ASSERT_FALSE(std::is_trivially_copyable_v<NonTrivial>);
    ASSERT_FALSE(std::is_trivially_destructible_v<NonTrivial>);

    auto info = ::myth::core::type_info_generator::info<NonTrivial>();

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

    // Non-trivial swapper uses std::swap.
    info._swapper(dest1, dest2);
    ASSERT_EQ(static_cast<NonTrivial*>(dest1)->value, 10);
    ASSERT_EQ(static_cast<NonTrivial*>(dest2)->value, 42);

    // Non-trivial destructor calls ~NonTrivial().
    info._destructor(dest1);
    operator delete(dest1, std::align_val_t(alignof(NonTrivial)));
    info._destructor(dest2);
    operator delete(dest2, std::align_val_t(alignof(NonTrivial)));
}

// ============================================================================
// info<T>() for move-only types - not trivially copyable, but still swappable
// ============================================================================
TEST(TypeInfoGenerator, InfoForMoveOnlyType) {
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

    auto info = ::myth::core::type_info_generator::info<MoveOnly>();

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

// ============================================================================
// Idempotency - calling info<T>() twice returns the same static record
// ============================================================================
TEST(TypeInfoGenerator, InfoIsIdempotent) {
    auto info1 = ::myth::core::type_info_generator::info<int>();
    auto info2 = ::myth::core::type_info_generator::info<int>();

    // Same type produces the identical type_info via static caching.
    ASSERT_EQ(info1._size, info2._size);
    ASSERT_EQ(info1._align, info2._align);
    ASSERT_EQ(info1._constructor, info2._constructor);
    ASSERT_EQ(info1._destructor, info2._destructor);
    ASSERT_EQ(info1._swapper, info2._swapper);
}

// ============================================================================
// Self-swap - swapping an element with itself is safe
// ============================================================================
TEST(TypeInfoGenerator, SwapperSelfSwap) {
    auto info = ::myth::core::type_info_generator::info<int>();

    int x = 42;
    void* p = operator new(sizeof(int), std::align_val_t(alignof(int)));
    info._constructor(p, &x);
    ASSERT_EQ(*static_cast<int*>(p), 42);

    // Self-swap must not corrupt the value.
    info._swapper(p, p);
    ASSERT_EQ(*static_cast<int*>(p), 42);

    info._destructor(p);
    operator delete(p, std::align_val_t(alignof(int)));
}

// ============================================================================
// id<T>() - sequential assignment - each new type gets the next counter value
// ============================================================================
TEST(TypeInfoGenerator, IdSequential) {
    auto id0 = ::myth::core::type_info_generator::id<int>();
    auto id1 = ::myth::core::type_info_generator::id<double>();

    // Different types receive distinct identifiers.
    ASSERT_NE(id0, id1);
}

// ============================================================================
// id<T>() - idempotency - same type always returns the same cached identifier
// ============================================================================
TEST(TypeInfoGenerator, IdIdempotent) {
    auto id0 = ::myth::core::type_info_generator::id<int>();
    auto id1 = ::myth::core::type_info_generator::id<int>();

    // Same type returns the same cached identifier.
    ASSERT_EQ(id0, id1);
}