
#pragma once

#include <immu/detail/free_list.hpp>
#include <immu/detail/ref_count_base.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <array>
#include <memory>
#include <cassert>

namespace immu {
namespace detail {

constexpr auto branching_log  = 5u;
constexpr auto branching      = 1u << branching_log;
constexpr std::size_t branching_mask = branching - 1;

template <typename T>
struct node;

template <typename T>
using node_ptr = boost::intrusive_ptr<node<T> >;

template <typename T>
using leaf_node  = std::array<T, branching>;

template <typename T>
using inner_node = std::array<node_ptr<T>, branching>;

template <typename T, typename Deriv=void>
struct node_base : ref_count_base<Deriv>
{
    using leaf_node_t  = leaf_node<T>;
    using inner_node_t = inner_node<T>;

    enum
    {
        leaf_kind,
        inner_kind
    } kind;

    union data_t
    {
        leaf_node_t  leaf;
        inner_node_t inner;
        data_t(leaf_node_t n)  : leaf(std::move(n)) {}
        data_t(inner_node_t n) : inner(std::move(n)) {}
        ~data_t() {}
    } data;

    ~node_base()
    {
        switch (kind) {
        case leaf_kind:
            data.leaf.~leaf_node_t();
            break;
        case inner_kind:
            data.inner.~inner_node_t();
            break;
        }
    }

    node_base(leaf_node<T> n)
        : kind{leaf_kind}
        , data{std::move(n)}
    {}

    node_base(inner_node<T> n)
        : kind{inner_kind}
        , data{std::move(n)}
    {}

    inner_node_t& inner() & {
        assert(kind == inner_kind);
        return data.inner;
    }
    const inner_node_t& inner() const& {
        assert(kind == inner_kind);
        return data.inner;
    }
    inner_node_t&& inner() && {
        assert(kind == inner_kind);
        return std::move(data.inner);
    }

    leaf_node_t& leaf() & {
        assert(kind == leaf_kind);
        return data.leaf;
    }
    const leaf_node_t& leaf() const& {
        assert(kind == leaf_kind);
        return data.leaf;
    }
    leaf_node_t&& leaf() && {
        assert(kind == leaf_kind);
        return std::move(data.leaf);
    }
};

template <typename T>
struct node : node_base<T, node<T>>
            , with_thread_local_free_list<sizeof(node_base<T, node<T>>)>
{
    using node_base<T, node<T>>::node_base;
};

template <typename T, typename ...Ts>
auto make_node(Ts&& ...xs) -> boost::intrusive_ptr<node<T>>
{
    return new node<T>(std::forward<Ts>(xs)...);
}

} /* namespace detail */
} /* namespace immu */