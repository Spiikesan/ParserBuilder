#include "Lexer.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

using namespace PB;

Lexer::Lexer ()
    : ignore_ws ( false ), generateMode ( false ), current ( 0 ), backPos ( 0 ), escaped ( false ), lastValidPos ( 0 )
{
}

Lexer::~Lexer ()
{
}

bool Lexer::openFile ( const std::string &p_filename )
{
  std::ifstream file ( p_filename.c_str () );

  if ( file.good () ) {
    resetStreamStatus ();
    file.seekg ( 0, std::ios::end );
    stream.reserve ( file.tellg () );
    file.seekg ( 0, std::ios::beg );
    stream.assign ( ( std::istreambuf_iterator< char > ( file ) ),
                    std::istreambuf_iterator< char > () );
    return true;
  }
  return false;
}

bool Lexer::openString ( const std::string &p_string )
{
  stream = p_string;
  resetStreamStatus ();
  return true;
}

void Lexer::close ()
{
  stream.clear ();
  resetStreamStatus ();
}

bool Lexer::haveNext () const
{
  size_t position = current;
  if ( ignore_ws ) {
    position += countWhitespaces ();
  }
  return position < stream.size ();
}

int Lexer::next ()
{
  std::string::value_type c = 0;
  if ( haveNext () ) {
    c = get ();
    for ( size_t i = 0; i < captures.size (); i++ ) {
      captures[ i ]->push_back ( c );
    }
  }
  return c;
}

int Lexer::nextNoWS ()
{
  skipWhitespaces ();
  return next ();
}

int Lexer::back ()
{
  int ret = 0;
  int len = 0;
  if ( current > 0 ) {
    len = unget ();
  }
  else {
    ret = -1;
  }
  if ( ret == 0 ) {
    for ( size_t i = 0; i < captures.size (); i++ ) {
      int currlen = captures[ i ]->size ();
      if ( currlen > 0 ) {
        if ( len > currlen )
          len = currlen;
        captures[ i ]->erase ( currlen - len );
      }
    }
  }
  return ret;
}

void Lexer::savePos ()
{
  pos.push_back ( { current, 0 } );
  if ( lastValidPos < current ) {
    lastValidPos = current;
  }
}

void Lexer::restorePos ()
{
  if ( !pos.empty () ) {
    size_t lastSize = current - pos.back ().first - pos.back ().second;
    current         = pos.back ().first;
    pos.pop_back ();
    if ( lastSize > 0 && !captures.empty () ) {
      // std::cout << "restorePos: " << captures.size () << " captures." << std::endl;
      for ( size_t i = 0; i < captures.size (); i++ ) {
        if ( captures[ i ]->size () > 0 ) {
          int removeIndex = captures[ i ]->size () - lastSize;
          if ( removeIndex < 0 )
            removeIndex = 0;
          // std::cout << " - Capture(" << captures[ i ] << "): '" << *captures[ i ] << "' -> lastSize " << lastSize;
          captures[ i ]->erase ( removeIndex );
          // std::cout << ", '" << *captures[ i ] << "'" << std::endl;
        }
      }
    }
  }
}

void Lexer::clearPos ()
{
  pos.pop_back ();
}

void Lexer::beginCapture ( std::string &p_string )
{
  p_string.clear ();
  captures.push_back ( &p_string );
}

void Lexer::endCapture ()
{
  if ( !captures.empty () ) {
    captures.pop_back ();
  }
}

void Lexer::skipWhitespaces ()
{
  if ( ignore_ws ) {
    size_t spaces = countWhitespaces ();
    current += spaces;
    if ( !pos.empty () ) {
      pos.back ().second += spaces;
    }
  }
}

size_t Lexer::countWhitespaces () const
{
  size_t pos          = 0;
  bool inLineComment  = false;
  bool inBlockComment = false;
  if ( !comment_block_begin.empty() && stream.substr ( current + pos, comment_block_begin.size () ) == comment_block_begin ) {
      inBlockComment = true;
      pos += comment_block_begin.size ();
    }
  else if ( !comment_line.empty() && stream.substr ( current + pos, comment_line.size () ) == comment_line ) {
    inLineComment = true;
    pos += comment_line.size ();
  }
  while ( ( ( current + pos ) < stream.size () )
          && ( std::isspace ( stream[ current + pos ] ) || inLineComment || inBlockComment )
          && ( ignore_nl
               || ( !ignore_nl && stream[ current + pos ] != '\n' && stream[ current + pos ] != '\r' ) ) ) {
    pos++;

    if ( !comment_block_begin.empty() && !inLineComment && stream.substr ( current + pos, comment_block_begin.size () ) == comment_block_begin ) {
      inBlockComment = true;
      pos += comment_block_begin.size ();
    }
    else if ( !comment_line.empty() && !inBlockComment && stream.substr ( current + pos, comment_line.size () ) == comment_line ) {
      inLineComment = true;
      pos += comment_line.size ();
    }
    if ( inBlockComment && stream.substr ( current + pos, comment_block_end.size () ) == comment_block_end ) {
      inBlockComment = false;
      pos += comment_block_end.size ();
    }
    else if ( inLineComment ) {
      if ( stream[ current + pos ] == '\n' ) {
        inLineComment = false;
        pos++;
      }
      else if ( stream[ current + pos ] == '\r' ) {
        pos++;
        if ( stream[ current + pos ] == '\n' ) {
          pos++;
        }
        inLineComment = false;
      }
    }
  }
  return pos;
}

