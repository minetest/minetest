#pragma once
#include <type_traits>
#include "irrlichttypes.h"
#include "IReferenceCounted.h"

enum class Grab
{
	already_owned = 0,
	do_grab = 1,
};

template <class ReferenceCounted, class = typename std::enable_if <
	std::is_base_of <IReferenceCounted, ReferenceCounted>::value>::type>
class irr_ptr
{
	ReferenceCounted *value = nullptr;
public:
	irr_ptr() = default;

	irr_ptr(std::nullptr_t) noexcept
	{
	}

	template <typename B, class = typename std::enable_if <
		std::is_convertible <B *, ReferenceCounted *>::value>::type>
	irr_ptr(const irr_ptr<B> &b) noexcept
	{
		reset(b.get());
	}

	template <typename B, class = typename std::enable_if <
		std::is_convertible <B *, ReferenceCounted *>::value>::type>
	irr_ptr(irr_ptr<B> &&b) noexcept
	{
		reset(b.release(), Grab::already_owned);
	}

	irr_ptr(ReferenceCounted *object, Grab grab) noexcept
	{
		reset(object, grab);
	}

	~irr_ptr()
	{
		reset();
	}

	template <typename B, class = typename std::enable_if <
		std::is_convertible <B *, ReferenceCounted *>::value>::type>
	irr_ptr &operator= (const irr_ptr<B> &b) noexcept
	{
		reset(b.get());
		return *this;
	}

	template <typename B, class = typename std::enable_if <
		std::is_convertible <B *, ReferenceCounted *>::value>::type>
	irr_ptr &operator= (irr_ptr<B> &&b) noexcept
	{
		reset(b.release(), Grab::already_owned);
		return *this;
	}

	ReferenceCounted &operator*() const noexcept { return *value; }
	ReferenceCounted *operator->() const noexcept { return value; }
	explicit operator ReferenceCounted*() const noexcept { return value; }
	explicit operator bool() const noexcept { return !!value; }

	ReferenceCounted *get() const noexcept
	{
		return value;
	}

	ReferenceCounted *release() noexcept
	{
		ReferenceCounted *object = value;
		value = nullptr;
		return object;
	}

	void reset() noexcept
	{
		if (value)
			value->drop();
		value = nullptr;
	}

	void reset(ReferenceCounted *object, Grab grab) noexcept
	{
		reset();
		value = object;
		if (value && grab == Grab::do_grab)
			value->grab();
	}
};
