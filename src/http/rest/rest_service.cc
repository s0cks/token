#ifdef TOKEN_ENABLE_REST_SERVICE

#include <mutex>
#include <condition_variable>
#include "pool.h"
#include "wallet.h"
#include "blockchain.h"
#include "job/scheduler.h"
#include "http/rest/rest_service.h"

#include "utils/qrcode.h"

namespace Token{
  static pthread_t thread_;
  static std::mutex mutex_;
  static std::condition_variable cond_;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

  static RestService::State state_ = RestService::kStopped;
  static RestService::Status status_ = RestService::kOk;
  static HttpRouter router_;
  static uv_async_t shutdown_;

  RestService::State RestService::GetState(){
    LOCK_GUARD;
    return state_;
  }

  void RestService::SetState(State state){
    LOCK;
    state_ = state;
    UNLOCK;
    SIGNAL_ALL;
  }

  void RestService::WaitForState(State state){
    LOCK;
    while(state_ != state) WAIT;
    UNLOCK;
  }

  RestService::Status RestService::GetStatus(){
    LOCK_GUARD;
    return status_;
  }

  void RestService::SetStatus(Status status){
    LOCK;
    status_ = status;
    UNLOCK;
  }

  bool RestService::Start(){
    if(!IsStopped()){
      LOG(WARNING) << "the rest service is already running.";
      return false;
    }

    LOG(INFO) << "starting the rest service....";
    InfoController::Initialize(&router_);
    ObjectPoolController::Initialize(&router_);
    WalletController::Initialize(&router_);
    BlockChainController::Initialize(&router_);
    return Thread::StartThread(&thread_, "rest-svc", &HandleServiceThread, 0);
  }

  void RestService::OnShutdown(uv_async_t* handle){
    SetState(RestService::kStopping);
    if(!HttpService::ShutdownService(handle->loop)){
      LOG(WARNING) << "couldn't shutdown the rest service.";
    }
    SetState(RestService::kStopped);
  }

  bool RestService::Stop(){
    if(!IsRunning()){
      return true;
    } // should we return false?
    uv_async_send(&shutdown_);
    return Thread::StopThread(thread_);
  }

  void RestService::HandleServiceThread(uword parameter){
    SetState(RestService::kStarting);
    uv_loop_t* loop = uv_loop_new();
    uv_async_init(loop, &shutdown_, &OnShutdown);

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    int32_t port = FLAGS_service_port;
    if(!Bind(&server, port)){
      LOG(WARNING) << "couldn't bind the rest service on port: " << port;
      goto exit;
    }

    int result;
    if((result = uv_listen((uv_stream_t*) &server, 100, &OnNewConnection)) != 0){
      LOG(WARNING) << "the rest service couldn't listen on port " << port << ": " << uv_strerror(result);
      goto exit;
    }

    LOG(INFO) << "rest service listening @" << port;
    SetState(State::kRunning);
    uv_run(loop, UV_RUN_DEFAULT);
    LOG(INFO) << "the rest service has stopped.";
    exit:
    pthread_exit(nullptr);
  }

  void RestService::OnNewConnection(uv_stream_t* stream, int status){
    HttpSession* session = new HttpSession(stream);
    if(!Accept(stream, session)){
      LOG(WARNING) << "couldn't accept new connection.";
      return;
    }

    int result;
    if((result = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
      LOG(WARNING) << "server read failure: " << uv_strerror(result);
      return;
    }
  }

  void RestService::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
    HttpSession* session = (HttpSession*) stream->data;
    if(nread == UV_EOF){
      return;
    }
    if(nread < 0){
      LOG(WARNING) << "server read failure: " << uv_strerror(nread);
      session->Close();
      return;
    }

    HttpRequestPtr request = HttpRequest::NewInstance(session, buff->base, buff->len);
    HttpRouterMatch match = router_.Find(request);
    if(match.IsNotFound()){
      std::stringstream ss;
      ss << "Cannot find: " << request->GetPath();
      return session->Send(NewNotFoundResponse(session, ss));
    } else if(match.IsMethodNotSupported()){
      std::stringstream ss;
      ss << "Method Not Supported for: " << request->GetPath();
      return session->Send(NewNotSupportedResponse(session, ss));
    } else{
      assert(match.IsOk());

      // weirdness :(
      request->SetParameters(match.GetParameters());
      HttpRouteHandler& handler = match.GetHandler();
      handler(session, request);
    }
  }

/*****************************************************************************
 *                      InfoController
 *****************************************************************************/
  void InfoController::HandleGetStats(HttpSession* session, const HttpRequestPtr& request){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kHttpOk);

