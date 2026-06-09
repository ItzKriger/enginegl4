function createCircularBuffer(limit)
    local buffer = {}
    local size = 0
    local head = 1

    return
	{
        push = function(val)
            buffer[head] = val
            head = head % limit + 1
            if size < limit then
                size = size + 1
            end
        end,
        getValues = function()
            local result = {}
            for i = 1, size do
                local pos = (head - size + i - 1) % limit + 1
                table.insert(result, buffer[pos])
            end
            return result
        end,
        count = function() return size end
    }
end
