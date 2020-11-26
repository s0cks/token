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

#define STATUS_CODE_OK 200
#define STATUS_CODE_CLIENT_ERROR 400
#define STATUS_CODE_NOTFOUND 404
#define STATUS_CODE_INTERNAL_SERVER_ERROR 500
#define CONTENT_TEXT_PLAIN "text/plain"
#define CONTENT_APPLICATION_JSON "application/json"

    class HttpResponse{
        friend class HttpSession;
    private:
        HttpSession* session_;
        int status_code_;
        std::string content_type_;
        std::string body_;
    public:
        HttpResponse(HttpSession* session, int status_code, const std::string& content_type, const std::string& body):
            session_(session),
            status_code_(status_code),
            content_type_(content_type),
            body_(body){}
        HttpResponse(HttpSession* session, int status_code, const std::string& body):
            HttpResponse(session, status_code, CONTENT_TEXT_PLAIN, body){}
        HttpResponse(HttpSession* session, const std::string& body):
            HttpResponse(session, STATUS_CODE_OK, body){}
        HttpResponse(HttpSession* session, int status_code, const std::string& content_type, const std::stringstream& ss):
            HttpResponse(session, status_code, content_type, ss.str()){}
        HttpResponse(HttpSession* session, int status_code, const std::stringstream& ss):
            HttpResponse(session, status_code, CONTENT_TEXT_PLAIN, ss){}
        HttpResponse(HttpSession* session, const std::stringstream& ss):
            HttpResponse(session, STATUS_CODE_OK, ss){}
        HttpResponse(HttpSession* session, bool is_not_found=false):
            HttpResponse(session, is_not_found ? STATUS_CODE_NOTFOUND : STATUS_CODE_OK, is_not_found ? "Not Found" : "Ok"){}
        ~HttpResponse() = default;

        HttpSession* GetSession() const{
            return session_;
        }

        int GetStatusCode() const{
            return status_code_;
        }

        std::string GetContentType() const{
            return content_type_;
        }

        int GetContentLength() const{
            return body_.size();
        }

        std::string GetBody() const{
            return body_;
        }
    };
}

#endif //TOKEN_REQUEST_H