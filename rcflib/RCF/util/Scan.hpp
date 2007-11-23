
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_UTIL_SCAN_HPP
#define INCLUDE_UTIL_SCAN_HPP

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace util {

    class Scan {
    public:
        Scan( std::string s, std::string format ) : i_(0), ok_(true)
        {
            int n = 0;
            std::string::size_type pos = 0;
            std::string::size_type prev_pos = 0;
            std::string::size_type npos = std::string::npos;

            // Extract format string literals
            std::vector< std::pair<std::string, std::string> > literals;
            while(pos != npos) {
                n++;
                std::string specifier1 = makeSpecifier(n);
                std::string specifier2 = makeSpecifier(n+1);
                pos = format.find(specifier1, prev_pos);
                std::string literal1 = (pos == npos) ? format.substr(prev_pos) : format.substr(prev_pos, pos - prev_pos);
                if (pos != npos) pos += specifier1.length();
                prev_pos = pos;
                pos = format.find(specifier2, prev_pos);
                std::string literal2 = (pos == npos) ? format.substr(prev_pos) : format.substr(prev_pos, pos - prev_pos);
                literals.push_back( std::make_pair( literal1, literal2 ) );
            }

            // Extract arg string values
            pos = 0;
            for (std::vector< std::pair<std::string, std::string> >::iterator it = literals.begin(); it != literals.end(); it++) {
                if (pos != npos && pos == s.find( (*it).first, pos)) {
                    std::string::size_type a = pos + (*it).first.length();
                    std::string::size_type b = s.find( (*it).second, a );
                    values_.push_back( s.substr( a , (b == a) ? npos : b-a ) );
                    pos = b;
                }
                else {
                    ok_ = false;
                }
                   
            }
           
        }

        template<typename T> Scan &operator()(T &t)
        {
            std::istringstream istr( getNextValue() );
            istr >> t;
            return *this;
        }
       
        operator bool() const
        {
            return ok();
        }

        bool ok() const
        {
            return ok_;
        }

    private:

        std::string makeSpecifier( int n )
        {
            std::ostringstream ostr;
            ostr << "%" << n;
            return ostr.str();
        }

        std::string getNextValue()
        {
            if (i_<values_.size())
                return values_[ i_++ ];
            else {
                ok_ = false;
                return "";
            }
        }

        std::vector<std::string> values_;
        unsigned int i_;
        bool ok_;
    };

} // namespace util


#endif // ! INCLUDE_UTIL_SCAN_HPP
