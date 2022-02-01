#include <vector>
#include <memory>
#include <Arduino.h>

#include "./executors.h"


void task::on(service& s) {
  s.add(*this);
}
