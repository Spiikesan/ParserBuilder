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
      case ParserNode::NodeType::PT_Group:
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

AstNode *Ast::allocNode ( const ParserNode *p_node ) const
{
  return new AstNode ( p_node );
}

void Ast::print () const
{
  if ( root ) {
    print ( root, 0 );
  }
}

void Ast::print ( const AstNode *p_node, size_t p_indent = 0 ) const
{
  if ( p_node && p_indent < 32 ) {
    std::string indent;

    indent.reserve ( p_indent * 2 );
    for ( size_t i = 0; i < p_indent; i++ ) {
      indent += "| ";
    }
    if ( p_node->rule->type == ParserNode::NodeType::PT_Literal ) {
      std::cout << indent << "\"" << p_node->rule->value << "\"" << std::endl;
    }
    else {
      std::cout << indent << "<" << p_node->rule->value << ">" << std::endl;
    }
    if ( p_node->child ) {
      print ( p_node->child, p_indent + 1 );
      if ( !p_node->next ) {
        std::cout << indent << "/" << std::endl;
        if ( p_indent > 0 ) {
          indent.erase ( indent.size () - 1 );
          std::cout << indent << "/" << std::endl;
        }
      }
    }
    if ( p_node->next ) {
      //std::cout << indent << "|" << std::endl;
      print ( p_node->next, p_indent );
    }
  }
}

void Ast::rewrite () const
{
  if ( root ) {
    std::string value;
    rewrite ( root, value );
    std::cout << value << std::endl;
  }
}

void Ast::rewrite ( std::string &p_str ) const
{
  if ( root ) {
    rewrite ( root, p_str );
  }
}

void Ast::rewrite ( const AstNode *p_node, std::string &p_str ) const
{
  if ( p_node ) {
    if ( p_node->rule->type == ParserNode::NodeType::PT_Literal ) {
      if ( !p_str.empty () ) {
        p_str += " ";
      }
      p_str += p_node->rule->value;
    }
    rewrite ( p_node->child, p_str );
    rewrite ( p_node->next, p_str );
  }
}

void Ast::rewriteToFile ( const std::string &p_filename, const AstWorkers &p_workers ) const
{
  (void) p_filename;
  (void) p_workers;
}

void Ast::rewriteToFile ( const AstNode *p_node, const std::string &p_filename, const AstWorkers &p_workers ) const
{
  (void) p_node;
  (void) p_filename;
  (void) p_workers;
}

void Ast::printTree () const
{
  std::vector< std::string > tab;
  size_t max_depth = 0;
  printTree ( root, 0, 0, tab, max_depth );
  max_depth++;
  for ( size_t i = 0; i < max_depth * 2; i++ ) {
    std::cout << tab[ i ] << std::endl;
  }
}

int Ast::printTree ( const AstNode *p_node, size_t p_offset, size_t p_depth, std::vector< std::string > &p_tab, size_t &p_max_depth ) const
{
  std::string b;
  size_t width = 0, len = 0, child = 0, next = 0;
  if ( p_node ) {
    if ( p_max_depth < p_depth ) {
      p_max_depth = p_depth;
    }
    if ( p_node->rule->type == ParserNode::NodeType::PT_Literal ) {
      b = "\"" + p_node->rule->value + "\"";
    }
    else {
      b = p_node->rule->value;
    }
    width = b.size ();
    len   = p_offset + width + 2;
    child = printTree ( p_node->child, p_offset, p_depth + 1, p_tab, p_max_depth );
    if ( child > len ) {
      len = child;
    }
    next = printTree ( p_node->next, len, p_depth, p_tab, p_max_depth );

    if ( p_tab.size () < 2 * ( p_depth + 1 ) ) {
      p_tab.resize ( 2 * ( p_depth + 1 ) );
    }
    if ( p_tab[ 2 * p_depth ].size () < p_offset + width ) {
      p_tab[ 2 * p_depth ].resize ( p_offset + width, ' ' );
      p_tab[ 2 * p_depth + 1 ].resize ( p_offset + 2, ' ' );
    }
    for ( size_t i = 0; i < width; i++ ) {
      p_tab[ 2 * p_depth ][ p_offset + i ] = b[ i ];
    }
    if ( p_node->child ) {
      p_tab[ 2 * p_depth + 1 ][ p_offset + 1 ] = '|';
    }
    if ( p_node->next ) {
      if ( p_tab[ 2 * p_depth ].size () < ( len - ( width + p_offset ) ) + 1 ) {
        p_tab[ 2 * p_depth ].resize ( ( len - ( width + p_offset ) ) + 1, ' ' );
        p_tab[ 2 * p_depth + 1 ].resize ( ( len - ( width + p_offset ) ) + 1, ' ' );
      }
      for ( size_t i = 0; i < ( len - ( width + p_offset ) ); i++ ) {
        p_tab[ 2 * p_depth ][ p_offset + width + i ] = '-';
      }
      p_tab[ 2 * p_depth ][ len - 1 ] = '>';
    }
    if ( next > len ) {
      return next;
    }
  }
  return len;
}
