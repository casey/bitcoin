#include <rpc/schema.h>

#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/string.h>

using util::SplitString;

class Schema {
public:
    static UniValue Command(const std::string& category, const RPCHelpMan& command) {
        UniValue value{UniValue::VOBJ};

        value.pushKV("name", command.m_name);
        value.pushKV("category", category);
        value.pushKV("description", command.m_description);

        auto examples = Schema::Examples(command.m_examples);

        if (examples) {
            value.pushKV("examples", examples.value());
        }

        UniValue arguments{UniValue::VOBJ};

        arguments.pushKV("$schema", "https://json-schema.org/draft/2020-12/schema");
        arguments.pushKV("$id", std::format("https://bitcoincore.org/{}-arguments.json", command.m_name));
        arguments.pushKV("type", "object");

        if (!command.m_args.empty()) {
            UniValue required{UniValue::VARR};

            UniValue properties{UniValue::VOBJ};
            for (const auto& argument : command.m_args) {
                assert(!argument.m_names.empty());

                bool argument_required = std::holds_alternative<RPCArg::Optional>(argument.m_fallback)
                    && std::get<RPCArg::Optional>(argument.m_fallback) == RPCArg::Optional::NO;

                for (auto const& name: SplitString(argument.m_names, '|')) {
                    assert(!properties.exists(name));
                    properties.pushKV(name, Schema::Argument(argument));
                    if (argument_required) {
                        required.push_back(name);
                    }
                }
            }
            arguments.pushKV("properties", properties);

            if (!required.empty()) {
                arguments.pushKV("required", required);
            }

            arguments.pushKV("additionalProperties", false);
        }

        value.pushKV("arguments", arguments);

        return value;
    }

    static UniValue Argument(const RPCArg& argument) {
        UniValue value{UniValue::VOBJ};

        if (!argument.m_description.empty()) {
            value.pushKV("description", argument.m_description);
        }

        value.pushKV("type", Schema::ArgumentType(argument.m_type));

        if (argument.m_type == RPCArg::Type::STR_HEX) {
            value.pushKV("pattern", "^([0-9][a-f]{2})+$");
        }

        if (argument.m_type == RPCArg::Type::OBJ_NAMED_PARAMS) {
            value.pushKV("format", "named");
        }

        if (!argument.m_inner.empty()) {
            assert(
                argument.m_type == RPCArg::Type::ARR
                || argument.m_type == RPCArg::Type::OBJ
                || argument.m_type == RPCArg::Type::OBJ_NAMED_PARAMS
                || argument.m_type == RPCArg::Type::OBJ_USER_KEYS
            );

            if (argument.m_type == RPCArg::Type::ARR) {
                UniValue items{UniValue::VARR};
                for (const auto& inner : argument.m_inner) {
                    items.push_back(Argument(inner));
                }
                assert(!argument.m_inner.empty());
                value.pushKV("items", items);
            } else if (argument.m_type == RPCArg::Type::OBJ_USER_KEYS) {
                assert(argument.m_inner.size() == 1);
                value.pushKV("additionalProperties", Schema::Argument(argument.m_inner[0]));
            } else {
                UniValue properties{UniValue::VOBJ};
                UniValue required{UniValue::VARR};
                for (const auto& inner : argument.m_inner) {
                    assert(!inner.m_names.empty());
                    bool argument_required = std::holds_alternative<RPCArg::Optional>(inner.m_fallback)
                        && std::get<RPCArg::Optional>(inner.m_fallback) == RPCArg::Optional::NO;
                    for (auto const& name: SplitString(inner.m_names, '|')) {
                        assert(!properties.exists(name));
                        properties.pushKV(name, Argument(inner));
                        if (argument_required) {
                            required.push_back(name);
                        }
                    }
                }
                value.pushKV("properties", properties);
                value.pushKV("additionalProperties", false);
                if (!required.empty()) {
                    value.pushKV("required", required);
                }
            }
        }

        return value;
    }

    static UniValue ArgumentType(const RPCArg::Type& type) {
        UniValue value{UniValue::VARR};

        switch (type) {
            case RPCArg::Type::AMOUNT:
                value.push_back("number");
                value.push_back("string");
                return value;
            case RPCArg::Type::ARR:
                return "array";
            case RPCArg::Type::BOOL:
                return "boolean";
            case RPCArg::Type::NUM:
                return "number";
            case RPCArg::Type::OBJ:
                return "object";
            case RPCArg::Type::OBJ_NAMED_PARAMS:
                return "object";
            case RPCArg::Type::OBJ_USER_KEYS:
                return "object";
            case RPCArg::Type::RANGE:
                value.push_back("number");
                value.push_back("array");
                return value;
            case RPCArg::Type::STR:
                return "string";
            case RPCArg::Type::STR_HEX:
                return "string";
            default:
                NONFATAL_UNREACHABLE();
        }
    }

private:
    static std::optional<UniValue> Examples(const RPCExamples& examples) {
        if (examples.m_examples.empty()) {
            return {};
        } else {
            return {examples.m_examples};
        }
    }
};

UniValue APISchema(const std::map<std::string, std::vector<const CRPCCommand*>>& mapCommands) {
    UniValue value{UniValue::VOBJ};

    UniValue commands{UniValue::VOBJ};

    for (const auto& entry: mapCommands) {
        assert(entry.second.size() == 1);

        UniValue aliases{UniValue::VARR};

        for (const auto& command: entry.second) {
            RPCHelpMan man = ((RpcMethodFnType)command->unique_id)();
            aliases.push_back(Schema::Command(command->category, man));
        }

        commands.pushKV(entry.first, aliases);
    }

    value.pushKV("commands", commands);

    return value;
}
