#ifndef TOKEN_ROUTER_H
#define TOKEN_ROUTER_H

#include "trie.h"
#include "http/session.h"
#include "http/request.h"

namespace Token{
    class HttpRouter{
    public:
        typedef void (*HttpRoute)(HttpSession*, HttpRequest*);
    private:
        typedef Trie<std::string, HttpRoute, 64> HttpRouteTrie;

        HttpRouteTrie trie_;
        HttpRoute default_;
    public:
        HttpRouter(HttpRoute default_route):
            trie_(),
            default_(default_route){}
        ~HttpRouter(){}

        HttpRoute GetDefaultRoute() const{
            return default_;
        }

        HttpRoute GetRoute(HttpRequest* request){
            return trie_.Search(request->GetPath());
        }

        HttpRoute GetRouteOrDefault(HttpRequest* request){
            if(!trie_.Contains(request->GetPath()))
                return GetDefaultRoute();
            return trie_.Search(request->GetPath());
        }

        void Get(const std::string& path, HttpRoute route){
            trie_.Insert(path, route);
        }
    };
}

#endif //TOKEN_ROUTER_H