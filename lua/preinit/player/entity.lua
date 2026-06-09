function IsClient()
	return (Engine.Client ~= nil)
end

function IsServer()
	return (Engine.Server ~= nil)
end

function IsConnectedClient()
	local cl = Engine.Client
	if cl == nil then return false end
	
	return cl:IsTransceiverValid()
end

function detectGround(self)
	local phys = Engine.Game.Physics
	local physical = self.physical
	local transformable = self.transformable
	
	local _start = CreateTransform(transformable)
	local _end = CreateTransform(_start)
	
	local world = phys:GetWorld(0)
	
	self.onGround = false
    self.groundNormal = vec3f.new(0.0, 1.0, 0.0)
    local upVector = vec3f.new(0.0, 1.0, 0.0)
	
	local thisUnit = physical.Unit
	
	local overlaps = world:GetGhostOverlaps(thisUnit)
	local numOverlaps = overlaps:GetNumOverlappingUnits()
	
	local kGroundEpsilon = 0.05;

    local bestDot = -1.0;
    local bestNormal = CreateVector3f(upVector)
	
	for i = 0, (numOverlaps - 1) do --zero based
		local overlapUnit = overlaps:GetOverlappingUnit(i)
		if overlapUnit:GetType() ~= EUnitType.Rigid then
			local manifolds = overlaps:GetContactManifolds(overlapUnit)
			for i = 1, #manifolds do
				local manifold = manifolds[i]
				local ncontacts = manifold:GetNumContacts()
				for j = 1, #ncontacts do
					local contact = manifold:GetContact(j)
					if contact:GetDistance() < kGroundEpsilon then
						local n = CreateVector3f(contact:GetNormalWorldOnB())
						if manifold.UnitB == thisUnit then
							n = -n
						end
						
						local dot = n:dot(upVector)
						if dot > 0.6 and dot > bestDot then
							bestDot = dot
							bestNormal = n
						end
					end
				end
			end
		end
	end
	
	if bestDot > 0.6 then
		self.onGround = true
		self.groundNormal = bestNormal
	end
end

function player_init(self)
	if not IsConnectedClient() then
		
	end
end

function player_postinit(self)
end

function player_update(self)
end

local player_entity_table =
{
	type = "player",
	root_type = "entity",
	init = player_init,
	postinit = player_postinit,
	update = player_update
}

local result = Engine:Register(player_entity_table)
