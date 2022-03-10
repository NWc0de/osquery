/**
 * Copyright (c) 2014-present, The osquery authors
 *
 * This source code is licensed as defined by the LICENSE file found in the
 * root directory of this source tree.
 *
 * SPDX-License-Identifier: (Apache-2.0 OR GPL-2.0-only)
 */

#include <osquery/core/system.h>
#include <osquery/sdk/sdk.h>
#include <osquery/sql/dynamic_table_row.h>

#include "PingUtils.h"

using namespace osquery;

class PingTable : public TablePlugin {
 private:
  TableColumns columns() const {
    return {
        std::make_tuple("Host", TEXT_TYPE, ColumnOptions::INDEX),
        std::make_tuple(
            "Latency", INTEGER_TYPE, ColumnOptions::DEFAULT),
    };
  }

  TableRows generate(QueryContext& request) {
    TableRows results;
    std::set<std::string> hostList;
    PULONG pLatency;
    PCHAR pHost;
    ULONG status;

    pLatency = NULL;
    pHost = NULL;
    status = STATUS_PING_FAILED;

    if (!request.constraints["Host"].exists(EQUALS))
    {
      goto Exit;
    }

    hostList = request.constraints["Host"].getAll(EQUALS);
    pLatency = static_cast<ULONG*>(malloc(sizeof(ULONG)));

    for (std::string host : hostList)
    {
        status = PingUtils::GetPingLatency(host, pLatency);

        if (status != STATUS_SUCCESS)
        {
            continue;
        }

        auto r = make_table_row();
        r["Host"] = std::string(host);
        r["Latency"] = INTEGER(*pLatency);
        results.push_back(std::move(r));
    }

Exit:
    if (pLatency != NULL)
    {
      free(pLatency);
    }
    return results;
  }
};

REGISTER_EXTERNAL(PingTable, "table", "ping");

int main(int argc, char* argv[]) {
  ULONG status2;
  WSADATA wsaData;
  osquery::Initializer runner(argc, argv, ToolType::EXTENSION);

  status2 = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (status2 != STATUS_SUCCESS) {
	  printf("WSAStartup failed: %d\n", status2);
		return -1;
	}

  auto status = startExtension("ping", "0.0.1");
  if (!status.ok()) {
    LOG(ERROR) << status.getMessage();
    runner.requestShutdown(status.getCode());
  }

  status2 = WSACleanup();

	if (status2 != STATUS_SUCCESS)
	{
		printf("WSACleanup failed with status %d, GLE: %d\n", status, GetLastError());
	}

  // Finally wait for a signal / interrupt to shutdown.
  runner.waitForShutdown();
  return runner.shutdown(0);
}
