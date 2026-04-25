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
#include <vector>
#include <string>
#include <sstream>

namespace {
  typedef std::pair<std::string,std::string> PSS;
  const std::vector<PSS>field_vals_model11 = {{"user_id", ""}};
  const std::vector<PSS>field_vals_model12 = {{"password",""}};
  const std::vector<PSS>field_vals_model2 = {{"user_id", ""}, {"password",""}};
  
  std::vector<PSS> GetFieldVals(const std::string &username, const std::string &password){
    auto res = field_vals_model2;
    res[0].second = username;
    res[1].second = password;
    return res;
  }
  std::vector<PSS> GetFieldVals1(const std::string &username){
    auto res = field_vals_model11;
    res[0].second = username;
    return res;
  }
  std::vector<PSS> GetFieldVals2(const std::string &username){
    auto res = field_vals_model12;
    res[0].second = username;
    return res;
  }
}

void User::HandleRegister(const HttpObjs& hs) {
  rapidjson::Document& dom = hs.request->GetDom();
  std::string username = dom["username"].GetString();
  std::string password = dom["password"].GetString();

  std::string msg;
  hs.server->GetMysqlPool()->WithConnection([&](std::unique_ptr<Mysql>& db) {
    db->Insert(std::string("users", 5), GetFieldVals(username, password), msg);
  });

  hs.response->AddHeader(":status", "200");
  hs.response->SetBody(msg);
  hs.response->AddHeader("content-type", "text/html");
}

void User::HandleLogin(const HttpObjs& hs) {
  rapidjson::Document& dom = hs.request->GetDom();
  std::string username = dom["username"].GetString();
  std::string password = dom["password"].GetString();
  std::string msg;

  bool res_db = false;
  hs.server->GetMysqlPool()->WithConnection([&](std::unique_ptr<Mysql>& db) {
    res_db = db->Select(std::string("users", 5), GetFieldVals1(username), msg);
  });

  std::stringstream msg_in(msg);
  std::string id = std::string("password", 8), last;
  while (msg_in >> msg) {
    if (last == id) break;
    last = msg;
  }

  if (msg == "NULL") msg.clear();

  if(res_db){
    if (msg != password) {
      msg = "password incorrect";
    } else {
      msg = "success login";
      User* user = hs.server->AddUser(username);
      user->SetUsername(username);
      hs.conn->SetUser(user);
      hs.server->AddConn(username, hs.conn);
      hs.conn->SetOnCloseBusi([user_name = username, server = hs.server]() {
        server->RemoveUser(user_name);
        server->RemoveConn(user_name);
      });
    }
  }else{
    msg = "no such username";
  }

  hs.response->AddHeader(":status", "200");
  hs.response->SetBody(msg);
  hs.response->AddHeader("content-type", "text/html");
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
  rapidjson::Document& dom = hs.request->GetDom();
  std::string username = dom["username"].GetString();
  std::string password = dom["password"].GetString();
  std::string msg;

  hs.server->GetMysqlPool()->WithConnection([&](std::unique_ptr<Mysql>& db) {
    db->Update(std::string("users", 5), GetFieldVals1(username), GetFieldVals2(password), msg);
  });

  hs.response->AddHeader(":status", "200");
  hs.response->SetBody(msg);
  hs.response->AddHeader("content-type", "text/html");
}

void User::HandleTransport(const HttpObjs& hs) {
  std::string content; // whitch is insert into db
  std::string receiver_id = hs.request->GetHeader("receiver_id");
  std::string sender_id = hs.request->GetHeader("sender_id");
  const std::shared_ptr<TcpConnection>& receiver_conn = hs.server->GetConn(receiver_id);
  if (receiver_conn) {
    HttpContext* context = receiver_conn->GetContext();
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
      path = "/from_id/" + sender_id;
    } 
    else {
      std::string filename = hs.request->GetHeader("filename");
      dom.AddMember("filename", rapidjson::Value(filename.c_str(), allocator), allocator);
      dom.AddMember("download", rapidjson::Value(("/download/" + filename + " -button").c_str(), allocator), allocator);
      path = "../public/files/" + filename;
           
      std::ofstream ofs(path.c_str(), std::ios::binary); // cover the init file , waiting for handle
      const std::string data = hs.request->GetData();
      ofs.write(data.c_str(), data.size());
            
      path = path.substr(2);
    }
    
    content = hs.request->ParseJson2String(dom);
    std::string msg;
    std::vector<PSS> field_vals = {{"sender_id", sender_id}, {"receiver_id", receiver_id}, {"content",content}}; 
    hs.server -> GetMysqlPool() -> WithConnection([&](std::unique_ptr<Mysql> &db){
      db -> Insert("msgs", field_vals, msg);
    });
    
    hs.response->SetBody(msg);
    
    response.SetBody(content);
    response.ParseResponse(context->GetSession(), response.PushPromise(
      context->GetSession(),
      context->push_id_,
      "POST",
      "http",
      hs.server->GetAuthority(),
      path
    ));
     
    receiver_conn->Send(context->GetMessage());
  }
  else{  // no such user's conn
    hs.response -> SetBody(receiver_id + " is not online");
  }

  hs.response->AddHeader(":status", "200");
  hs.response->AddHeader("content-type", "text/html");
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
