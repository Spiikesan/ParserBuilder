#include "ParserBuilder.hpp"

#include <iostream>
#include <string>
using namespace PB;

// Parsed syntax :
// syntax @n ::= { lexer | rule } eof ;
// lexer ::= "@" "lex" "." ( lex_comment ) ";" ;
// lex_comment ::= "comment" "." ( "line" quoted_symbol | "block" quoted_symbol quoted_symbol ) ;
// rule ::= { comment } identifier [option] "::=" expression ";" ;
// comment @wx ::= "#" { any_excluding_endline } eol ;
// expression ::= term { "|" term } ;
// term ::= factor { factor } ;
// factor ::= exclusion | identifier | range_factor | quoted_symbol | group_factor | optional_factor | repeated_factor ;
// group_factor ::= "(" expression ")" ;
// optional_factor ::= "[" expression "]";
// repeated_factor ::= "{" expression "}";
// range_factor ::= quoted_symbol ".." quoted_symbol ;
// exclusion ::= "-" factor ;
// identifier ::= "<" raw_id ">" | raw_id ;
// option ::= "@" { "c" | "s" | "n" | "x" | "w" } ;
// raw_id ::= letter { letter | digit | "_" | "-" };
// quoted_symbol ::= """ """ """ | """ any_excluding_quote { any_excluding_quote } """ ;
// any_excluding_quote ::= - """ any_character ;
// any_excluding_endline ::= - eol any_character ;
//
// builtin rules :
//  letter => isalpha()
//  digit => isdigit()
//  space => isspace()
//  any_character => Lexer::next()
//  eof => Lexer::next() == 0;
//  eol => Lexer::next() in [ '\n', '\r' ];

//#define DEBUG
#ifdef DEBUG
static int level__x                = 0;
static const unsigned int level__c = ' ';
#define begin()                                                                                \
  std::cout << std::string ( level__x++, level__c ) << "begin: " << __FUNCTION__ << std::endl; \
  begin ()
#define good() std::cout << std::string ( --level__x, level__c ) << "good: " << __FUNCTION__ << std::endl, good ()
#define bad() std::cout << std::string ( --level__x, level__c ) << "bad: " << __FUNCTION__ << std::endl, bad ()
#undef DEBUG
#endif

bool ParserBuilder::generate ()
{
  l.ignore_ws    = true;
  l.ignore_nl    = true;
  l.generateMode = true;
  state          = true;
  clearRules ();
  if ( !g_syntax () ) {
    l.printError ();
    return false;
  }
  else {
    for ( size_t i = 0; i < rules.size (); i++ ) {
      if ( !applyRulesIndexes ( rules[ i ].first ) ) {
        return false;
      }
    }
  }
  return state;
}

// { rule }
bool ParserBuilder::g_syntax ()
{
  while ( l.haveNext () && ( g_lexer () || g_rule () ) ) {
    for ( size_t i = 0; i < rules.size () - 1; i++ ) {
      if ( root->value == rules[ i ].first->value ) {
        std::cerr << "ERROR: The rule '" << root->value << "' is duplicated from index " << i << " to index " << rules.size () << std::endl;
        return false;
      }
    }
  }
  return state;
}

bool ParserBuilder::g_lexer ()
{
  begin ();

  if ( g_terminal ( "@" ) && g_terminal ( "lex" ) && g_terminal ( "." )
       && ( g_lex_comment () ) && g_terminal ( ";" ) ) {
    return good ();
  }

  return bad ();
}

bool ParserBuilder::g_lex_comment ()
{
  begin ();

  if ( g_terminal ( "comment" ) && g_terminal ( "." ) ) {
    if ( g_terminal ( "line" ) && g_quotedSymbol () ) {
      l.comment_line = root->value;
      freeNode ( root );
      root = NULL;
      // std::cout << "lex comment line = '" << l.comment_line << "'" << std::endl;
      return good ();
    }
    else if ( g_terminal ( "block" ) && g_quotedSymbol () ) {
      l.comment_block_begin = root->value;
      freeNode ( root );
      root = NULL;
      if ( !l.comment_block_begin.empty () ) {
        if ( g_quotedSymbol () ) {
          l.comment_block_end = root->value;
          freeNode ( root );
          root = NULL;
          if ( !l.comment_block_end.empty () ) {
            // std::cout << "lex comment block = '" << l.comment_block_begin << "' ... '" << l.comment_block_end << "'" << std::endl;
            return good ();
          }
        }
      }
      l.comment_block_begin.clear ();
      l.comment_block_end.clear ();
    }
  }
  return bad ();
}

