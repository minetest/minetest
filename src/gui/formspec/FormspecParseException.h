#pragma once

class FormspecParseException : public std::exception {
public:
	const bool serious;
	const std::string msg;

	explicit FormspecParseException(bool serious, const std::string &msg):
		serious(serious), msg(msg)
	{}

	const char *what() const noexcept override { return msg.c_str(); }
};

