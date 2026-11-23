function BeginPlay()
    print("[BeginPlay] " .. Actor.UUID)
end

function Tick(DeltaTime)
end

function EndPlay()
    print("[EndPlay] " .. Actor.UUID)
end
