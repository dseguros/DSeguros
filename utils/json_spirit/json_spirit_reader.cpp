#include "json_spirit_reader.h"
#include "json_spirit_reader_template.h"

using namespace json_spirit;

bool json_spirit::read( const std::string& s, Value& value )
{
    return read_string( s, value );
}

void json_spirit::read_or_throw( const std::string& s, Value& value )
{
    read_string_or_throw( s, value );
}

bool json_spirit::read( std::istream& is, Value& value )
{
    return read_stream( is, value );
}

void json_spirit::read_or_throw( std::istream& is, Value& value )
{
    read_stream_or_throw( is, value );
}

bool json_spirit::read( std::string::const_iterator& begin, std::string::const_iterator end, Value& value )
{
    return read_range( begin, end, value );
}

void json_spirit::read_or_throw( std::string::const_iterator& begin, std::string::const_iterator end, Value& value )
{
    begin = read_range_or_throw( begin, end, value );
}

#endif