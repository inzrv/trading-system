#pragma once

#include <boost/json.hpp>

struct IConfig
{
    virtual ~IConfig() = default;
    virtual void from_json(const boost::json::object& json) = 0;
};
