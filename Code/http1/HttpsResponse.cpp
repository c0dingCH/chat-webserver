#include"HttpResponse.h"

HttpResponse::HttpResponse(bool close_connection)
: version_("HTTP/2.0"),
  status_code_(HttpStatusCode::kUnknow), 
  close_connection_(close_connection)
{}

HttpResponse::~HttpResponse(){}


void HttpResponse::SetStatusCode(HttpStatusCode status_code){
  status_code_ = status_code;
}

void HttpResponse::SetStatusMessage(const std::string & status_message){
  status_message_ = status_message;
}

void HttpResponse::SetCloseConnection(bool close_connection){
  close_connection_ = close_connection;
}

void HttpResponse::AddHeader(const std::string & key, const std::string &value){
  headers_[key] = value;
}

std::string HttpResponse::GetHeader(const std::string &key) const {
  std::string res;
  auto it = headers_.find(key);
  if(it != headers_.end())res = it->second;
  return res;
}

void HttpResponse::SetBody(const std::string & body){
  body_ = body;
  SetContentLength(body_.size());
}
void HttpResponse::SetContentLength(int content_length){
  content_length_ = content_length;
}
int HttpResponse::GetContentLength() const{
  return content_length_;
}


bool HttpResponse::IsInCloseConnection(){
  return close_connection_;
} 

std::string HttpResponse::GetMessage(){
  return GetBeforeBody() + body_ + "\r\n";

}
//这里http版本先用2.0 兼容性暂无
std::string HttpResponse::GetBeforeBody(){
  std::string message;
  message += (version_ + 
              " " +  
              std::to_string(status_code_) +
              " " + 
              status_message_ + "\r\n" 
  );
  
  if(close_connection_){
    message += "Connection: close\r\n";
  }
  else{
    message += "Content-Length: " + std::to_string(content_length_) + "\r\n";
    message += "Connection: Keep-Alive\r\n";
  }

  for(const auto &header  : headers_){
    message += header.first + ":" + header.second + "\r\n";
  }

  message += "\r\n";

  return message;
} 


void HttpResponse::SetFileFd(int filefd){
  filefd_ = filefd;
}

int HttpResponse::GetFileFd(){
  return filefd_;
}




