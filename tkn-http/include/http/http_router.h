#ifndef TOKEN_ROUTER_H
#define TOKEN_ROUTER_H

#include <glog/logging.h>

#include "trie.h"
#include "http_common.h"

namespace token{
  namespace http{
    typedef std::unordered_map<std::string, std::string> ParameterMap;

    typedef void (*RouteHandlerFunction)(const ControllerPtr&, http::Session*, const RequestPtr&);

    class Route{
      friend class Router;
     private:
      ControllerPtr controller_;

      std::string path_;
      Method method_;
      RouteHandlerFunction handler_;
     public:
      Route():
        controller_(),
        path_(),
        method_(),
        handler_(){}
      Route(const ControllerPtr& controller, std::string path, const Method& method, RouteHandlerFunction handler):
        controller_(controller),
        path_(std::move(path)),
        method_(method),
        handler_(handler){}
      Route(const Route& route) = default;
      ~Route() = default;

      ControllerPtr GetController() const{
        return controller_;
      }

      std::string GetPath() const{
        return path_;
      }

      Method GetMethod() const{
        return method_;
      }

      RouteHandlerFunction GetHandler() const{
        return handler_;
      }

      RouteHandlerFunction& GetHandler(){
        return handler_;
      }

      Route& operator=(const Route& route) = default;
    };

#define FOR_EACH_HTTP_ROUTER_MATCH_STATUS(V) \
    V(Ok)                                    \
    V(NotFound)                              \
    V(NotSupported)

    class RouterMatch{
      friend class Router;
     public:
      enum Status{
#define DEFINE_STATUS(Name) k##Name,
        FOR_EACH_HTTP_ROUTER_MATCH_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
      };

