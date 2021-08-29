#include "ParserBuilder.hpp"

#include <cctype>
#include <iostream>

using namespace PB;

//#define DEBUG
#ifdef DEBUG
static int level__x                = 0;
static const unsigned int level__c = ' ';

#define begin()                                                                                                                                                \
  std::cout << std::string ( level__x++ * 2, level__c ) << "begin: " << __FUNCTION__ << ", " << ( p_node ? p_node->toString () : "<null_node>" ) << std::endl; \
  begin ()
#define good() std::cout << std::string ( --level__x * 2, level__c ) << "good: " << __FUNCTION__ << ", " << ( p_node ? p_node->toString () : "<null_node>" ) << std::endl, good ()
#define bad() std::cout << std::string ( --level__x * 2, level__c ) << "bad: " << __FUNCTION__ << ", " << ( p_node ? p_node->toString () : "<null_node>" ) << std::endl, bad ()

#endif

const ParserBuilder::validation_func_t ParserBuilder::func_node_associations[] = {
  &ParserBuilder::v_rule,
  &ParserBuilder::v_alternative,
  &ParserBuilder::v_sequence,
  &ParserBuilder::v_group,
  &ParserBuilder::v_option,
  &ParserBuilder::v_repeat,
  &ParserBuilder::v_exclude,
  &ParserBuilder::v_range,
  &ParserBuilder::v_identifier,
  &ParserBuilder::v_quotedSymbol,
  &ParserBuilder::v_nothing,
  &ParserBuilder::v_nothing,
  &ParserBuilder::v_letter,
  &ParserBuilder::v_digit,
  &ParserBuilder::v_space,
  &ParserBuilder::v_spaceNoNl,
  &ParserBuilder::v_anyCharacter,
  &ParserBuilder::v_eof,
  &ParserBuilder::v_eol,
};

bool ParserBuilder::parse ()
{
  l.generateMode = false;
  l.ignore_nl    = false;
  l.ignore_ws    = false;
  debug          = rules[ 0 ].second.have ( RuleOption::PR_Option_Trace );
  level          = 0;
  l.skipWhitespaces ();
  if ( l.haveNext () ) {
    if ( !v_dispatch ( v_getRule ( rules[ 0 ].first ) ) ) {
      l.printError ();
      if ( lastRule != NULL ) {
        std::cerr << "lastRule = " << lastRule->first->value << std::endl;
      }
    }
  }
  else {
    std::cerr << "No more data to parse." << std::endl;
    state = false;
  }
  return state;
}

ParserNode *ParserBuilder::v_getRule ( const ParserNode *p_node )
{
  if ( p_node->index == -1 ) {
    std::cerr << __FUNCTION__ << ": Index is -1 for node " << p_node->toString () << std::endl;
    Rule *rule = getRule ( p_node->value );
    if ( rule ) {
      return rule->first;
    }
    else {
      std::cerr << __FUNCTION__ << ": No rule found for node " << p_node->toString () << std::endl;
    }
  }
  else {
    if ( rules[ p_node->index ].first->access_pos == l.getPos () ) {
      std::cerr << __FUNCTION__ << ": Looping of the rule " << p_node->value << " at pos " << l.getPos () << std::endl;
    }
    else {
      rules[ p_node->index ].first->access_pos = l.getPos ();
      return rules[ p_node->index ].first;
    }
  }
  return NULL;
}

bool ParserBuilder::v_dispatch ( ParserNode *p_node )
{
  begin ();
  //std::cout << std::string ( level__x++*2, level__c ) << "begin: " << __FUNCTION__ << ", " << p_node->toString () << std::endl;
  if ( p_node ) {
    if ( p_node->type == ParserNode::NodeType::PT_Identifier ) {
      if ( p_node->access_pos == l.getPos () ) {
        std::cerr << "Looping of the rule '" << p_node->value << "' at pos " << l.getPos () << std::endl;
        freeNode ( root );
        root = NULL;
        //std::cout << std::string ( --level__x*2, level__c ) << "bad: " << __FUNCTION__ << ", " << p_node->toString () << std::endl;
        return bad ();
      }
      else {
        p_node->access_pos = l.getPos ();
      }
    }
    if ( ( this->*func_node_associations[ p_node->type.value ] ) ( p_node ) ) {
//std::cout << std::string ( --level__x*2, level__c ) << "good: " << __FUNCTION__ << ", " << p_node->toString () << std::endl;
#ifdef DEBUG
      l.showCurrent ();
      //system ( "pause" );
#endif
      p_node->access_pos = (size_t) -1;
      return good ();
    }
    p_node->access_pos = (size_t) -1;
  }
  freeNode ( root );
  root = NULL;
  //std::cout << std::string ( --level__x*2, level__c ) << "bad: " << __FUNCTION__ << ", " << p_node->toString () << std::endl;
  return bad ();
}

