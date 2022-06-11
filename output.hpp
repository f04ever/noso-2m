#ifndef __NOSO2M_OUTPUT_HPP__
#define __NOSO2M_OUTPUT_HPP__
#include <fstream>
#include <iostream>
#include "logging.hpp"
#include "textui.hpp"
#include "noso-2m.hpp"

#define NOSO_STDOUT std::cout
#define NOSO_STDERR std::cerr

#ifndef NO_TEXTUI
static std::shared_ptr<CTextUI> _TEXTUI { CTextUI::GetInstance() };
#define NOSO_TUI_StartTUI()         _TEXTUI->StartTUI()
#define NOSO_TUI_WaitKeyPress()     _TEXTUI->WaitKeyPress()
#define NOSO_TUI_HandleEventLoop()  _TEXTUI->HandleEventLoop()
#define NOSO_TUI_OutputHeadPad( msg )   _TEXTUI->OutputHeadPad( (msg) )
#define NOSO_TUI_OutputHeadWin()        _TEXTUI->OutputHeadWin()
#define NOSO_TUI_OutputHistPad( msg )   _TEXTUI->OutputHistPad( (msg) )
#define NOSO_TUI_OutputHistWin()        _TEXTUI->OutputHistWin()
#define NOSO_TUI_OutputInfoPad( msg )   _TEXTUI->OutputInfoPad( (msg) )
#define NOSO_TUI_OutputInfoWin()        _TEXTUI->OutputInfoWin()
#define NOSO_TUI_OutputStatPad( msg )   _TEXTUI->OutputStatPad( (msg) )
#define NOSO_TUI_OutputStatWin()        _TEXTUI->OutputStatWin()
#define NOSO_TUI_OutputActiWinDefault()             _TEXTUI->OutputActiWinDefault()
#define NOSO_TUI_OutputActiWinBlockNum( param )     _TEXTUI->OutputActiWinBlockNum( (param) )
#define NOSO_TUI_OutputActiWinLastHash( param )     _TEXTUI->OutputActiWinLastHash( (param) )
#define NOSO_TUI_OutputActiWinMiningMode( param )   _TEXTUI->OutputActiWinMiningMode( (param) )
#define NOSO_TUI_OutputActiWinMiningDiff( param )   _TEXTUI->OutputActiWinMiningDiff( (param) )
#define NOSO_TUI_OutputActiWinAcceptedSol( param )  _TEXTUI->OutputActiWinAcceptedSol( (param) )
#define NOSO_TUI_OutputActiWinRejectedSol( param )  _TEXTUI->OutputActiWinRejectedSol( (param) )
#define NOSO_TUI_OutputActiWinFailuredSol( param )  _TEXTUI->OutputActiWinFailuredSol( (param) )
#define NOSO_TUI_OutputActiWinTillBalance( param )  _TEXTUI->OutputActiWinTillBalance( (param) )
#define NOSO_TUI_OutputActiWinTillPayment( param )  _TEXTUI->OutputActiWinTillPayment( (param) )
#define NOSO_TUI_OutputActiWinMiningSource( param ) _TEXTUI->OutputActiWinMiningSource( (param) )
static std::ofstream _NOSO_LOGGING_OFS;
static CLogFile _NOSO_LOGGING_LOGSTREAM( _NOSO_LOGGING_OFS );
#define NOSO_LOG_INIT() _NOSO_LOGGING_OFS.open( DEFAULT_LOGGING_FILENAME )
#else // OF #ifndef NO_TEXTUI
#define NOSO_TUI_StartTUI()         ((void)0)
#define NOSO_TUI_WaitKeyPress()     ((void)0)
#define NOSO_TUI_HandleEventLoop()  (0)
#define NOSO_TUI_OutputHeadPad( msg )   ((void)0)
#define NOSO_TUI_OutputHeadWin()        ((void)0)
#define NOSO_TUI_OutputHistPad( msg )   ((void)0)
#define NOSO_TUI_OutputHistWin()        ((void)0)
#define NOSO_TUI_OutputInfoPad( msg )   NOSO_STDOUT << (msg) << std::endl
#define NOSO_TUI_OutputInfoWin()        ((void)0)
#define NOSO_TUI_OutputStatPad( msg )   ((void)0)
#define NOSO_TUI_OutputStatWin()        ((void)0)
#define NOSO_TUI_OutputActiWinDefault()             ((void)0)
#define NOSO_TUI_OutputActiWinBlockNum( param )     ((void)0)
#define NOSO_TUI_OutputActiWinLastHash( param )     ((void)0)
#define NOSO_TUI_OutputActiWinMiningMode( param )   ((void)0)
#define NOSO_TUI_OutputActiWinMiningDiff( param )   ((void)0)
#define NOSO_TUI_OutputActiWinAcceptedSol( param )  ((void)0)
#define NOSO_TUI_OutputActiWinRejectedSol( param )  ((void)0)
#define NOSO_TUI_OutputActiWinFailuredSol( param )  ((void)0)
#define NOSO_TUI_OutputActiWinTillBalance( param )  ((void)0)
#define NOSO_TUI_OutputActiWinTillPayment( param )  ((void)0)
#define NOSO_TUI_OutputActiWinMiningSource( param ) ((void)0)
static CLogFile _NOSO_LOGGING_LOGSTREAM( std::cout );
#define NOSO_LOG_INIT() ((void)0)
#endif // OF #ifndef NO_TEXTUI ... #else
#define _LOG_FATAL CLogEntry<CLogFile>( _NOSO_LOGGING_LOGSTREAM ).GetStream<CLogLevel::FATAL>()
#define _LOG_ERROR CLogEntry<CLogFile>( _NOSO_LOGGING_LOGSTREAM ).GetStream<CLogLevel::ERROR>()
#define _LOG_WARN  CLogEntry<CLogFile>( _NOSO_LOGGING_LOGSTREAM ).GetStream<CLogLevel::WARN >()
#define _LOG_INFO  CLogEntry<CLogFile>( _NOSO_LOGGING_LOGSTREAM ).GetStream<CLogLevel::INFO >()
#define _LOG_DEBUG CLogEntry<CLogFile>( _NOSO_LOGGING_LOGSTREAM ).GetStream<CLogLevel::DEBUG>()
#define NOSO_LOG_INFO  _LOG_INFO  << "(" << std::setfill( '0' ) << std::setw( 3 ) << NOSO_BLOCK_AGE << ")"
#define NOSO_LOG_WARN  _LOG_WARN  << "(" << std::setfill( '0' ) << std::setw( 3 ) << NOSO_BLOCK_AGE << ")"
#define NOSO_LOG_ERROR _LOG_ERROR << "(" << std::setfill( '0' ) << std::setw( 3 ) << NOSO_BLOCK_AGE << ")"
#define NOSO_LOG_FATAL _LOG_FATAL << "(" << std::setfill( '0' ) << std::setw( 3 ) << NOSO_BLOCK_AGE << ")"
#define NOSO_LOG_DEBUG _LOG_DEBUG << "(" << std::setfill( '0' ) << std::setw( 3 ) << NOSO_BLOCK_AGE << ")"
#endif // __NOSO2M_OUTPUT_HPP__
