/**
 *
 *  AuthenticationCoroFilter.h
 *
 */

#pragma once

#include <drogon/HttpFilter.h>

class AuthenticationCoroFilter : public drogon::HttpCoroFilter<AuthenticationCoroFilter>
{
  public:
    drogon::Task<drogon::HttpResponsePtr> doFilter(const drogon::HttpRequestPtr &req) override;
};
