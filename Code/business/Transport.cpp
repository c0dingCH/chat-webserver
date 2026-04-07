#include"Transport.h"

#include<rapidjson/document.h>
#include<string>
#include<memory>

namespace api::transport{
  void HandleMessage(const HttpObjs & hs){
    std::string reciever_id = hs.request->GetHeader("Reciever_id");
    const std::shared_ptr<TcpConnection> & reciever_conn = hs.server->GetConn(reciever_id);
    hs.response->SetStatusCode(HttpResponse::HttpStatusCode::k100Continue);
    if(reciever_conn){
      hs.response->AddHeader("Content-Type", "application/json");
      
      rapidjson::Document dom;
      dom.SetObject();

      rapidjson::Document::AllocatorType & allocator = dom.GetAllocator();
      dom.AddMember("content", rapidjson::Value(hs.request->GetDom()["content"].GetString(),allocator), allocator);
     
      hs.response->SetBody(hs.request->ParseJson2String(dom));
      reciever_conn->Send(hs.response->GetMessage());
    }
    hs.response->SetBody("");
    hs.response->AddHeader("Content-Type", "text/html");
    hs.response->SetStatusMessage(reciever_conn ? "ok !" : "failed !");
  }

}
