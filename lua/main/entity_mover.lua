ent_move_handle = nil
ent_move_mode = "pos"

function sol_type(obj)
    local mt = getmetatable(obj)
    if mt and mt.__name then
        return mt.__name
    end
    return type(obj)
end

Engine.CommandProcessor.Commands:Register("move", CmdType.Server, CmdRight.Client, function(args, sender)
	if args[1] == "no" then
		ent_move_handle = nil
		return CmdStatus.OK
	end

	if #args < 2 then return CmdStatus.Error end
	
	local num_world = tonumber(args[1])
	if num_world == nil then
		Engine.Logger:Errln("World number is invalid")
		return CmdStatus.Error
	end
	
	local num_ent = tonumber(args[2])
	if num_ent == nil then
		Engine.Logger:Errln("Entity number is invalid")
		return CmdStatus.Error
	end
	
	local world = Engine.Game.Worlds:GetWorld(num_world)
	if world == nil then
		Engine.Logger:Errln("World is invalid")
		return CmdStatus.Error
	end
	
	local ent = world.Entities:GetEntityByID(num_ent)
	if ent == nil then
		Engine.Logger:Errln("Entity is invalid")
		return CmdStatus.Error
	end

	ent_move_handle = ent
	
	Engine.Logger:Outln("Moving entity "..tostring(ent_move_handle:Get():GetUUID())..":"..ent_move_handle:Get():GetType())
	
	if args[3] == "pos" then
		ent_move_mode = "pos"
	elseif args[3] == "trans" then
		ent_move_mode = "trans"
	else
		ent_move_mode = "pos"
	end
	return CmdStatus.OK
end)

Hook.Set("update", function()
	if ent_move_handle == nil then return end
	if not ent_move_handle:IsValid() then return end
	
	local ent = ent_move_handle:Get()
	if ent == nil then return end
	
	local cameraTrans = Engine:GetMainCameraTransform()
	local newTrans = CreateTransform(ent.transformable.Transform)
	
	local dist = Engine.ConVarManager:GetConVar("move.dist").Value
	
	if ent_move_mode == "pos" then
		newTrans.Position = cameraTrans.Position + (cameraTrans.Forward * dist)
	elseif ent_move_mode == "trans" then
		newTrans:SetPRS(cameraTrans:GetPRS())
		newTrans.Position = newTrans.Position + (newTrans.Forward * dist)
	end
	
	ent.transformable.Transform:SetPRS(newTrans)
end)

Hook.Set("mainInit", function()
	Engine.ConVarManager:CreateConVar("move.dist", CVarType.Float, 20.0)
end)
