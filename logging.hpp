#ifndef __NOSO2M_LOGGING_HPP__
#define __NOSO2M_LOGGING_HPP__

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>
#include <sstream>
#include <iomanip>

enum class CLogLevel { FATAL, ERROR, WARN, INFO, DEBUG, };

template <typename OFS>
class CLogEntry {
protected:
    OFS & ofs;
    std::ostringstream oss;
public:
    CLogEntry( OFS & ofs ) : ofs { ofs } {};
    CLogEntry( CLogEntry const & obj ) = delete;
    virtual ~CLogEntry() {
        ofs.Output( oss.str() );
    }
    CLogEntry& operator=( CLogEntry const & obj ) = delete;
    template <CLogLevel LEVEL>
    std::ostringstream& GetStream( CLogLevel level ) {
        if ( LEVEL > level ) oss.setstate( std::ios_base::badbit );
        std::time_t now { std::time( 0 ) };
        struct std::tm* ptm = std::localtime( &now );
        oss << "[" << std::put_time( ptm, "%x %X" ) << "]" << LogLevelString<LEVEL>() << ":";
        return oss;
    }
    template <CLogLevel LEVEL>
    static std::string LogLevelString() {
        return
              LEVEL == CLogLevel::FATAL ? "FATAL" :
            ( LEVEL == CLogLevel::ERROR ? "ERROR" :
            ( LEVEL == CLogLevel::WARN  ? " WARN" :
            ( LEVEL == CLogLevel::INFO  ? " INFO" :
            ( LEVEL == CLogLevel::DEBUG ? "DEBUG" : "     " ) ) ) );
    }
};

class CLogFile {
private:
    std::ostream & m_ost;
public:
    CLogFile( std::ostream & ost ) : m_ost { ost } {}
    void Output( std::string const & msg ) {
        m_ost << msg << std::flush;
    }
};

#endif // __NOSO2M_LOGGING_HPP__

