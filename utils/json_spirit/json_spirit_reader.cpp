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

#endif
