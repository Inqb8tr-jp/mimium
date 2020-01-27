#include "runtime/scheduler/scheduler.hpp"

namespace mimium {

bool Scheduler::Greater::operator()(const key_type &l, const key_type &r)const{
      return l.first > r.first;
}

bool Scheduler::incrementTime() {
  bool res = false;
  bool hastask = !tasks.empty();
  if (!hastask && !runtime->hasDsp()) {
    res = true;
  } else {
    time += 1;
    if (hastask && time > tasks.top().first) {
      executeTask(tasks.top().second);
    }
  }
  return res;
};
void Scheduler::addTask(double time, void* addresstofn, double arg,
                        void* addresstocls) {
  tasks.emplace(static_cast<int64_t>(time),
                TaskType{addresstofn, arg, addresstocls});
}

void Scheduler::executeTask(const TaskType& task) {
  // todo
  auto& [addresstofn, arg, addresstocls] = task;

  if (addresstocls == nullptr) {
    auto fn = reinterpret_cast<void (*)(double)>(addresstofn);
    fn(arg);
  } else {
    auto fn = reinterpret_cast<void (*)(double, void*)>(addresstofn);
    fn(arg, addresstocls);
  }
  tasks.pop();
  if (tasks.empty() && !runtime->hasDsp()) {
    stop();
  } else {
    if (time > tasks.top().first) {
      this->executeTask(tasks.top().second);
    }
  }
}
void Scheduler::haltRuntime(){
  isactive = false;
  {
    std::lock_guard<std::mutex> lock(waitc.mtx);
    waitc.isready = true;
  }
  waitc.cv.notify_all();  // notify to exit runtime;
}

void Scheduler::addAudioDriver(std::shared_ptr<AudioDriver> a){
  this->audio = a;
}


void Scheduler::start() { audio->start(); }

void Scheduler::stop() {
  audio->stop();
  //cannnot call haltRuntime()???
  isactive = false;
  {
    std::lock_guard<std::mutex> lock(waitc.mtx);
    waitc.isready = true;
  }
  waitc.cv.notify_all();  // notify to exit runtime;
}
void Scheduler::setDsp(DspFnType fn, void *cls){
    audio->setDspFn(fn,cls);
  }

// SchedulerSndFile::SchedulerSndFile(
//     std::shared_ptr<LLVMRuntime> runtime_i, WaitController& waitc)
//     : Scheduler(runtime_i, waitc) {
//   sfinfo.channels = 2;
//   sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);
//   sfinfo.samplerate = 48000;
//   sfinfo.frames = 20 * 48000;  // temporary 20sec
//   buffer.reserve(sfinfo.channels * sfinfo.frames);
// }

// void SchedulerSndFile::start() {
//   fp = sf_open("temp.wav", SFM_WRITE, &sfinfo);
//   if (fp != nullptr) {
//     std::runtime_error("opening file failed");
//   };
//   for (int i = 0; i < 20 * 48000; i++) {
//     incrementTime();
//     for (int chan = 0; chan < sfinfo.channels; chan++) {
//       if (chan % 2) {
//         //                 buffer[sfinfo.channels*i+chan] =
//         // std::get<double>(interpreter->findVariable("dacL"));
//       } else {
//         //            buffer[sfinfo.channels*i+ chan] =
//         // std::get<double>(interpreter->findVariable("dacR"));
//       }
//     }
//   }
//   auto res = sf_writef_double(fp, buffer.data(), 20 * 48000);
//   std::cout << res << std::endl;
//   stop();
// }

// void SchedulerSndFile::stop() {
//   if (sf_close(fp) != 0) {
//     throw std::runtime_error("File is not correctly closed");
//   } else {
//     Logger::debug_log("File is closed", Logger::INFO);
//     std::exit(0);
//   }
// }

}  // namespace mimium