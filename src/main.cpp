#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <memory>
#include <map>
#include <sstream>

#include "./executors.h"
#include "./matcher.h"

const char* ssid = "netis";
const char* password = "75246905";

WiFiServer server(80);
service service_;





struct associated_state;

using state_t = std::shared_ptr<associated_state>;


using states_t = std::vector< std::shared_ptr<associated_state> >;

struct associated_state {


  virtual bool is_alive() = 0;
  virtual state_t next() = 0;

  virtual ~associated_state() {};
  
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

    if (not sent) {
      client.print(to_write);
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

    auto res = captured->next();

    if (res) {
      return std::make_shared<loop_state>(res, restart, client);
    } else if (not end) {
      return std::make_shared<loop_state>(restart, restart, client);
    } else {
      return nullptr;
    }
  }

};

volatile bool state = 1;

struct start_state : associated_state {
  WiFiClient client;

  start_state(WiFiClient client): client(client) {}

  virtual bool is_alive() override {
    return client.connected();
  }

  String dispatch(register String& str) {
    std::stringstream stream;

    static auto matcher_ = matcher<String, void>(
      [](String a, String b) -> bool { return a.startsWith(b); })
        .when("freemem", [&] { 
          stream << ESP.getFreeHeap();
        })
        .when("toggle", [&] {
          digitalWrite(LED_BUILTIN, state = !state);
          stream << std::boolalpha << state;
        })
        .on_nokey([&] {
          stream << str.substring(0, str.length() - 2).c_str();
        });

    matcher_.apply(str);
    stream << '\n';

    return stream.str().c_str();

  } 

  associated_state* clean_next() {

    auto avaliable = client.available();

    if (avaliable) {
      auto str = client.readStringUntil('\n');

      return new state_writter { 
        dispatch(str), client 
      };
    } else {
      return new start_state {client};
    }
  }

  virtual state_t next() override {
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

void accept_connection() {
  WiFiClient client = server.available();
        
  if (client) {
    auto start = std::make_shared<start_state>(client);
    states.push_back(state_t { 
      new loop_state {
        start, start, client
      }
    });
  }    
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);
  connect_to_wifi();
  server.begin();

  task::every(1000)
    .run([] {
      Serial.printf("states count: %d\n", states.size());
      Serial.printf("free space %u\n", ESP.getFreeHeap());
      Serial.printf("CPU clock %u\n", ESP.getCpuFreqMHz());
      Serial.println();
    })
    .on(service_);
  
  task::every(100)
    .run(accept_connection)
    .on(service_);
  
  task::every(1)
    .run([] {
      states = handle(filter(states));
    })
    .on(service_);
}


void loop() {
  service_.run();  
}