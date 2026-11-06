/**
 * @file translator.cpp
 * @brief Bidirectional YAML <-> .neodb translator implementation.
 */

#include <neo/translator.hpp>
#include "database_writer.hpp"
#include "yaml_decompiler.hpp"
#include "yaml_reader.hpp"

namespace neo {

bool NeoTranslator::yaml_to_neodb(const std::filesystem::path& yaml_in,
                                  const std::filesystem::path& neodb_out) {
    const auto spec = load_yaml_architecture(yaml_in);
    if (!spec.has_value()) {
        return false;
    }
    return write_neodb_file(*spec, neodb_out);
}

bool NeoTranslator::neodb_to_yaml(const std::filesystem::path& neodb_in,
                                  const std::filesystem::path& yaml_out) {
    return decompile_neodb_to_yaml(neodb_in, yaml_out);
}

} // namespace neo
