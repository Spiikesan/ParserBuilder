#include "Ast.hpp"
#include "ParserBuilder.hpp"

#include <iostream>
#include <set>
#include <string>

using namespace PB;

const std::string grammar (
    "rule @n ::= identifier [option] \"::=\" expression \";\" ;\n"
    "expression ::= term { \"|\" term } ;\n"
    "term ::= factor { factor } ;\n"
    "factor ::= exclusion | identifier | range_factor | quoted_symbol | group_factor | optional_factor | repeated_factor ;\n"
    "group_factor ::= \"(\" expression \")\" ;\n"
    "optional_factor ::= \"[\" expression \"]\";\n"
    "repeated_factor ::= \"{\" expression \"}\";\n"
    "range_factor ::= quoted_symbol \"..\" quoted_symbol ;\n"
    "exclusion ::= \"-\" factor ;\n"
    "identifier ::= \"<\" raw_id \">\" | raw_id ;\n"
    "option ::= \"@\" { \"c\" | \"s\" | \"n\" | \"x\" | \"w\" } ;\n"
    "raw_id ::= letter { letter | digit | \"_\" | \"-\" };\n"
    "quoted_symbol ::= \"\"\" \"\"\" \"\"\" | \"\"\" any_excluding_quote { any_excluding_quote } \"\"\" ;\n"
    "any_excluding_quote ::= - \"\"\" any_character ;\n" );

bool verify_loop ( std::set< const ParserNode * > &p_nodeSet, const ParserNode *p_node )
{
  std::pair< std::set< const PB::ParserNode * >::iterator, bool > ret;
  ret = p_nodeSet.insert ( p_node );
  if ( ret.second ) {
    if ( p_node->left ) {
      if ( !verify_loop ( p_nodeSet, p_node->left ) ) {
        std::cerr << "bad : [" << p_node << "] " << p_node->toString () << std::endl;
        return false;
      }
    }
    if ( p_node->right ) {
      if ( !verify_loop ( p_nodeSet, p_node->right ) ) {
        std::cerr << "bad : [" << p_node << "] " << p_node->toString () << std::endl;
        return false;
      }
    }
    return true;
  }
  std::cerr << "bad : [" << p_node << "] " << p_node->toString () << std::endl;
  return false;
}

bool verify_loop ( const ParserNode *p_node )
{
  std::set< const ParserNode * > nodeSet;
  return verify_loop ( nodeSet, p_node );
}

int main ( int ac, char **av )
{
  ParserBuilder pb;
  Ast ast;
  (void) ac;
  (void) av;
  //system ( "pause" );
  if ( ac >= 2 ) {
    pb.openFile ( av[ 1 ] );
  }
  else {
    pb.openString ( grammar );
  }
  if ( pb.generate () ) {
    const ParserBuilder::Rules &rules = pb.getRules ();
    std::cout << "Generation OK! " << rules.size () << " rule(s) built " << std::endl;
    for ( size_t i = 0; i < rules.size (); i++ ) {
      // std::cout << " -> [" << i << "]: " << rules[ i ].first->value << std::endl;
      if ( !verify_loop ( rules[ i ].first ) ) {
        std::cerr << "Error on rule " << rules[ i ].first->value << ": Loop detected !" << std::endl;
      }
      else {
        // std::cout << "Rule : " << rules[ i ].first->value << std::endl;
        // pb.printTree ( rules[ i ].first );
      }
    }
    pb.close ();
    if ( ac == 3 ) {
      pb.openFile ( av[ 2 ] );
    }
    else {
      pb.openString ( grammar );
    }
    while ( pb.parse () ) {
      std::cout << "Parsing OK!" << std::endl;
      if ( !verify_loop ( pb.getRootNode () ) ) {
        std::cerr << "Error on parsing : Loop detected !" << std::endl;
      }
      else {
        //pb.printTree ( true );
        if ( ast.generate ( pb.getRootNode () ) ) {
          //std::cout << "AST generation OK!" << std::endl;
          ast.printTree ();
          //ast.rewrite ();
        }
      }
    }
    std::cout << "class_name" << std::endl;
    pb.printTree(pb.getRule("class_name")->first->right);
    std::cout << "original_namespace_name" << std::endl;
    pb.printTree(pb.getRule("original_namespace_name")->first->right);
    std::cout << "template_name" << std::endl;
    pb.printTree(pb.getRule("template_name")->first->right);
  }
  else {
    std::cout << "Generation error" << std::endl;
  }
  //system ( "pause" );
}
