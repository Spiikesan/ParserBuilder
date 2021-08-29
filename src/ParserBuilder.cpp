#include "ParserBuilder.hpp"

#include <iostream>
#include <string>

using namespace PB;

ParserBuilder::ParserBuilder ()
    : Parser (), debug ( false ), level ( 0 ), lastRule ( NULL )
{
  extensions.push_back ( Rule ( new ParserNode ( ParserNode::NodeType::PT_EXTENSION_1, "letter" ), RuleOption () ) );
  extensions.back ().first->index = extensions.size () - 1;
  extensions.push_back ( Rule ( new ParserNode ( ParserNode::NodeType::PT_EXTENSION_2, "digit" ), RuleOption () ) );
  extensions.back ().first->index = extensions.size () - 1;
  extensions.push_back ( Rule ( new ParserNode ( ParserNode::NodeType::PT_EXTENSION_3, "space" ), RuleOption () ) );
  extensions.back ().first->index = extensions.size () - 1;
  extensions.push_back ( Rule ( new ParserNode ( ParserNode::NodeType::PT_EXTENSION_4, "spaceNoNl" ), RuleOption () ) );
  extensions.back ().first->index = extensions.size () - 1;
  extensions.push_back ( Rule ( new ParserNode ( ParserNode::NodeType::PT_EXTENSION_5, "any_character" ), RuleOption () ) );
  extensions.back ().first->index = extensions.size () - 1;
  extensions.push_back ( Rule ( new ParserNode ( ParserNode::NodeType::PT_EXTENSION_6, "eof" ), RuleOption () ) );
  extensions.back ().first->index = extensions.size () - 1;
  extensions.push_back ( Rule ( new ParserNode ( ParserNode::NodeType::PT_EXTENSION_7, "eol" ), RuleOption () ) );
  extensions.back ().first->index = extensions.size () - 1;
}

ParserBuilder::~ParserBuilder ()
{
  clearRules ();
  for ( size_t i = 0; i < extensions.size (); i++ ) {
    delete extensions[ i ].first;
  }
}

void ParserBuilder::RuleOption::setOptionsString ( const std::string &p_options )
{
  clear ();
  if ( p_options.find ( 't' ) != std::string::npos )
    add ( PR_Option_Trace );
  if ( p_options.find ( 'c' ) != std::string::npos )
    add ( PR_Option_Capture );
  if ( p_options.find ( 's' ) != std::string::npos )
    add ( PR_Option_Include_WS );
  if ( !have ( PR_Option_Include_WS ) && p_options.find ( 'n' ) != std::string::npos )
    add ( PR_Option_Ignore_WS );
  if ( !have ( PR_Option_Include_WS ) && p_options.find ( 'i' ) != std::string::npos )
    add ( PR_Option_Ignore_WS_NO_NL );
  if ( p_options.find ( 'x' ) != std::string::npos )
    add ( PR_Option_Exclude );
  if ( p_options.find ( 'w' ) != std::string::npos )
    add ( PR_Option_Wipe );
  if ( p_options.find ( 'p' ) != std::string::npos )
    add ( PR_Option_Push_Symbol );
}

ParserBuilder::Rule *ParserBuilder::getRule ( const std::string &p_ruleName )
{
  for ( size_t i = 0; i < rules.size (); i++ ) {
    if ( rules[ i ].first->value == p_ruleName ) {
      return &rules[ i ];
    }
  }
  for ( size_t i = 0; i < extensions.size (); i++ ) {
    if ( extensions[ i ].first->value == p_ruleName ) {
      return &extensions[ i ];
    }
  }
  return NULL;
}

ParserNode *ParserBuilder::getNodeFromName ( ParserNode *from, ParserNode::NodeType::value_type p_type, const std::string &p_ruleName )
{
  ParserNode *node = NULL;

  if ( from ) {
    if ( from->type == p_type && from->value == p_ruleName ) {
      node = from;
    }
    else {
      node = getNodeFromName ( from->left, p_type, p_ruleName );
      if ( !node ) {
        node = getNodeFromName ( from->right, p_type, p_ruleName );
      }
    }
  }
  return node;
}
const std::string &ParserBuilder::getDefaultEntryPoint () const
{
  static std::string empty;
  if ( root != NULL ) {
    return root->value;
  }
  else if ( !rules.empty () ) {
    return rules.front ().first->value;
  }
  return empty;
}

