#ifndef BITCOIN_RPC_SCHEMA_H
#define BITCOIN_RPC_SCHEMA_H

#include <map>
#include <string>
#include <vector>

class CRPCCommand;
class UniValue;

class Schema;

UniValue APISchema(const std::map<std::string, std::vector<const CRPCCommand*>>& mapCommands);

#endif // BITCOIN_RPC_SCHEMA_H
