#ifdef TOKEN_HEALTHCHECK_SUPPORT
#ifndef TOKEN_REQUEST_H
#define TOKEN_REQUEST_H

#include <uv.h>
#include <http_parser.h>
#include <string>
#include <sstream>

namespace Token{
    class HttpResponse;
    class HttpRequest{
    private:
        uv_tcp_t handle_;
        http_parser parser_;
        http_parser_settings parser_settings_;
        std::string route_;

        size_t Parse(const char* data, size_t len){
            return http_parser_execute(&parser_, &parser_settings_, data, len);
        }

        void SendResponse(HttpResponse* response);
        void SetRoute(const std::string& route){
            route_ = route;
        }

        static void OnClose(uv_handle_t* handle);
        static void OnResponseSent(uv_write_t* req, int status);
        static int OnParseUrl(http_parser* parser, const char* data, size_t len);

        friend class BlockChainHttpServer;
    public:
        HttpRequest():
            handle_(),
            parser_settings_(),
            parser_(),
            route_(){
            handle_.data = this;
            parser_.data = this;
            parser_settings_.on_url = OnParseUrl;
            http_parser_init(&parser_, HTTP_REQUEST);
        }
        ~HttpRequest(){}

        bool IsPost() const{
            return parser_.method == HTTP_POST;
        }

        bool IsGet() const{
            return parser_.method == HTTP_GET;
        }

        bool IsPut() const{
            return parser_.method == HTTP_PUT;
        }

        bool IsDelete() const{
            return parser_.method == HTTP_DELETE;
        }

        std::string GetRoute() const{
            return route_;
        }

        const char* GetError() const{
            return http_errno_description((enum http_errno) parser_.http_errno);
        }
    };

    class HttpResponse{
    private:
        int code_;
        std::string body_;
    public:
        HttpResponse(int code, const std::string& body):
            code_(code),
            body_(body){}
        HttpResponse(int code, const std::stringstream& stream):
            code_(code),
            body_(stream.str()){}
        ~HttpResponse(){}

        int GetCode() const{
            return code_;
        }

        int GetContentLength() const{
            return body_.size();
        }

        std::string GetBody() const{
            return body_;
        }

        friend std::stringstream& operator<<(std::stringstream& stream, const HttpResponse& response){
            stream << "HTTP/1.1 200 OK\r\n";
            stream << "Content-Type: text/plain\r\n";
            stream << "Content-Length: " << response.GetContentLength() << "\r\n";
            stream << "\r\n";
            stream << response.GetBody();
            return stream;
        }
    };
}

#endif //TOKEN_REQUEST_H
#endif//TOKEN_HEALTHCHECK_SUPPORT