-- save this as post.lua
wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"
wrk.body = '{"key1":"value1", "key2":"value2"}'

-- If you need to set a specific URL path dynamically, you can modify this script to do so.
-- For example, to send requests to different endpoints:
-- local paths = {"/endpoint1", "/endpoint2", "/endpoint3"}
-- function request()
--     local path = paths[math.random(1, #paths)]
--     return wrk.format(nil, path)
-- end
-- ./wrk -t16 -c35000 -d120s -s ../../kserver/test/post.lua --timeout 4  http://127.0.0.1:8099/userregister
