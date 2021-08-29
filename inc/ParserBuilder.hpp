#ifndef PARSER_H_
#define PARSER_H_

#include "Parser.hpp"

#include <utility>
#include <vector>

namespace PB {
  class ParserBuilder : public Parser {
  public:
    ParserBuilder ();
    virtual ~ParserBuilder ();

    struct RuleOption {
      enum value_type
      {
        PR_Option_Trace, // t: Show debug info when the rule is called.
        PR_Option_Exclude, // x: Do not capture the current rule => Will return the content if it's not a capture, or the capture.
        PR_Option_Capture,     // c: Capture the textual content of that rule.
        PR_Option_Wipe,        // w: Wipe all children rules.
        PR_Option_Ignore_WS,   // n: Force ignoring blank characters. Cannot be used with Include_WS
        PR_Option_Include_WS,   // s: Force to not ignoring blank characters. Cannot be used with Ignore_WS
        PR_Option_Ignore_WS_NO_NL,   // i: Force ignoring blank characters but not the newlines. Cannot be used with Include_WS
        PR_Option_Push_Symbol, // p: Need 2 rules name: rule1 and rule2, push the rule1 value as a QuotedSymbol into the rule2. 
      };
      RuleOption ()
          : value ( 0 ) {}
      void clear () { value = 0; }
      void setOptionsString ( const std::string &p_options );
      void add ( value_type p_value ) { value |= ( 1 << p_value ); }
      void remove ( value_type p_value ) { value &= ~( 1 << p_value ); }
      bool have ( value_type p_value ) const { return ( value & ( 1 << p_value ) ) != 0; }

      unsigned int value;
      std::vector<std::string> params;
    };

    typedef std::pair< ParserNode *, RuleOption > Rule;
    typedef std::vector< Rule > Rules;

    // Parse the e-BNF stream and build the internal parser. Must be called before parse(Parser&).
    bool generate ();

    // Parse to the new parser using the generated one.
    virtual bool parse ();

    // return the rule, or NULL if there is no rule with the given name.
    Rule *getRule ( const std::string &p_ruleName );
    ParserNode *getNodeFromName ( ParserNode *from, ParserNode::NodeType::value_type p_type, const std::string &p_ruleName );

    // return the rule, or NULL if there is no rule with the given name.
    const std::string &getDefaultEntryPoint () const;

    const Rules &getRules () const;
    const Rules &getExtensions () const;

    void clearRules ();

    void printTree(bool compact = false) const;
    void printTree(const ParserNode *p_node, bool compact = false) const;

  private:
    size_t printTree(bool compact, const ParserNode *p_node, bool is_left, size_t p_offset, size_t p_width, size_t p_depth, std::vector< std::string > &p_tab ) const;

    //Generator functions
    bool applyRulesIndexes ( ParserNode *p_node );

    bool g_syntax ();
    
    bool g_lexer ();
    bool g_lex_comment ();

    bool g_rule ();
    bool g_comments ();
    bool g_expression ();
    bool g_term ();
    bool g_factor ();
    bool g_groupFactor ();
    bool g_optionalFactor ();
    bool g_repeatedFactor ();
    bool g_rangeFactor ();
    bool g_exclusion ();
    bool g_identifier ();
    bool g_options ();
    bool g_rawId ();
    bool g_quotedSymbol ();
    bool g_terminal ( const std::string &p_str );

    //Parser functions
    typedef bool ( ParserBuilder::*validation_func_t ) ( const ParserNode *p_node );
    static const validation_func_t func_node_associations[];

    ParserNode *v_getRule ( const ParserNode *p_node );

    bool v_dispatch ( ParserNode *p_node );
    bool v_rule ( const ParserNode *p_node );
    bool v_alternative ( const ParserNode *p_node );
    bool v_sequence ( const ParserNode *p_node );
    bool v_group ( const ParserNode *p_node );
    bool v_option ( const ParserNode *p_node );
    bool v_repeat ( const ParserNode *p_node );
    bool v_exclude ( const ParserNode *p_node );
    bool v_range ( const ParserNode *p_node );
    bool v_identifier ( const ParserNode *p_node );
    bool v_quotedSymbol ( const ParserNode *p_node );
    bool v_nothing ( const ParserNode *p_node );

    bool v_letter ( const ParserNode *p_node );
    bool v_digit ( const ParserNode *p_node );
    bool v_space ( const ParserNode *p_node );
    bool v_spaceNoNl ( const ParserNode *p_node );
    bool v_anyCharacter ( const ParserNode *p_node );
    bool v_eof ( const ParserNode *p_node );
    bool v_eol ( const ParserNode *p_node );

    void PushSymbol(ParserNode *rule, const std::string &p_symbol);

    //attributes
    bool debug;
    int level;
    Rules rules;
    Rules extensions;
    Rule *lastRule;
  };

} // namespace PB

#endif /* PARSER_H_ */
