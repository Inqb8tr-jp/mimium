#pragma once
#include <map>
#include <memory>
#include <utility>

#include "ast.hpp"
#include "interpreter.hpp"
#include "audiodriver.hpp"
#include "helper_functions.hpp"

namespace mimium{
class Interpreter; //forward
class Scheduler :public std::enable_shared_from_this<Scheduler>{
    int64_t time;
    int nexttask_time;
    std::multimap<int, AST_Ptr> tasks;
    std::multimap<int, AST_Ptr>::iterator current_task_index;
    std::shared_ptr<Interpreter> interpreter;
    AudioDriver audio;
    public:
    struct CallbackData{
        Scheduler* scheduler;
        std::shared_ptr<Interpreter> interpreter;
        CallbackData():scheduler(),interpreter(){};
    };
    CallbackData userdata;
    Scheduler(Interpreter* itp): time(0),interpreter(itp),audio(){
        userdata.scheduler=this;
        userdata.interpreter=interpreter;
    };
    virtual ~Scheduler(){};

    void start();
    void stop();
    void incrementTime();
    void executeTask();
    void addTask(int time,AST_Ptr fn);
    inline int64_t getTime(){return time;}
    static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,double streamTime, RtAudioStreamStatus status, void* userdata);
};

};