bool ParserBuilder::v_rule ( const ParserNode *p_node )
{
  ParserNode *node = allocNode ( ParserNode::NodeType::PT_Rule );
  Rule *rule       = getRule ( p_node->value );
  RuleOption &opts = rule->second;
  bool currentWS   = l.ignore_ws;
  bool currentNl   = l.ignore_nl;

  begin ();
  node->value = p_node->value;
  node->index = p_node->index;
  // std::cout << "    - rule [" << node << "] " << node->toString () << std::endl;
  if ( opts.have ( RuleOption::PR_Option_Trace ) | debug ) {
    std::cout << std::string ( level++ * 2, ' ' ) << "Entering rule " << node->value << std::endl;
    l.showCurrent ();
  }
  if ( opts.have ( RuleOption::PR_Option_Ignore_WS ) ) {
    l.ignore_ws = true;
    l.ignore_nl = true;
  }
  if ( opts.have ( RuleOption::PR_Option_Ignore_WS_NO_NL ) ) {
    l.ignore_ws = true;
    l.ignore_nl = false;
  }
  if ( opts.have ( RuleOption::PR_Option_Include_WS ) ) {
    l.ignore_ws = false;
    l.ignore_nl = false;
  }
  if ( opts.have ( RuleOption::PR_Option_Capture ) ) {
    node->right = allocNode ( ParserNode::NodeType::PT_Literal );
    l.beginCapture ( node->right->value );
    if ( v_dispatch ( p_node->right ) ) {
      node->left = root;
    }
    l.endCapture ();
    if ( opts.have ( RuleOption::PR_Option_Trace ) | debug ) {
      std::cout << std::string ( --level * 2, ' ' ) << "After dispatch of rule " << node->value << ", state = '" << state << "', Captured: '" << node->right->value << "'" << std::endl;
    }
  }
  else {
    if ( v_dispatch ( p_node->right ) ) {
      node->left = root;
    }
    if ( opts.have ( RuleOption::PR_Option_Trace ) | debug ) {
      std::cout << std::string ( --level * 2, ' ' ) << "After dispatch of rule " << node->value << ", state = '" << state << "'" << std::endl;
    }
  }
  l.ignore_ws = currentWS;
  l.ignore_nl = currentNl;
  if ( state ) {
    if ( opts.have ( RuleOption::PR_Option_Push_Symbol ) ) {
      ParserNode *pushNode = getNodeFromName ( node, ParserNode::NodeType::PT_Rule, opts.params[ 0 ] );
      Rule *pushRule       = getRule ( opts.params[ 1 ] );
      if ( pushNode && pushRule && pushNode->left ) {
        ParserNode *existing = getNodeFromName ( pushRule->first, ParserNode::NodeType::PT_Literal, pushNode->left->value );
        if ( !existing ) {
          std::cout << "No existing node, OK , push " << pushNode->left->value << std::endl;
          PushSymbol(pushRule->first, pushNode->left->value);
        }
      }
    }
    if ( opts.have ( RuleOption::PR_Option_Wipe ) ) {
      // std::cout << "Rule [" << node << "] " << node->toString () << " have WIPE." << std::endl;
      freeNode ( node->left );
      node->left  = node->right;
      node->right = NULL;
    }
    if ( opts.have ( RuleOption::PR_Option_Exclude ) ) {
      ParserNode *tmp = node;
      node            = node->left;
      if ( ( node != NULL )
           && opts.have ( RuleOption::PR_Option_Capture )
           && !node->right ) {
        node->right = tmp->right;
      }
      tmp->right = NULL;
      tmp->left  = NULL;
      // std::cout << "Rule [" << tmp << "] " << tmp->toString () << " have Exclude. Node is [" << node << "]" << std::endl;
      freeNode ( tmp );
    }
    root     = node;
    lastRule = rule;
#ifdef DEBUG
    std::cout << "Rule OK : " << rule->first->value << ", node = " << node << std::endl;
    if ( opts.have ( RuleOption::PR_Option_Capture ) && node && node->right )
      std::cout << " - Capture(" << &node->right->value << ") = '" << node->right->value << "'" << std::endl;
#endif
    return good ();
  }
  freeNode ( node );
  return bad ();
}

