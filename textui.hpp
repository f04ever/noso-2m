#ifndef __NOSO2M_TEXTUI_HPP__
#define __NOSO2M_TEXTUI_HPP__

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <mutex>
#include <thread>
#include <chrono>
#include <form.h>
#include <ncurses.h>

#include "util.hpp"
#include "tool.hpp"
#include "comm.hpp"
#include "noso-2m.hpp"

#ifndef KEY_ESC
#define KEY_ESC (27)
#endif
#ifndef KEY_CTRL
#define KEY_CTRL(c) ((c) & 037)
#endif

/*
|---------------------------------------------------| <-- logs
|BLOCK 012345 [599] 0123456789ABCDEF0123456789AB(32)| <-- acti
|BLOCK 012345       0123456789ABCDEF0123456789AB(32)| <-- logs
| Mode Sourceee(12) POOL/BESTDIFFFFFFFFFFFFFFFFF(32)| <-- acti/logs
| Solo Mainnet      BESTDIFFFFFFFFFFFFFFFFFFFFFF(32)| <-- acti/logs: solo
| Pool f04ever      POOLDIFFFFFFFFFFFFFFFFFFFFFF(32)| <-- acti/logs: pool
| Sent 01234 / 0123 / 012 | 12345.12345678 NOSO [30]| <-- acti/logs
| Hashrate(Miner/Pool/Mnet) 012.0M / 012.0M / 012.0G| <-- acti/logs: pool
| Hashrate(Miner/Pool/Mnet) 012.0M /    n/a /    n/a| <-- acti/logs: solo
| Computed COUNT hashes within DURATION(3) minutes  |
| Yay! win this block BLK_NO(6)                     | <-- logs: solo
| Won total NUM(3) blocks                           | <-- logs: solo
| Not win yet a block!                              | <-- logs: solo
| Paid AMOUNT(14.8) NOSO                            | <-- logs: pool
| ORDER_ID(52)                                      | <-- logs: pool
**/
extern std::vector<std::tuple<std::string, std::string, std::string>> g_mining_pools;
extern std::vector<std::tuple<std::uint32_t, double>> g_last_block_thread_hashrates;

