local StandardMesh = "Meshes/Sisyphus/Standard.masset"
local StandardSkeleton = "Meshes/Sisyphus/StandardSkeleton.masset"

function BeginPlay()
    local Cloth = assert(Actor:GetSkeletalMeshComponent())
    Cloth:SetSkeletalMesh("", StandardSkeleton)
    assert(Cloth:SetSkeletalMesh(StandardMesh, StandardSkeleton), "Could not initialize cloth on " .. Actor:GetName())
end
