#ifndef TOKEN_ROUTER_H
#define TOKEN_ROUTER_H

#include <glog/logging.h>

#include "trie.h"
#include "http/http_common.h"

namespace token{
  namespace http{
    typedef std::unordered_map<std::string, std::string> ParameterMap;

    typedef void (*RouteHandlerFunction)(const ControllerPtr&, const SessionPtr&, const RequestPtr&);

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

      Route route_;

      RouterMatch(std::string path,
                      const Status& status,
                      ParameterMap params_path,
                      ParameterMap params_query,
                      const Route& route):
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
          route_(){}
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

      Route& GetRoute(){
        return route_;
      }

      Route GetRoute() const{
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

#define FOR_EACH_URL_ENCODED_CHARACTER(V) \
  V(0, '\0', "%00")                       \
  V(1, ' ', "%20")                        \
  V(2, '!', "%21")                        \
  V(3, '"', "%22") \
  V(4, '#', "%23") \
  V(5, '$', "%24") \
  V(6, '%', "%25") \
  V(7, '&', "%26") \
  V(8, '\'', "%27") \
  V(9, '(', "%28") \
  V(10, ')', "%29") \
  V(11, '*', "%2A") \
  V(12, '+', "%2B") \
  V(13, ',', "%2C") \
  V(14, '-', "%2D") \
  V(15, '.', "%2E") \
  V(16, '/', "%2F") \
  V(17, '0', "%30") \
  V(18, '1', "%31") \
  V(19, '2', "%32") \
  V(20, '3', "%33") \
  V(21, '4', "%34") \
  V(22, '5', "%35") \
  V(23, '6', "%36") \
  V(24, '7', "%37") \
  V(25, '8', "%38") \
  V(26, '9', "%39") \
  V(27, ':', "%3A") \
  V(28, ';', "%3B") \
  V(29, '<', "%3C") \
  V(30, '=', "%3D") \
  V(31, '>', "%3E") \
  V(32, '?', "%3F") \
  V(33, '@', "%40") \
  V(34, 'A', "%41") \
  V(35, 'B', "%42") \
  V(36, 'C', "%43") \
  V(37, 'D', "%44") \
  V(38, 'E', "%45") \
  V(39, 'F', "%46") \
  V(40, 'G', "%47") \
  V(41, 'H', "%48") \
  V(42, 'I', "%49") \
  V(43, 'J', "%4A") \
  V(44, 'K', "%4B") \
  V(45, 'L', "%4C") \
  V(46, 'M', "%4D") \
  V(47, 'N', "%4E") \
  V(48, 'O', "%4F") \
  V(49, 'P', "%50") \
  V(50, 'Q', "%51") \
  V(51, 'R', "%52") \
  V(52, 'S', "%53") \
  V(53, 'T', "%54") \
  V(54, 'U', "%55") \
  V(55, 'V', "%56") \
  V(56, 'W', "%57") \
  V(57, 'X', "%58") \
  V(58, 'Y', "%59") \
  V(59, 'Z', "%5A") \
  V(60, '[', "%5B") \
  V(61, '\\', "%5C") \
  V(62, ']', "%5D") \
  V(63, '^', "%5E") \
  V(64, '_', "%5F") \
  V(65, '`', "%60") \
  V(66, 'a', "%61") \
  V(67, 'b', "%62") \
  V(68, 'c', "%63") \
  V(69, 'd', "%64") \
  V(70, 'e', "%65") \
  V(71, 'f', "%66") \
  V(72, 'g', "%67") \
  V(73, 'h', "%68") \
  V(74, 'i', "%69") \
  V(75, 'j', "%6A") \
  V(76, 'k', "%6B") \
  V(77, 'l', "%6C") \
  V(78, 'm', "%6D") \
  V(79, 'n', "%6E") \
  V(80, 'o', "%6F") \
  V(81, 'p', "%70") \
  V(82, 'q', "%71") \
  V(83, 'r', "%72") \
  V(84, 's', "%73") \
  V(85, 't', "%74") \
  V(86, 'u', "%75") \
  V(87, 'v', "%76") \
  V(88, 'w', "%77") \
  V(89, 'x', "%78") \
  V(90, 'y', "%79") \
  V(91, 'z', "%7A") \
  V(92, '{', "%7B") \
  V(93, '|', "%7C") \
  V(94, '}', "%7D") \
  V(95, '~', "%7E")

    static const int64_t kHttpPathParameter = 96;
    static const int64_t kHttpQueryParameter = 97;
    static const int64_t kHttpAlphabetSize = 98;

    class Router : Trie<char, Route, kHttpAlphabetSize>{
     private:
      using Base = Trie<char, Route, kHttpAlphabetSize>;
      using Node = TrieNode<char, Route, kHttpAlphabetSize>;

      static inline int
      GetPosition(char c){
        switch (c){
#define DEFINE_CHECK(Decimal, Char, Encoding) \
        case (Char):                          \
          return (Decimal);
          FOR_EACH_URL_ENCODED_CHARACTER(DEFINE_CHECK)
#undef DEFINE_CHECK
          default:return (int) c;
        }
        return 0;
      }

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

            if (!curr->HasChild(kHttpPathParameter)){
              if (!curr->SetChild(kHttpPathParameter, new Node(':', Route(owner, name, method, handler)))){
                LOG(WARNING) << "couldn't create child @" << kHttpPathParameter << " for " << name;
                return false;
              }
            }

            curr = (Node*) curr->GetChild(kHttpPathParameter);
          } else{
            int pos = GetPosition(c);
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
          if (curr->HasChild(kHttpPathParameter)){
            curr = (Node*) curr->GetChild(kHttpPathParameter);

            std::string value(1, c);
            do{
              value += (*i);
            } while ((++i) != path.end() && (*i) != '/' && (*i) != '?');

            Route& route = curr->GetValue();
            path_params.insert({route.GetPath(), value});
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
            int pos = GetPosition(c);
            if (!curr->HasChild(pos)){
              LOG(WARNING) << "cannot find child " << c << "@" << pos << " (parent: " << curr->GetKey() << ")";
              return RouterMatch(path, RouterMatch::kNotFound);
            }
            curr = (Node*) curr->GetChild(pos);
          }
        }

        Route& route = curr->GetValue();
        if (route.GetMethod() != method)
          return RouterMatch(path, RouterMatch::kNotSupported);
        return RouterMatch(path, RouterMatch::kOk, path_params, query_params, route);
      }
     public:
      Router(): Base(){}
      explicit Router(const RouteHandlerFunction& default_handler):
        Base(){}
      ~Router() = default;

      void Get(const ControllerPtr& owner, const std::string& path, const RouteHandlerFunction& handler){
        Insert(GetRoot(), owner, Method::kGet, path, handler);
      }

      void Put(const ControllerPtr& owner, const std::string& path, const RouteHandlerFunction& handler){
        Insert(GetRoot(), owner, Method::kPut, path, handler);
      }

      void Post(const ControllerPtr& owner, const std::string& path, const RouteHandlerFunction& handler){
        Insert(GetRoot(), owner, Method::kPost, path, handler);
      }

      void Delete(const ControllerPtr& owner, const std::string& path, const RouteHandlerFunction& handler){
        Insert(GetRoot(), owner, Method::kDelete, path, handler);
      }

      RouterMatch Find(const RequestPtr& request);

      static inline RouterPtr
      NewInstance(){
        return std::make_shared<Router>();
      }

      static inline RouterPtr
      NewInstance(const RouteHandlerFunction& default_handler){
        return std::make_shared<Router>(default_handler);
      }
    };
  }
}

#endif //TOKEN_ROUTER_H