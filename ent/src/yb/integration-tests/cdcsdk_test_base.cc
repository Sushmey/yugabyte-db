// Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.

#include "yb/integration-tests/cdcsdk_test_base.h"

#include <algorithm>
#include <utility>
#include <string>
#include <chrono>
#include <boost/assign.hpp>
#include <gtest/gtest.h>

#include "yb/cdc/cdc_service.h"
#include "yb/cdc/cdc_service.proxy.h"

#include "yb/client/client.h"
#include "yb/client/meta_cache.h"
#include "yb/client/schema.h"
#include "yb/client/session.h"
#include "yb/client/table.h"
#include "yb/client/table_alterer.h"
#include "yb/client/table_creator.h"
#include "yb/client/table_handle.h"
#include "yb/client/transaction.h"
#include "yb/client/yb_op.h"

#include "yb/common/common.pb.h"
#include "yb/common/entity_ids.h"
#include "yb/common/ql_value.h"

#include "yb/gutil/stl_util.h"
#include "yb/gutil/strings/join.h"
#include "yb/gutil/strings/substitute.h"

#include "yb/integration-tests/mini_cluster.h"

#include "yb/master/catalog_manager.h"
#include "yb/master/cdc_consumer_registry_service.h"
#include "yb/master/master.h"
#include "yb/master/master_client.pb.h"
#include "yb/master/master_ddl.pb.h"
#include "yb/master/master_ddl.proxy.h"
#include "yb/master/master_replication.proxy.h"
#include "yb/master/mini_master.h"
#include "yb/master/sys_catalog_initialization.h"

#include "yb/rpc/rpc_controller.h"

#include "yb/tablet/tablet.h"
#include "yb/tablet/tablet_peer.h"

#include "yb/tserver/mini_tablet_server.h"
#include "yb/tserver/tablet_server.h"
#include "yb/tserver/ts_tablet_manager.h"

#include "yb/util/test_util.h"

#include "yb/yql/pgwrapper/libpq_utils.h"
#include "yb/yql/pgwrapper/pg_wrapper.h"

