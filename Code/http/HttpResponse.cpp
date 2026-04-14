#include"HttpResponse.h"
#include"Buffer.h"
#include"HttpContext.h"

#include<vector>
#include<cstring>
#include<string>
#include<memory>

static ssize_t data_read_callback(nghttp2_session * session, int32_t stream_id, uint8_t * buf, size_t length, uint32_t * data_flags, nghttp2_data_source * source, void * user_data){ 
  auto data = static_cast<Buffer *>(source->ptr);
  size_t cp_len = std::min((size_t)data->readablebytes(), length);
  
  memcpy(buf, data->RetrieveAsString(cp_len).c_str(), cp_len);
 
  if(!data->readablebytes()){
    *data_flags |= NGHTTP2_DATA_FLAG_EOF;
  }
  
  return cp_len;
}
//-------------------------------------------------------------------------------

HttpResponse::HttpResponse(bool close_connection)
: close_connection_(close_connection)
{
  AddHeader(":status", "1");
  body_ = std::make_unique<Buffer>();
}

HttpResponse::~HttpResponse(){}


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
  if(body_ -> readablebytes())body_ -> RetrieveAll();
  body_ -> Append(body.c_str(), body.size());
}


void HttpResponse::SetCloseConnection(bool close_connection){
  close_connection_ = close_connection;
}

bool HttpResponse::IsInCloseConnection(){
  return close_connection_;
} 

void HttpResponse::ParseResponse(nghttp2_session * session, int stream_id){
  std::vector<nghttp2_nv>nvs;

  for(const auto &header : headers_){
    nvs.push_back(nghttp2_nv{
      (uint8_t *)header.first.data(), (uint8_t *)header.second.data(),
      header.first.size(), header.second.size(),
      NGHTTP2_NV_FLAG_NONE
    });
  }
  nghttp2_data_provider data_prd;
  data_prd.read_callback = data_read_callback;
  data_prd.source.ptr = body_.get();
 
  nghttp2_submit_response(session, stream_id, nvs.data(), nvs.size(), &data_prd);
  nghttp2_session_send(session);
} 


void HttpResponse::SetFileFd(int filefd){
  filefd_ = filefd;
}

int HttpResponse::GetFileFd(){
  return filefd_;
}




