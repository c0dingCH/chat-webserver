#include"Transport.h"
#include"Api.h"
#include"Logging.h"

#include<rapidjson/document.h>
#include<string>
#include<memory>
#include<fstream>

namespace api::transport{
  void HandleMessage(const HttpObjs & hs){
    std::string reciever_id = hs.request->GetHeader("reciever_id");
    const std::shared_ptr<TcpConnection> & reciever_conn = hs.server->GetConn(reciever_id);
    if(reciever_conn){
      HttpContext * context = reciever_conn -> GetContext();
      HttpResponse response(true);
      
      response.AddHeader(":status","200");
      response.AddHeader("content-type", "application/json");
      

      if(!context -> push_id_){
        LOG_ERROR << "No stream to push";
        return;
      }
      
      //创建json然后parse后返回
      rapidjson::Document dom;
      dom.SetObject();
      rapidjson::Document::AllocatorType & allocator = dom.GetAllocator();
      std::string path;
      
      if(hs.request->GetHeader("content-type") == "application/json"){
        dom.AddMember("content", rapidjson::Value(hs.request->GetDom()["content"].GetString(),allocator), allocator);
        path = "/from_id/" + hs.request->GetHeader("sender_id");
      }
      else{
        //下载文件并给出下载按钮
        std::string filename = hs.request->GetHeader("filename");
        dom.AddMember("filename",rapidjson::Value(filename.c_str(), allocator), allocator);
        dom.AddMember("download",rapidjson::Value(("/download/" + filename).c_str() , allocator), allocator);
        path = "../public/files/" + filename;
        
        std::ofstream ofs(path.c_str(), std::ios::binary); // 这里直接覆盖源文件了
        const std::string data = hs.request->GetData();
        ofs.write(data.c_str(), data.size());
      
        path = path.substr(2);
      }

     
      response.SetBody(hs.request -> ParseJson2String(dom));
      response.ParseResponse(context -> GetSession(), response.PushPromise(
          context -> GetSession() ,
          context -> push_id_ , 
          "POST" ,  
          "http" , 
          hs.server->GetAuthority(),
          path
        ));
     
      reciever_conn -> Send(context -> GetMessage());
    }

    hs.response->AddHeader(":status","200");
    hs.response->AddHeader("content-type", "text/html");
    hs.response->SetBody(reciever_conn ? "Send ok !" : (reciever_id + " is not online"));
  }

}