// comment | identifier "::=" expression ";"
bool ParserBuilder::g_rule ()
{
  ParserNode *node = NULL;
  begin ();
  g_comments ();
  if ( g_identifier () ) {
    node = allocNode ( ParserNode::NodeType::PT_Rule );
    rules.push_back ( std::make_pair ( node, RuleOption () ) );
    g_option ();
    node->left = root;
    if ( g_terminal ( "::=" ) && g_expression () && g_terminal ( ";" ) ) {
      node->right = root;
      node->index = rules.size () - 1;
      node->value = node->left->value;
      root        = node;
      return good ();
    }
    rules.pop_back ();
    freeNode ( node );
  }
  return bad ();
}

bool ParserBuilder::g_comments ()
{
  while ( g_terminal ( "#" ) ) {
    int c = l.next ();
    while ( c != '\0' && c != '\n' && c != '\r' ) {
      c = l.next ();
    }
  }
  return true;
}

// term { "|" term }
bool ParserBuilder::g_expression ()
{
  begin ();
  if ( g_term () ) {
    while ( g_terminal ( "|" ) ) {
      ParserNode *node = allocNode ( ParserNode::NodeType::PT_Alternative );
      node->left       = root;
      if ( !g_term () ) {
        freeNode ( node );
        return bad ();
      }
      node->right = root;
      root        = node;
    }
    return good ();
  }
  return bad ();
}

// factor { factor }
bool ParserBuilder::g_term ()
{
  begin ();
  ParserNode *prev = NULL;
  if ( g_factor () ) {
    prev = root;
    while ( g_factor () ) {
      ParserNode *node = allocNode ( ParserNode::NodeType::PT_Sequence );
      node->left       = prev;
      node->right      = root;
      prev             = node;
    }
    root = prev;
    return good ();
  }
  freeNode ( prev );
  return bad ();
}

// exclusion | identifier | range_factor | quoted_symbol | group_factor | optional_factor | repeated_factor
bool ParserBuilder::g_factor ()
{
  begin ();
  if ( g_exclusion ()
       || g_identifier ()
       || g_rangeFactor ()
       || g_quotedSymbol ()
       || g_groupFactor ()
       || g_optionalFactor ()
       || g_repeatedFactor () ) {
    return good ();
  }
  return bad ();
}

// "(" expression ")"
bool ParserBuilder::g_groupFactor ()
{
  ParserNode *node = allocNode ( ParserNode::NodeType::PT_Group );
  begin ();
  if ( g_terminal ( "(" ) && g_expression () && g_terminal ( ")" ) ) {
    node->left = root;
    root       = node;
    return good ();
  }
  freeNode ( node );
  return bad ();
}

// "[" expression "]"
bool ParserBuilder::g_optionalFactor ()
{
  ParserNode *node = allocNode ( ParserNode::NodeType::PT_Option );
  begin ();
  if ( g_terminal ( "[" ) && g_expression () && g_terminal ( "]" ) ) {
    node->left = root;
    root       = node;
    return good ();
  }
  freeNode ( node );
  return bad ();
}

// "{" expression "}"
bool ParserBuilder::g_repeatedFactor ()
{
  ParserNode *node = allocNode ( ParserNode::NodeType::PT_Repeat );
  begin ();
  if ( g_terminal ( "{" ) && g_expression () && g_terminal ( "}" ) ) {
    node->left = root;
    root       = node;
    return good ();
  }
  freeNode ( node );
  return bad ();
}

// quoted_symbol ".." quoted_symbol
bool ParserBuilder::g_rangeFactor ()
{
  ParserNode *node = allocNode ( ParserNode::NodeType::PT_Range );
  begin ();
  if ( g_quotedSymbol () ) {
    node->left = root;
    if ( g_terminal ( ".." ) && g_quotedSymbol () ) {
      node->right = root;
      root        = node;
      return good ();
    }
  }
  freeNode ( node );
  return bad ();
}

