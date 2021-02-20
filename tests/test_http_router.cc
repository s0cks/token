#include "test_http_router.h"

namespace token{
  TEST_F(HttpRouterTest, test_match){
    HttpRouterMatch m1 = FindMatch(HttpPutRequestBody("/hello/world"));
    ASSERT_TRUE(m1.IsOk());
    HttpRouterMatch m2 = FindMatch(HttpGetRequestBody("/hello/world"));
    ASSERT_TRUE(m2.IsNotSupported());
    HttpRouterMatch m3 = FindMatch(HttpPutRequestBody("/test/world"));
    ASSERT_TRUE(m3.IsNotFound());
  }

  TEST_F(HttpRouterTest, test_query_params){
    //TODO: implement
    LOG(WARNING) << "not implemented yet.";
  }

  TEST_F(HttpRouterTest, test_path_params){
    //TODO: implement
    LOG(WARNING) << "not implemented yet.";
  }
}