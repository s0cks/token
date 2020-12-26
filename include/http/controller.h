#ifndef TOKEN_CONTROLLER_H
#define TOKEN_CONTROLLER_H

#include <json/value.h>
#include <json/writer.h>
#include "http/router.h"
#include "http/request.h"

namespace Token{
    class HttpController{
    protected:
        HttpController() = delete;

        static inline void
        SendText(HttpSession* session, const std::string& body, const HttpStatusCode& status_code=STATUS_CODE_OK){
            HttpResponse response(session, status_code, body);
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
        SendJson(HttpSession* session, Json::Value& value, const HttpStatusCode& status_code=STATUS_CODE_OK){
            Json::StreamWriterBuilder builder;
            builder["commentStyle"] = "None";
            builder["indentation"] = "";
            std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
            std::ostringstream os;
            writer->write(value, &os);

            std::string body = os.str();
            HttpResponse response(session, status_code, body);
            response.SetHeader("Content-Type", CONTENT_TYPE_APPLICATION_JSON);
            response.SetHeader("Content-Length", body.size());
            session->Send(&response);
        }
    public:
        virtual ~HttpController() = delete;
    };
}

#endif //TOKEN_CONTROLLER_H