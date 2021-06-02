#pragma once

#include <exception>
#include <string>
#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/info_tuple.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/throw_exception.hpp>
#include <boost/tuple/tuple.hpp>
#include "CommonData.h"
#include "FixedHash.h"

namespace dev
{

/// Base class for all exceptions.
struct Exception: virtual std::exception, virtual boost::exception
{
	Exception(std::string _message = std::string()): m_message(std::move(_message)) {}
	const char* what() const noexcept override { return m_message.empty() ? std::exception::what() : m_message.c_str(); }

private:
	std::string m_message;
};

}
