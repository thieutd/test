/**
 *
 *  PasswordHasher.cc
 *
 */

#include "PasswordHasher.h"

#include <botan/argon2.h>
#include <botan/hex.h>

using namespace drogon;

void PasswordHasher::initAndStart(const Json::Value &config)
{
    parallel_threads_ = config.get("parallel_threads", 4).asUInt();
    max_memory_mbs_ = config.get("max_memory", 1024).asUInt();
    iterations_ = config.get("iterations", 2).asUInt();
    thread_pool_size_ = config.get("thread_pool_size", std::thread::hardware_concurrency() >> 1).asUInt();
// REFERENCE: https://github.com/randombit/botan/blob/bc555cd3c114497a50b49c4649d6606150881f5b/src/lib/utils/thread_utils/thread_pool.cpp#L20
#if defined(_WIN32) && defined(_MSC_VER)
    _putenv_s("BOTAN_THREAD_POOL_SIZE", std::to_string(thread_pool_size_).c_str());
#else
    setenv("BOTAN_THREAD_POOL_SIZE", std::to_string(thread_pool_size_).c_str(), 0);
#endif
}

void PasswordHasher::shutdown()
{
}

std::expected<std::string, Botan::Exception> PasswordHasher::HashPassword(const std::string &password) const noexcept
{
    try
    {
        Botan::AutoSeeded_RNG rng;
        return Botan::argon2_generate_pwhash(password.data(), password.length(), rng, parallel_threads_,
                                              max_memory_mbs_, iterations_);
    }
    catch (const Botan::Exception &e)
    {
        return std::unexpected(e);
    }
}

std::expected<bool, Botan::Exception> PasswordHasher::VerifyPassword(const std::string &password,
                                                                     const std::string &hash) const noexcept
{
    try
    {
        return Botan::argon2_check_pwhash(password.data(), password.length(), hash);
    }
    catch (const Botan::Exception &e)
    {
        return std::unexpected(e);
    }
}
