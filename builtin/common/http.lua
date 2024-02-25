-- HTTP callback interface

core.set_http_api_lua(function(httpenv)
	httpenv.fetch = function(req, callback)
		local handle = httpenv.fetch_async(req)

		local function update_http_status()
			local res = httpenv.fetch_async_get(handle)
			if res.completed then
				callback(res)
			else
				core.after(0, update_http_status)
			end
		end
		core.after(0, update_http_status)
	end

	return httpenv
end)
core.set_http_api_lua = nil