int Lexer::getHexValue ( char p_val )
{
  if ( p_val >= '0' && p_val <= '9' )
    return p_val - '0';
  if ( p_val >= 'a' && p_val <= 'f' )
    return p_val - 'a' + 10;
  if ( p_val >= 'A' && p_val <= 'F' )
    return p_val - 'A' + 10;
  return 0;
}

int Lexer::get ()
{
  std::string::value_type c = stream[ current ];
  backPos                   = current;
  current++;

  if ( generateMode && c == '\\' ) { //Cas special du caractere echappe
    escaped = true;
    switch ( stream[ current ] ) {
      case '\\':
        c = '\\';
        current++;
        break;
      case '"':
        c = '"';
        current++;
        break;
      default:
        c = Lexer::getHexValue ( stream[ current ] ) << 4 | Lexer::getHexValue ( stream[ current + 1 ] );
        current += 2; //On saute les deux caracteres necessaires pour encoder la valeur
        break;
    }
  }
  else {
    escaped = false;
  }
  return c;
}

int Lexer::unget ()
{
  int diff = current - backPos;
  current  = backPos;
  return diff;
}

void Lexer::getLastValidPos ( size_t p_current, size_t &line, size_t &column, size_t &pos, size_t &line_begin, size_t &line_end ) const
{
  // std::cout << "getLastValidPos(beg): lastValidPos ? " << lastValidPos <<",  p_current = " << p_current << " current = " << current << ", lastValidPos = " << lastValidPos << ", line = " << line << ", column = " << column << ", pos = " << pos << ", line_begin = " << line_begin << ", line_end = " << line_end << std::endl;
  // std::cout.flush();
  size_t rfind = stream.rfind ( '\n', p_current );
  line         = std::count ( stream.begin (), stream.begin () + p_current, '\n' ) + 1;
  column       = p_current;
  pos          = p_current + line - 1;
  if ( rfind != std::string::npos ) { //other lines
    column -= rfind;
    line_begin = p_current - column + 1;
  }
  else { //Line == 1
    line_begin = p_current - column;
    if ( p_current > 0 )
      column += 1;
  }
  line_end = stream.find ( '\n', line_begin ) - line_begin;
  // std::cout << "getLastValidPos(end): current = " << current << ", lastValidPos = " << lastValidPos << ", rfind = " << rfind << ", line = " << line << ", column = " << column << ", pos = " << pos << ", line_begin = " << line_begin << ", line_end = " << line_end << std::endl;
  // std::cout.flush();
}

void Lexer::printError () const
{
  size_t pos = 0, line = 0, column = 0, line_begin = 0, line_end = 0;
  getLastValidPos ( generateMode ? lastValidPos : ( lastValidPos > 0 ? lastValidPos - 1 : 0 ), line, column, pos, line_begin, line_end );
  std::cerr << "Parsing error line " << line << ", col " << ( column + 1 ) << ", character position " << ( pos + 1 ) << std::endl;
  std::cerr << "Line : " << getStream ().substr ( line_begin, line_end ) << std::endl;
  std::cerr << "Here : " << std::string ( column, '-' ) << "^" << std::endl;
}

void Lexer::showCurrent () const
{
  size_t pos = 0, line = 0, column = 0, line_begin = 0, line_end = 0;
  getLastValidPos ( generateMode ? current : ( current > 0 ? current - 1 : 0 ), line, column, pos, line_begin, line_end );
  std::cerr << "Info line " << line << ", col " << ( column + 1 ) << ", character position " << ( pos + 1 ) << std::endl;
  std::cerr << "Line : " << getStream ().substr ( line_begin, line_end ) << std::endl;
  std::cerr << "Here : " << std::string ( column, '-' ) << "^" << std::endl;
}

void Lexer::resetStreamStatus ()
{
  current      = 0;
  lastValidPos = 0;
  pos.clear ();
  captures.clear ();
}
