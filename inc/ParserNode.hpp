#ifndef PB_PARSER_NODE_HPP_
#define PB_PARSER_NODE_HPP_

#include <string>

namespace PB {
  struct ParserNode {
    struct NodeType {
      enum value_type
      {
        PT_Rule = 0,
        PT_Alternative,
        PT_Sequence,
        PT_Group,
        PT_Option,
        PT_Repeat,
        PT_Exclude,
        PT_Range,
        PT_Identifier,
        PT_Literal,
        PT_Nothing,
        PT_COUNT,
        PT_EXTENSION_1, //letter
        PT_EXTENSION_2, //digit
        PT_EXTENSION_3, //space
        PT_EXTENSION_4, //spaceNoNl (no newline as space)
        PT_EXTENSION_5, //any_character
        PT_EXTENSION_6,  //eof
        PT_EXTENSION_7,  //eol
        PT_TOTAL
      };
      NodeType ()
          : value ( PT_Nothing ) {}
      NodeType ( value_type p_value )
          : value ( p_value ) {}
      value_type value;
      NodeType &operator= ( value_type p_value )
      {
        value = p_value;
        return *this;
      }
      operator value_type () const { return value; }
      operator int () const { return static_cast< int > ( value ); }
      const std::string &toString () const;
    };

    ParserNode ()
        : left ( NULL ), right ( NULL ), index ( -1 ), access_pos ( (size_t) -1 ) {}
    ParserNode ( NodeType::value_type p_type )
        : left ( NULL ), right ( NULL ), type ( p_type ), index ( -1 ), access_pos ( (size_t) -1 ) {}
    ParserNode ( NodeType::value_type p_type, const std::string &p_nodeValue )
        : left ( NULL ), right ( NULL ), type ( p_type ), index ( -1 ), access_pos ( (size_t) -1 ), value ( p_nodeValue ) {}
    ~ParserNode ()
    {
      delete left;
      left = NULL;
      delete right;
      right = NULL;
    }

    ParserNode *left;
    ParserNode *right;
    NodeType type;
    int index;
    size_t access_pos;
    std::string value;

    void clear ()
    {
      left       = NULL;
      right      = NULL;
      type       = NodeType::PT_Nothing;
      index      = -1;
      access_pos = (size_t) -1;
      if ( !value.empty () )
        value.clear ();
    }

    static ParserNode DEFAULT_NODE;

    std::string toString () const;
  };

} // namespace PB

#endif /* PB_PARSER_NODE_HPP_ */
