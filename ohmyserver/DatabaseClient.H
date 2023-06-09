#pragma once

#include "DatabaseUtils.H"
#include "WowLogger.H"

#include <optional>

#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include "db.grpc.pb.h"

class OhMyDBClient
{
public:
    OhMyDBClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(ohmydb::OhMyDB::NewStub(channel)) {}
    int32_t Ping(int32_t cmd);

    std::optional<ohmydb::Ret> Put(int key, int value);
    std::optional<ohmydb::Ret> Get(int key);

private:
    std::unique_ptr<ohmydb::OhMyDB::Stub> stub_;
};

// Note this method is only used for Testing
inline int32_t OhMyDBClient::Ping(int32_t cmd)
{
    ohmydb::Cmd msg;
    msg.set_sup(cmd);
    ohmydb::Ack reply;

    grpc::ClientContext context;

    auto status = stub_->TestCall(&context, msg, &reply);
    if ( status.ok() ) {
        LogInfo( "Received OK on Ping" );
        return reply.ok();
    } else {
        LogWarn( "Ping RPC Failed" );
        return -1;
    }
}

inline std::optional<ohmydb::Ret> OhMyDBClient::Put(int key, int value)
{
    ohmydb::PutRequest request;
    request.set_key(key);
    request.set_value(value);
    ohmydb::PutResponse response;

    grpc::ClientContext context;

    auto status = stub_->Put(&context, request, &response);
    if ( status.ok() ) {
      return ohmydb::Ret { 
        static_cast<ohmydb::ErrorCode>(response.error_code()),
        response.leader_addr(), -1
      };
    }
    else {
      LogError("Put: RPC Failed");
      return {};
    }
}

inline std::optional<ohmydb::Ret> OhMyDBClient::Get(int key)
{
    ohmydb::GetRequest request;
    request.set_key(key);
    ohmydb::GetResponse response;

    grpc::ClientContext context;

    auto status = stub_->Get(&context, request, &response);
    if ( status.ok() ) {
        return ohmydb::Ret {
          static_cast<ohmydb::ErrorCode>(response.error_code()),
          response.leader_addr(), response.value()
        };
    } else {
        LogError("Get: RPC Failed");
        return {};
    }
}

