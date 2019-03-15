// Copyright 2018-2019 Rene Rivera
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_PARSER_HPP
#define LYRA_PARSER_HPP

#include "lyra/parser_result.hpp"
#include "lyra/args.hpp"
#include "lyra/detail/bound.hpp"
#include "lyra/detail/from_string.hpp"
#include "lyra/detail/result.hpp"
#include "lyra/detail/tokens.hpp"

#include <memory>
#include <string>
#include <tuple>

namespace lyra
{

// Customization of parsing
struct parser_customization
{
    virtual std::string token_delimiters() const = 0;
    virtual std::string option_prefix() const = 0;
};

struct default_parser_customization : parser_customization
{
    std::string token_delimiters() const override { return " ="; }
    std::string option_prefix() const override { return "-"; }
};

namespace detail
{
    class parse_state
    {
        public:
        parse_state(parser_result_type type, token_iterator const& remainingTokens)
            : m_type(type)
            , m_remainingTokens(remainingTokens)
        {
        }

        auto type() const -> parser_result_type { return m_type; }
        auto remainingTokens() const -> token_iterator { return m_remainingTokens; }

        private:
        parser_result_type m_type;
        token_iterator m_remainingTokens;
    };
} // namespace detail

class parser_base
{
    public:

    struct help_text_item
    {
        std::string option;
        std::string description;
    };

    using help_text = std::vector<help_text_item>;
    using parse_result = detail::basic_result<detail::parse_state>;

    virtual ~parser_base() = default;
    virtual auto validate() const -> result { return result::ok(); }
    virtual auto parse(std::string const& exe_name, detail::token_iterator const& tokens, parser_customization const& customize) const
        -> parse_result = 0;
    virtual auto cardinality() const -> std::tuple<size_t, size_t>
    {
        return std::make_tuple(0, 1);
    }

    auto cardinality_count() const -> size_t
    {
        auto c = cardinality();
        return std::get<1>(c) - std::get<0>(c);
    }

    auto is_optional() const -> bool
    {
        auto c = cardinality();
        return (std::get<0>(c) == 0) && (std::get<1>(c) > 0);
    }

    virtual std::string get_usage_text() const { return ""; }

    virtual help_text get_help_text() const { return {}; }

    auto parse(args const& args, parser_customization const& customize = default_parser_customization()) const -> parse_result
    {
        return parse(args.exe_name(), detail::token_iterator(args, customize.token_delimiters(), customize.option_prefix()), customize);
    }

    virtual std::unique_ptr<parser_base> clone() const { return nullptr; }
};

template <typename DerivedT>
class composable_parser : public parser_base
{
};

// Common code and state for args and Opts
template <typename DerivedT>
class bound_parser : public composable_parser<DerivedT>
{
    protected:
    std::shared_ptr<detail::BoundRef> m_ref;
    std::string m_hint;
    std::string m_description;
    std::tuple<size_t, size_t> m_cardinality;

    explicit bound_parser(std::shared_ptr<detail::BoundRef> const& ref)
        : m_ref(ref)
    {
        if (m_ref->isContainer())
            m_cardinality = std::make_tuple(0, 0);
        else
            m_cardinality = std::make_tuple(0, 1);
    }

    public:
    template <typename T>
    bound_parser(T& ref, std::string const& hint)
        : bound_parser(std::make_shared<detail::BoundValueRef<T>>(ref))
    {
        m_hint = hint;
    }

    template <typename LambdaT>
    bound_parser(LambdaT const& ref, std::string const& hint)
        : bound_parser(std::make_shared<detail::BoundLambda<LambdaT>>(ref))
    {
        m_hint = hint;
    }

    auto operator()(std::string const& description) -> DerivedT&
    {
        m_description = description;
        return static_cast<DerivedT&>(*this);
    }

    auto optional() -> DerivedT& { return this->cardinality(0,1); }

    auto required() -> DerivedT& { return this->cardinality(1,1); }

    auto cardinality(size_t n) -> DerivedT&
    {
        m_cardinality = std::make_tuple(n, n);
        return static_cast<DerivedT&>(*this);
    }

    auto cardinality(size_t n, size_t m) -> DerivedT&
    {
        m_cardinality = std::make_tuple(n, m);
        return static_cast<DerivedT&>(*this);
    }

    auto cardinality() const -> std::tuple<size_t, size_t> override
    {
        return m_cardinality;
    }

    auto hint() const -> std::string { return m_hint; }
};

} // namespace lyra

#endif