// Copyright 2018-2019 Rene Rivera
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_ARG_HPP
#define LYRA_ARG_HPP

#include "lyra/parser.hpp"

namespace lyra
{
/* tag::reference[]

[#lyra_arg]
= `lyra::arg`

A parser for regular arguments, i.e. not `--` or `-` prefixed. This is simply
a way to get values of arguments directly specified in the cli.

Is-a <<lyra_bound_parser>>.

*/ // end::reference[]
class arg : public bound_parser<arg>
{
	public:
	using bound_parser::bound_parser;

	virtual std::string get_usage_text() const override
	{
		std::ostringstream oss;
		if (!m_hint.empty())
		{
			auto c = cardinality();
			if (c.is_required())
			{
				for (size_t i = 0; i < c.minimum; ++i)
					oss << (i > 0 ? " " : "") << "<" << m_hint << ">";
				if (c.is_unbounded())
					oss << (c.is_required() ? " " : "") << "[<" << m_hint << ">" << "...]";
			}
			else if (c.is_unbounded())
			{
				oss << "[<" << m_hint << ">" << "...]";
			}
		}
		return oss.str();
	}

	virtual help_text get_help_text() const override
	{
		return { { get_usage_text(), m_description } };
	}

	using parser_base::parse;

	auto parse(
		std::string const&, detail::token_iterator const& tokens,
		parser_customization const&) const -> parse_result override
	{
		auto validationResult = validate();
		if (!validationResult) return parse_result(validationResult);

		auto remainingTokens = tokens;
		auto const& token = *remainingTokens;
		if (token.type != detail::token_type::argument)
			return parse_result::ok(detail::parse_state(
				parser_result_type::no_match, remainingTokens));

		auto valueRef = static_cast<detail::BoundValueRefBase*>(m_ref.get());

		if (value_choices)
		{
			auto choice_result = value_choices->contains_value(token.name);
			if (!choice_result) return parse_result(choice_result);
		}

		auto result = valueRef->setValue(remainingTokens->name);
		if (!result)
			return parse_result(result);
		else
			return parse_result::ok(detail::parse_state(
				parser_result_type::matched, ++remainingTokens));
	}

	virtual std::unique_ptr<parser_base> clone() const override
	{
		return std::unique_ptr<parser_base>(new arg(*this));
	}
};
} // namespace lyra

#endif
