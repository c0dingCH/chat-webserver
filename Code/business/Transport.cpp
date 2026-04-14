#include"Transport.h"

#include<rapidjson/document.h>
#include<string>
#include<memory>

namespace api::transport{
  void HandleMessage(const HttpObjs & hs){
    std::string reciever_id = hs.request->GetHeader("reciever_id");
    const std::shared_ptr<TcpConnection> & reciever_conn = hs.server->GetConn(reciever_id);
    hs.response->AddHeader(":status","200");
    if(reciever_conn){
      //设置返回头
      hs.response->AddHeader("content-type", "application/json");
      
      //创建json然后parse后返回
      rapidjson::Document dom;
      dom.SetObject();

      rapidjson::Document::AllocatorType & allocator = dom.GetAllocator();
      dom.AddMember("content", rapidjson::Value(hs.request->GetDom()["content"].GetString(),allocator), allocator);
      
      
      hs.response->SetBody(hs.request->ParseJson2String(dom));
      hs.server->Send(reciever_conn, hs.response);
    }

    hs.response->AddHeader("content-type", "text/html");
    hs.response->SetBody(reciever_conn ? "Send ok !" : (reciever_id + " is not online"));
  }

}
