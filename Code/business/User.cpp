#include"TcpConnection.h"
#include"HttpServer.h"
#include"HttpResponse.h"
#include"HttpRequest.h"
#include"HttpServer.h"
#include"Logging.h"

#include"User.h"
#include"Mysql.h"

#include<rapidjson/document.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<fstream>

void api::user::HandleRegister(const HttpObjs& hs){
  WithConnection(hs.server->GetMysqlPool(),[&](std::unique_ptr<Mysql> &db){
    rapidjson::Document & dom = hs.request->GetDom();       
    auto res_db = db->Insert(dom["username"].GetString(), dom["password"].GetString());
    
    hs.response->AddHeader(":status","200");

    hs.response->SetBody(db->GetMsg(res_db));
    hs.response->AddHeader("content-type","text/html");

  });
}

void api::user::HandleLogin(const HttpObjs& hs){
  WithConnection(hs.server->GetMysqlPool(),[&](std::unique_ptr<Mysql> &db){
    rapidjson::Document & dom = hs.request->GetDom();      
    auto res_db = db->Login(dom["username"].GetString(), dom["password"].GetString());
    if(res_db == Mysql::MysqlStatus::kSuccess){
      hs.server->AddConn(dom["username"].GetString(), hs.conn);
      hs.response->AddHeader(":status","200");
       
      hs.conn->SetOnCloseBusi([id = std::string(dom["username"].GetString()), server = hs.server](){ 
        server->RemoveConn(std::move(id));
      });

    }
    else{
      hs.response->AddHeader(":status","200");
    }
    
    hs.response->SetBody(db->GetMsg(res_db));
    hs.response->AddHeader("content-type","text/html");
  });
}

void api::user::HandleProfile(const HttpObjs& hs){
  WithConnection(hs.server->GetMysqlPool(),[&](std::unique_ptr<Mysql> &db){
    rapidjson::Document & dom = hs.request->GetDom();       
    auto res_db = db->Update(dom["username"].GetString(), dom["password"].GetString());
    if(res_db == Mysql::MysqlStatus::kSuccess){
      hs.response->AddHeader(":status","200");
    }
    else{
      hs.response->AddHeader(":status","200");
    }
  
    hs.response->SetBody(db->GetMsg(res_db));
    hs.response->AddHeader("content-type","text/html");
  
  });
}

//code 不能为info，想想后续如何设置请求错误的状态码



void api::user::Download(const HttpObjs & hs, const std::string & path){
  int file_fd = ::open( ("../public/files" + path).c_str() , O_RDONLY);
  if(file_fd < 0){
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
  mr.addr = static_cast<uint8_t *>(mmap(
    nullptr,
    mr.size,
    PROT_READ,
    MAP_PRIVATE,
    file_fd,
    0
  ));

  hs.response->SetMapRead(mr);
}

void api::user::Upload(const HttpObjs & hs, const std::string &path){
  std::ofstream ofs(("../public/files/" + path).c_str(), std::ios::binary | std::ios::app);
  if(!ofs.is_open()){
    LOG_ERROR << "file :"<< path <<"open error";
    hs.response->AddHeader(":status", "500");
    return;
  }
  const std::string & binarys = hs.request->GetData();
  ofs.write(binarys.c_str(), binarys.size());
  ofs.close();
  
  hs.response->AddHeader(":status", "200");

}
