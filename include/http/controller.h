#ifndef TOKEN_CONTROLLER_H
#define TOKEN_CONTROLLER_H

#include <json/value.h>
#include <json/writer.h>
#include "http/router.h"

namespace Token{
    static inline void
    SendText(HttpSession* session, const std::string& body, const int& status_code=STATUS_CODE_OK){
        HttpResponse response(session, status_code, body);
        session->Send(&response);
    }

    static inline void
    SendText(HttpSession* session, const std::stringstream& ss, const int& status_code=STATUS_CODE_OK){
        return SendText(session, ss.str(), status_code);
    }

    static inline void
    SendOk(HttpSession* session){
        HttpResponse response(session, STATUS_CODE_OK, "Ok");
        session->Send(&response);
    }

    static inline void
    SendInternalServerError(HttpSession* session, const std::string& msg="Internal Server Error"){
        std::stringstream ss;
        ss << "Internal Server Error: " << msg;
        HttpResponse response(session, STATUS_CODE_INTERNAL_SERVER_ERROR, ss);
        session->Send(&response);
    }

    static inline void
    SendNotSupported(HttpSession* session, const std::string& path){
        std::stringstream ss;
        ss << "Not Supported.";
        HttpResponse response(session, STATUS_CODE_NOTSUPPORTED, ss);
        session->Send(&response);
    }

    static inline void
    SendNotFound(HttpSession* session, const std::string& path){
        std::stringstream ss;
        ss << "Not Found: " << path;
        HttpResponse response(session, STATUS_CODE_NOTFOUND, ss);
        session->Send(&response);
    }

    static inline void
    SendJson(HttpSession* session, Json::Value& value, int status=STATUS_CODE_OK){
        Json::StreamWriterBuilder builder;
        builder["commentStyle"] = "None";
        builder["indentation"] = "";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        std::ostringstream os;
        writer->write(value, &os);

        HttpResponse response(session, status, CONTENT_APPLICATION_JSON, os.str());
        session->Send(&response);
    }

    class StatusController{
    private:
        StatusController() = delete;

        static void HandleOverallStatus(HttpSession* session, HttpRequest* request);
        static void HandleTestStatus(HttpSession* session, HttpRequest* request);
    public:
        ~StatusController() = delete;

        static inline bool
        Initialize(HttpRouter* router){
            router->Get("/status", &HandleOverallStatus);
            router->Get("/status/:name", &HandleTestStatus);
            return true;
        }
    };

    class HealthController{
    private:
        HealthController() = delete;

        static void HandleReadyEndpoint(HttpSession* session, HttpRequest* request){
            SendOk(session);
        }

        static void HandleLiveEndpoint(HttpSession* session, HttpRequest* request){
            SendOk(session);
        }
    public:
        ~HealthController() = delete;

        static inline bool
        Initialize(HttpRouter* router){
            router->Get("/ready", &HandleReadyEndpoint);
            router->Get("/live", &HandleLiveEndpoint);
            return true;
        }
    };
}

#endif //TOKEN_CONTROLLER_H