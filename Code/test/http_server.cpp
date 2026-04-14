/*
http_server_explain:
  auto close connection
  login in
  ...  

*/
#include <iostream>
#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "EventLoop.h"
#include "Logging.h"
#include "MysqlPool.h"
#include "Mysql.h"

#include "User.h"
#include "Transport.h"

#include <fcntl.h>
#include <dirent.h>
#include <thread>
#include <string>
#include <fstream>
#include <sys/stat.h>

std::string ReadFile(const char * file_path){
  std::ifstream is(file_path, std::ifstream::in);
  if(!is){
    LOG_ERROR <<"File open error !";
    return nullptr;
  }

  is.seekg(0,is.end);
  int file_len = is.tellg();
  is.seekg(0,is.beg);

  std::vector<char> buffer(file_len);
  is.read(buffer.data(), file_len);


  return std::string(buffer.data(), file_len);
}

std::vector<std::string> FindAllFiles(const char * path){
  std::vector<std::string> files(0);
  DIR * dir = ::opendir(path);
  if(!dir){
    LOG_ERROR << "Dir open error !";
    return files;
  }

  dirent * dir_entry{nullptr};

  while((dir_entry = readdir(dir)) != nullptr){
    std::string name = dir_entry->d_name;
    if(name == "." || name == "..")continue;
    files.emplace_back(std::move(name));
  }

  return files;
}

std::string BuildFileHtml(){
  std::string msg;
  std::vector<std::string>file_names = FindAllFiles("../static/files");
  
  if(file_names.size() == 0){
    msg = "<tr><td> no files </td></tr>";
  }
  else for(auto & name : file_names){
    msg += "<tr><td>" + name + "</td>" +
            "<td>" + "<br>" + 
            "<a href=\"/download/" + name + "\">下载</a>" + "<br>" + 
            "<a href=\"/delete/" + name + "\">删除</a>" + "<br>" + 
            "</td></tr>" + "<br>";
  }
  
  std::string tmp = "<!--filelist-->";
  std::string html = ReadFile("../static/fileserver.html");
  int it = html.find(tmp);
  
  if(it == -1){
    LOG_ERROR << "Can't find where the list to put";
  }
  else{
    html.replace(it,tmp.size(),msg);
  }

  return html;
}

bool RemoveFile(std::string & filename){
  if(::remove(("../static/files/" + filename).c_str()) != -1){
    return true; 
  }
  else{
    LOG_ERROR << "remove file error";
    return false;
  }
}

bool DownLoadFile(const std::string & filename, HttpResponse * response){ 
  int filefd = ::open(("../static/files/" + filename).c_str(), O_RDONLY);
  if(filefd == -1){
    LOG_ERROR << "OPEN FILE ERROR";
    return false;
  }
  
  struct stat file_state;
  fstat(filefd, &file_state);
 
  response->AddHeader("content-type","application/octet-stream");
  response->SetFileFd(filefd);
  response->AddHeader("content-length",std::to_string(file_state.st_size));
  
  return true;
}


void HttpResponseCallback(const HttpObjs & hs){
  const auto method = hs.request->GetHeader(":method");
  const auto url = hs.request->GetHeader(":path");
  
  if(method != "GET" && method != "POST"){
    hs.response->AddHeader(":status","400");
    hs.response->SetCloseConnection(true);
  }

  if(method == "GET"){
    if(url == "/"){     
      const std::string body = ReadFile("../static/files/index.html"); 
      hs.response->AddHeader(":status", "200");
      hs.response->SetBody(body);
      hs.response->AddHeader("content-type","text/html");
    }
    else if(url.substr(0,9) == "/download"){
      if(DownLoadFile(url.substr(10), hs.response)){
        hs.response->AddHeader(":status", "200");
        hs.response->AddHeader("content-type","application/octet-stream");
      }
      else{
        hs.response->AddHeader(":status", "302");
        hs.response->SetBody("Downloading error");
        hs.response->AddHeader("content-type","text/html");
      }
    }
    
    else{
      hs.response->AddHeader(":status", "404");
      hs.response->SetBody("Sorry Not Found\n");
      hs.response->SetCloseConnection(true);
    }
  }
  else if(method == "POST"){
    if(url.substr(0,7) == "/upload"){
      hs.response->AddHeader(":status", "200");
      hs.response->SetBody("Upload successfully");
      hs.response->AddHeader("content-type","text/html");
    }
    else if(url.substr(0,4) == "/api"){    
      if(url.substr(4,5) == "/user"){    
        if(url.substr(9,9) == "/register"){ //注册   
          api::user::HandleRegister(hs);    
        }
        else if(url.substr(9,6) == "/login"){//登陆
          api::user::HandleLogin(hs);    
        }
        else if(url.substr(9,8) == "/profile"){//修改
          api::user::HandleProfile(hs);    
        }
      }  
      else if(url.substr(4,10) == "/transport"){ //msg   
        api::transport::HandleMessage(hs);
      }
    }
  }
  return;
}

int main(int argc, char *argv[]){
  int port;
  if (argc <= 1)
  {
      port = 8888;
  }else if (argc == 2){
      port = atoi(argv[1]);
  }else{
      printf("error");
      exit(0);
  }
  int size = std::thread::hardware_concurrency();
  HttpServer *server = new HttpServer("0.0.0.0", port);
  server->SetHttpCallback(HttpResponseCallback);
  server->SetThreadNums(size);
  //server->SetAutoCloseConn(true);
  server->Start();
  
  //delete loop;
  //delete server;
  return 0;
}
