#include "server_certificate.hpp"

#include <boost/asio/buffer.hpp>

#include <string_view>

namespace test::helpers
{
/*
    The certificate was generated from linux:
    openssl dhparam -out dh.pem 2048
    openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days \
    10000 -out cert.pem -subj \
    "/C=FR/ST=FR/L=Paris/O=fetchpp/CN=www.example.com"
*/
namespace
{
std::string_view constexpr cert =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDjTCCAnWgAwIBAgIULNgESYLQ1bgs7OhZmLBs4toaHqgwDQYJKoZIhvcNAQEL\n"
    "BQAwVjELMAkGA1UEBhMCRlIxCzAJBgNVBAgMAkZSMQ4wDAYDVQQHDAVQYXJpczEQ\n"
    "MA4GA1UECgwHZmV0Y2hwcDEYMBYGA1UEAwwPd3d3LmV4YW1wbGUuY29tMB4XDTIw\n"
    "MTExMjE2NTkxN1oXDTQ4MDMzMDE2NTkxN1owVjELMAkGA1UEBhMCRlIxCzAJBgNV\n"
    "BAgMAkZSMQ4wDAYDVQQHDAVQYXJpczEQMA4GA1UECgwHZmV0Y2hwcDEYMBYGA1UE\n"
    "AwwPd3d3LmV4YW1wbGUuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC\n"
    "AQEA8WdJAWhwNUxfcRrc8irC/TP8I7zqtFzOZAmMYnYomJ2Tg79ckyqh5njmF2ov\n"
    "Bokp5lfS3I14ollBcsi6cVkoLmy53eYzXbOgZQ4oUYAwQx+S7ICnNniI/y8do+/d\n"
    "C+9Jmjvq4wwmmQmXRmba119Ykm3lqah/fbnztcp9dtU6suZper+IQz89l/KtZXam\n"
    "c/sMHdnm41JCYPOMezx7eH667xogfdfawd7Y+0ib5ws6sgU0ADnwY0JfxDeh22gN\n"
    "BqeWSPAzMWk0jdqxifrsn7Zw+e4JgL7g5l7RmQRHr2Wmv8DNAqnJ8YWA1pk54CAT\n"
    "bjzMq5q7GYayc72J83IJzmlptwIDAQABo1MwUTAdBgNVHQ4EFgQUood+lkv+65TL\n"
    "bficEUHVyVX9238wHwYDVR0jBBgwFoAUood+lkv+65TLbficEUHVyVX9238wDwYD\n"
    "VR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAqswN9wKPhl2KthvftfNY\n"
    "0xG2QxCXnpL8mzAJFDjMTFLscGXLzFb4IMgaxuUDjCzWstnkYMVW1+HsqjeaSi2A\n"
    "HNjDxZiuwfvFx0dyCPfGZKOkVdBqAfNZOlVVW5LrJqrbcaRxIneBiecX/zIP1hAi\n"
    "U05Pxz4iMkww9ZIk3+LTXeXwlLhzOMMApRjeRBqAV1p9sk9fmpagDEJ4s++yLPY7\n"
    "fKpm1VXoLXjQN+U+m/zgMM9GCpd2jR7FB9temJ+TM/T4UN+4m8Ce004RhBuXaR+N\n"
    "Tvz1D5tytrjzv25foSSF6AC67M/FJAHxZLIyFZY/QcQ2DVbQejGzpUSKBDiv44o7\n"
    "Mg==\n"
    "-----END CERTIFICATE-----\n";

std::string_view constexpr key =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDxZ0kBaHA1TF9x\n"
    "GtzyKsL9M/wjvOq0XM5kCYxidiiYnZODv1yTKqHmeOYXai8GiSnmV9LcjXiiWUFy\n"
    "yLpxWSgubLnd5jNds6BlDihRgDBDH5LsgKc2eIj/Lx2j790L70maO+rjDCaZCZdG\n"
    "ZtrXX1iSbeWpqH99ufO1yn121Tqy5ml6v4hDPz2X8q1ldqZz+wwd2ebjUkJg84x7\n"
    "PHt4frrvGiB919rB3tj7SJvnCzqyBTQAOfBjQl/EN6HbaA0Gp5ZI8DMxaTSN2rGJ\n"
    "+uyftnD57gmAvuDmXtGZBEevZaa/wM0CqcnxhYDWmTngIBNuPMyrmrsZhrJzvYnz\n"
    "cgnOaWm3AgMBAAECggEASgIcQ649GFn9uLM/oY6ykOXrGxnD6t+49rpmq5kGASPu\n"
    "Ian9O7EKSZovEGahXCOZEOFD6BIHNd6fTg1z+2QWCYWTxg/ZF5UGS3T3r2TZZvq7\n"
    "FH6sxOpXNcTAFY5n+ky2B+7uqAUFoE1sb00BMp4sjVfb3ROnYmgr60PVCyV8BXQ9\n"
    "22be47A4FedZzcA+oiDwUwwD9cjugoe/U+BR6CmeTaJNGOVHwFv6NWISwLux0Aj/\n"
    "x6kwSsRLfBI2/4RSOyAzNMPYhLA10gL+eLe+jtP2rh/bkEcPnRgHPgwWQG9o7eqf\n"
    "9OMwAm3TtdOAl5JMSURjuDdaF0GnlIYlAdLs99G6QQKBgQD6mfNobnFb+AiE7nz7\n"
    "AXMtIJEwtI7sCvasvs5yDjZ88RFKGmtqtd0XivzdLo3bfxaHVIf+RU7NlBifmZFF\n"
    "537dSHzuZwneK42dz1QDYu5c1aBbwzI4jruJRk57AGS6FgND7a0dsqsw2CgAoIYQ\n"
    "oa6RbNUB97Cuw5PfMsyqZ8JUSQKBgQD2mpvHoNSpoy8EKcmNpzwN07EaDV6GC+f0\n"
    "VLgrQAmtDkjq1fa//bBhwku+RvVP9rTHrIyWr3NdLdtjrmd/a5ORVpgZt8K9QVDk\n"
    "sQrQFCE/yu2wY4/Rn1nWPFhGILzRLPt8WMg3xMRRriFuhO3M1QHcFbS45SOmV6oh\n"
    "tN+xUZHN/wKBgQCh62Ebw+iY3P1cLuAwKrKpoNZPGwsRts/FtA+eIFLSjcx3DfUD\n"
    "4Pg24wYZP6BHB6mdzV+FSnDtYdg7HzV/bmFJRzH5tDfrBkcdhT2qZnzPHPTc9ZV4\n"
    "d7jyrKu+y/VJSznW5TYq7yuvhqrqJM4a5uztZ92FxO2zLglYePFG1X35iQKBgF2I\n"
    "TJeN17szqoyAsPKqQGvaI+0GrxhWgba5P1UgJ8tchKmVV86AERszD3lu/nJC11R4\n"
    "jKZGi5IG55RKPPUmP0U7u9rdSN5xXJYw1DIRwH6qoDZrvMu8Dd3k63JFznfkAMqr\n"
    "/dyxI+j7C7EYd/1duSPZk78hIcFgtKWuLb3ae1vPAoGATTRkPEgaZhCi9RKpR+7S\n"
    "b3XRops02leQg/uTlllY5K0B70XNJf1I4qCRZjJPmJEBOTKlJ1iME9wPXk1Kxpm6\n"
    "QHLGlRLqZtELAC31wm1PIDAgIqQb49I/KEWqN2f8k0gngoYNdhernvAfmE5XPnZj\n"
    "G2Dp5IZB0CuJJ/TFnKu8ehE=\n"
    "-----END PRIVATE KEY-----\n";

std::string_view constexpr dh =
    "-----BEGIN DH PARAMETERS-----\n"
    "MIIBCAKCAQEAgagEkC2vRzTwFDaF7tnoaBdD9i+FKYQYDAxf8wmcU9IIOqLHW5ne\n"
    "+RrBkKkoZTkXPr0wd0L4ys1F62KYu7n9Mg1byxCDAu9SkEnw9vNEq39HfBB0ehyg\n"
    "b56gIfLKSqjmm1afCkbYlguLsohXGuyLqKhYD2ZfwjrNVQTMuwhJ95Tr3pKD/aBz\n"
    "zuRVd03PwmY4SEa3VF4yjED+RGUD4RQri5H/Jwdr4zsOAa+IR+Kvu/ljlThanZME\n"
    "m1y+bbhgBrJgpdO4zCiUZd6sbiKoywQgzV1UTummW2qHZ7lZy02huDb6kS5c4AYb\n"
    "CB7nlHVWsLzVmgVv9p7WEVRdpWQlNZeSkwIBAg==\n"
    "-----END DH PARAMETERS-----\n";
}
void load_server_certificate(boost::asio::ssl::context& ctx)
{
  ctx.set_password_callback(
      [](std::size_t, boost::asio::ssl::context_base::password_purpose) {
        return "test";
      });

  ctx.set_options(boost::asio::ssl::context::default_workarounds |
                  boost::asio::ssl::context::no_sslv2 |
                  boost::asio::ssl::context::single_dh_use);

  ctx.use_certificate_chain(boost::asio::buffer(cert.data(), cert.size()));

  ctx.use_private_key(boost::asio::buffer(key.data(), key.size()),
                      boost::asio::ssl::context::file_format::pem);

  ctx.use_tmp_dh(boost::asio::buffer(dh.data(), dh.size()));
}
}