const ParserBuilder::Rules &ParserBuilder::getRules () const
{
  return rules;
}

const ParserBuilder::Rules &ParserBuilder::getExtensions () const
{
  return extensions;
}

void ParserBuilder::clearRules ()
{
  for ( size_t i = 0; i < rules.size (); i++ ) {
    delete rules[ i ].first;
  }
  rules.clear ();
}

void ParserBuilder::printTree ( bool compact ) const
{
  std::vector< std::string > tab;
  printTree ( compact, root, false, 0, 0, 0, tab );
  for ( size_t i = 0; i < tab.size (); i++ ) {
    std::cout << tab[ i ] << std::endl;
  }
}

void ParserBuilder::printTree ( const ParserNode *p_node, bool compact ) const
{
  std::vector< std::string > tab;
  printTree ( compact, p_node, false, 0, 0, 0, tab );
  for ( size_t i = 0; i < tab.size (); i++ ) {
    std::cout << tab[ i ] << std::endl;
  }
}

size_t ParserBuilder::printTree ( bool compact, const ParserNode *p_node, bool is_left, size_t p_offset, size_t p_width, size_t p_depth, std::vector< std::string > &p_tab ) const
{
  std::string b;
  size_t width = 0, left = 0, right = 0, size, graph_depth = compact ? p_depth : 2 * p_depth;

  if ( !p_node ) {
    return 0;
  }
  b     = p_node->toString ();
  width = b.size ();
  left  = printTree ( compact, p_node->left, true, p_offset, width, p_depth + 1, p_tab );
  right = printTree ( compact, p_node->right, false, p_offset + left + width / 2, width, p_depth + 1, p_tab );
  if ( p_tab.size () < ( graph_depth + 1 ) ) {
    p_tab.resize ( graph_depth + 1 );
  }
  if ( p_tab[ graph_depth ].size () < p_offset + left + width ) {
    p_tab[ graph_depth ].resize ( p_offset + left + width, ' ' );
  }
  for ( size_t i = 0; i < width; i++ ) {
    p_tab[ graph_depth ][ p_offset + left + i ] = b[ i ];
  }
  if ( p_depth > 0 ) {
    if ( is_left ) { /////// GAUCHE ////////
      size = p_offset + left + width + right + p_width / 2 + 1;
      if ( p_tab[ graph_depth - 1 ].size () < size ) {
        p_tab[ graph_depth - 1 ].resize ( size, ' ' );
      }
      size_t end = (size_t) ( ( ( (float) width ) + 0.5 ) / 2.0 + right + ( ( (float) p_width ) + 0.5 ) / 2.0 );
      for ( size_t i = 1; i < end; i++ ) {
        p_tab[ graph_depth - 1 ][ p_offset + left + width / 2 + i ] = '-';
      }
      p_tab[ graph_depth - 1 ][ p_offset + left + width / 2 ] = compact ? '.' : '+';
      if ( !compact ) {
        p_tab[ graph_depth - 1 ][ p_offset + left + width + right + p_width / 2 ] = '+';
      }
    }
    else { //////// DROITE /////////
      size = p_offset + left + width / 2 + 1;
      if ( p_tab[ graph_depth - 1 ].size () < size ) {
        p_tab[ graph_depth - 1 ].resize ( size, ' ' );
      }
      for ( size_t i = 1; i < width / 2 + left; i++ ) {
        p_tab[ graph_depth - 1 ][ p_offset + i ] = '-';
      }
      p_tab[ graph_depth - 1 ][ p_offset + left + width / 2 ] = compact ? '.' : '+';
      if ( !compact ) {
        p_tab[ graph_depth - 1 ][ p_offset ] = '+';
      }
    }
  }
  return left + width + right;
}

void ParserBuilder::PushSymbol(ParserNode *rule, const std::string &p_symbol)
{
  ParserNode *node = rule->right;
  std::cout << "Rule = " << rule->toString() << ", Node = " << node->toString() << std::endl;
  (void)p_symbol;
  return ;
  while (node) {
    if (node->type == ParserNode::NodeType::PT_Alternative) {

    }
  }
}