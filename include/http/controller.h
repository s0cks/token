#ifndef TOKEN_CONTROLLER_H
#define TOKEN_CONTROLLER_H

#include <rapidjson/document.h>
#include "http/router.h"
#include "http/request.h"

namespace Token{
    class HttpController{
    protected:
        HttpController() = delete;

        static inline void
        SendText(HttpSession* session, const std::string& body, const HttpStatusCode& status_code=STATUS_CODE_OK){
            HttpTextResponse response(session, status_code, body);
            response.SetHeader("Content-Type", CONTENT_TYPE_TEXT_PLAIN);
            response.SetHeader("Content-Length", body.size());
            session->Send(&response);
        }

        static inline void
        SendText(HttpSession* session, const std::stringstream& ss, const HttpStatusCode& status_code=STATUS_CODE_OK){
            return SendText(session, ss.str(), status_code);
        }

        static inline void
        SendOk(HttpSession* session){
            return SendText(session, "Ok");
        }

        static inline void
        SendInternalServerError(HttpSession* session, const std::string& msg="Internal Server Error"){
            return SendText(session, msg, STATUS_CODE_INTERNAL_SERVER_ERROR);
        }

        static inline void
        SendNotSupported(HttpSession* session, const std::string& path){
            std::stringstream ss;
            ss << "Not Supported.";
            return SendText(session, ss, STATUS_CODE_NOTSUPPORTED);
        }

        static inline void
        SendNotFound(HttpSession* session, const std::string& path){
            std::stringstream ss;
            ss << "Not Found: " << path;
            return SendText(session, ss, STATUS_CODE_NOTFOUND);
        }

        static inline void
        SendNotFound(HttpSession* session, const Hash& hash){
            std::stringstream ss;
            ss << "Not Found: " << hash;
            return SendText(session, ss, STATUS_CODE_NOTFOUND);
        }

        static inline void
        SendJson(HttpSession* session, const rapidjson::Document& doc, const HttpStatusCode& status_code=STATUS_CODE_OK){
            HttpJsonResponse response(session, status_code, doc);
            response.SetHeader("Content-Type", CONTENT_TYPE_APPLICATION_JSON);
            response.SetHeader("Content-Length", response.GetContentLength());
            session->Send(&response);
        }

        static inline void
        SendJson(HttpSession* session, const HashList& hashes, const HttpStatusCode& status_code=STATUS_CODE_OK){
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
            writer.StartArray();
            for(auto& it : hashes){
                std::string hash = it.HexString();
                writer.String(hash.data(), 64);
            }
            writer.EndArray();

            HttpRawJsonResponse response(session, status_code, sb);
            response.SetHeader("Content-Type", CONTENT_TYPE_APPLICATION_JSON);
            response.SetHeader("Content-Length", response.GetContentLength());
            session->Send(&response);
        }
    public:
        virtual ~HttpController() = delete;
    };

#define HTTP_CONTROLLER_ENDPOINT(Name) \
    static void Handle##Name(HttpSession* session, HttpRequest* request)

#define HTTP_CONTROLLER_GET(Path, Name) \
    router->Get((Path), &Handle##Name)
#define HTTP_CONTROLLER_PUT(Path, Name) \
    router->Put((Path), &Handle##Name)
#define HTTP_CONTROLLER_POST(Path, Name) \
    router->Post((Path), &Handle##Name)
#define HTTP_CONTROLLER_DELETE(Path, Name) \
    router->Delete((Path), &Handle##Name)

#define HTTP_CONTROLLER_INIT() \
    static inline void Initialize(HttpRouter* router)
}

#endif //TOKEN_CONTROLLER_H