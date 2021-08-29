#ifndef PB_PARSER_H_
#define PB_PARSER_H_

#include "Lexer.hpp"
#include "ParserNode.hpp"

#include <string>
#include <vector>

namespace PB {
  class Parser {
  public:
    Parser ();
    virtual ~Parser ();

    bool openFile ( const std::string &p_filename );
    bool openString ( const std::string &p_string );
    void close ();

    virtual bool parse () = 0;

    void setIgnoreWs(bool p_ignoreWs) { l.ignore_ws = p_ignoreWs; }

    ParserNode *getRootNode() const { return root; }

  protected:

    //Begin parsing by preparing the lexer.
    void begin();

    //Parsing OK, continue and clear last pos. return true.
    bool good();

    // Parsing KO, reset last pos. Return false.
    bool bad();

    ParserNode *allocNode ( ParserNode::NodeType::value_type p_type );
    void freeNode ( ParserNode *p_node );
    void freeNode_in ( ParserNode *p_node );

    bool state;
    ParserNode *root;
    Lexer l;
    std::vector< ParserNode * > unused_nodes;
  };

} // namespace PB

#endif /* PB_PARSER_H_ */
