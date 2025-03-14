/**
 *
 *  PasswordHasher.h
 *
 */

#pragma once

#include <botan/auto_rng.h>
#include <drogon/plugins/Plugin.h>

class PasswordHasher : public drogon::Plugin<PasswordHasher>
{
  public:
    void initAndStart(const Json::Value &config) override;
    void shutdown() override;

    std::expected<std::string, Botan::Exception> HashPassword(const std::string &password) const noexcept;
    std::expected<bool, Botan::Exception> VerifyPassword(const std::string &password,
                                                         const std::string &hash) const noexcept;

  private:
    uint32_t parallel_threads_{};
    uint32_t max_memory_mbs_{};
    uint32_t iterations_{};
    uint32_t thread_pool_size_{};
};
