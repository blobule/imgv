#ifndef UTIL_HPP
#define UTIL_HPP

//
// various simple useful things
//

//
// convert a number (or anything else) into a string
//
// Usage: NumberToString<type> ( Number );
//
template <typename T>
  string NumberToString ( T Number )
  {
     ostringstream ss;
     ss << Number;
     return ss.str();
  }


//
// convert a string into a number (or anything else)
//
// Usage: StringToNumber<type> ( String );
//
template <typename T>
  T StringToNumber ( const string &Text )
  {
     istringstream ss(Text);
     T result;
     return ss >> result ? result : 0;
  }


#endif