    JsonString& json = response->GetBody();
    if(!JobScheduler::GetWorkerStatistics(json))
      return session->Send(NewInternalServerErrorResponse(session, "Cannot get job scheduler statistics."));

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", json.GetSize());
    return session->Send(std::static_pointer_cast<HttpResponse>(response));
  }

/*****************************************************************************
 *                      BlockChainController
 *****************************************************************************/
  void BlockChainController::HandleGetBlockChain(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewNoContentResponse(session, "Cannot get the list of blocks in the blockchain."));
  }

  void BlockChainController::HandleGetBlockChainHead(HttpSession* session, const HttpRequestPtr& request){
    BlockPtr head = BlockChain::GetHead();
    HttpResponsePtr response = NewOkResponse(session, head);
    return session->Send(response);
  }

  void BlockChainController::HandleGetBlockChainBlock(HttpSession* session, const HttpRequestPtr& request){
    Hash hash = Hash::FromHexString(request->GetParameterValue("hash"));
    if(!BlockChain::HasBlock(hash))
      return session->Send(NewNoContentResponse(session, hash));
    BlockPtr blk = BlockChain::GetBlock(hash);
    return session->Send(NewOkResponse(session, blk));
  }

/*****************************************************************************
 *                      ObjectPoolController
 *****************************************************************************/
  void ObjectPoolController::HandleGetStats(HttpSession* session, const HttpRequestPtr& request){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kHttpOk);

    JsonString& body = response->GetBody();
    if(!ObjectPool::GetStats(body))
      return session->Send(NewInternalServerErrorResponse(session, "Cannot get object pool stats."));

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return session->Send(std::static_pointer_cast<HttpResponse>(response));
  }

  void ObjectPoolController::HandleGetBlock(HttpSession* session, const HttpRequestPtr& request){
    Hash hash = Hash::FromHexString(request->GetParameterValue("hash"));
    if(!ObjectPool::HasBlock(hash))
      return session->Send(NewNoContentResponse(session, hash));
    BlockPtr blk = ObjectPool::GetBlock(hash);
    return session->Send(NewOkResponse(session, blk));
  }

  void ObjectPoolController::HandleGetBlocks(HttpSession* session, const HttpRequestPtr& request){

  }

  void ObjectPoolController::HandleGetTransaction(HttpSession* session, const HttpRequestPtr& request){
    Hash hash = Hash::FromHexString(request->GetParameterValue("hash"));
    if(!ObjectPool::HasTransaction(hash))
      return session->Send(NewNoContentResponse(session, hash));
    TransactionPtr tx = ObjectPool::GetTransaction(hash);
    return session->Send(NewOkResponse(session, tx));
  }

  void ObjectPoolController::HandleGetTransactions(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewNotImplementedResponse(session, "Not Implemented."));
  }

  void ObjectPoolController::HandleGetUnclaimedTransaction(HttpSession* session, const HttpRequestPtr& request){
    Hash hash = request->GetHashParameterValue();
    if(!ObjectPool::HasUnclaimedTransaction(hash))
      return session->Send(NewNoContentResponse(session, hash));
    UnclaimedTransactionPtr utxo = ObjectPool::GetUnclaimedTransaction(hash);
    return session->Send(NewOkResponse(session, utxo));
  }

  void ObjectPoolController::HandleGetUnclaimedTransactions(HttpSession* session, const HttpRequestPtr& request){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kHttpOk);

    JsonString& body = response->GetBody();
    if(!ObjectPool::GetUnclaimedTransactions(body))
      return session->Send(NewInternalServerErrorResponse(session, "Cannot get the list of unclaimed transactions in the object pool."));

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return session->Send(std::static_pointer_cast<HttpResponse>(response));
  }

  void WalletController::HandleGetUserWalletTokenCode(HttpSession* session, const HttpRequestPtr& request){
    User user = request->GetUserParameterValue();
    Hash hash = request->GetHashParameterValue();

    if(!ObjectPool::HasUnclaimedTransaction(hash))
      return session->Send(NewNoContentResponse(session, hash));

    UnclaimedTransactionPtr utxo = ObjectPool::GetUnclaimedTransaction(hash);
    BufferPtr data = Buffer::NewInstance(65536);
    if(!WriteQRCode(data, hash)){
      std::stringstream ss;
      ss << "Cannot generate qr-code for: " << hash;
      return session->Send(NewInternalServerErrorResponse(session, ss));
    }
    return session->Send(HttpBinaryResponse::NewInstance(session, HttpStatusCode::kHttpOk, HTTP_CONTENT_TYPE_IMAGE_PNG, data));
  }

  void WalletController::HandleGetUserWallet(HttpSession* session, const HttpRequestPtr& request){
    User user = request->GetUserParameterValue();

    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kHttpOk);

    JsonString& body = response->GetBody();
    JsonWriter writer(body);
    writer.StartObject();
      writer.Key("data");
      writer.StartObject();
        SetField(writer, "user", user);
        writer.Key("wallet");
        if(!WalletManager::GetWallet(user, writer)){
          std::stringstream ss;
          ss << "Cannot find wallet for: " << user;
          return session->Send(NewNoContentResponse(session, ss));
        }
      writer.EndObject();
    writer.EndObject();

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return session->Send(std::static_pointer_cast<HttpResponse>(response));
  }

  void WalletController::HandlePostUserWalletSpend(HttpSession* session, const HttpRequestPtr& request){
    User user = request->GetUserParameterValue();

    JsonDocument doc;
    request->GetBody(doc);

    InputList inputs = {};
    if(!doc.HasMember("token"))
      return session->Send(NewInternalServerErrorResponse(session, "request is missing 'token' field."));
    if(!doc["token"].IsString())
      return session->Send(NewInternalServerErrorResponse(session, "'token' field is not a string."));
    Hash in_hash = Hash::FromHexString(doc["token"].GetString());
    if(!ObjectPool::HasUnclaimedTransaction(in_hash)){
      std::stringstream ss;
      ss << "Cannot find token: " << in_hash;
      return session->Send(NewNotFoundResponse(session, ss));
    }
    UnclaimedTransactionPtr in_val = ObjectPool::GetUnclaimedTransaction(in_hash);
    inputs.push_back(Input(in_val->GetReference(), in_val->GetUser())); //TODO: is this the right user?

    OutputList outputs = {};
    if(!doc.HasMember("recipient"))
      return session->Send(NewInternalServerErrorResponse(session, "request is missing 'recipient' field."));
    if(!doc["recipient"].IsString())
      return session->Send(NewInternalServerErrorResponse(session, "'recipient' field is not a string."));
    User recipient(std::string(doc["recipient"].GetString()));
    outputs.push_back(Output(recipient, in_val->GetProduct()));

    TransactionPtr tx = Transaction::NewInstance(0, inputs, outputs);
    Hash hash = tx->GetHash();
    if(!ObjectPool::PutTransaction(hash, tx)){
      std::stringstream ss;
      ss << "Cannot insert newly created transaction into object pool: " << hash;
      return session->Send(NewInternalServerErrorResponse(session, ss));
    }
    return session->Send(NewOkResponse(session, tx));
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE