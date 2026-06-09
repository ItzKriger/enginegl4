Engine.CommandProcessor.Commands:Register("crcomp", CmdType.Server, CmdRight.Client, function(args, sender)
	if #args == 0 then
		Engine.Logger:Errln("No component type specified")
		return CmdStatus.Error
	end
	
	local comp = Engine.Components:Create(args[1])
	if comp == nil then
		Engine.Logger:Errln("Can't create component typed "..args[1])
		return CmdStatus.Error
	end

	Engine.Logger:Outln("Promised to create "..args[1])
	return CmdStatus.OK
end)
