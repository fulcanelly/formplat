#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <memory>
#include "./executors.h"

const char* ssid = "***REMOVED***";
const char* password = "***REMOVED***";

WiFiServer server(80);
service service_;





struct associated_state;

using state_t = std::shared_ptr<associated_state>;


using states_t = std::vector< std::shared_ptr<associated_state> >;

struct associated_state {


  virtual bool is_alive() = 0;
  virtual state_t next() = 0;
  
};

struct state_writter : associated_state {
  String to_write;
  WiFiClient client;

  state_writter(String to_write, WiFiClient client): to_write(to_write), client(client) {}

  bool sent = false;
  
  virtual bool is_alive() override {
    return client.connected() and not sent;
  }

  virtual state_t next() {
    Serial.println(__PRETTY_FUNCTION__);

    if (not sent) {
      client.println(to_write);
      sent = true;
    }
    return nullptr;
  }
};

struct loop_state : associated_state {

  state_t captured;

  state_t restart;
  WiFiClient client;

  bool end;

  loop_state(state_t captured, state_t restart, WiFiClient client, bool end = false): 
      captured(captured), restart(restart), client(client), end(end) {
  }

  virtual bool is_alive() override {
    return captured->is_alive();
  }

  virtual state_t next() override {
    Serial.println(__PRETTY_FUNCTION__);

    auto res = captured->next();
    Serial.printf("ptr is %p\n", res);

   // Serial.printf("ptr is %p\n", res.get());
    if (res) {
      return std::make_shared<loop_state>(res, restart, client);
    } else if (not end) {
      return std::make_shared<loop_state>(restart, restart, client);
    } else {
      return nullptr;
    }
    /*
    if (res) {
      return state_t { 
        new loop_state { res, restart }
      };
    } else {
      return state_t { 
        new loop_state { restart, restart }
      };
    }*/
  }

};

struct start_state : associated_state {
  WiFiClient client;

  start_state(WiFiClient client): client(client) {}

  virtual bool is_alive() override {
    return client.connected();
  }

  associated_state* clean_next() {

    auto avaliable = client.available();
    Serial.println(__PRETTY_FUNCTION__);

    if (avaliable) {
      Serial.printf("avalibale: %d\n", avaliable);
      auto str = client.readStringUntil('\n');
      Serial.printf("got: '%s'\n", str.c_str());

      return new state_writter { str, client };
    } else {
      return this;
    }
  }

  virtual state_t next() override {
    Serial.println(__PRETTY_FUNCTION__);
    return state_t { clean_next() };
  }

};

template<class T>
struct callback {

};



struct state_reader : associated_state {
  WiFiClient client;
  state_t callback;
  
  virtual bool is_alive() override {
    return client.connected();
  }

  virtual state_t next() {
    Serial.println(__PRETTY_FUNCTION__);
    return nullptr;
  }
};


std::vector< std::shared_ptr<associated_state> > states;

states_t filter(states_t &input) {
  states_t result;
  for (auto state : input) {
    if (state->is_alive()) {
      result.push_back(state);
    }
  }
  return result;
}

states_t handle(states_t &&input) {
  if (input.size()) {
    Serial.printf("\n\n");
  }
  states_t result;
  for (auto state : input) {
    auto next = state->next();

    if (next) {
      result.push_back(next);
    }
  }
  return result;
}



void connect_to_wifi() {

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  connect_to_wifi();
  server.begin();

  task::every(3000)
    .run([] {
      Serial.printf("states count: %d\n", states.size());
    })
    .on(service_);
  
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    auto start = std::make_shared<start_state>(client);
    states.push_back(state_t { 
      new loop_state {
        start, start, client
      }
    });
  }

  states = handle(filter(states));

  service_.run();
   
}