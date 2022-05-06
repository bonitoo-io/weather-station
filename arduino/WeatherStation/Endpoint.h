#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <ESPAsyncWebServer.h>
#include <map>

struct static_params {
  const char *contentType;
  const uint8_t* content;
  size_t len;
};

struct route {
  static_params *params;      
};

//auto cmpLambda = [](const char *a, const char *b){ return strcmp(a,b)<0; };
//typedef std::map<const char *, route *, decltype(cmpLambda)> routeMap;
struct comparator {
    bool operator()(const char * lhs, const char * rhs) const {
        return strcmp(lhs, rhs) < 0;
    }
};

typedef std::map<const char *, route *, comparator> routeMap;

typedef std::function<void(AsyncWebServerRequest *request, route *r)> GetRequestHandler;
typedef std::function<void(AsyncWebServerRequest *request, JsonVariant &json, route *r)> PostRequestHandler;

struct get_route: route {
  GetRequestHandler handler;
};

struct post_route: route {
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