bool ParserBuilder::v_alternative ( const ParserNode *p_node )
{
  begin ();
  if ( v_dispatch ( p_node->left )
       || v_dispatch ( p_node->right ) ) {
    return good ();
  }
  return bad ();
}

bool ParserBuilder::v_sequence ( const ParserNode *p_node )
{
  ParserNode *node = allocNode ( ParserNode::NodeType::PT_Sequence );
  begin ();
  l.skipWhitespaces ();
  if ( v_dispatch ( p_node->left ) ) {
    node->left = root;
    l.skipWhitespaces ();
    if ( v_dispatch ( p_node->right ) ) {
      l.skipWhitespaces ();
      node->right = root;
      root        = node;
      return good ();
    }
  }
  freeNode ( node );
  freeNode ( root );
  root = NULL;
  return bad ();
}

bool ParserBuilder::v_group ( const ParserNode *p_node )
{
  ParserNode *node = allocNode ( ParserNode::NodeType::PT_Group );
  begin ();
  if ( v_dispatch ( p_node->left ) ) {
    node->left = root;
    root       = node;
    return good ();
  }
  freeNode ( node );
  return bad ();
}

bool ParserBuilder::v_option ( const ParserNode *p_node )
{
  ParserNode *node = allocNode ( ParserNode::NodeType::PT_Option );
  if ( l.haveNext () && v_dispatch ( p_node->left ) ) {
    node->left = root;
  }
  root = node;
  return true; //no possible error.
}

bool ParserBuilder::v_repeat ( const ParserNode *p_node )
{
  ParserNode *prev = NULL;
  bool dispatch    = true;
  size_t beginPos  = (size_t) -1;
  while ( l.haveNext () && beginPos != l.getPos () && dispatch ) {
    beginPos = l.getPos ();
    dispatch = v_dispatch ( p_node->left );
    if ( dispatch ) {
      ParserNode *tmp = allocNode ( ParserNode::NodeType::PT_Repeat );
      tmp->left       = prev;
      tmp->right      = root;
      prev            = tmp;
    }
  }
  root = prev;
  return true;
}

bool ParserBuilder::v_exclude ( const ParserNode *p_node )
{
  begin ();
  if ( v_dispatch ( p_node->left ) ) {
    freeNode ( root );
    root = NULL;
    return bad ();
  }
  return good ();
}

bool ParserBuilder::v_range ( const ParserNode *p_node )
{
  begin ();
  root  = allocNode ( ParserNode::NodeType::PT_Range );
  int c = l.next ();
  if ( ( c != 0 )
       && ( c >= p_node->left->value[ 0 ] )
       && ( c <= p_node->right->value[ 0 ] ) ) {
    root->value = c;
    return good ();
  }
  freeNode ( root );
  root = NULL;
  return bad ();
}

bool ParserBuilder::v_identifier ( const ParserNode *p_node )
{
  // std::cout << std::string ( level__x++*2, level__c ) << "begin: " << __FUNCTION__ << ", " << p_node->toString () << std::endl;
  bool ret = v_dispatch ( v_getRule ( p_node ) );
  // std::cout << std::string ( --level__x*2, level__c ) << "end: " << __FUNCTION__ << ", " << p_node->toString () << std::endl;
  return ret;
}

