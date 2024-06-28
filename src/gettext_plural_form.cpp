/*
Minetest
Copyright (C) 2024 y5nw <y5nw@protonmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "gettext_plural_form.h"
#include "util/string.h"

// TODO: merge this with UnaryOperation using stsd::identity (C++20)?
class Identity: public GettextPluralForm {
	NumT operator()(const NumT n) const override {
		return n;
	}
};

class ConstValue: public GettextPluralForm {
	public:
		ConstValue(NumT val): value(val) {};
		NumT operator()(const NumT n) const override{
			return value;
		}
	private:
		NumT value;
};

template<template<typename> typename F>
class UnaryOperation: public GettextPluralForm {
	public:
		UnaryOperation(const Ptr &op): op(op) {}
		NumT operator()(const NumT n) const override {
			if (op)
				return func((*op)(n));
			return 0;
		}
	private:
		Ptr op;
		static constexpr F<NumT> func = {};
};

template<template<typename> typename F>
class BinaryOperation: public GettextPluralForm {
	public:
		BinaryOperation(const Ptr &lhs, const Ptr &rhs): lhs(lhs), rhs(rhs) {}
		NumT operator()(const NumT n) const override {
			if (lhs && rhs)
				return func((*lhs)(n), (*rhs)(n));
			return 0;
		}
	private:
		Ptr lhs, rhs;
		static constexpr F<NumT> func = {};
};

class TernaryOperation: public GettextPluralForm {
	public:
		TernaryOperation(const Ptr &cond, const Ptr &val, const Ptr &alt): cond(cond), val(val), alt(alt) {}
		NumT operator()(const NumT n) const override {
			if (cond && val && alt)
				return (*cond)(n) ? (*val)(n) : (*alt)(n);
			return 0;
		}
	private:
		Ptr cond, val, alt;
};

typedef std::pair<GettextPluralForm::Ptr, std::wstring_view> ParserResult;
typedef ParserResult (*Parser)(const std::wstring_view &);

static ParserResult parse_expr(const std::wstring_view &str);

template<Parser Parser, template<typename> typename Operator>
static ParserResult reduce_ltr(const ParserResult &res, const wchar_t* pattern)
{
	if (!str_starts_with(res.second, pattern))
		return ParserResult(nullptr, res.second);
	auto next = Parser(res.second.substr(std::char_traits<wchar_t>::length(pattern)));
	if (!next.first)
		return next;
	next.first = GettextPluralForm::Ptr(new BinaryOperation<Operator>(res.first, next.first));
	next.second = trim(next.second);
	return next;
}

template<Parser Parser>
static ParserResult reduce_ltr(const ParserResult &res, const wchar_t**)
{
	return ParserResult(nullptr, res.second);
}

template<Parser Parser, template<typename> typename Operator, template<typename> typename... Operators>
static ParserResult reduce_ltr(const ParserResult &res, const wchar_t** patterns) {
	auto next = reduce_ltr<Parser, Operator>(res, patterns[0]);
	if (next.first || next.second != res.second)
		return next;
	return reduce_ltr<Parser, Operators...>(res, patterns+1);
}

template<Parser Parser, template<typename> typename Operator, template<typename> typename... Operators>
static ParserResult parse_ltr(const std::wstring_view &str, const wchar_t** patterns) {
	auto pres = Parser(str);
	if (!pres.first)
		return pres;
	pres.second = trim(pres.second);
	while (!pres.second.empty())
	{
		auto next = reduce_ltr<Parser, Operator, Operators...>(pres, patterns);
		if (!next.first)
			return pres;
		next.second = trim(next.second);
		pres = next;
	}
	return pres;
}

static ParserResult parse_atomic(const std::wstring_view &str)
{
	if (str.empty())
		return ParserResult(nullptr, str);
	if (str[0] == 'n')
		return ParserResult(new Identity(), trim(str.substr(1)));

	wchar_t* endp;
	auto val = wcstoul(str.data(), &endp, 10);
	return ParserResult(new ConstValue(val), trim(str.substr(endp-str.data())));
}

static ParserResult parse_parenthesized(const std::wstring_view &str)
{
	if (str.empty())
		return ParserResult(nullptr, str);
	if (str[0] != '(')
		return parse_atomic(str);
	auto result = parse_expr(str.substr(1));
	if (result.first) {
		if (result.second.empty() || result.second[0] != ')')
			result.first = nullptr;
		else
			result.second = trim(result.second.substr(1));
	}
	return result;
}

static ParserResult parse_negation(const std::wstring_view &str)
{
	if (str.empty())
		return ParserResult(nullptr, str);
	if (str[0] != '!')
		return parse_parenthesized(str);
	auto result = parse_negation(trim(str.substr(1)));
	if (result.first)
		result.first = GettextPluralForm::Ptr(new UnaryOperation<std::logical_not>(result.first));
	return result;
}

static ParserResult parse_multiplicative(const std::wstring_view &str)
{
	static const wchar_t *patterns[] = { L"*", L"/", L"%" };
	return parse_ltr<parse_negation, std::multiplies, std::divides, std::modulus>(str, patterns);
}

static ParserResult parse_additive(const std::wstring_view &str)
{
	static const wchar_t *patterns[] = { L"+", L"-" };
	return parse_ltr<parse_multiplicative, std::plus, std::minus>(str, patterns);
}

static ParserResult parse_comparison(const std::wstring_view &str)
{
	static const wchar_t *patterns[] = { L"<=", L">=", L"<", L">" };
	return parse_ltr<parse_additive, std::less_equal, std::greater_equal, std::less, std::greater>(str, patterns);
}

static ParserResult parse_equality(const std::wstring_view &str)
{
	static const wchar_t *patterns[] = { L"==", L"!=" };
	return parse_ltr<parse_comparison, std::equal_to, std::not_equal_to>(str, patterns);
}

static ParserResult parse_conjunction(const std::wstring_view &str)
{
	static const wchar_t *and_pattern[] = { L"&&" };
	return parse_ltr<parse_equality, std::logical_and>(str, and_pattern);
}

static ParserResult parse_disjunction(const std::wstring_view &str)
{
	static const wchar_t *or_pattern[] = { L"||" };
	return parse_ltr<parse_conjunction, std::logical_or>(str, or_pattern);
}

static ParserResult parse_ternary(const std::wstring_view &str)
{
	auto pres = parse_disjunction(str);
	if (pres.second.empty() || pres.second[0] != '?') // no ? :
		return pres;
	auto cond = pres.first;
	pres = parse_ternary(trim(pres.second.substr(1)));
	if (pres.second.empty() || pres.second[0] != ':')
		return ParserResult(nullptr, pres.second);
	auto val = pres.first;
	pres = parse_ternary(trim(pres.second.substr(1)));
	return ParserResult(new TernaryOperation(cond, val, pres.first), pres.second);
}

static ParserResult parse_expr(const std::wstring_view &str)
{
	return parse_ternary(trim(str));
}

GettextPluralForm::Ptr GettextPluralForm::parse(const std::wstring_view &str) {
	auto result = parse_expr(str);
	if (!result.second.empty())
		return nullptr;
	return result.first;
}

GettextPluralForm::Ptr GettextPluralForm::parseHeaderLine(const std::wstring_view &str) {
	if (!str_starts_with(str, L"Plural-Forms: nplurals=") || !str_ends_with(str, L";"))
		return nullptr;
	auto pos = str.find(L"plural=");
	if (pos == str.npos)
		return nullptr;
	return parse(str.substr(pos+7, str.size()-pos-8));
}
