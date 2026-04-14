#include"HttpContext.h"
#include"HttpRequest.h"
#include"Logging.h"
#include"Buffer.h"

#include<memory>
#include<iostream>
#include<nghttp2/nghttp2.h>

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

static ssize_t on_send_callback(nghttp2_session * session, const uint8_t * data, size_t length, int flags, void * user_data){
  static_cast<HttpContext *>(user_data) -> Append((const char *)data, length);
  return length;
}


static int on_frame_recv_callback(nghttp2_session * session, 
                                        const nghttp2_frame * frame, 
                                        void * user_data){
//
//  puts("On frame");
//  if(frame->hd.type == NGHTTP2_SETTINGS){
//    puts("On settings");
//  }
//  else if(frame->hd.type == NGHTTP2_HEADERS){
//    puts("On headers");
//  }
//  else if(frame->hd.type == NGHTTP2_DATA){
//    puts("On data");
//  }
//  else if(frame->hd.type == NGHTTP2_GOAWAY){
//    puts("On goaway");
//  }
//  else if(frame->hd.type == NGHTTP2_PING){
//    puts("On ping");
//  }
//  else if(frame->hd.type == NGHTTP2_WINDOW_UPDATE){
//    puts("On window update");
//  }
//  else if(frame->hd.type == NGHTTP2_RST_STREAM){
//    puts("On rst stream");
//  }
//  else if(frame->hd.type == NGHTTP2_PRIORITY){
//    puts("On priority");
//  }
//  else if(frame->hd.type == NGHTTP2_CONTINUATION){
//    puts("On continuation");
//  }
//  else if(frame->hd.type == NGHTTP2_PUSH_PROMISE){
//    puts("On push promise");
//  }
//  else if(frame->hd.type == NGHTTP2_ALTSVC){
//    puts("On alt svc");
//  }
//  else if(frame->hd.type == NGHTTP2_ORIGIN){
//    puts("On origin");
//  }
//

  auto context = static_cast<HttpContext *>(user_data);
  if(frame->hd.type == NGHTTP2_SETTINGS && !(frame->hd.flags & NGHTTP2_FLAG_ACK)){
    context->SetHandshakeDone(1);
  }
  else if(frame->hd.type == NGHTTP2_HEADERS || frame->hd.type == NGHTTP2_DATA){
    context -> current_id_ = frame->hd.stream_id;
    auto request = context -> GetRequest();
    if(!request){
      LOG_ERROR << "stream id error";
      return 1;
    }

    if(frame->hd.type == NGHTTP2_HEADERS){
      request -> SetRequestStatus(HttpRequest::HttpRequestStatus::kData);
    }
    else if(frame->hd.type == NGHTTP2_DATA){
      request -> SetRequestStatus(HttpRequest::HttpRequestStatus::kComplete);
    }

  }
  return 0;
}


static int on_begin_headers_callback(nghttp2_session * session, const nghttp2_frame * frame, void *user_data){
//  puts("On begin header");
  auto context = static_cast<HttpContext *>(user_data);
  context -> current_id_ = frame->hd.stream_id;
  context -> AddStream();
  return 0;
}

static int on_header_callback(nghttp2_session *session,
                              const nghttp2_frame *frame,
                              const uint8_t *name, size_t namelen,
                              const uint8_t *value, size_t valuelen,
                              uint8_t flags, void *user_data) {
  
//  puts("On header");
  auto context = static_cast<HttpContext *>(user_data);
  auto request = context -> GetRequest();
  if(!request){
    LOG_ERROR << "stream id error";
    return 1;
  }
  else{
    request -> AddHeader(
      std::string((char*)name, namelen),
      std::string((char*)value, valuelen)
    );
  }
  return 0;
}

static int on_data_chunk_recv_callback(nghttp2_session * session, 
                                uint8_t flags, 
                                int32_t stream_id,
                                const uint8_t * data,
                                size_t len,
                                void *user_data){
//  puts("On data");
  auto context = static_cast<HttpContext *>(user_data);
  context -> current_id_ = stream_id;
  auto request = context -> GetRequest();
  if(!request){
    LOG_ERROR << "stream id error";
    return 1;
  }
  else{
    request -> Append(data, len); 
  }

  return 0;
}
static int on_stream_close_callback(nghttp2_session * session,
                             int32_t stream_id,
                             uint32_t error_code,
                             void * user_data){
  puts("on_stream_close_callback");
  auto context = static_cast<HttpContext *>(user_data);
  context -> current_id_ = stream_id;
  context -> RemoveStream();
  
  return 0;
}

//-------------------------------------------------------------------------------

HttpContext::HttpContext(){
  buffer_ = std::make_unique<Buffer>();
  

  //init_callbacks
  nghttp2_session_callbacks *callbacks = nullptr;
  nghttp2_session_callbacks_new(&callbacks);
  nghttp2_session_callbacks_set_on_invalid_frame_recv_callback(callbacks, on_invalid_frame_recv);
 
  nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks,on_frame_recv_callback);
  
  nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks,on_begin_headers_callback);
  nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
  nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks,on_data_chunk_recv_callback);
  nghttp2_session_callbacks_set_on_stream_close_callback(callbacks,on_stream_close_callback);
  
  nghttp2_session_callbacks_set_send_callback(callbacks, on_send_callback);
  callbacks_.reset(callbacks);
  
  //option

  //init_session
  nghttp2_session * session = nullptr;
  nghttp2_session_server_new(&session, callbacks, this);
  session_.reset(session);
}

HttpContext::~HttpContext(){}


void HttpContext::ParseRequest(const char * msg, size_t len){ 
//  for(size_t i = 0;i < len;i++)printf("%02X ", (unsigned char)msg[i]);
//  puts("");

  if(handshake_done_ == 0){
    nghttp2_submit_settings(session_.get(), NGHTTP2_FLAG_NONE, nullptr, 0);
  }
  

  size_t offset = 0;
  int n = 0;
  while(offset < len){
    n = nghttp2_session_mem_recv(session_.get(), (const uint8_t * )(msg + offset), len - offset);
    if(n < 0){
      LOG_ERROR << "nghttp2 parse error";
      parse_status_ = false;
      break;
    }
    offset += n;
  }

  if(handshake_done_ == 1){
    if(current_id_)handshake_done_ = 2;
    else nghttp2_session_send(session_.get());
  }
  nghttp2_session_send(session_.get());
}

void HttpContext::AddStream(){
  if(requests_.count(current_id_)){
    LOG_ERROR << "the stream"<< current_id_ <<" not been del";
  }
  requests_[current_id_] = std::make_unique<HttpRequest>(current_id_);
}

void HttpContext::RemoveStream(){
  auto it = requests_.find(current_id_);
  if(it == requests_.end()){
    LOG_ERROR << "stream remove error";
  }
  else{
    requests_.erase(it);
  }
}


HttpRequest * HttpContext::GetRequest(){
  HttpRequest * request = nullptr;
  
  auto it = requests_.find(current_id_);
  if(it != requests_.end()) request = it->second.get();

  return request;
}

void HttpContext::SetHandshakeDone(int done){
  handshake_done_ = done;
}

int HttpContext::GetHandshakeDone(){
  return handshake_done_;
}

nghttp2_session * HttpContext::GetSession() const{
  return session_.get();
}

void HttpContext::Append(const char * msg, size_t len){
  buffer_ -> Append(msg, len);
}

std::string HttpContext::GetMessage() const{
  return buffer_->RetrieveAllAsString();
}

bool HttpContext::GetParseStatus() const{
  return parse_status_;
}
