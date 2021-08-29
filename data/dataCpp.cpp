#include "Ast.hpp"

#include <iostream>
#include <sstream>
#include <string>

using namespace PB;

bool Ast::generate ( const ParserNode *p_node )
{
  AstNode *last = NULL;
  clear ();
  root = astConvertFromParsing ( p_node, last, 0 );
  return root != NULL;
}

AstNode *Ast::astConvertFromParsing ( const ParserNode *p_node, AstNode *&p_last, int level ) const
{
  AstNode *left       = NULL;
  AstNode *left_last  = NULL;
  AstNode *right      = NULL;
  AstNode *right_last = NULL;

  if ( p_node ) {
    right = astConvertFromParsing ( p_node->right, right_last, level + 1 );
    left  = astConvertFromParsing ( p_node->left, left_last, level + 1 );
    switch ( p_node->type.value ) {
      case ParserNode::NodeType::PT_Rule:
        {
          AstNode *node = allocNode ( p_node );
          if ( left ) {
            node->child  = left;
            left->parent = node;
          }
          if ( right ) {
            if ( !left ) {
              node->child = right;
            }
            else {
              right->next       = node->child->next;
              node->child->next = right;
            }
            right->parent = node;
          }
          return node;
        }
      case ParserNode::NodeType::PT_Sequence:
      case ParserNode::NodeType::PT_Repeat:
        {
          if ( right_last ) {
            p_last = right_last;
          }
          else if ( right ) {
            p_last = right;
          }
          else if ( left_last ) {
            p_last = left_last;
          }
          else if ( left ) {
            p_last = left;
          }
          if ( left ) {
            if ( right ) {
              if ( left_last ) {
                left_last->next = right;
                right->parent     = left_last;
              }
              else {
                left->next  = right;
                right->parent = left;
              }
            }
            return left;
          }
          return right;
        }
      //fall-through
      case ParserNode::NodeType::PT_Literal:
        {
          return allocNode ( p_node );
        }
      default:
        return left;
    }
  }
  return NULL;
}
