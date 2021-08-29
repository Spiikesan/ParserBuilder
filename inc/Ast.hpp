#ifndef PB_AST_H_
#define PB_AST_H_

#include "AstNode.hpp"
#include "AstWorkers.hpp"
#include "ParserNode.hpp"

#include <vector>

namespace PB {
  class Ast {
  public:
    Ast ()
        : root ( NULL ) {}
    ~Ast () { clear (); }

    AstNode *getRoot () const { return root; }
    bool generate ( const ParserNode *p_node );
    void clear ()
    {
      delete root;
      root = NULL;
    }

    void print () const;
    void printTree () const;
    void rewrite () const;
    void rewrite ( std::string &p_str ) const;
    void rewriteToFile ( const std::string &p_filename, const AstWorkers &p_workers ) const;

  private:
    AstNode *astConvertFromParsing ( const ParserNode *p_node, AstNode *&p_last, int level ) const;
    AstNode *allocNode ( const ParserNode *p_node ) const;
    void print ( const AstNode *p_node, size_t p_indent ) const;
    int printTree ( const AstNode *p_node, size_t p_offset, size_t p_depth, std::vector< std::string > &p_tab, size_t &p_max_depth ) const;
    void rewrite ( const AstNode *p_node, std::string &p_str ) const;
    void rewriteToFile ( const AstNode *p_node, const std::string &p_filename, const AstWorkers &p_workers ) const;

    AstNode *root;
  };

} // namespace PB

#endif /* PB_AST_H_ */
