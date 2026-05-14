#pragma once

#include <limits>

#include "core/concept.hpp"

namespace myth::ecs {
    /*
     * @brief A basic entity in the ECS, identified by ID and version.
     *
     * @tparam EntityIdentityT The unsigned integral type used for the entity ID.
     * @tparam EntityVersionT The unsigned integral type used for the entity version.
     */
    template<
        ::myth::core::UnsignedIntegralType EntityIdentityT,
        ::myth::core::UnsignedIntegralType EntityVersionT
    >
    class basic_entity final {
    public:
        /*! @brief The type used for the entity ID. */
        using entity_id_type = EntityIdentityT;
        /*! @brief The type used for the entity version. */
        using entity_version_type = EntityVersionT;
        /*! @brief The type of the basic entity itself. */
        using self_type = basic_entity<entity_id_type, entity_version_type>;

        /*! @brief A constant representing a null entity ID. */
        static constexpr entity_id_type null_entity_id = std::numeric_limits<entity_id_type>::max();

        /*! @brief Default constructor that creates an entity with default ID and version. */
        basic_entity() noexcept : _id(0), _version(0) {};

        /*! @brief Constructor that creates an entity with the specified ID and version. */
        basic_entity(entity_id_type id, entity_version_type version = 0) noexcept : _id(id), _version(version) {}

        /*! @brief Copy constructor. */
        basic_entity(const basic_entity&) = default;

        /*! @brief Move constructor. */
        basic_entity(basic_entity&&) noexcept = default;

        /*! @brief Destructor. */
        ~basic_entity() = default;

        /*! @brief Copy assignment operator. */
        basic_entity& operator=(const basic_entity&) = default;

        /*! @brief Move assignment operator. */
        basic_entity& operator=(basic_entity&&) noexcept = default;

    public:
        /*! @brief Gets the ID of the entity. */
        [[nodiscard]] entity_id_type id() const noexcept { return _id; }

        /*! @brief Gets the version of the entity. */
        [[nodiscard]] entity_version_type version() const noexcept { return _version; }

        /*! @brief Checks if the entity is valid (i.e., has a non-null ID). */
        [[nodiscard]] bool valid() const noexcept { return _id != null_entity_id; }

        /*! @brief Checks if two entities are equal. */
        friend bool operator==(const self_type& lhs, const self_type& rhs) noexcept { return lhs._id == rhs._id && lhs._version == rhs._version; }

        /*! @brief Checks if two entities are not equal. */
        friend bool operator!=(const self_type& lhs, const self_type& rhs) noexcept { return !(lhs == rhs); }

    private:
        entity_id_type _id;
        entity_version_type _version;
    };
}