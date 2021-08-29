#ifndef PB_LEXER_H_
#define PB_LEXER_H_

#include <string>
#include <vector>

namespace PB {
  class Lexer {
  public:
    Lexer ();
    ~Lexer ();

    bool openFile ( const std::string &p_filename );

    bool openString ( const std::string &p_string );

    void close ();

    bool haveNext () const;

    int curr () const { return stream[current]; }

    int next ();

    int nextNoWS ();

    int back ();

    // Save current stream position
    void savePos ();

    // Restore last stream position
    void restorePos ();

    // Delete last stream position without modifying the current position.
    void clearPos ();

    size_t getPos() const { return current; }

    // Begin a new capture by adding the reference of the current node to the list
    void beginCapture ( std::string &p_capture );
    // Delete the last added node value reference of the list
    void endCapture ();

    void skipWhitespaces ();

    bool ignore_ws; //whitespaces
    bool ignore_nl; //newlines
    bool generateMode;

    const std::string &getStream () const { return stream; }

    void printError ( ) const;
    void showCurrent( ) const;
    void resetStreamStatus ();
    bool isEscaped() const { return escaped; }

    std::string comment_line;
    std::string comment_block_begin;
    std::string comment_block_end;

  private:
    void getLastValidPos ( size_t p_correction, size_t &line, size_t &column, size_t &pos, size_t &line_begin, size_t &line_end ) const;
    size_t countWhitespaces () const;
    static int getHexValue ( char p_val );
    int get ();
    int unget ();

    size_t current;
    size_t backPos;
    bool escaped;
    size_t lastValidPos; //Used for error trace
    std::string stream;
    std::vector< std::pair< size_t, size_t> > pos;
    std::vector< std::string * > captures;
  };

} // namespace PB

#endif /* PB_LEXER_H_ */
