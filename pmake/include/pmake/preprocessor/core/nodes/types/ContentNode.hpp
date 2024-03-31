#pragma once

#include "../INode.hpp"

#include <string>

namespace pmake::preprocessor {

struct ContentNode : INode
{
    NODE_TYPE(INode::Type::CONTENT);
    virtual ~ContentNode() override = default;
    std::string content {};
};

} // pmake::preprocessor