      friend std::ostream& operator<<(std::ostream& stream, const Status& status){
        switch (status){
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
      std::string path_;
      Status status_;

      ParameterMap params_path_;
      ParameterMap params_query_;
      Route* route_;

      RouterMatch(std::string path,
                      const Status& status,
                      ParameterMap params_path,
                      ParameterMap params_query,
                      Route* route):
        path_(std::move(path)),
        status_(status),
        params_path_(std::move(params_path)),
        params_query_(std::move(params_query)),
        route_(route){}
      RouterMatch(std::string path, const Status& status):
        path_(std::move(path)),
        status_(status),
        params_path_(),
        params_query_(),
        route_(nullptr){}
     public:
      RouterMatch(const RouterMatch& match):
        path_(match.path_),
        status_(match.status_),
        params_path_(),
        params_query_(),
        route_(match.route_){}
      ~RouterMatch() = default;

      std::string GetPath() const{
        return path_;
      }

      Status GetStatus() const{
        return status_;
      }

      Route* GetRoute() const{
        return route_;
      }

      ParameterMap& GetPathParameters(){
        return params_path_;
      }

      ParameterMap GetPathParameters() const{
        return params_path_;
      }

      bool HasPathParameter(const std::string& name){
        auto pos = params_path_.find(name);
        return pos != params_path_.end();
      }

      std::string GetPathParameterValue(const std::string& name){
        return params_path_[name];
      }

      ParameterMap& GetQueryParameters(){
        return params_query_;
      }

      ParameterMap GetQueryParameters() const{
        return params_query_;
      }

      bool HasQueryParameter(const std::string& name) const{
        auto pos = params_query_.find(name);
        return pos != params_query_.end();
      }

      std::string GetQueryParameterValue(const std::string& name){
        return params_query_[name];
      }

#define DEFINE_CHECK(Name) \
        bool Is##Name() const{ return GetStatus() == Status::k##Name; }
      FOR_EACH_HTTP_ROUTER_MATCH_STATUS(DEFINE_CHECK)
#undef DEFINE_CHECK

      RouterMatch& operator=(const RouterMatch& match) = default;

      friend std::ostream& operator<<(std::ostream& stream, const RouterMatch& match){
        return stream << "HttpRouterMatch(" << match.GetPath() << ", " << match.GetStatus() << ")";
      }
    };

    static const size_t kAlphabetSize = 128;//TODO: fix allocation
    class Router : Trie<char, Route, kAlphabetSize>{
    private:
      static const int64_t kPathParameter = 126;

      class Node : public TrieNode<char, Route, kAlphabetSize>{
      protected:
        std::string path_;
      public:
        Node():
          TrieNode<char, Route, kAlphabetSize>(),
          path_(){}
        Node(const char& key, const Route& route):
          TrieNode<char, Route, kAlphabetSize>(key, new Route(route)){}
        ~Node() override = default;

        std::string GetPath() const{
          return path_;
        }

        virtual bool IsPathParameter() const{
          return false;
        }
      };

      class PathParameterNode : public Node{
      public:
        explicit PathParameterNode(const Route& route):
          Node(':', route){}
        PathParameterNode(const ControllerPtr& controller, const std::string& path, const Method& method, const RouteHandlerFunction& handler):
          PathParameterNode(Route(controller, path, method, handler)){}
        ~PathParameterNode() override = default;

        bool IsPathParameter() const override{
          return true;
        }
      };

      static inline bool
      Insert(Node* node,
          const ControllerPtr& owner,
          const Method& method,
          const std::string& path,
          const RouteHandlerFunction& handler){
        Node* curr = node;
        for (auto i = path.begin(); i < path.end();){
          char c = *(i++);
          if (c == ':'){
            std::string name;
            do{
              name += (*i);
            } while ((++i) != path.cend() && (*i) != '/');

            auto new_node = new PathParameterNode(owner, name, method, handler);
            if(!curr->SetChild(kPathParameter, new_node)){
              LOG(ERROR) << "cannot create child @" << kPathParameter << " for route: " << name;
              return false;
            }
            curr = new_node;
          } else{
            int pos = c - 32;
            if (pos < 0){
              LOG(WARNING) << "invalid url character " << c << "(" << pos << ")";
              return false;
            }

            if (!curr->HasChild(pos)){
              if (!curr->SetChild(pos, new Node(c, Route(owner, std::string(1, c), method, handler)))){
                LOG(WARNING) << "couldn't create child @" << pos << " for " << c;
                return false;
              }
            }
            curr = (Node*) curr->GetChild(pos);
          }
        }
        return true;
      }

      static inline RouterMatch
      Search(Node* node, const Method& method, const std::string& path){
        ParameterMap path_params;
        ParameterMap query_params;

        Node* curr = node;
        for (auto i = path.begin(); i < path.end();){
          char c = *(i++);
          if(curr->HasChild(kPathParameter)){
            curr = (Node*)curr->GetChild(kPathParameter);

            std::string value(1, c);
            do{
              value += (*i);
            } while ((++i) != path.end() && (*i) != '/' && (*i) != '?');

            auto route = curr->GetValue();
            path_params.insert({route->GetPath(), value});
          } else if (c == '?'){
            do{
              if ((*i) == '&')
                i++;

              std::string key;
              do{
                key += (*i);
              } while ((++i) != path.end() && (*i) != '=' && (*i) != '/');

              i++;

              std::string value;
              do{
                value += (*i);
              } while ((++i) != path.end() && (*i) != '&' && (*i) != '/');

              query_params.insert({key, value});
            } while (i != path.end() && (*i) == '&' && (*i) != '/');
            break;
          } else{
            int pos = c - 32;
            if (!curr->HasChild(pos)){
              LOG(WARNING) << "cannot find child " << c << "@" << pos << " (parent: " << curr->GetKey() << ")";
              return RouterMatch(path, RouterMatch::kNotFound);
            }
            curr = (Node*) curr->GetChild(pos);
          }
        }

        //TODO: investigate problems routing:
        // - /pool/blocks

        auto route = curr->GetValue();
        if (route->GetMethod() != method)
          return RouterMatch(path, RouterMatch::kNotSupported);
        return RouterMatch(path, RouterMatch::kOk, path_params, query_params, route);
      }
    public:
      Router() = default;
      ~Router() = default;

      void Get(const ControllerPtr& owner, const std::string& path, const RouteHandlerFunction& handler){
        Insert((Node*)GetRoot(), owner, Method::kGet, path, handler);
      }

      void Put(const ControllerPtr& owner, const std::string& path, const RouteHandlerFunction& handler){
        Insert((Node*)GetRoot(), owner, Method::kPut, path, handler);
      }

      void Post(const ControllerPtr& owner, const std::string& path, const RouteHandlerFunction& handler){
        Insert((Node*)GetRoot(), owner, Method::kPost, path, handler);
      }

      void Delete(const ControllerPtr& owner, const std::string& path, const RouteHandlerFunction& handler){
        Insert((Node*)GetRoot(), owner, Method::kDelete, path, handler);
      }

      RouterMatch Find(const RequestPtr& request);
    };
  }
}

#endif //TOKEN_ROUTER_H