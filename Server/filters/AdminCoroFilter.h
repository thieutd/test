/**
 *
 *  AdminCoroFilter.h
 *
 */

#pragma once

#include <drogon/HttpFilter.h>

class AdminCoroFilter : public drogon::HttpCoroFilter<AdminCoroFilter>
{
  public:
    drogon::Task<drogon::HttpResponsePtr> doFilter(const drogon::HttpRequestPtr &req) override;
};

