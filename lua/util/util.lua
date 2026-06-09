function startsWith(str, prefix)
    return string.sub(str, 1, #prefix) == prefix
end

function endsWith(str, suffix)
    return string.sub(str, -#suffix) == suffix
end

function isNaN(x)
	if(x == nil) then return false end
	if(type(x) == "userdata") then
		if x.type ~= nil and startsWith(tostring(x), "vec") then
			for i = 1, x:getLength() do
				if(isNaN(x[i])) then return true end
			end
			return false
		end
	end
    return x ~= x
end
