#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
std::string id;

void func1(std::string & req){
  std::cout<<"Login:"<<std::endl;
  std::string username, password;
  std::cout <<"请输入用户名: ";
  std::cin >> username;
  std::cout <<"请输入密码: ";
  std::cin >> password;
  std::ostringstream body;
  body << "{"
       << "\"username\":\"" << username << "\","
       << "\"password\":\"" << password << "\""
       << "}";
  req.replace(req.find("body"),4,body.str());
  req.replace(req.find("length"), 6,std::to_string(body.str().size()));
  req.replace(req.find("url"), 3, "/api/user/login");
  id = username;
}

void func2(std::string & req){
  std::cout<<"Register:"<<std::endl;
  std::string username, password;
  std::cout <<"请输入用户名: ";
  std::cin >> username;
  std::cout <<"请输入密码: ";
  std::cin >> password;
  std::ostringstream body;
  body << "{"
       << "\"username\":\"" << username << "\","
       << "\"password\":\"" << password << "\""
       << "}";
  req.replace(req.find("body"),4,body.str());
  req.replace(req.find("length"), 6, std::to_string(body.str().size()));
  req.replace(req.find("url"), 3, "/api/user/register");
}

void func3(std::string &req){
  std::cout<<"Profile:"<<std::endl;
  std::string username, password;
  std::cout <<"请输入用户名: ";
  std::cin >> username;
  std::cout <<"请输入密码: ";
  std::cin >> password;
  std::ostringstream body;
  body << "{"
       << "\"username\":\"" << username << "\","
       << "\"password\":\"" << password << "\""
       << "}";
  req.replace(req.find("body"),4,body.str());
  req.replace(req.find("length"), 6,std::to_string(body.str().size()));
  req.replace(req.find("url"), 3, "/api/user/profile");
}

void func4(std::string &req){
  std::string username, msg;
  std::cout <<"请输入用户名: ";
  std::cin >> username;
  std::cout <<"请输入msg: ";
  std::cin>> msg;

  std::ostringstream body;
  body << "{"
       << "\"content\":\"" << msg << "\""
       << "}";
  req.replace(req.find("body"),4,body.str());
  req.replace(req.find("length"), 6,std::to_string(body.str().size()));
  req.replace(req.find("url"), 3, "/api/transport");
  req.replace(req.find("sender_id"),9 , id);
  req.replace(req.find("reciever_id"),11 ,username);
}

int main() {
    const char* host = "0.0.0.0";   
    int port = 8888;                  

    // 1. 创建 socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket error");
        return -1;
    }

    // 2. 设置服务器地址
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, host, &servaddr.sin_addr);

    // 3. 连接服务器
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        close(sockfd);
        return -1;
    }
    
    std::string demo = "POST url HTTP/2.0\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: length\r\n"
      "Sender_id: sender_id\r\n"
      "Reciever_id: reciever_id\r\n"
      "\r\n"
      "body";

    std::thread th([&sockfd](){
      while(1){
        char buffer[4096];
        int n;
        n = recv(sockfd, buffer, sizeof(buffer)-1, 0); 
        buffer[n] = '\0';
        std::cout <<buffer << std::endl;
      }
    });

    std::cout<<"登陆1，注册2，修改密码3, 发消息4 ："<<std::endl;
    while(1){
      // 5. 发送请求
      std::string req = demo;
      int chose = 0;
      std::cin>>chose;

      switch(chose){
        case 1: func1(req); break;
        case 2: func2(req); break;
        case 3: func3(req); break;
        case 4: func4(req); break;
      }
      if(chose > 4 || chose < 1){
        std::cout<<"无效选择"<<std::endl;
        continue;
      }
      std::cout<<req<<std::endl; 
      send(sockfd, req.c_str(), req.size(), 0);

      // 6. 接收响应
      
    }



    // 7. 关闭 socket
    close(sockfd);
    return 0;
}

