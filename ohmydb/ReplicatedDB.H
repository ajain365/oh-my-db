#pragma once

#include <string>
#include <optional>
#include <utility>
#include <memory>
#include <chrono>

#include "DatabaseClient.H"

namespace ohmydb {

class ReplicatedDB {
public:
  ReplicatedDB(std::map<int32_t, ServerInfo> serverInfo);

  std::optional<int32_t> get( int32_t key );
  bool put( std::pair<int32_t, int32_t> kvp );

private:
  static constexpr const int32_t MAX_TRIES = 1000;
  OhMyDBClient client_;
  std::string serverAddr_;
  std::map<int32_t, ServerInfo> serverInfo_;
  void updateChannel(std::string serverAddr);

};

inline ReplicatedDB::ReplicatedDB( std::map<int32_t, ServerInfo> serverInfo )
  : client_( grpc::CreateChannel( std::string(serverInfo[0].ip) + ":" + std::to_string(serverInfo[0].db_port), 
                                  grpc::InsecureChannelCredentials() ) )
, serverInfo_( serverInfo )
{ }

//Update the server we are connecting to for RPC
void ReplicatedDB::updateChannel(std::string serverAddr)
{
    serverAddr_ = serverAddr;
    client_ = OhMyDBClient(
      grpc::CreateChannel( serverAddr_, grpc::InsecureChannelCredentials() ));
}

inline std::optional<int32_t> ReplicatedDB::get( int32_t key )
{
  uint32_t backupID = 0;
  auto iters = MAX_TRIES;
  while ( iters-- ) {
    // try until you find a leader
    auto retOpt = client_.Get( key );

    if ( ! retOpt.has_value()) {
      LogError( "Failed to connect to DB server: RPC Failed, contacting server " + std::to_string(backupID) );
      auto serverAddr = std::string(serverInfo_[backupID].ip) + ":" + std::to_string(serverInfo_[backupID].db_port);
      updateChannel(serverAddr);
      backupID++;
      if(backupID >= serverInfo_.size())
      {
          backupID = 0;
      }
      continue;
    } else if ( retOpt.value().errorCode == ErrorCode::NOT_LEADER ) {
      auto serverAddr = retOpt.value().leaderAddr;
      LogError( "Failed to connect to DB server: Not Leader, contacting server " + serverAddr );
      updateChannel(serverAddr);
      continue;
    }


    auto ret = retOpt.value();

    switch ( ret.errorCode ) {
      case ErrorCode::NOT_LEADER: {
        LogError("Hit NOT_LEADER in switch, this should not happen.");
        break;
      }
      case ErrorCode::KEY_NOT_FOUND: {
        return {};
      }
      case ErrorCode::OK: {
        return ret.value;
      }
    }
  }
  LogError( "Exceeded MAX_TRIES, could not find leader. Likely a bug in Consensus!");
  return {};
}

inline bool ReplicatedDB::put( std::pair<int32_t, int32_t> kvp )
{
  auto iters = MAX_TRIES;
  uint32_t backupID = 0;
  while ( iters-- ) {
    auto retOpt = client_.Put( kvp.first, kvp.second );

    //Either failed to connect to server, or its not the leader. Try another server.
    if ( ! retOpt.has_value()) {
      LogError( "Failed to connect to DB server: RPC Failed, contacting server " + std::to_string(backupID) );
      auto serverAddr = std::string(serverInfo_[backupID].ip) + ":" + std::to_string(serverInfo_[backupID].db_port);
      updateChannel(serverAddr);
      backupID++;
      if(backupID >= serverInfo_.size())
      {
          backupID = 0;
      }
      continue;
    } else if ( retOpt.value().errorCode == ErrorCode::NOT_LEADER ) {
      auto serverAddr = retOpt.value().leaderAddr;
      LogError( "Failed to connect to DB server: Not Leader, contacting server " + serverAddr );
      updateChannel(serverAddr);
      continue;
    }

    auto ret = retOpt.value();

    switch ( ret.errorCode ) {
      case ErrorCode::NOT_LEADER: {
        LogError("Hit NOT_LEADER in switch, this should not happen.");
        break;
      }
      case ErrorCode::KEY_NOT_FOUND: {
        LogError( "Unexpected error code returned by server, for put." );
        return false;
      }
      case ErrorCode::OK: {
        return !! ret.value;
      }
    }
  }

  LogError( "Exceeded MAX_TRIES, could not find leader. Likely a bug in Consensus!");
  return false;
}


} // end namespace ohmydb
