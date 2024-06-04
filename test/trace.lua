wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"
wrk.body = '{"uuid":"fileuuid7", "page":0}'
--./wrk -t8 -c35000 -d120s -s ../../kserver/test/trace.lua --timeout 4  https://127.0.0.1:8099/filegettrace
-- nghttpx -f127.0.0.1,8080 -b localhost,443 --tls-cert=/home/yrb/kserver/cert/client.pem --tls-key=/home/yrb/kserver/cert/client.key
-- ./wrk -t12 -c400 -d30s -s ../../kserver/test/trace.lua --timeout 4 http://127.0.0.1:8080