#ifndef TOKEN_REQUEST_H
#define TOKEN_REQUEST_H

#include <uv.h>
#include <string>
#include <sstream>
#include <unordered_map>
#include <http_parser.h>

namespace Token{
    enum HttpMethod{
        kGet,
        kPut,
        kPost,
        kDelete,
    };

    typedef std::unordered_map<std::string, std::string> ParameterMap;
    typedef std::unordered_map<std::string, std::string> HttpHeadersMap;

    class HttpSession;
    class HttpRequest{
        friend class HttpSession;
    private:
        HttpSession* session_;
        http_parser parser_;
        http_parser_settings settings_;
        std::string path_;
        ParameterMap parameters_;

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
            path_(),
            parameters_(){
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

        void SetParameters(const ParameterMap& parameters){
            parameters_.clear();
            parameters_.insert(parameters.begin(), parameters.end());
        }

        std::string GetPath() const{
            return path_;
        }

        HttpMethod GetMethod() const{
            switch(parser_.method){
                case HTTP_GET:
                    return HttpMethod::kGet;
                case HTTP_PUT:
                    return HttpMethod::kPut;
                case HTTP_POST:
                    return HttpMethod::kPost;
                case HTTP_DELETE:
                    return HttpMethod::kDelete;
                default:
                    // should we do this?
                    return HttpMethod::kGet;
            }
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

        bool HasParameter(const std::string& name) const{
            auto pos = parameters_.find(name);
            return pos != parameters_.end();
        }

        HttpSession* GetSession() const{
            return session_;
        }

        std::string GetParameterValue(const std::string& name) const{
            auto pos = parameters_.find(name);
            return pos != parameters_.end() ? pos->second : "";
        }
    };

#define STATUS_CODE_OK 200
#define STATUS_CODE_CLIENT_ERROR 400
#define STATUS_CODE_NOTFOUND 404
#define STATUS_CODE_NOTSUPPORTED 405
#define STATUS_CODE_INTERNAL_SERVER_ERROR 500

#define CONTENT_TYPE_TEXT_PLAIN "text/plain"
#define CONTENT_TYPE_APPLICATION_JSON "application/json"

    typedef int HttpStatusCode;

    class HttpResponse{
        friend class HttpSession;
    private:
        HttpSession* session_;
        HttpStatusCode status_code_;
        HttpHeadersMap headers_;
        std::string body_;
    public:
        HttpResponse(HttpSession* session, const HttpStatusCode& code, const std::string& body):
            session_(session),
            status_code_(code),
            body_(body){}
        ~HttpResponse() = default;

        HttpSession* GetSession() const{
            return session_;
        }

        int GetStatusCode() const{
            return status_code_;
        }

        int GetContentLength() const{
            return body_.size();
        }

        std::string GetBody() const{
            return body_;
        }

        void SetHeader(const std::string& key, const std::string& value){
            auto pos = headers_.insert({ key, value });
            if(!pos.second)
                LOG(WARNING) << "cannot add header " << key << ":" << value;
        }

        void SetHeader(const std::string& key, const long& value){
            std::stringstream val;
            val << value;
            auto pos = headers_.insert({ key, val.str() });
            if(!pos.second)
                LOG(WARNING) << "cannot add header " << key << ":" << value;
        }

        bool HasHeaderValue(const std::string& key){
            auto pos = headers_.find(key);
            return pos != headers_.end();
        }

        std::string GetHeaderValue(const std::string& key){
            auto pos = headers_.find(key);
            return pos->second;
        }
    };
}

#endif //TOKEN_REQUEST_H