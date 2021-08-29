#ifndef PB_AST_NODE_H_
#define PB_AST_NODE_H_

#include "ParserNode.hpp"

namespace PB {
  struct AstNode {
    AstNode ()
        : rule ( NULL ), parent ( NULL ), child ( NULL ), next ( NULL ) {}
    AstNode ( const ParserNode *p_node )
        : rule ( p_node ), parent ( NULL ), child ( NULL ), next ( NULL ) {}
    ~AstNode () {
      parent = NULL;
      delete child;
      child = NULL;
      delete next;
      next = NULL;
    }

    const ParserNode *rule;
    AstNode *parent;
    AstNode *child;
    AstNode *next;
  };

} // namespace PB

#endif /* PB_AST_NODE_H_ */
