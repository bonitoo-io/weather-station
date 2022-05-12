#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <ESPAsyncWebServer.h>
#include <map>

struct static_params {
  const char *contentType;
  const uint8_t* content;
  size_t len;
};

class route {
public:
  static_params *params = nullptr;      
  virtual ~route() { delete params; }
};

struct comparator {
    bool operator()(const char * lhs, const char * rhs) const {
        return strcmp(lhs, rhs) < 0;
    }
};

typedef std::map<const char *, route *, comparator> routeMap;

typedef std::function<void(AsyncWebServerRequest *request, route *r)> GetRequestHandler;
typedef std::function<void(AsyncWebServerRequest *request, JsonVariant &json, route *r)> PostRequestHandler;

class get_route: public route {
public:  
  GetRequestHandler handler;
};

class post_route: public route {
public:
  PostRequestHandler handler;
};

class EndpointRegistrator {
public:
  virtual void registerGetHandler(const char *uri, GetRequestHandler handler) = 0;
  virtual void registerDeleteHandler(const char *uri, GetRequestHandler handler) = 0;
  virtual void registerPostHandler(const char *uri, PostRequestHandler handler) = 0;
};

class Endpoint {
  public:
    virtual void registerEndpoints(EndpointRegistrator *pRegistrator) = 0;
};


#endif //ENDPOINT