class CTextUI { // A singleton pattern class
private:
    FORM * m_cmdl_frm {  NULL };
    FIELD * m_cmdl_fld[3] { NULL, NULL, NULL };
    struct __logs_pad_t {
        WINDOW * pad { NULL };
        int cols { 0 };
        int rows { 0 };
        int clen { 0 };
        int cpos { 0 };
    } * m_head_pad { NULL },
      * m_stat_pad { NULL },
      * m_acti_pad { NULL },
      * m_hist_pad { NULL },
      * m_info_pad { NULL },
      * m_logs_pad { NULL };
    const int m_page_rows { 8 };
    const int
        m_head_rows = { 5 },
        m_acti_rows = { 5 },
        m_cmdl_rows = { 1 },
        m_stat_rows = { 1 },
        m_main_xloc = { 0 },
        m_head_xloc = { m_main_xloc },
        m_logs_xloc = { m_main_xloc },
        m_acti_xloc = { m_main_xloc },
        m_cmdl_xloc = { m_main_xloc },
        m_stat_xloc = { m_main_xloc },
        m_main_yloc = { 0 },
        m_head_yloc = { m_main_yloc },
        m_logs_yloc = { m_head_yloc + m_head_rows + 1 };
    int m_main_cols = { COLS },
        m_head_cols = { COLS },
        m_logs_cols = { COLS },
        m_acti_cols = { COLS },
        m_cmdl_cols = { COLS },
        m_stat_cols = { COLS },
        m_main_rows = { LINES },
        m_logs_rows = { LINES - m_head_rows - 1 - m_acti_rows - 1 - m_cmdl_rows - m_stat_rows },
        m_acti_yloc = { m_head_yloc + m_head_rows + 1 + m_logs_rows + 1 },
        m_cmdl_yloc = { m_head_yloc + m_head_rows + 1 + m_logs_rows + 1 + m_acti_rows },
        m_stat_yloc = { m_head_yloc + m_head_rows + 1 + m_logs_rows + 1 + m_acti_rows + m_cmdl_rows };
    mutable std::mutex m_mutex_main_win;
    mutable std::mutex m_mutex_head_pad;
    mutable std::mutex m_mutex_logs_pad;
    mutable std::mutex m_mutex_acti_pad;
    mutable std::thread m_timer_thread;
    bool m_timer_running { false };
    void StartTimer() {
        static auto prev_age { NOSO_BLOCK_AGE };
        this->StopTimer();
        m_timer_running = true;
        m_timer_thread = std::thread( [&]() {
            while( m_timer_running ) {
                auto start_point { std::chrono::system_clock::now() };
                auto curr_age { NOSO_BLOCK_AGE };
                if ( curr_age != prev_age ) {
                    prev_age = curr_age;
                    this->OutputActiPadBlockAge( curr_age );
                    this->OutputActiWinBlockAge();
                }
                std::this_thread::sleep_until( start_point + std::chrono::milliseconds( 999 ) );
            }
        } ); };
    void StopTimer() {
        m_timer_running = false;
        if ( m_timer_thread.joinable() ) m_timer_thread.join();
    };
    void Startup() {
        if ( initscr() == NULL )
            throw std::runtime_error( "Error initialising ncurses terminal window" );
        if ( ( m_head_pad->pad = newpad( m_head_pad->rows, m_head_pad->cols ) ) == NULL )
            throw std::runtime_error( "Error initialising ncurses header pad" );
        if ( ( m_hist_pad->pad = newpad( m_hist_pad->rows, m_hist_pad->cols ) ) == NULL )
            throw std::runtime_error( "Error initialising ncurses history pad" );
        if ( ( m_info_pad->pad = newpad( m_info_pad->rows, m_info_pad->cols ) ) == NULL )
            throw std::runtime_error( "Error initialising ncurses information pad" );
        if ( ( m_acti_pad->pad = newpad( m_acti_pad->rows, m_acti_pad->cols ) ) == NULL )
            throw std::runtime_error( "Error initialising ncurses activity pad" );
        if ( ( m_stat_pad->pad = newpad( m_stat_pad->rows, m_stat_pad->cols ) ) == NULL )
            throw std::runtime_error( "Error initialising ncurses status pad" );
        scrollok( m_head_pad->pad, FALSE );
        scrollok( m_acti_pad->pad, FALSE );
        scrollok( m_stat_pad->pad, TRUE );
        scrollok( m_hist_pad->pad, TRUE );
        scrollok( m_info_pad->pad, TRUE );
        this->StartTimer();
    };
    void Cleanup() {
        this->StopTimer();
        this->RemoveWins();
        if ( m_info_pad->pad ) {
            delwin( m_info_pad->pad );
            m_info_pad->pad = NULL;
        }
        if ( m_hist_pad->pad ) {
            delwin( m_hist_pad->pad );
            m_hist_pad->pad = NULL;
        }
        if ( m_stat_pad->pad ) {
            delwin( m_stat_pad->pad );
            m_stat_pad->pad = NULL;
        }
        if ( m_acti_pad->pad ) {
            delwin( m_acti_pad->pad );
            m_acti_pad->pad = NULL;
        }
        if ( m_head_pad->pad ) {
            delwin( m_head_pad->pad );
            m_head_pad->pad = NULL;
        }
    };
    void _EnsureWinSizes() {
        m_main_cols = COLS;
        m_head_cols = COLS;
        m_logs_cols = COLS;
        m_acti_cols = COLS;
        m_cmdl_cols = COLS;
        m_stat_cols = COLS;
        m_main_rows = LINES;
        m_logs_rows = LINES - m_head_rows - 1 - m_acti_rows - 1 - m_cmdl_rows - m_stat_rows;
        m_acti_yloc = m_head_yloc + m_head_rows + 1 + m_logs_rows + 1;
        m_cmdl_yloc = m_head_yloc + m_head_rows + 1 + m_logs_rows + 1 + m_acti_rows;
        m_stat_yloc = m_head_yloc + m_head_rows + 1 + m_logs_rows + 1 + m_acti_rows + m_cmdl_rows;
    };
    void RemoveWins() {
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        endwin();
        if ( m_cmdl_frm ) {
            unpost_form( m_cmdl_frm );
            free_form( m_cmdl_frm );
            m_cmdl_frm = NULL;
        }
        for ( int i = 0; i < 2; i ++ ) {
            if ( m_cmdl_fld[i] ) {
                free_field( m_cmdl_fld[i] );
                m_cmdl_fld[i] = NULL;
            }
        }
    };
    void ResizeWins() {
        this->RemoveWins();
        this->CreateWins();
    };
    void OutputWins() {
        this->OutputMainWin();
        this->OutputHeadWin();
        this->OutputLogsWin();
        this->OutputActiWin();
        this->OutputCmdlWin();
        this->OutputStatWin();
    };
    void OutputActiPadBlockAge( std::uint32_t age ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        mvwprintw( m_acti_pad->pad, 0, 14, "%03u", age );
    }
    void OutputActiWinBlockAge() {
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        if ( m_acti_pad->pad == NULL ) return;
        prefresh( m_acti_pad->pad, 0, 14, m_acti_yloc + 1, m_acti_xloc + 14, m_acti_yloc + 0 + 1, m_acti_xloc + 14 + 03 );
    }
    void OutputCmdlWin() {
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        if ( m_cmdl_frm == NULL ) return;
        pos_form_cursor( m_cmdl_frm );
        form_driver( m_cmdl_frm, REQ_LAST_FIELD );
        form_driver( m_cmdl_frm, REQ_END_LINE );
    };
    void _ScrollLogsPadRows( int num_rows ) {
        if ( num_rows < 0 ) {
            if ( m_logs_pad->cpos > 0 ) m_logs_pad->cpos += num_rows;
            if ( m_logs_pad->cpos < 0 ) m_logs_pad->cpos = 0;
        }
        if ( num_rows > 0 ) {
            if ( m_logs_pad->cpos < m_logs_pad->clen - m_logs_rows ) m_logs_pad->cpos += num_rows;
            if ( m_logs_pad->cpos > m_logs_pad->clen - m_logs_rows ) m_logs_pad->cpos = m_logs_pad->clen - m_logs_rows;
            if ( m_logs_pad->cpos < 0 ) m_logs_pad->cpos = 0;
        }
    };
    void ScrollLogsPadRows( int num_rows ) {
        std::unique_lock<std::mutex> unique_lock_logs_pad( m_mutex_logs_pad );
        this->_ScrollLogsPadRows( num_rows );
    }
    void ScrollLogsPadPageUp() {
        std::unique_lock<std::mutex> unique_lock_logs_pad( m_mutex_logs_pad );
        unsigned int scroll_rows = m_logs_rows >= m_page_rows ? m_page_rows : m_logs_rows;
        this->_ScrollLogsPadRows( +scroll_rows );
    };
    void ScrollLogsPadPageDown() {
        std::unique_lock<std::mutex> unique_lock_logs_pad( m_mutex_logs_pad );
        unsigned int scroll_rows = m_logs_rows >= m_page_rows ? m_page_rows : m_logs_rows;
        this->_ScrollLogsPadRows( -scroll_rows );
    };
    void ScrollLogsPadHome() {
        std::unique_lock<std::mutex> unique_lock_logs_pad( m_mutex_logs_pad );
        m_logs_pad->cpos = m_logs_pad->clen - m_logs_rows;
        if ( m_logs_pad->cpos < 0 ) m_logs_pad->cpos = 0;
    };
    void ScrollLogsPadEnd() {
        std::unique_lock<std::mutex> unique_lock_logs_pad( m_mutex_logs_pad );
        m_logs_pad->cpos = 0;
    };
    void _ExtendLogsPadRows( struct __logs_pad_t * logs_pad, unsigned int num_rows ) {
        if ( logs_pad->clen < logs_pad->rows ) logs_pad->clen += num_rows;
        if ( logs_pad->clen > logs_pad->rows ) logs_pad->clen = logs_pad->rows;
        if ( logs_pad->clen > m_logs_rows ) logs_pad->cpos = logs_pad->clen - m_logs_rows;
        if ( logs_pad->cpos < 0 ) logs_pad->cpos = 0;
    }
    std::string command_string() {
        if ( m_cmdl_frm == NULL ) return "";
        form_driver( m_cmdl_frm, REQ_NEXT_FIELD );
        form_driver( m_cmdl_frm, REQ_PREV_FIELD );
        std::string cmdstr { field_buffer( m_cmdl_fld[1], 0 ) };
        cmdstr = rtrim( ltrim( cmdstr ) );
        return cmdstr;
    };
    int command() {
        std::string cmdstr = command_string();
        if ( m_cmdl_frm ) {
            pos_form_cursor( m_cmdl_frm );
            form_driver( m_cmdl_frm, REQ_LAST_FIELD );
            form_driver( m_cmdl_frm, REQ_END_LINE );
        }
        if ( iequal( "exit", cmdstr ) ) return ( -1 );
        if ( cmdstr == "" ) {
            this->ToggleLogsPad();
            this->OutputLogsWin();
        } else if ( iequal( "help",     cmdstr ) ) {
            this->OutputInfoPad( "Supported commands:" );
            this->OutputInfoPad( "" );
            this->OutputInfoPad( "  threads - Show hashrate per thread last mining block" );
            this->OutputInfoPad( "  pools   - Show information of configured pools" );
            this->OutputInfoPad( "  nodes   - Show current mining nodes" );
            this->OutputInfoPad( "  help    - Show this list of supported commands" );
            this->OutputInfoPad( "  exit    - Exit noso-2m, same as Ctrl+C" );
            this->OutputInfoPad( "" );
            this->OutputInfoPad( "--" );
            this->OutputInfoWin();
        } else if ( iequal( "nodes",    cmdstr ) ) {
            this->OutputStatPad( "Showing nodes information" );
            this->OutputStatWin();
            CTools::ShowNodeInformation( CCommThread::GetInstance()->GetMiningNodes() );
        } else if ( iequal( "pools",    cmdstr ) ) {
            this->OutputStatPad( "Showing pools information" );
            this->OutputStatWin();
            CTools::ShowPoolInformation( g_mining_pools );
        } else if ( iequal( "threads",  cmdstr ) ) {
            this->OutputStatPad( "Showing hashrate per thread" );
            this->OutputStatWin();
            CTools::ShowThreadHashrates( g_last_block_thread_hashrates );
        } else {
            this->OutputStatPad( ( "Unknown command '" + cmdstr + "'!" ).c_str() );
            this->OutputStatWin();
        }
        set_field_buffer( m_cmdl_fld[1], 0, "" );
        form_driver( m_cmdl_frm, REQ_END_LINE );
        return ( 0 );
    };
private:
    CTextUI() {
        m_head_pad = new __logs_pad_t { NULL, 300, 005, 0, 0 };
        m_stat_pad = new __logs_pad_t { NULL, 300, 001, 0, 0 };
        m_acti_pad = new __logs_pad_t { NULL, 300, 003, 0, 0 };
        m_hist_pad = new __logs_pad_t { NULL, 300, 500, 0, 0 };
        m_info_pad = new __logs_pad_t { NULL, 300, 100, 0, 0 };
        m_logs_pad = m_hist_pad;
        m_cmdl_fld[0] = NULL;
        m_cmdl_fld[1] = NULL;
        m_cmdl_fld[2] = NULL;
    };
public:
    CTextUI( const CTextUI& ) = delete; // Copy prohibited
    CTextUI( CTextUI&& ) = delete; // Move prohibited
    void operator=( const CTextUI& ) = delete; // Assignment prohibited
    CTextUI& operator=( CTextUI&& ) = delete; // Move assignment prohibited
    ~CTextUI() {
        this->Cleanup();
        delete m_head_pad;
        delete m_stat_pad;
        delete m_acti_pad;
        delete m_hist_pad;
        delete m_info_pad;
    }
    static std::shared_ptr<CTextUI> GetInstance() {
        static std::shared_ptr<CTextUI> singleton { new CTextUI() };
        return singleton;
    }
    void CreateWins() {
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        erase();
        this->_EnsureWinSizes();
        if ( ( m_cmdl_fld[0] = new_field( 1, 9,               m_cmdl_yloc, m_cmdl_xloc + 0, 0, 0 ) ) == NULL )
            throw std::runtime_error( "Error initialising ncurses command field[0]" );
        set_field_opts( m_cmdl_fld[0], O_PUBLIC | O_VISIBLE | O_STATIC | O_AUTOSKIP);
        set_field_back( m_cmdl_fld[0], A_STANDOUT | A_BOLD );
        set_field_buffer( m_cmdl_fld[0], 0, "COMMAND:" );
        if ( ( m_cmdl_fld[1] = new_field( 1, m_cmdl_cols - 9, m_cmdl_yloc, m_cmdl_xloc + 9, 0, 0 ) ) == NULL )
            throw std::runtime_error( "Error initialising ncurses command field[1]" );
        set_field_opts( m_cmdl_fld[1], O_PUBLIC | O_VISIBLE | O_EDIT | O_ACTIVE);
        set_field_back( m_cmdl_fld[1], A_STANDOUT | A_BOLD );
        set_field_buffer( m_cmdl_fld[1], 0, "help" );
        m_cmdl_fld[2] = NULL;
        if ( ( m_cmdl_frm = new_form( m_cmdl_fld ) ) == NULL )
            throw std::runtime_error( "Error initialising ncurses command form" );
        set_form_win( m_cmdl_frm, stdscr );
        set_form_sub( m_cmdl_frm, stdscr );
        post_form( m_cmdl_frm );
    };
    int HandleEventLoop(){
        raw();
        noecho();
        curs_set( 0 );
        leaveok( stdscr, true );
        keypad( stdscr, TRUE );
        notimeout( stdscr, TRUE );
        int key { 0 };
        do {
            key = getch();
            if ( key == KEY_RESIZE ) {
                this->ResizeWins();
                this->OutputWins();
            } else if ( key == KEY_UP ) {
                this->ScrollLogsPadRows( -1 );
                this->OutputLogsWin();
            } else if ( key == KEY_DOWN ) {
                this->ScrollLogsPadRows( +1 );
                this->OutputLogsWin();
            } else if ( key == KEY_NPAGE ) {
                this->ScrollLogsPadPageDown();
                this->OutputLogsWin();
            } else if ( key == KEY_PPAGE ) {
                this->ScrollLogsPadPageUp();
                this->OutputLogsWin();
            } else if ( key == KEY_END ) {
                this->ScrollLogsPadEnd();
                this->OutputLogsWin();
            } else if ( key == KEY_HOME ) {
                this->ScrollLogsPadHome();
                this->OutputLogsWin();
            } else if ( key == KEY_LEFT ) {
                if ( m_cmdl_frm ) form_driver( m_cmdl_frm, REQ_PREV_CHAR );
            } else if ( key == KEY_RIGHT ) {
                if ( m_cmdl_frm ) form_driver( m_cmdl_frm, REQ_NEXT_CHAR );
            } else if ( key == KEY_BACKSPACE || key == 127 ) {
                if ( m_cmdl_frm ) form_driver( m_cmdl_frm, REQ_DEL_PREV);
            } else if ( key == KEY_DC ) {
                if ( m_cmdl_frm ) form_driver( m_cmdl_frm, REQ_DEL_CHAR);
            } else if ( key == KEY_ENTER || key == 10 ) {
                if ( command() < 0 ) break;
            } else if ( key == KEY_ESC ) {
                if ( m_cmdl_fld[1] ) set_field_buffer( m_cmdl_fld[1], 0, "" );
            } else {
                if ( m_cmdl_frm ) form_driver( m_cmdl_frm, key );
            }
        } while( key != KEY_CTRL( 'c' ) );
        return key;
    };
    void OutputHeadPad( const char* out ) {
        std::unique_lock<std::mutex> unique_lock_head_pad( m_mutex_head_pad );
        if ( m_head_pad->pad == NULL ) return;
        wprintw( m_head_pad->pad, "%s\n", out );
    };
    void OutputLogsPad( const char* out, int num_rows=1 ) {
        std::unique_lock<std::mutex> unique_lock_logs_pad( m_mutex_logs_pad );
        if ( m_logs_pad->pad == NULL ) return;
        wprintw( m_logs_pad->pad, "%s\n", out );
        this->_ExtendLogsPadRows( m_logs_pad, num_rows );
    };
    void OutputHistPad( const char* out, int num_rows=1 ) {
        std::unique_lock<std::mutex> unique_lock_logs_pad( m_mutex_logs_pad );
        if ( m_hist_pad->pad == NULL ) return;
        wprintw( m_hist_pad->pad, "%s\n", out );
        this->_ExtendLogsPadRows( m_hist_pad, num_rows );
    };
    void OutputInfoPad( const char* out, int num_rows=1 ) {
        std::unique_lock<std::mutex> unique_lock_logs_pad( m_mutex_logs_pad );
        if ( m_info_pad->pad == NULL ) return;
        wprintw( m_info_pad->pad, "%s\n", out );
        this->_ExtendLogsPadRows( m_info_pad, num_rows );
    };
    void OutputActiPad( const char* out ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        wprintw( m_acti_pad->pad, "%s\n", out );
    };
    void OutputStatPad( const char* out ) {
        if ( m_stat_pad->pad == NULL ) return;
        wprintw( m_stat_pad->pad, "\n%s", out );
    };
    void OutputMainWin() {
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        mvhline( m_head_yloc + m_head_rows, m_head_xloc, 0, m_head_cols );
        mvhline( m_acti_yloc - 1, m_acti_xloc, 0, m_acti_cols );
        refresh();
    };
    void OutputHeadWin() {
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        if ( m_head_pad->pad == NULL ) return;
        prefresh( m_head_pad->pad, m_head_pad->cpos, 0, m_head_yloc, m_head_xloc, m_head_yloc + m_head_rows - 1, m_head_xloc + m_head_cols - 1 );
    };
    void OutputLogsWin() {
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        if ( m_logs_pad->pad == NULL ) return;
        prefresh( m_logs_pad->pad, m_logs_pad->cpos, 0, m_logs_yloc, m_logs_xloc, m_logs_yloc + m_logs_rows - 1, m_logs_xloc + m_logs_cols - 1 );
    };
    void OutputHistWin() {
        this->SwitchHistPad();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        if ( m_hist_pad->pad == NULL ) return;
        prefresh( m_hist_pad->pad, m_hist_pad->cpos, 0, m_logs_yloc, m_logs_xloc, m_logs_yloc + m_logs_rows - 1, m_logs_xloc + m_logs_cols - 1 );
    };
    void OutputInfoWin() {
        this->SwitchInfoPad();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        if ( m_info_pad->pad == NULL ) return;
        prefresh( m_info_pad->pad, m_info_pad->cpos, 0, m_logs_yloc, m_logs_xloc, m_logs_yloc + m_logs_rows - 1, m_logs_xloc + m_logs_cols - 1 );
    };
    void OutputActiWin() {
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        if ( m_acti_pad->pad == NULL ) return;
        prefresh( m_acti_pad->pad, m_acti_pad->cpos, 0, m_acti_yloc + 1, m_acti_xloc, m_acti_yloc + m_acti_rows - 1, m_acti_xloc + m_acti_cols - 1 );
    };
    void ResetActiPad() {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        werase( m_acti_pad->pad );
    };
    void ResetHeadPad() {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_head_pad->pad == NULL ) return;
        werase( m_head_pad->pad );
    }
    void OutputActiWinBlockNum( std::uint32_t blck_no ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        mvwprintw( m_acti_pad->pad, 0, 06, "%06u", blck_no );
        unique_lock_acti_pad.unlock();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        prefresh( m_acti_pad->pad, 0, 06, m_acti_yloc + 1, m_acti_xloc + 06, m_acti_yloc + 0 + 1, m_acti_xloc + 06 + 06 );
    }
    void OutputActiWinLastHash( const std::string& lb_hash ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        mvwprintw( m_acti_pad->pad, 0, 19, "%-32s", lb_hash.c_str() );
        unique_lock_acti_pad.unlock();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        prefresh( m_acti_pad->pad, 0, 19, m_acti_yloc + 1, m_acti_xloc + 19, m_acti_yloc + 0 + 1, m_acti_xloc + 19 + 32 );
    }
    void OutputActiWinMiningMode( bool solo ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        mvwprintw( m_acti_pad->pad, 1, 01, "%-4s", ( solo ? "Solo" : "Pool" ) );
        unique_lock_acti_pad.unlock();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        prefresh( m_acti_pad->pad, 1, 01, m_acti_yloc + 2, m_acti_xloc + 01, m_acti_yloc + 1 + 1, m_acti_xloc + 01 + 04 );
    }
    void OutputActiWinMiningSource( const std::string& source ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        mvwprintw( m_acti_pad->pad, 1, 06, "%-12s", source.c_str() );
        unique_lock_acti_pad.unlock();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        prefresh( m_acti_pad->pad, 1, 06, m_acti_yloc + 2, m_acti_xloc + 06, m_acti_yloc + 1 + 1, m_acti_xloc + 06 + 13 );
    }
    void OutputActiWinMiningDiff( const std::string& diff ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        mvwprintw( m_acti_pad->pad, 1, 19, "%-32s", diff.c_str() );
        unique_lock_acti_pad.unlock();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        prefresh( m_acti_pad->pad, 1, 19, m_acti_yloc + 2, m_acti_xloc + 19, m_acti_yloc + 1 + 1, m_acti_xloc + 19 + 32 );
    }
    void OutputActiWinAcceptedSol( std::uint32_t num ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        mvwprintw( m_acti_pad->pad, 2, 06, "%5u", num );
        unique_lock_acti_pad.unlock();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        prefresh( m_acti_pad->pad, 2, 06, m_acti_yloc + 3, m_acti_xloc + 06, m_acti_yloc + 2 + 1, m_acti_xloc + 06 + 05 );
    }
    void OutputActiWinRejectedSol( std::uint32_t num ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        mvwprintw( m_acti_pad->pad, 2, 14, "%4u", num );
        unique_lock_acti_pad.unlock();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        prefresh( m_acti_pad->pad, 2, 14, m_acti_yloc + 3, m_acti_xloc + 14, m_acti_yloc + 2 + 1, m_acti_xloc + 14 + 04 );
    }
    void OutputActiWinFailuredSol( std::uint32_t num ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        mvwprintw( m_acti_pad->pad, 2, 21, "%3u", num );
        unique_lock_acti_pad.unlock();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        prefresh( m_acti_pad->pad, 2, 21, m_acti_yloc + 3, m_acti_xloc + 21, m_acti_yloc + 2 + 1, m_acti_xloc + 21 + 03 );
    }
    void OutputActiWinTillBalance( std::uint32_t balance ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        mvwprintw( m_acti_pad->pad, 2, 27, "%14.8g", balance / 100'000'000.0 );
        unique_lock_acti_pad.unlock();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        prefresh( m_acti_pad->pad, 2, 27, m_acti_yloc + 3, m_acti_xloc + 27, m_acti_yloc + 2 + 1, m_acti_xloc + 27 + 14 );
    }
    void OutputActiWinTillPayment( std::uint32_t num ) {
        std::unique_lock<std::mutex> unique_lock_acti_pad( m_mutex_acti_pad );
        if ( m_acti_pad->pad == NULL ) return;
        mvwprintw( m_acti_pad->pad, 2, 48, "%2u", num );
        unique_lock_acti_pad.unlock();
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        prefresh( m_acti_pad->pad, 2, 48, m_acti_yloc + 3, m_acti_xloc + 48, m_acti_yloc + 2 + 1, m_acti_xloc + 48 + 02 );
    }
    void OutputHeadWinDefault() {
        this->ResetHeadPad();
        this->OutputHeadPad( "noso-2m - A miner for Nosocryptocurrency Protocol-2" );
        this->OutputHeadPad( "f04ever (c) 2022 https://github.com/f04ever/noso-2m" );
        this->OutputHeadPad( (std::string("version ") + NOSO_2M_VERSION).c_str() );
        this->OutputHeadPad( "--" );
        this->OutputHeadWin();
    }
    void OutputActiWinDefault() {
        this->ResetActiPad();
        this->OutputActiPad( "BLOCK ...... [...] ................................" );
        this->OutputActiPad( " .... ............ ................................" );
        this->OutputActiPad( " Sent ..... / .... / ... | .............. NOSO [..]" );
        this->OutputActiWin();
    }
    void OutputStatWin() {
        std::unique_lock<std::mutex> unique_lock_main_win( m_mutex_main_win );
        if ( m_stat_pad->pad == NULL ) return;
        prefresh( m_stat_pad->pad, m_stat_pad->cpos, 0, m_stat_yloc, m_stat_xloc, m_stat_yloc + m_stat_rows, m_stat_xloc + m_stat_cols - 1 );
    };
    void SwitchHistPad() {
        std::unique_lock<std::mutex> unique_lock_logs_pad( m_mutex_logs_pad );
        m_logs_pad = m_hist_pad;
    };
    void SwitchInfoPad() {
        std::unique_lock<std::mutex> unique_lock_logs_pad( m_mutex_logs_pad );
        m_logs_pad = m_info_pad;
    };
    void ToggleLogsPad() {
        std::unique_lock<std::mutex> unique_lock_logs_pad( m_mutex_logs_pad );
        if ( m_logs_pad == m_hist_pad ) m_logs_pad = m_info_pad;
        else m_logs_pad = m_hist_pad;
    }
    void WaitKeyPress() {
        this->OutputLogsPad( "Press any key to exit..." );
        this->OutputLogsWin();
        getch();
    };
    void StartTUI() {
        this->Startup();
        this->CreateWins();
        this->OutputMainWin();
        this->OutputHeadWinDefault();
        this->OutputLogsWin();
        this->OutputActiWinDefault();
        this->OutputCmdlWin();
        this->OutputStatWin();
    }
};

#endif // __NOSO2M_TEXTUI_HPP__
