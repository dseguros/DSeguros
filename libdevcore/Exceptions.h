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

#define DEV_SIMPLE_EXCEPTION(X) struct X: virtual Exception { const char* what() const noexcept override { return #X; } }

/// Base class for all RLP exceptions.
struct RLPException: virtual Exception { RLPException(std::string _message = std::string()): Exception(_message) {} };
#define DEV_SIMPLE_EXCEPTION_RLP(X) struct X: virtual RLPException { const char* what() const noexcept override { return #X; } }

DEV_SIMPLE_EXCEPTION_RLP(BadCast);
DEV_SIMPLE_EXCEPTION_RLP(BadRLP);
DEV_SIMPLE_EXCEPTION_RLP(OversizeRLP);
DEV_SIMPLE_EXCEPTION_RLP(UndersizeRLP);

}
