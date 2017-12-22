#pragma once

#include <memory>
#include <fstream>

using namespace std;

enum nes_tracer_level : uint8_t
{
    nes_tracer_level_quiet = 0,    
    nes_tracer_level_minimal = 1, 
    nes_tracer_level_normal = 2,
    nes_tracer_level_detail = 3,
    nes_tracer_level_diag = 4,
    nes_tracer_level_debug = 5,         // like diag, but only exist in debug
};

class nes_tracer
{
public :
    nes_tracer()
    {

#ifdef _DEBUG
        _level = nes_tracer_level_detail;
#else
        _level = nes_tracer_level_minimal;
#endif
    }

    void init(const char *filename)
    {
        if (_stream)
        {
            _stream->close();
        }
        else
        {
            _stream = std::make_unique<ofstream>();
        }

        _file_name = filename;
        _stream->open(_file_name);
    }

    ~nes_tracer()
    {
        if (_stream)
        {
            _stream->close();
            _stream = nullptr;
        }
    }

    void set_level(nes_tracer_level level)
    {
        _level = level;
    }

    bool is_enabled(nes_tracer_level level)
    {
        return (level <= _level);
    }

    void trace(string str)
    {
        if (_stream)
            _stream->write(str.c_str(), str.size());
    }

    void trace(const char *str)
    {
        if (_stream)
            _stream->write(str, strlen(str));
    }

    static nes_tracer &get()
    {
        static nes_tracer s_trace;
        return s_trace;
    }

    ofstream &stream()
    {
        return *_stream;
    }

private :
    string _service_name;
    string _file_name;
    unique_ptr<ofstream> _stream;

    nes_tracer_level _level;            // current level of tracing
};

static ostream& operator <<(ostream &os, const string &str)
{
    os << str.c_str();
    return os;
}

#define INIT_TRACE(filename) nes_tracer::get().init(filename);
#define INIT_TRACE_LEVEL(filename, level) { nes_tracer::get().init(filename); nes_tracer::get().set_level(level); }

#define INIT_TRACE_DIAG(filename) { nes_tracer::get().init(filename); nes_tracer::get().set_level(nes_tracer_level_diag); }
#define INIT_TRACE_DEBUG(filename) { nes_tracer::get().init(filename); nes_tracer::get().set_level(nes_tracer_level_debug); }

// No need to flush - endl automatically flushes  
#define NES_LOG(expr) nes_tracer::get().stream() << expr << endl;
#define NES_LOG_IF(level, expr) if (nes_tracer::get().is_enabled(level)) { nes_tracer::get().stream() << expr << endl; }

#define NES_TRACE0(expr) NES_LOG_IF(nes_tracer_level_quiet, expr);
#define NES_TRACE1(expr) NES_LOG_IF(nes_tracer_level_minimal, expr); 
#define NES_TRACE2(expr) NES_LOG_IF(nes_tracer_level_normal, expr);
#define NES_TRACE3(expr) NES_LOG_IF(nes_tracer_level_detail, expr);
#define NES_TRACE4(expr) NES_LOG_IF(nes_tracer_level_diag, expr);

#ifdef _DEBUG
#define NES_DBG(expr) NES_LOG_IF(nes_tracer_level_debug, expr); 
#else
#define NES_DBG(expr) ;
#endif