bool ParserBuilder::v_quotedSymbol ( const ParserNode *p_node )
{
  begin ();
  root            = allocNode ( ParserNode::NodeType::PT_Literal );
  const char *str = p_node->value.c_str ();
  char c          = l.next ();
  if ( ( c != 0 ) && ( c == *str ) ) {
    str++;
    while ( *str && ( ( c = l.next () ) == *str ) ) {
      str++;
    }
    if ( !*str ) {
      root->value = p_node->value;
      return good ();
    }
  }
  freeNode ( root );
  root = NULL;
  return bad ();
}

bool ParserBuilder::v_nothing ( const ParserNode *p_node )
{
  (void) p_node;
  return false;
}

bool ParserBuilder::v_letter ( const ParserNode *p_node )
{
  root = allocNode ( ParserNode::NodeType::PT_EXTENSION_1 );
  begin ();
  char c = l.next ();
  (void) p_node;
#ifdef DEBUG
  std::cout << __FUNCTION__ << ": '" << c << "'" << std::endl;
#endif
  if ( c && std::isalpha ( c ) ) {
    root->value = c;
    return good ();
  }
  freeNode ( root );
  root = NULL;
  return bad ();
}

bool ParserBuilder::v_digit ( const ParserNode *p_node )
{
  root = allocNode ( ParserNode::NodeType::PT_EXTENSION_2 );
  begin ();
  char c = l.next ();
  (void) p_node;
#ifdef DEBUG
  std::cout << __FUNCTION__ << ": '" << c << "'" << std::endl;
#endif
  if ( c && std::isdigit ( c ) ) {
    root->value = c;
    return good ();
  }
  freeNode ( root );
  root = NULL;
  return bad ();
}

bool ParserBuilder::v_space ( const ParserNode *p_node )
{
  root = allocNode ( ParserNode::NodeType::PT_EXTENSION_3 );
  begin ();
  char c = l.next ();
  (void) p_node;
#ifdef DEBUG
  std::cout << __FUNCTION__ << ": '" << c << "'" << std::endl;
#endif
  if ( c && std::isspace ( c ) ) {
    root->value = c;
    return good ();
  }
  freeNode ( root );
  root = NULL;
  return bad ();
}

bool ParserBuilder::v_spaceNoNl ( const ParserNode *p_node )
{
  root = allocNode ( ParserNode::NodeType::PT_EXTENSION_4 );
  begin ();
  char c = l.next ();
  (void) p_node;
#ifdef DEBUG
  std::cout << __FUNCTION__ << ": '" << c << "'" << std::endl;
#endif
  if ( c && c != '\n' && c != '\r' && std::isspace ( c ) ) {
    root->value = c;
    return good ();
  }
  freeNode ( root );
  root = NULL;
  return bad ();
}

bool ParserBuilder::v_anyCharacter ( const ParserNode *p_node )
{
  root   = allocNode ( ParserNode::NodeType::PT_EXTENSION_5 );
  char c = l.next ();
  (void) p_node;
#ifdef DEBUG
  std::cout << __FUNCTION__ << ": '" << c << "'" << std::endl;
#endif
  if ( c != 0 ) {
    root->value = c;
    return true;
  }
  freeNode ( root );
  root = NULL;
  return false;
}

bool ParserBuilder::v_eof ( const ParserNode *p_node )
{
  root   = allocNode ( ParserNode::NodeType::PT_EXTENSION_6 );
  char c = l.next ();
  (void) p_node;
#ifdef DEBUG
  std::cout << __FUNCTION__ << ": '" << c << "'" << std::endl;
#endif
  if ( c == 0 ) {
    return true;
  }
  freeNode ( root );
  root = NULL;
  return false;
}

bool ParserBuilder::v_eol ( const ParserNode *p_node )
{
  root   = allocNode ( ParserNode::NodeType::PT_EXTENSION_7 );
  char c = l.next ();
  (void) p_node;
#ifdef DEBUG
  std::cout << __FUNCTION__ << ": '" << c << "'" << std::endl;
#endif
  if ( c == '\n' ) { //Linux
    return true;
  }
  if ( c == '\r' ) {
    c = l.next ();
    if ( c != '\0' && c != '\n' ) {
      l.back (); //Mac
    }            //else Windows
    return true;
  }
  freeNode ( root );
  root = NULL;
  return false;
}

#ifdef DEBUG
#undef DEBUG
#endif
