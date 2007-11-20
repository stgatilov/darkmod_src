
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_UTIL_COMMANDLINE_HPP
#define INCLUDE_UTIL_COMMANDLINE_HPP

#include <exception>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#ifndef ASSERT
#include "Assert.hpp"
#define ASSERT(x) UTIL_ASSERT(x, std::runtime_error("Assertion failure (command line parser)"))
#define REMEMBER_TO_UNDEFINE_ASSERT
#endif

//*****************************************
// Command line parsing utility

namespace util {

    class I_CommandLineOption {
    public:
        virtual ~I_CommandLineOption() {}
        virtual void notify_begin() = 0;
        virtual void notify( std::string) = 0;
        virtual void notify_end() = 0;
        virtual std::string getName() = 0;
        virtual std::string getDefaultValue() = 0;
        virtual std::string getHelpString() = 0;
    };

    class CommandLine
    {
    private:
        CommandLine() {}
    public:
        static CommandLine &getSingleton()
        {
            static CommandLine commandLine;
            return commandLine;
        }

        void parse(int argc, char **argv)
        {
            parse(argc, const_cast<const char **>(argv));
        }

#if defined(_MSC_VER) && _MSC_VER <= 1200
#define for if (0) {} else for
#endif

        void parse(int argc, const char **argv)
        {
            getOptionValues().clear();
            getArgs().clear();
            int i=1;

            // If there is no "help" command line option registered, and the only command line option found is "help",
            // then we print out the helpstrings for all known command line options, and exit.
            if (argc == 2 && isKey(argv[1]) && toKey(argv[1]) == "help") {
                if (getOptions().find( "help" ) == getOptions().end()) {
                    std::cout << "Available command line switches:\n";
                    for (OptionIterator it = getOptions().begin(); it != getOptions().end(); it++) {
                        if ((*it).second) {
                            I_CommandLineOption *option = (*it).second;
                            std::cout << "-" << option->getName() << "\n";
                            std::cout << "\t" << option->getHelpString() << "\n";
                            std::cout << "\tDefault: " << option->getDefaultValue() << "\n";
                        }
                    }
                    exit(0);
                }
            }

            // Parse the command line
            while (i < argc) {
                std::string arg1 = argv[i];
                i++;
                std::string arg2 = (i<argc) ? argv[i] : "";
                i++;
                if (isKey(arg1)) {
                    if (!isKey(arg2) )
                        getOptionValues()[ toKey(arg1) ].push_back( arg2 );
                    else {
                        getOptionValues()[ toKey(arg1) ].push_back( "" );
                        i--;
                    }
                    if (getOptions().find( toKey(arg1) ) == getOptions().end()) {
                        std::cout << "Unrecognized option \"" << arg1 << "\"; type \"-help\" to list all options.\n";
                    }
                }
                else {
                    getArgs().push_back( arg1 );
                    i--;
                }
            }

            // Notify the registered I_CommandLineOption objects
            for (OptionIterator it = getOptions().begin(); it != getOptions().end(); it++)
                if ((*it).second)
                    (*it).second->notify_begin();

            for (OptionValueIterator it = getOptionValues().begin(); it != getOptionValues().end(); it++) {
                if (getOptions()[ (*it).first ]) {
                    for (ValueIterator iter = (*it).second.begin(); iter != (*it).second.end(); iter++)
                        getOptions()[ (*it).first ]->notify( *iter );
                }
            }

            for (OptionIterator it = getOptions().begin(); it != getOptions().end(); it++)
                if ((*it).second)
                    (*it).second->notify_end();
        }

#if defined(_MSC_VER) && _MSC_VER <= 1200
#undef for
#endif
       
        void registerOption(I_CommandLineOption *option)
        {
            getOptions()[ option->getName() ] = option;
        }

        void unregisterOption(I_CommandLineOption *option)
        {
            OptionIterator it = getOptions().find( option->getName() );
            if (it != getOptions().end() && (*it).second == option)
            {
                getOptions().erase( it );
            }
        }
       
        void clear()
        {
            getOptions().clear();
            getOptionValues().clear();
            getArgs().clear();
        }
       
