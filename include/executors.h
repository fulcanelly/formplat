#include <vector>
#include <memory>
#include <Arduino.h>


struct timer_ {
  int interval;

  ulong last_run;

  void reset() {
    last_run = millis();
//    Serial.printf("current = %lu, last_run = %lu, interval = %d\n", millis(), last_run, interval);

  }

  bool is_ready() {
    auto current = millis();
    if (interval == -1) {
      return false;
    }
   // Serial.printf("current = %lu, last_run = %lu, interval = %d\n\n", current, last_run, interval);
    return current >= last_run + interval;
  }
};

struct service;

struct task {

  timer_ timer;
  std::function<void(void)> func;

  static task every(int ms) {
    return task {
      .timer = timer_ {
        .interval = ms,
      }
    };
  }

  task run(std::function<void(void)> func) {
    this->func = func;
    timer.reset();
    return *this;
  }

  void on(service& s);
  
  void exec() {
    func();
    timer.reset();

  }
};



struct service {
  std::vector<task> tasks;

  void add(task task) {
    tasks.push_back(task);
  }

  void run() {
    for (auto&& task : tasks) {
      if (task.timer.is_ready()) {
        task.exec();
      }
    }

  //  Serial.printf("delay\n");
    //delay(100);
  }
};


