#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
#include <nghttp2/nghttp2.h>
#include <map>
#include <vector>

std::string id;
int stream_id;
std::string msg;

nghttp2_session_callbacks * callbacks;
nghttp2_session * session;

static int on_invalid_frame_recv(nghttp2_session *session,
                                 const nghttp2_frame *frame,
                                 int lib_error_code,
                                 void *user_data) {
    printf("Invalid frame received!\n");
    printf("  Type:        %d\n", frame->hd.type);
    printf("  Stream ID:   %d\n", frame->hd.stream_id);
    printf("  Flags:       0x%02x\n", frame->hd.flags);
    printf("  Error code:  %d (%s)\n", lib_error_code, nghttp2_strerror(lib_error_code));

    return 0; 
}

static int on_header_callback(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data) {
    std::string header_name((const char *)name, namelen);
    std::string header_value((const char *)value, valuelen);
    std::cout<<header_name<<" "<<header_value<<std::endl;
    return 0;
}

static int on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data) {
    std::string chunk((const char *)data, len);
    std::cout<<chunk<<std::endl<<std::endl;
    return 0;
}

static ssize_t on_send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data) {
    std::string *binary_string = static_cast<std::string *>(user_data);
    binary_string->append((const char *)data, length);
    return length;
}

static ssize_t data_read_callback(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t len, uint32_t *data_flags, nghttp2_data_source *source, void *user_data) {
    std::string *data = static_cast<std::string *>(source->ptr);
    size_t cp_len = std::min(len, data->size());
    memcpy(buf, data->data(), cp_len);
    data->erase(0, cp_len);

    if (data->empty()) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    }
    return cp_len;
}

void func1(std::map<std::string,std::string> & headers, std::string & body_){
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

  body_ = body.str();
  headers.clear();
  headers[":method"] = "POST";
  headers[":scheme"] = "http";
  headers[":authority"] = "0.0.0:8888";
  headers[":path"] = "/api/user/login";
  headers["content-type"] = "application/json";
  
  id = username;
}

void func2(std::map<std::string,std::string> & headers, std::string & body_){
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
  body_ = body.str();
  headers.clear();
  headers[":method"] = "POST";
  headers[":scheme"] = "http";
  headers[":authority"] = "0.0.0:8888";
  headers[":path"] = "/api/user/register";
  headers["content-type"] = "application/json";
}

void func3(std::map<std::string,std::string> & headers, std::string & body_){
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
  body_ = body.str();
  headers.clear();
  headers[":method"] = "POST";
  headers[":scheme"] = "http";
  headers[":authority"] = "0.0.0:8888"; 
  headers[":path"] = "/api/user/profile";
  headers["content-type"] = "application/json";
}

void func4(std::map<std::string,std::string> & headers, std::string & body_){
  std::string username, msg;
  std::cout <<"请输入用户名: ";
  std::cin >> username;
  std::cout <<"请输入msg: ";
  std::cin>> msg;

  std::ostringstream body;
  body << "{"
       << "\"content\":\"" << msg << "\""
       << "}";

  body_ = body.str();
  headers.clear();
  headers[":method"] = "POST";
  headers[":scheme"] = "http";
  headers[":authority"] = "0.0.0:8888";
  headers[":path"] = "/api/transport";
  headers["content-type"] = "application/json";
  headers["sender_id"] = id;
  headers["reciever_id"] = username;  
}


void recv(int sockfd){
  bool flag = 0;
  while(1){
    char buf[1024];
    memset(buf,0,sizeof 0);
    
    if(!flag){
      flag = 1;
      nghttp2_submit_settings(session,NGHTTP2_FLAG_NONE, nullptr, 0);
      nghttp2_session_send(session);

      write(sockfd, msg.c_str(), msg.size());
      msg.clear();
      
      int n = read(sockfd,buf,sizeof buf);  
     
      //for(int i = 0;i < n;i++)printf("%02X ", (unsigned char)buf[i]);
      //puts("");

      nghttp2_session_mem_recv(session,(const uint8_t * )buf, n);
      
      //write(sockfd, msg.c_str(), msg.size());
      std::cout<<"shake hand ok !"<<std::endl;
      continue;
    }

   
    int n = read(sockfd,buf,sizeof buf);
    for(int i = 0;i < n;i++)printf("%02X ", buf[i]);
    //puts("");
   

    nghttp2_session_mem_recv(session,(const uint8_t * )buf, n);  
  }

  nghttp2_session_callbacks_del(callbacks);
  nghttp2_session_del(session);
    
}
 
void get_request(std::map<std::string,std::string> & headers, std::string & body){
  nghttp2_nv headers_nv[headers.size()];
  int i = 0;
  for(auto & kv : headers){
    headers_nv[i].name = (uint8_t *)kv.first.c_str();
    headers_nv[i].value = (uint8_t *)kv.second.c_str();
    headers_nv[i].namelen = kv.first.size();
    headers_nv[i].valuelen = kv.second.size();
    headers_nv[i].flags = NGHTTP2_NV_FLAG_NONE;
    i++;    
  }
  nghttp2_data_provider data_prd;
  data_prd.source.ptr = &body;
  data_prd.read_callback = data_read_callback;

  nghttp2_submit_request(session, nullptr, headers_nv, headers.size(), &data_prd, nullptr);
  nghttp2_session_send(session);

}

int init(){
  nghttp2_session_callbacks_new(&callbacks);
   
  nghttp2_session_callbacks_set_on_invalid_frame_recv_callback(callbacks, on_invalid_frame_recv); //debug

  nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
  nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
  nghttp2_session_callbacks_set_send_callback(callbacks, on_send_callback);
  
  nghttp2_session_client_new(&session,callbacks,&msg);
 
  
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
  return sockfd;
}

int main() {
  int sockfd = init();
  std::thread th([&sockfd](){
    recv(sockfd);
  });

  std::cout<<"登陆1，注册2，修改密码3, 发消息4 ："<<std::endl;
  while(1){
    // 5. 发送请求
    int chose = 0;
    std::cin>>chose;
    std::map<std::string,std::string> headers;
    std::string body;

    switch(chose){
      case 1: func1(headers,body); break;
      case 2: func2(headers,body); break;
      case 3: func3(headers,body); break;
      case 4: func4(headers,body); break;
    }
    if(chose > 4 || chose < 1){
      std::cout<<"无效选择"<<std::endl;
      continue;
    }
    
    msg.clear();

    get_request(headers,body);
   
    write(sockfd, msg.c_str(), msg.size());
  }



  // 7. 关闭 socket
  close(sockfd);
  return 0;
}

