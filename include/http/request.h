#ifndef TOKEN_REQUEST_H
#define TOKEN_REQUEST_H

#include <uv.h>
#include <string>
#include <sstream>

#include <http_parser.h>

namespace Token{
    class HttpSession;
    class HttpRequest{
        friend class HttpSession;
    private:
        HttpSession* session_;
        http_parser parser_;
        http_parser_settings settings_;
        std::string path_;

        static int
        OnParseURL(http_parser* parser, const char* data, size_t len){
            HttpRequest* request = (HttpRequest*)parser->data;
            request->path_ = std::string(data, len);
            return 0;
        }
    public:
        HttpRequest(HttpSession* session, const char* data, size_t len):
            session_(session),
            parser_(),
            settings_(),
            path_(){
            parser_.data = this;
            settings_.on_url = &OnParseURL;
            http_parser_init(&parser_, HTTP_REQUEST);
            size_t parsed;
            if((parsed = http_parser_execute(&parser_, &settings_, data, len)) == 0){
                LOG(WARNING) << "http parser parsed nothing";
                return;
            }
        }
        ~HttpRequest(){}

        std::string GetPath() const{
            return path_;
        }

        bool IsGet() const{
            return parser_.method == HTTP_GET;
        }

        bool IsPost() const{
            return parser_.method == HTTP_POST;
        }

        bool IsPut() const{
            return parser_.method == HTTP_PUT;
        }

        bool IsDelete() const{
            return parser_.method == HTTP_DELETE;
        }

        HttpSession* GetSession() const{
            return session_;
        }
    };

    class HttpResponse{
        friend class HttpSession;
    private:
        HttpSession* session_;
        int code_;
        std::string message_;
    public:
        HttpResponse(HttpSession* session, int code, const std::string& message):
            session_(session),
            code_(code),
            message_(message){}
        ~HttpResponse() = default;

        HttpSession* GetSession() const{
            return session_;
        }

        int GetCode() const{
            return code_;
        }

        int GetContentLength() const{
            return message_.size();
        }

        std::string GetBody() const{
            return message_;
        }
    };
}

#endif //TOKEN_REQUEST_H