        template<typename T>
        T get(std::string name)
        {
            T t;
            lexical_cast( getOptionValues()[name][0], t );
            return t;
        }
       
        const std::vector<std::string> &getArguments()
        {
            return getArgs();
        }
       
        const std::vector<std::string> &getValues(std::string name)
        {
            return getOptionValues()[name];
        }
       
        template<typename T>
        T lexical_cast(std::string strValue, T* = NULL)
        {
            T t;
            //t = boost::lexical_cast<T, std::string>(strValue);
            lexical_cast(strValue, t);
            return t;
        }

    private:
       
        typedef std::map< std::string, I_CommandLineOption *>::iterator OptionIterator;
        std::map< std::string, I_CommandLineOption *> &getOptions()
        {
            static std::map< std::string, I_CommandLineOption *> options;
            return options;
        }

        typedef std::map<std::string, std::vector<std::string> >::iterator OptionValueIterator;
        typedef std::vector<std::string>::iterator ValueIterator;
        std::map<std::string, std::vector<std::string> > &getOptionValues()
        {
            static std::map<std::string, std::vector<std::string> > optionValues;
            return optionValues;
        }

        std::vector<std::string> &getArgs()
        {
            static std::vector<std::string> args;
            return args;
        }

        bool isKey(const std::string &arg)
        {
            return (arg.size() > 1 && (arg[0] == '-' || arg[0] == '/') );
        }

        std::string toKey(const std::string &arg)
        {
            assert( isKey(arg) );
            //return std::string(arg, 1);
            return arg.substr(1);
        }

        void lexical_cast( const std::string &strValue, bool &value )
        {
            if (strValue == "1" || strValue == "true" || strValue == "")
                value = true;
            else
                value = false;
        }
       
        void lexical_cast( const std::string &strValue, int &value )
        {
            value = atoi(strValue.c_str());
        }

        void lexical_cast( const std::string &strValue, unsigned int &value )
        {
            value = static_cast<unsigned int>(atoi(strValue.c_str()));
        }

        void lexical_cast( const std::string &strValue, std::string &value )
        {
            value = strValue;
        }

    };

    template<typename T>
    class CommandLineOption : public I_CommandLineOption {
    public:

        CommandLineOption(std::string name, T default_value, std::string helpString) : name(name), default_value(default_value), helpString(helpString)
        {
            CommandLine::getSingleton().registerOption(this);
        }

        ~CommandLineOption()
        {
            CommandLine::getSingleton().unregisterOption(this);
        }
       
        operator T() const
        {
            return get();
        }
       
        //const std::vector<std::string> &getValues() const
        const std::vector<T> &getValues() const
        {
            return values;
        }
       
        T get() const
        {
            return (values.empty()) ? default_value : values[0];
        }
       
        void set(T t)
        {
            values.clear();
            values.push_back( t );
        }

        virtual void on_notify_end()
        {}
       
    private:

        void notify_begin()
        {
            values.clear();
        }

        void notify_end()
        {
            on_notify_end();
        }

        void notify( std::string value )
        {
            //values.push_back( CommandLine::getSingleton().template lexical_cast<T>(value) );
            values.push_back( CommandLine::getSingleton().lexical_cast(value, (T *) 0));
        }
       
        std::string getName()
        {
            return name;
        }
       
        std::string getHelpString()
        {
            return helpString;
        }

        std::string getDefaultValue()
        {
            std::ostringstream ostr;
            ostr << default_value ;
            return ostr.str();
        }

    private:
        std::string name;
        T default_value;
        std::vector<T> values;
        std::string helpString;
    };

    template<typename T>
    inline std::istream &operator>>(std::istream &is, CommandLineOption<T> &option)
    {
        T t;
        is >> t;
        option.set(t);
        return is;
    }

    template<typename T>
    inline std::ostream &operator<<(std::ostream &os, CommandLineOption<T> &option)
    {
        os << option.get();
        return os;
    }

} // namespace util

#ifdef REMEMBER_TO_UNDEFINE_ASSERT
#undef REMEMBER_TO_UNDEFINE_ASSERT
#undef ASSERT
#endif

#endif // ! INCLUDE_UTIL_COMMANDLINE_HPP
