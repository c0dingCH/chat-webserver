#include "TcpConnection.h"
#include "HttpServer.h"
#include "HttpResponse.h"
#include "HttpRequest.h"
#include "HttpContext.h"
#include "Logging.h"

#include "User.h"
#include "Mysql.h"

#include <rapidjson/document.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fstream>

void User::HandleRegister(const HttpObjs& hs) {
    WithConnection(hs.server->GetMysqlPool(), [&](std::unique_ptr<Mysql>& db) {
        rapidjson::Document& dom = hs.request->GetDom();
        auto res_db = db->Insert(dom["username"].GetString(), dom["password"].GetString());
        
        hs.response->AddHeader(":status", "200");
        hs.response->SetBody(db->GetMsg(res_db));
        hs.response->AddHeader("content-type", "text/html");
    });
}

void User::HandleLogin(const HttpObjs& hs) {
    WithConnection(hs.server->GetMysqlPool(), [&](std::unique_ptr<Mysql>& db) {
        rapidjson::Document& dom = hs.request->GetDom();
        std::string username = dom["username"].GetString();
        auto res_db = db->Login(username, dom["password"].GetString());
        
        if (res_db == Mysql::MysqlStatus::kSuccess) {
            User* user = hs.server->AddUser(username);
            user->SetUsername(username);
            
            hs.conn->SetUser(user);
            hs.server->AddConn(username, hs.conn);
            
            hs.response->AddHeader(":status", "200");
            hs.conn->SetOnCloseBusi([user_name = username, server = hs.server]() {
                server->RemoveUser(user_name);
                server->RemoveConn(user_name);
            });
        } else {
            hs.response->AddHeader(":status", "200");
        }
        
        hs.response->SetBody(db->GetMsg(res_db));
        hs.response->AddHeader("content-type", "text/html");
    });
}

void User::HandleLogout(const HttpObjs& hs) {
  User* user = hs.conn->GetUser();
  if (user) { 
    std::string username = user->GetUsername();
    hs.server->RemoveUser(username);
    hs.server->RemoveConn(username);
    hs.conn->SetUser(nullptr);
  }
    
  hs.response->AddHeader(":status", "200");
  hs.response->SetBody("logout success");
  hs.response->AddHeader("content-type", "text/html");
}

void User::HandleProfile(const HttpObjs& hs) {
    WithConnection(hs.server->GetMysqlPool(), [&](std::unique_ptr<Mysql>& db) {
        rapidjson::Document& dom = hs.request->GetDom();
        auto res_db = db->Update(dom["username"].GetString(), dom["password"].GetString());
        if (res_db == Mysql::MysqlStatus::kSuccess) {
            hs.response->AddHeader(":status", "200");
        } else {
            hs.response->AddHeader(":status", "200");
        }
        hs.response->SetBody(db->GetMsg(res_db));
        hs.response->AddHeader("content-type", "text/html");
    });
}

void User::HandleTransport(const HttpObjs& hs) {
  std::string reciever_id = hs.request->GetHeader("reciever_id");
  const std::shared_ptr<TcpConnection>& reciever_conn = hs.server->GetConn(reciever_id);
  if (reciever_conn) {
    HttpContext* context = reciever_conn->GetContext();
    HttpResponse response(true);
        
    response.AddHeader(":status", "200");
    response.AddHeader("content-type", "application/json");
        
    if (!context->push_id_) {
      LOG_ERROR << "No stream to push";
      return;
    }
       
    rapidjson::Document dom;
    dom.SetObject();
    rapidjson::Document::AllocatorType& allocator = dom.GetAllocator();
    std::string path;
       
    if (hs.request->GetHeader("content-type") == "application/json") {
      dom.AddMember("content", rapidjson::Value(hs.request->GetDom()["content"].GetString(), allocator), allocator);
      path = "/from_id/" + hs.request->GetHeader("sender_id");
    } 
    else {
      std::string filename = hs.request->GetHeader("filename");
      dom.AddMember("filename", rapidjson::Value(filename.c_str(), allocator), allocator);
      dom.AddMember("download", rapidjson::Value(("/download/" + filename).c_str(), allocator), allocator);
      path = "../public/files/" + filename;
           
      std::ofstream ofs(path.c_str(), std::ios::binary);
      const std::string data = hs.request->GetData();
      ofs.write(data.c_str(), data.size());
            
      path = path.substr(2);
    }
        
    response.SetBody(hs.request->ParseJson2String(dom));
    response.ParseResponse(context->GetSession(), response.PushPromise(
      context->GetSession(),
      context->push_id_,
      "POST",
      "http",
      hs.server->GetAuthority(),
      path
    ));
        
    reciever_conn->Send(context->GetMessage());
  }
    
  hs.response->AddHeader(":status", "200");
  hs.response->AddHeader("content-type", "text/html");
  hs.response->SetBody(reciever_conn ? "Send ok !" : (reciever_id + " is not online"));
}

void User::Download(const HttpObjs& hs, const std::string& path) {
  int file_fd = ::open(("../public/files" + path).c_str(), O_RDONLY);
  if (file_fd < 0) {
    LOG_ERROR << "OPEN FILE ERROR";
    hs.response->AddHeader(":status", "300");
    hs.response->SetBody("No such file");
    return;
  }
    
  hs.response->AddHeader(":status", "200");
  hs.response->AddHeader("content-type", "application/octet-stream");
  hs.response->AddHeader("filename", path.substr(1));
    
  struct stat file_stat;
  fstat(file_fd, &file_stat);
  HttpResponse::MmapRead mr;
    
  mr.offset = 0;
  mr.size = file_stat.st_size;
  mr.addr = static_cast<uint8_t*>(mmap(
    nullptr,
    mr.size,
    PROT_READ,
    MAP_PRIVATE,
    file_fd,
    0
  ));
    
  hs.response->SetMapRead(mr);
}

void User::Upload(const HttpObjs& hs, const std::string& path) {
  std::ofstream ofs(("../public/files/" + path).c_str(), std::ios::binary | std::ios::app);
  if (!ofs.is_open()) {
    LOG_ERROR << "file :" << path << "open error";
    hs.response->AddHeader(":status", "500");
    return;
  }
  const std::string& binarys = hs.request->GetData();
  ofs.write(binarys.c_str(), binarys.size());
  ofs.close();
    
  hs.response->AddHeader(":status", "200");
}
