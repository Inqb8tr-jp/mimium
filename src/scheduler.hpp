#pragma once
#include <map>
#include <memory>
#include <utility>

#include "ast.hpp"
#include "interpreter_visitor.hpp"
#include "audiodriver.hpp"
#include "helper_functions.hpp"

namespace mimium{
class InterpreterVisitor; //forward
class Scheduler :public std::enable_shared_from_this<Scheduler>{

    public:
    struct CallbackData{
        Scheduler* scheduler;
        std::shared_ptr<InterpreterVisitor> interpreter;
        CallbackData():scheduler(),interpreter(){};
    };
    explicit Scheduler(InterpreterVisitor* itp): time(0),nexttask_time(0),interpreter(itp),audio(){
        userdata.scheduler=this;
        userdata.interpreter=interpreter;
    };
    Scheduler(Scheduler& sch)=default;//copy
    Scheduler(Scheduler&& sch)=default;//move
    Scheduler &operator=(const Scheduler&)=default;
    Scheduler &operator=(Scheduler&&)=default;

    virtual ~Scheduler(){};

    void start();
    void stop();
    void incrementTime();
    void addTask(int time,AST_Ptr fn);
    inline int64_t getTime(){return time;}
    static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,double streamTime, RtAudioStreamStatus status, void* userdata);
    private:
    int64_t time;
    int nexttask_time;
    std::multimap<int, AST_Ptr> tasks;
    std::multimap<int, AST_Ptr>::iterator current_task_index;
    std::shared_ptr<InterpreterVisitor> interpreter;
    AudioDriver audio;
    CallbackData userdata;
    void executeTask();
};

};