// "-" factor
bool ParserBuilder::g_exclusion ()
{
  ParserNode *node = allocNode ( ParserNode::NodeType::PT_Exclude );
  begin ();
  if ( g_terminal ( "-" ) && g_factor () ) {
    node->left = root;
    root       = node;
    return good ();
  }
  freeNode ( node );
  return bad ();
}

// "<" raw_id ">" | raw_id
bool ParserBuilder::g_identifier ()
{
  begin ();
  if ( ( g_terminal ( "<" ) && g_rawId () && g_terminal ( ">" ) )
       || g_rawId () ) {
    return good ();
  }
  return bad ();
}

// "@" identifier
bool ParserBuilder::g_option ()
{
  ParserNode *node = root;
  begin ();
  if ( g_terminal ( "@" ) && g_identifier () ) {
    rules.back ().second.setOptionsString ( root->value );
    freeNode ( root );
    root = node;
    return good ();
  }
  return bad ();
}

// letter { letter | digit | "_" | "-" }
bool ParserBuilder::g_rawId ()
{
  begin ();
  ParserNode *node = allocNode ( ParserNode::NodeType::PT_Identifier );
  l.beginCapture ( node->value );
  if ( std::isalpha ( l.nextNoWS () ) ) {
    int c = l.next ();
    while ( std::isalnum ( c ) || c == '_' || c == '-' ) {
      c = l.next ();
    }
    if ( c != 0 ) {
      l.back ();
      l.endCapture ();
      root = node;
      return good ();
    }
  }
  l.endCapture ();
  freeNode ( node );
  return bad ();
}

// """ """ """ | """ any_excluding_quote { any_excluding_quote } """
// any_excluding_quote ::= - """ any_character
bool ParserBuilder::g_quotedSymbol ()
{
  ParserNode *node = allocNode ( ParserNode::NodeType::PT_Literal );
  begin ();
  if ( g_terminal ( "\"\"\"" ) ) {
    node->value = "\"";
    root        = node;
    return good ();
  }
  else if ( l.nextNoWS () == '"' && !l.isEscaped () ) {
    l.beginCapture ( node->value );
    // std::cout << "Quoted symbol: [" << l.getPos() << "]" <<  std::endl;
    int c = l.next ();
    // std::cout << "Quoted symbol: [" << l.getPos() << "] '" << (char)c << "' (" << (int)c << "), e = " << l.isEscaped() <<  std::endl;
    while ( c != '\0' && ( c != '"' || l.isEscaped () ) ) {
      c = l.next ();
      // std::cout << "  Quoted symbol: [" << l.getPos() << "] '" << (char)c << "' (" << (int)c << "), e = " << l.isEscaped() <<  std::endl;
    }
    if ( c != 0 && c == '"' ) {
      // std::cout << "OK !" << std::endl;
      l.back ();
      l.endCapture ();
      l.next ();
      root = node;
      return good ();
    }
  }
  freeNode ( node );
  return bad ();
}

bool ParserBuilder::g_terminal ( const std::string &p_str )
{
  const char *str = p_str.c_str ();
  begin ();
  if ( l.nextNoWS () == *str ) {
    str++;
    while ( *str && l.next () == *str ) {
      str++;
    }
    if ( !*str ) {
      return good ();
    }
  }
  return bad ();
}

bool ParserBuilder::applyRulesIndexes ( ParserNode *p_node )
{
  if ( p_node ) {
    if ( p_node->type == ParserNode::NodeType::PT_Identifier && p_node->index == -1 ) {
      const Rule *rule = getRule ( p_node->value );
      if ( rule ) {
        if ( rule->first->type == ParserNode::NodeType::PT_Rule ) {
          p_node->index = rule->first->index;
        }
        else { //This is an extension.
          *p_node = *rule->first;
        }
      }
      else {
        std::cerr << "ERROR : Rule '" << p_node->value << "' does not exist !" << std::endl;
        return false;
      }
    }
    if ( p_node->left ) {
      if ( !applyRulesIndexes ( p_node->left ) ) {
        if ( !p_node->value.empty () )
          std::cerr << " > At: " << p_node->toString () << std::endl;
        return false;
      }
    }
    if ( p_node->right ) {
      if ( !applyRulesIndexes ( p_node->right ) ) {
        if ( !p_node->value.empty () )
          std::cerr << " > At: " << p_node->toString () << std::endl;
        return false;
      }
    }
  }
  return true;
}
