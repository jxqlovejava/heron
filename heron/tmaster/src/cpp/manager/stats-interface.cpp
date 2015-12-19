#include "tmaster/src/cpp/manager/stats-interface.h"

#include <iostream>
#include <sstream>

#include "basics/basics.h"
#include "errors/errors.h"
#include "threads/threads.h"
#include "network/network.h"
#include "proto/tmaster.pb.h"
#include "metrics/tmaster-metrics.h"
#include "manager/tmetrics-collector.h"

namespace heron { namespace tmaster {

StatsInterface::StatsInterface(EventLoop* eventLoop,
			       const NetworkOptions& _options,
			       TMetricsCollector* _collector)
  : metrics_collector_(_collector)
{
  http_server_ = new HTTPServer(eventLoop, _options);
  // Install the handlers
  auto cbHandleStats = [this] (IncomingHTTPRequest* request) {
    this->HandleStatsRequest(request);
  };

  auto cbHandleException =  [this] (IncomingHTTPRequest* request) {
    this->HandleExceptionRequest(request);
  };

  auto cbHandleExceptionSummary = [this] (IncomingHTTPRequest* request) {
    this->HandleExceptionSummaryRequest(request);
  };

  auto cbHandleUnknown = [this] (IncomingHTTPRequest* request) {
    this->HandleUnknownRequest(request);
  };

  http_server_->InstallCallBack("/stats", std::move(cbHandleStats));
  http_server_->InstallCallBack("/exceptions", std::move(cbHandleException));
  http_server_->InstallCallBack("/exceptionsummary", std::move(cbHandleExceptionSummary));
  http_server_->InstallGenericCallBack(std::move(cbHandleUnknown));
  CHECK(http_server_->Start() == SP_OK);
}

StatsInterface::~StatsInterface()
{
  delete http_server_;
}

void StatsInterface::HandleStatsRequest(IncomingHTTPRequest* _request)
{
  LOG(INFO) << "Got a stats request " << _request->GetQuery();
  // get the entire stuff
  unsigned char* pb = _request->ExtractFromPostData(0, _request->GetPayloadSize());
  proto::tmaster::MetricRequest req;
  if (!req.ParseFromArray(pb, _request->GetPayloadSize())) {
    LOG(ERROR) << "Unable to decipher post data specified in StatsRequest";
    http_server_->SendErrorReply(_request, 400);
    delete _request;
    return;
  }
  proto::tmaster::MetricResponse* res = metrics_collector_->GetMetrics(req);
  sp_string response_string;
  CHECK(res->SerializeToString(&response_string));
  OutgoingHTTPResponse* response = new OutgoingHTTPResponse(_request);
  response->AddHeader("Content-Type", "application/octet-stream");
  std::ostringstream content_length; content_length << response_string.size();
  response->AddHeader("Content-Length", content_length.str());
  response->AddResponse(response_string);
  http_server_->SendReply(_request, 200, response);
  delete res;
  delete _request;
  LOG(INFO) << "Done with stats request ";
}

void StatsInterface::HandleExceptionRequest(IncomingHTTPRequest* _request) {
  LOG(INFO) << "Request for exceptions" << _request->GetQuery();
  // Get the Exception request proto.
  unsigned char* request_data = _request->ExtractFromPostData(0, _request->GetPayloadSize());
  heron::proto::tmaster::ExceptionLogRequest exception_request;
  if (!exception_request.ParseFromArray(request_data, _request->GetPayloadSize())) {
    LOG(ERROR) << "Unable to decipher post data specified in ExceptionRequest" << std::endl;
    http_server_->SendErrorReply(_request, 400);
    delete _request;
    return;
  }
  heron::proto::tmaster::ExceptionLogResponse* exception_response =
      metrics_collector_->GetExceptions(exception_request);
  sp_string response_string;
  CHECK(exception_response->SerializeToString(&response_string));
  OutgoingHTTPResponse* http_response = new OutgoingHTTPResponse(_request);
  http_response->AddHeader("Content-Type", "application/octet-stream");
  std::ostringstream length_str;
  length_str << response_string.size();
  http_response->AddHeader("Content-Length", length_str.str());
  http_response->AddResponse(response_string);
  http_server_->SendReply(_request, 200, http_response);
  delete exception_response;
  delete _request;
  LOG(INFO) << "Returned exceptions response";
}

void StatsInterface::HandleExceptionSummaryRequest(IncomingHTTPRequest* _request) {
  LOG(INFO) << "Request for exception summary " << _request->GetQuery();
  unsigned char* request_data = _request->ExtractFromPostData(0, _request->GetPayloadSize());
  heron::proto::tmaster::ExceptionLogRequest exception_request;
  if (!exception_request.ParseFromArray(request_data, _request->GetPayloadSize())) {
    LOG(ERROR) << "Unable to decipher post data specified in ExceptionRequest" << std::endl;
    http_server_->SendErrorReply(_request, 400);
    delete _request;
    return;
  }
  heron::proto::tmaster::ExceptionLogResponse* exception_response =
      metrics_collector_->GetExceptionsSummary(exception_request);
  sp_string response_string;
  CHECK(exception_response->SerializeToString(&response_string));
  OutgoingHTTPResponse* http_response = new OutgoingHTTPResponse(_request);
  http_response->AddHeader("Content-Type", "application/octet-stream");
  std::ostringstream length_str;
  length_str << response_string.size();
  http_response->AddHeader("Content-Length", length_str.str());
  http_response->AddResponse(response_string);
  http_server_->SendReply(_request, 200, http_response);
  delete exception_response;
  delete _request;
  LOG(INFO) << "Returned exceptions response";
}

void StatsInterface::HandleUnknownRequest(IncomingHTTPRequest* _request)
{
  LOG(WARNING) << "Got an unknown request " << _request->GetQuery();
  http_server_->SendErrorReply(_request, 400);
  delete _request;
}

}} // end of namespace
