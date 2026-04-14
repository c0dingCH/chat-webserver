#include"Transport.h"
#include"Api.h"
#include"Logging.h"

#include<rapidjson/document.h>
#include<string>
#include<memory>

namespace api::transport{
  void HandleMessage(const HttpObjs & hs){
    std::string reciever_id = hs.request->GetHeader("reciever_id");
    const std::shared_ptr<TcpConnection> & reciever_conn = hs.server->GetConn(reciever_id);
    if(reciever_conn){
      HttpContext * context = reciever_conn -> GetContext();
      HttpResponse response(true);

      //创建json然后parse后返回
      rapidjson::Document dom;
      dom.SetObject();

      rapidjson::Document::AllocatorType & allocator = dom.GetAllocator();
      dom.AddMember("content", rapidjson::Value(hs.request->GetDom()["content"].GetString(),allocator), allocator);
      
      if(!context -> push_id_){
        LOG_ERROR << "No stream to push";
        return;
      }

      response.AddHeader(":status","200");
      response.AddHeader("content-type", "application/json");
      response.SetBody(hs.request -> ParseJson2String(dom));
      response.ParseResponse(context -> GetSession(), response.PushPromise(
            context -> GetSession() ,
            context -> push_id_ , 
            "POST" ,  
            "http" , 
            hs.server->GetAuthority(),
            "/from_id_" + hs.request -> GetHeader("sender_id")
            ));
     
      reciever_conn -> Send(context -> GetMessage());
      
    }
    else{
      //placeholders
    }

    hs.response->AddHeader(":status","200");
    hs.response->AddHeader("content-type", "text/html");
    hs.response->SetBody(reciever_conn ? "Send ok !" : (reciever_id + " is not online"));
  }

}