namespace yb {
using client::YBClient;
using client::YBTableName;

namespace cdc {
namespace enterprise {

void CDCSDKTestBase::TearDown() {
  YBTest::TearDown();

  LOG(INFO) << "Destroying cluster for CDCSDK";

  if (test_cluster()) {
    if (test_cluster_.pg_supervisor_) {
      test_cluster_.pg_supervisor_->Stop();
    }
    test_cluster_.mini_cluster_->Shutdown();
    test_cluster_.mini_cluster_.reset();
  }
  test_cluster_.client_.reset();
}

std::unique_ptr<CDCServiceProxy> CDCSDKTestBase::GetCdcProxy() {
  YBClient *client_ = test_client();
  const auto mini_server = test_cluster()->mini_tablet_servers().front();
  std::unique_ptr<CDCServiceProxy> proxy = std::make_unique<CDCServiceProxy>(
      &client_->proxy_cache(), HostPort::FromBoundEndpoint(mini_server->bound_rpc_addr()));
  return proxy;
}

// Create a test database to work on.
Status CDCSDKTestBase::CreateDatabase(
    Cluster* cluster,
    const std::string& namespace_name,
    bool colocated) {
  auto conn = VERIFY_RESULT(cluster->Connect());
      RETURN_NOT_OK(conn.ExecuteFormat(
      "CREATE DATABASE $0$1", namespace_name, colocated ? " colocated = true" : ""));
  return Status::OK();
}

Status CDCSDKTestBase::InitPostgres(Cluster* cluster) {
  auto pg_ts = RandomElement(cluster->mini_cluster_->mini_tablet_servers());
  auto port = cluster->mini_cluster_->AllocateFreePort();
  pgwrapper::PgProcessConf pg_process_conf =
      VERIFY_RESULT(pgwrapper::PgProcessConf::CreateValidateAndRunInitDb(
          AsString(Endpoint(pg_ts->bound_rpc_addr().address(), port)),
          pg_ts->options()->fs_opts.data_paths.front() + "/pg_data",
          pg_ts->server()->GetSharedMemoryFd()));
  pg_process_conf.master_addresses = pg_ts->options()->master_addresses_flag;
  pg_process_conf.force_disable_log_file = true;
  FLAGS_pgsql_proxy_webserver_port = cluster->mini_cluster_->AllocateFreePort();

  LOG(INFO) << "Starting PostgreSQL server listening on " << pg_process_conf.listen_addresses
            << ":" << pg_process_conf.pg_port << ", data: " << pg_process_conf.data_dir
            << ", pgsql webserver port: " << FLAGS_pgsql_proxy_webserver_port;
  cluster->pg_supervisor_ = std::make_unique<pgwrapper::PgSupervisor>(
      pg_process_conf, nullptr /* tserver */);
  RETURN_NOT_OK(cluster->pg_supervisor_->Start());

  cluster->pg_host_port_ = HostPort(pg_process_conf.listen_addresses, pg_process_conf.pg_port);
  return Status::OK();
}

// Set up a cluster with the specified parameters.
Status CDCSDKTestBase::SetUpWithParams(
    uint32_t replication_factor,
    uint32_t num_masters,
    bool colocated) {
  master::SetDefaultInitialSysCatalogSnapshotFlags();
  CDCSDKTestBase::SetUp();
  FLAGS_enable_ysql = true;
  FLAGS_master_auto_run_initdb = true;
  FLAGS_hide_pg_catalog_table_creation_logs = true;
  FLAGS_pggate_rpc_timeout_secs = 120;
  FLAGS_cdc_max_apply_batch_num_records = 1;
  FLAGS_cdc_enable_replicate_intents = true;
  FLAGS_replication_factor = replication_factor;

  MiniClusterOptions opts;
  opts.num_masters = num_masters;
  opts.num_tablet_servers = replication_factor;
  opts.cluster_id = "cdcsdk_cluster";

  test_cluster_.mini_cluster_ = std::make_unique<MiniCluster>(opts);

  RETURN_NOT_OK(test_cluster()->StartSync());
  RETURN_NOT_OK(test_cluster()->WaitForTabletServerCount(replication_factor));
  RETURN_NOT_OK(WaitForInitDb(test_cluster()));
  test_cluster_.client_ = VERIFY_RESULT(test_cluster()->CreateClient());
    RETURN_NOT_OK(InitPostgres(&test_cluster_));
    RETURN_NOT_OK(CreateDatabase(&test_cluster_, kNamespaceName, colocated));

  cdc_proxy_ = GetCdcProxy();

  LOG(INFO) << "Cluster created successfully for CDCSDK";
  return Status::OK();
}

Result<YBTableName> CDCSDKTestBase::GetTable(
    Cluster* cluster,
    const std::string& namespace_name,
    const std::string& table_name,
    bool verify_table_name,
    bool exclude_system_tables) {
  master::ListTablesRequestPB req;
  master::ListTablesResponsePB resp;

  req.set_name_filter(table_name);
  req.mutable_namespace_()->set_name(namespace_name);
  req.mutable_namespace_()->set_database_type(YQL_DATABASE_PGSQL);
  if (!exclude_system_tables) {
    req.set_exclude_system_tables(true);
    req.add_relation_type_filter(master::USER_TABLE_RELATION);
  }

  master::MasterDdlProxy master_proxy(
      &cluster->client_->proxy_cache(),
      VERIFY_RESULT(cluster->mini_cluster_->GetLeaderMasterBoundRpcAddr()));

  rpc::RpcController rpc;
  rpc.set_timeout(MonoDelta::FromSeconds(kRpcTimeout));
      RETURN_NOT_OK(master_proxy.ListTables(req, &resp, &rpc));
  if (resp.has_error()) {
    return STATUS(IllegalState, "Failed listing tables");
  }

  // Now need to find the table and return it.
  for (const auto& table : resp.tables()) {
    // If !verify_table_name, just return the first table.
    if (!verify_table_name ||
        (table.name() == table_name && table.namespace_().name() == namespace_name)) {
      YBTableName yb_table;
      yb_table.set_table_id(table.id());
      yb_table.set_namespace_id(table.namespace_().id());
      return yb_table;
    }
  }
  return STATUS_FORMAT(
      IllegalState, "Unable to find table $0 in namespace $1", table_name, namespace_name);
}

Result<YBTableName> CDCSDKTestBase::CreateTable(
    Cluster* cluster,
    const std::string& namespace_name,
    const std::string& table_name,
    const uint32_t num_tablets,
    const bool add_primary_key,
    bool colocated,
    const int table_oid,
    const bool enum_value,
    const std::string& enum_suffix,
    const std::string& schema_name) {
  auto conn = VERIFY_RESULT(cluster->ConnectToDB(namespace_name));

  if (enum_value) {
    if (schema_name != "public") {
      RETURN_NOT_OK(conn.ExecuteFormat("create schema $0;", schema_name));
    }
    RETURN_NOT_OK(conn.ExecuteFormat(
        "CREATE TYPE $0.coupon_discount_type$1 AS ENUM ('FIXED$2','PERCENTAGE$3');",
        schema_name, enum_suffix, enum_suffix, enum_suffix));
  }

  std::string table_oid_string = "";
  if (table_oid > 0) {
    // Need to turn on session flag to allow for CREATE WITH table_oid.
    RETURN_NOT_OK(conn.Execute("set yb_enable_create_with_table_oid=true"));
    table_oid_string = Format("table_oid = $0,", table_oid);
  }
  RETURN_NOT_OK(conn.ExecuteFormat(
      "CREATE TABLE $0.$1($2 int $3, $4 $5) WITH ($6colocated = $7) "
      "SPLIT INTO $8 TABLETS",
      schema_name, table_name + enum_suffix, kKeyColumnName, (add_primary_key) ? "PRIMARY KEY" : "",
      kValueColumnName,
      enum_value ? (schema_name + "." + "coupon_discount_type" + enum_suffix) : "int",
      table_oid_string, colocated, num_tablets));
  return GetTable(cluster, namespace_name, table_name + enum_suffix);
}

Result<std::string> CDCSDKTestBase::GetNamespaceId(const std::string& namespace_name) {
  master::GetNamespaceInfoResponsePB namespace_info_resp;

  RETURN_NOT_OK(test_client()->GetNamespaceInfo(
      std::string(), kNamespaceName, YQL_DATABASE_PGSQL, &namespace_info_resp));

  // Return namespace_id.
  return namespace_info_resp.namespace_().id();
}

Result<std::string> CDCSDKTestBase::GetTableId(
    Cluster* cluster,
    const std::string& namespace_name,
    const std::string& table_name,
    bool verify_table_name,
    bool exclude_system_tables) {
  master::ListTablesRequestPB req;
  master::ListTablesResponsePB resp;

  req.set_name_filter(table_name);
  req.mutable_namespace_()->set_name(namespace_name);
  req.mutable_namespace_()->set_database_type(YQL_DATABASE_PGSQL);
  if (!exclude_system_tables) {
    req.set_exclude_system_tables(true);
    req.add_relation_type_filter(master::USER_TABLE_RELATION);
  }

  master::MasterDdlProxy master_proxy(
      &cluster->client_->proxy_cache(),
      VERIFY_RESULT(cluster->mini_cluster_->GetLeaderMasterBoundRpcAddr()));

  rpc::RpcController rpc;
  rpc.set_timeout(MonoDelta::FromSeconds(kRpcTimeout));
      RETURN_NOT_OK(master_proxy.ListTables(req, &resp, &rpc));
  if (resp.has_error()) {
    return STATUS(IllegalState, "Failed listing tables");
  }

  // Now need to find the table and return it.
  for (const auto& table : resp.tables()) {
    // If !verify_table_name, just return the first table.
    if (!verify_table_name ||
        (table.name() == table_name && table.namespace_().name() == namespace_name)) {
      return table.id();
    }
  }
  return STATUS_FORMAT(
      IllegalState, "Unable to find table id for $0 in $1", table_name, namespace_name);
}

// Initialize a CreateCDCStreamRequest to be used while creating a DB stream ID.
void CDCSDKTestBase::InitCreateStreamRequest(
    CreateCDCStreamRequestPB* create_req,
    const CDCCheckpointType& checkpoint_type,
    const std::string& namespace_name) {
  create_req->set_namespace_name(namespace_name);
  create_req->set_checkpoint_type(checkpoint_type);
  create_req->set_record_type(CDCRecordType::CHANGE);
  create_req->set_record_format(CDCRecordFormat::PROTO);
  create_req->set_source_type(CDCSDK);
}

// This creates a DB stream on the database kNamespaceName by default.
Result<std::string> CDCSDKTestBase::CreateDBStream(CDCCheckpointType checkpoint_type) {
  CreateCDCStreamRequestPB req;
  CreateCDCStreamResponsePB resp;

  rpc::RpcController rpc;
  rpc.set_timeout(MonoDelta::FromMilliseconds(FLAGS_cdc_write_rpc_timeout_ms));

  InitCreateStreamRequest(&req, checkpoint_type);

  RETURN_NOT_OK(cdc_proxy_->CreateCDCStream(req, &resp, &rpc));

  return resp.db_stream_id();
}

} // namespace enterprise
} // namespace cdc
} // namespace yb
