function setVector(vec, x, y, z)
	vec.x = x
	vec.y = y
	vec.z = z
end

function zeroVector(vec)
	setVector(vec, 0.0, 0.0, 0.0)
end

function sol_type(obj)
    local mt = getmetatable(obj)
    if mt and mt.__name then
        return mt.__name
    end
    return type(obj)
end

function camera_movement_init(self)
	Engine.Logger:Outln("self type is "..sol_type(self))

	self.cameraAngles = angles.new()
	self.target_position = vec3f.new()
	self.__dir = vec3f.new()
	self.__global_up = vec3f.new()
	
	setVector(self.__global_up, 0.0, 1.0, 0.0)
end

function camera_movement_postinit(self)
	Engine.convarmanager:CreateConVar("cam.smoothing", CVarType.Float, 5.0)
	Engine.convarmanager:GetConVar("cam.speed").Value = 100.0
end

function camera_movement_handle_orientation(self, camera_trans)
	if not Engine.WindowManager:GetRelativeMouseMode() then return end

	local mousevec = Engine.WindowManager:GetRelativeMouseState()
	local sensitivity = Engine.convarmanager:GetConVar("cam.sensitivity").Value
	
	local deltaYaw = mousevec.x * sensitivity
	local deltaPitch = mousevec.y * sensitivity
	
	self.cameraAngles.y = self.cameraAngles.y - angle.radians(deltaYaw)
	self.cameraAngles.x = self.cameraAngles.x - angle.radians(deltaPitch)

	self.cameraAngles.x:clamp180deg(-89.0, 89.0)
	
	local qPitch = Math.AngleAxis(self.cameraAngles.x, vec3f.new(1.0, 0.0, 0.0))
	local qYaw = Math.AngleAxis(self.cameraAngles.y, vec3f.new(0.0, 1.0, 0.0))
	
	local orientation = (qYaw * qPitch):normalize()
	camera_trans.Rotation = orientation
end

function camera_movement_handle_position(self, camera_trans)
	local speed = Engine.convarmanager:GetConVar("cam.speed").Value
	local smoothing = Engine.convarmanager:GetConVar("cam.smoothing").Value
	
	local isW = Engine.WindowManager:GetKeyState("w")
	local isA = Engine.WindowManager:GetKeyState("a")
	local isS = Engine.WindowManager:GetKeyState("s")
	local isD = Engine.WindowManager:GetKeyState("d")
	local isShift = Engine.WindowManager:GetKeyState("lshift")
	local isCtrl = Engine.WindowManager:GetKeyState("lctrl")
	
	zeroVector(self.__dir)
	
	local forward = camera_trans.Forward
	local right = camera_trans.Right
	local up = camera_trans.Up
	
	if isW then self.__dir = self.__dir + forward end
	if isS then self.__dir = self.__dir - forward end
	if isD then self.__dir = self.__dir + right end
	if isA then self.__dir = self.__dir - right end
	if isShift then self.__dir = self.__dir + self.__global_up end
	if isCtrl then self.__dir = self.__dir - self.__global_up end
	
	if self.__dir:getLength() > 0.0 then
		self.__dir:normalize()
	end
	
	self.target_position = self.target_position + (self.__dir * speed * Engine.Time.DeltaTime)

	local pos = camera_trans.Position
	local delta = self.target_position - pos
	local newPos = pos + delta * smoothing * Engine.Time.DeltaTime
	
	if not isNaN(newPos) then
		camera_trans.Position = newPos
	end
end

function camera_movement_update(self)
	local camera_trans = Engine:GetMainCameraTransform()
	if camera_trans == nil then return end
	
	camera_movement_handle_orientation(self, camera_trans)
	camera_movement_handle_position(self, camera_trans)
end

camera_movement_table = 
{
	type = "cameramovement",
	root_type = "component",
	init = camera_movement_init,
	postinit = camera_movement_postinit,
	update = camera_movement_update
}

Engine.Components:Register(camera_movement_table)

if Engine.WindowManager ~= nil then
	Engine.Components:Create("cameramovement", false)
	Engine.CameraMovement:SetPriority(priority.after, "windowmanager")
end
