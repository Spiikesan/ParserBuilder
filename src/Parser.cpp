#include "Parser.hpp"

#include <string>
#include <iostream>

using namespace PB;

Parser::Parser ()
    : state ( 0 ), root ( NULL )
{
}

Parser::~Parser ()
{
}

bool Parser::openFile ( const std::string &p_filename )
{
  return l.openFile ( p_filename );
}

bool Parser::openString ( const std::string &p_string )
{
  return l.openString ( p_string );
}

void Parser::close ()
{
  l.close ();
}

ParserNode *Parser::allocNode ( ParserNode::NodeType::value_type p_type )
{
  ParserNode *node;
  if ( !unused_nodes.empty () ) {
    node = unused_nodes.back ();
    unused_nodes.pop_back ();
    node->type = p_type;
    // std::cout << "Reuse node [" << node << "] " << node->toString() << std::endl;
  }
  else {
    node = new ParserNode(p_type);
    // std::cout << "Alloc node [" << node << "] " << node->toString() << std::endl;
  }
  return node;
}

void Parser::freeNode ( ParserNode *p_node )
{
  if ( ( p_node != NULL )
      && ( p_node != &ParserNode::DEFAULT_NODE ) ) {
    // std::cout << "Free node begin [" << p_node << "] " << p_node->toString() << ", left = " << p_node->left << ", right = " << p_node->right << std::endl;
    ParserNode *left  = p_node->left;
    ParserNode *right = p_node->right;
    p_node->clear ();
    unused_nodes.push_back ( p_node );
    freeNode_in ( left );
    freeNode_in ( right );
    // std::cout << "Free node end [" << p_node << "] " << p_node->toString() << std::endl;
  }
}

void Parser::freeNode_in ( ParserNode *p_node )
{
  static int limit = 0;
  if ( ( p_node != NULL )
      && ( p_node != &ParserNode::DEFAULT_NODE )
       && ( limit++ < 10000 ) ) {
    // std::cout << std::string(limit, '|') << " Free node in (" << limit << ") [" << p_node << "] " << p_node->toString() << ", left = " << p_node->left << ", right = " << p_node->right << std::endl;
    ParserNode *left  = p_node->left;
    ParserNode *right = p_node->right;
    p_node->clear ();
    unused_nodes.push_back ( p_node );
    freeNode_in ( left );
    freeNode_in ( right );
    limit--;
  }
}

void Parser::begin()
{
  l.savePos();
}

//Parsing OK, continue and clear last pos. return true.
bool Parser::good()
{
  l.clearPos();
  state = true;
  return true;
}

// Parsing KO, reset last pos. Return false.
bool Parser::bad()
{
  l.restorePos();
  state = false;
  return false;
}
