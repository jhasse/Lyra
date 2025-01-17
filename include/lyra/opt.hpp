// Copyright 2018-2019 Rene Rivera
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_OPT_HPP
#define LYRA_OPT_HPP

#include "lyra/parser.hpp"
#include <memory>

namespace lyra
{
/* tag::reference[]

[#lyra_opt]
= `lyra::opt`

A parser for one option with multiple possible names. The option value(s) are
communicated through a reference to a variable, a container, or a callback.

Is-a <<lyra_bound_parser>>.

end::reference[] */
class opt : public bound_parser<opt>
{
	protected:
	std::vector<std::string> opt_names;

	public:
	// Flag option ctors..

	explicit opt(bool& ref);

	template <typename LambdaT>
	explicit opt(LambdaT const& ref);

	// Value option ctors..

	template <typename T>
	opt(T& ref, std::string const& hint);

	template <typename LambdaT>
	opt(LambdaT const& ref, std::string const& hint);

	// Option specifications..

	opt& name(const std::string& opt_name);
	opt& operator[](std::string const& opt_name);

	// Internal..

	virtual std::string get_usage_text() const override
	{
		std::string result;
		for (std::size_t o = 0; o < opt_names.size(); ++o)
		{
			if (o > 0) result += "|";
			result += opt_names[o];
		}
		return result;
	}

	virtual help_text get_help_text() const override
	{
		std::ostringstream oss;
		bool first = true;
		for (auto const& opt : opt_names)
		{
			if (first)
				first = false;
			else
				oss << ", ";
			oss << opt;
		}
		if (!m_hint.empty()) oss << " <" << m_hint << ">";
		return { { oss.str(), m_description } };
	}

	bool isMatch(
		std::string const& optToken,
		parser_customization const& customize) const
	{
		auto normalisedToken = normaliseOpt(optToken, customize);
		for (auto const& name : opt_names)
		{
			if (normaliseOpt(name, customize) == normalisedToken) return true;
		}
		return false;
	}

	std::string normaliseOpt(
		std::string const& optName, parser_customization const& customize) const
	{
		auto is_prefix_char_0
			= customize.option_prefix().find(optName[0]) != std::string::npos;
		auto is_prefix_char_1
			= customize.option_prefix().find(optName[1]) != std::string::npos;
		if (is_prefix_char_0)
		{
			if (is_prefix_char_1)
				return "--" + optName.substr(2);
			else
				return "-" + optName.substr(1);
		}
		else
			return optName;
	}

	using parser_base::parse;

	parse_result parse(
		std::string const&, detail::token_iterator const& tokens,
		parser_customization const& customize) const override
	{
		auto validationResult = validate();
		if (!validationResult) return parse_result(validationResult);

		auto remainingTokens = tokens;
		if (remainingTokens
			&& remainingTokens->type == detail::token_type::option)
		{
			auto const& token = *remainingTokens;
			if (isMatch(token.name, customize))
			{
				if (m_ref->isFlag())
				{
					auto flagRef
						= static_cast<detail::BoundFlagRefBase*>(m_ref.get());
					auto result = flagRef->setFlag(true);
					if (!result) return parse_result(result);
					if (result.value() == parser_result_type::short_circuit_all)
						return parse_result::ok(detail::parse_state(
							result.value(), remainingTokens));
				}
				else
				{
					auto valueRef
						= static_cast<detail::BoundValueRefBase*>(m_ref.get());
					++remainingTokens;
					if (!remainingTokens)
						return parse_result::runtimeError(
							{ parser_result_type::no_match, remainingTokens },
							"Expected argument following " + token.name);
					auto const& argToken = *remainingTokens;
					if (argToken.type != detail::token_type::argument)
						return parse_result::runtimeError(
							{ parser_result_type::no_match, remainingTokens },
							"Expected argument following " + token.name);
					if (value_choices)
					{
						auto choice_result
							= value_choices->contains_value(argToken.name);
						if (!choice_result) return parse_result(choice_result);
					}
					auto result = valueRef->setValue(argToken.name);
					if (!result) return parse_result(result);
					if (result.value() == parser_result_type::short_circuit_all)
						return parse_result::ok(detail::parse_state(
							result.value(), remainingTokens));
				}
				return parse_result::ok(detail::parse_state(
					parser_result_type::matched, ++remainingTokens));
			}
		}
		return parse_result::ok(
			detail::parse_state(parser_result_type::no_match, remainingTokens));
	}

	result validate() const override
	{
		if (opt_names.empty())
			return result::logicError("No options supplied to opt");
		for (auto const& name : opt_names)
		{
			if (name.empty())
				return result::logicError("Option name cannot be empty");
			if (name[0] != '-')
				return result::logicError("Option name must begin with '-'");
		}
		return bound_parser::validate();
	}

	std::unique_ptr<parser_base> clone() const override
	{
		return std::unique_ptr<parser_base>(new opt(*this));
	}
};

/* tag::reference[]

[#lyra_opt_ctor]
== Construction

end::reference[] */

/* tag::reference[]

[#lyra_opt_ctor_flags]
=== Flags

[source]
----
lyra::opt::opt(bool& ref);

template <typename LambdaT>
lyra::opt::opt(LambdaT const& ref);

template <typename LambdaT>
lyra::opt::opt(LambdaT const& ref, std::string const& hint)
----

Constructs a flag option with a target `bool` to indicate if the flag is
present. The first form takes a reference to a variable to receive the
`bool`. The second takes a callback that is called with `true` when the
option is present.

end::reference[] */
inline opt::opt(bool& ref)
	: bound_parser(std::make_shared<detail::BoundFlagRef>(ref))
{
}
template <typename LambdaT>
opt::opt(LambdaT const& ref)
	: bound_parser(std::make_shared<detail::BoundFlagLambda<LambdaT>>(ref))
{
}

/* tag::reference[]

[#lyra_opt_ctor_values]
=== Values

[source]
----
template <typename T>
lyra::opt::opt(T& ref, std::string const& hint);

template <typename LambdaT>
lyra::opt::opt(LambdaT const& ref, std::string const& hint)
----

Constructs a value option with a target `ref`. The first form takes a reference
to a variable to receive the value. The second takes a callback that is called
with the value when the option is present.

end::reference[] */
template <typename T>
opt::opt(T& ref, std::string const& hint)
	: bound_parser(ref, hint)
{
}
template <typename LambdaT>
opt::opt(LambdaT const& ref, std::string const& hint)
	: bound_parser(ref, hint)
{
}

/* tag::reference[]

[#lyra_opt_specification]
== Specification

end::reference[] */

/* tag::reference[]

[#lyra_opt_name]
=== `lyra::opt::name`

[source]
----
lyra::opt& lyra::opt::name(const std::string &opt_name)
lyra::opt& lyra::opt::operator[](const std::string &optName)
----

Add a spelling for the option of the form `--<name>` or `-n`.

end::reference[] */
inline opt& opt::name(const std::string& opt_name)
{
	opt_names.push_back(opt_name);
	return *this;
}
inline opt& opt::operator[](const std::string& opt_name)
{
	return this->name(opt_name);
}

} // namespace lyra

#endif
