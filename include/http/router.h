#ifndef TOKEN_ROUTER_H
#define TOKEN_ROUTER_H

#include <unordered_map>
#include "http/session.h"
#include "http/request.h"

namespace Token{
  typedef void (* HttpRouteHandler)(HttpSession*, HttpRequest*);

  class HttpRoute{
    friend class HttpRouter;
   private:
    std::string path_;
    HttpMethod method_;
    HttpRouteHandler handler_;

    HttpRoute():
      path_(),
      method_(),
      handler_(){}
    HttpRoute(const HttpMethod& method, const HttpRouteHandler& handler):
      path_(),
      method_(method),
      handler_(handler){}
   public:
    ~HttpRoute() = default;

    std::string GetPath() const{
      return path_;
    }

    HttpMethod GetMethod() const{
      return method_;
    }

    HttpRouteHandler GetHandler() const{
      return handler_;
    }
  };

#define FOR_EACH_HTTP_ROUTER_MATCH_STATUS(V) \
    V(Ok)                                    \
    V(NotFound)                              \
    V(MethodNotSupported)

  class HttpRouterMatch{
    friend class HttpRouter;
   public:
    enum Status{
#define DEFINE_STATUS(Name) k##Name,
      FOR_EACH_HTTP_ROUTER_MATCH_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_HTTP_ROUTER_MATCH_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }
   private:
    Status status_;
    ParameterMap parameters_;
    HttpRouteHandler handler_;

    HttpRouterMatch(const Status& status, const ParameterMap& params, const HttpRouteHandler& handler):
      status_(status),
      parameters_(params),
      handler_(handler){}
    HttpRouterMatch(const Status& status):
      status_(status),
      parameters_(),
      handler_(nullptr){}
   public:
    HttpRouterMatch(const HttpRouterMatch& match):
      status_(match.status_),
      parameters_(match.parameters_){}
    ~HttpRouterMatch() = default;

    Status GetStatus() const{
      return status_;
    }

    HttpRouteHandler& GetHandler(){
      return handler_;
    }

    HttpRouteHandler GetHandler() const{
      return handler_;
    }

    ParameterMap& GetParameters(){
      return parameters_;
    }

    ParameterMap GetParameters() const{
      return parameters_;
    }

    std::string GetParameterValue(const std::string& name){
      return parameters_[name];
    }

#define DEFINE_CHECK(Name) \
        bool Is##Name() const{ return GetStatus() == Status::k##Name; }
    FOR_EACH_HTTP_ROUTER_MATCH_STATUS(DEFINE_CHECK)
#undef DEFINE_CHECK

    void operator=(const HttpRouterMatch& match){
      status_ = match.status_;
      parameters_ = match.parameters_;
      handler_ = match.handler_;
    }
  };

  class HttpRouter{
   public:
    static const int64_t kAlphabetSize = 29;

    class Node{
      friend class HttpRouter;
     private:
      Node* parent_;
      Node* children_[kAlphabetSize];
      std::string key_;
      HttpRoute route_;

      Node():
        parent_(nullptr),
        children_(),
        key_(),
        route_(){
        for(int idx = 0; idx < kAlphabetSize; idx++)
          children_[idx] = nullptr;
      }
      Node(const HttpMethod& method, const HttpRouteHandler& handler, const std::string& part):
        parent_(nullptr),
        children_(),
        key_(part),
        route_(method, handler){
        for(int idx = 0; idx < kAlphabetSize; idx++)
          children_[idx] = nullptr;
      }
      Node(const HttpMethod& method, const HttpRouteHandler& handler, const char& c):
        parent_(nullptr),
        children_(),
        key_(1, c),
        route_(method, handler){
        for(int idx = 0; idx < kAlphabetSize; idx++)
          children_[idx] = nullptr;
      }
    };
   private:
    Node* root_;

    void Insert(Node* node, const HttpMethod& method, const std::string& path, const HttpRouteHandler& handler){
      Node* curr = node;
      for(auto i = path.begin(); i < path.end(); i++){
        char c = (*i);

        if(c == ':'){
          int pos = 27;
          i++;

          std::string name = "";
          while((*i) != '/' && i < path.cend()){
            name += (*i);
            i++;
          }

          if(!curr->children_[pos])
            curr->children_[pos] = new Node(method, handler, name);
          curr = curr->children_[pos];
        } else if(c == '/'){
          int pos = 28;
          if(!curr->children_[pos])
            curr->children_[pos] = new Node(method, handler, "/");
          curr = curr->children_[pos];
        } else{
          int pos = (int) tolower(c) - 'a';
          if(!curr->children_[pos])
            curr->children_[pos] = new Node(method, handler, c);
          curr = curr->children_[pos];
        }
      }
    }

    HttpRouterMatch Search(Node* node, const HttpMethod& method, const std::string& path){
      ParameterMap params;

      Node* curr = node;
      for(auto i = path.begin(); i < path.end(); i++){
        char c = (*i);
        if(curr->children_[27]){
          curr = curr->children_[27];

          std::string value = "";
          while(i != path.end()){
            if((*i) == '/')
              break;
            value += (*i);
            i++;
          }

          params.insert({curr->key_, value});
        } else if(curr->children_[28]){
          int pos = 28;
          if(!curr->children_[pos])
            return HttpRouterMatch(HttpRouterMatch::kNotFound);
          curr = curr->children_[pos];
        } else{
          int pos = (int) tolower(c) - 'a';
          if(!curr->children_[pos])
            return HttpRouterMatch(HttpRouterMatch::kNotFound);
          curr = curr->children_[pos];
        }
      }

      HttpRoute& route = curr->route_;
      if(route.GetMethod() != method)
        return HttpRouterMatch(HttpRouterMatch::kMethodNotSupported);
      return HttpRouterMatch(HttpRouterMatch::kOk, params, route.handler_);
    }

    Node* GetRoot() const{
      return root_;
    }
   public:
    HttpRouter():
      root_(new Node()){}
    HttpRouter(const HttpRouteHandler& default_handler):
      root_(new Node()){}
    ~HttpRouter() = default;

    void Get(const std::string& path, const HttpRouteHandler& handler){
      Insert(GetRoot(), HttpMethod::kGet, path, handler);
    }

    void Put(const std::string& path, const HttpRouteHandler& handler){
      Insert(GetRoot(), HttpMethod::kPut, path, handler);
    }

    void Post(const std::string& path, const HttpRouteHandler& handler){
      Insert(GetRoot(), HttpMethod::kPost, path, handler);
    }

    void Delete(const std::string& path, const HttpRouteHandler& handler){
      Insert(GetRoot(), HttpMethod::kDelete, path, handler);
    }

    HttpRouterMatch Find(HttpRequest* request){
      return Search(GetRoot(), request->GetMethod(), request->GetPath());
    }
  };
}

#endif //TOKEN_ROUTER_H