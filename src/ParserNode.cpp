#include "ParserNode.hpp"

using namespace PB;

ParserNode ParserNode::DEFAULT_NODE;

static const std::string nodeTypes_strings[] = {
  "PT_Rule",
  "PT_Alternative",
  "PT_Sequence",
  "PT_Group",
  "PT_Option",
  "PT_Repeat",
  "PT_Exclude",
  "PT_Range",
  "PT_Identifier",
  "PT_Literal",
  "PT_Nothing",
  "PT_COUNT",
  "PT_EXTENSION_1",
  "PT_EXTENSION_2",
  "PT_EXTENSION_3",
  "PT_EXTENSION_4",
  "PT_EXTENSION_5",
  "PT_EXTENSION_6",
  "PT_EXTENSION_7"
};

const std::string &ParserNode::NodeType::toString () const
{
  if ( value >= PT_Rule && value < PT_TOTAL ) {
    return nodeTypes_strings[ value ];
  }
  return nodeTypes_strings[ PT_Nothing ];
}

std::string ParserNode::toString () const
{
  if ( value.empty () )
    return type.toString ();
  return type.toString () + "(\"" + value + "\")";
}
