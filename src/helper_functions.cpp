#include "helper_functions.hpp"
namespace mimium{

Logger::REPORT_LEVEL Logger::current_report_level = Logger::WARNING;
std::ostream* Logger::output = &std::cerr;

Logger::Logger(){
    setoutput(std::cerr);
    Logger::current_report_level = Logger::DEBUG;
}
Logger::Logger(std::ostream& out){
    setoutput(out);
    Logger::current_report_level = Logger::DEBUG;
}
void Logger::debug_log(const std::string& str, REPORT_LEVEL report_level){ //static!
    if(report_level<=Logger::current_report_level){
    *output << report_str.at(report_level) << ": " << str << norm << std::endl;
    }
    }
}//namespace mimium