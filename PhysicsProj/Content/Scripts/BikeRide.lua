-- 히로시 자전거 데모: 길 중심선을 따라 자동 주행 + 옆에서 따라가는 카메라
-- 키: W/A/S/D 누르면 직접 조종(카메라는 뒤에서), F 자동 주행으로 돌아가기, C 카메라 전환, R 처음으로
-- 자동 주행은 길 끝에서 멈춘다. 처음으로 돌아가는 건 R을 누르거나 길 밖으로 떨어졌을 때뿐.

local Settings = {
    TargetSpeed = 2.5,      -- m/s (천천히, 분위기 있게)
    LookAhead = 4.0,        -- 앞쪽 목표점 거리 (m)
    SteerGain = 1.6,
    MaxSteer = 0.8,
    CameraSide = 7.5,       -- 옆 거리 (망원 화각이라 멀리서 찍어야 배경이 크게 흐려진다)
    CameraHeight = 1.3,
    CameraLead = 0.8,       -- 진행 방향으로 살짝 앞
    CameraFollow = 1.5,     -- 따라가는 부드러움 (클수록 빨리)
    CameraTurn = 0.6,       -- 길 방향을 따라 도는 빠르기
    DriftSpeed = 0.25,      -- 카메라가 자전거 옆을 천천히 앞질러 가는 속도 (m/s)
    ChaseTurn = 3.0,        -- 뒤에서 따라갈 때 방향 따라가는 빠르기
    ChaseFollow = 4.0,
    ManualMaxSpeed = 4.0,   -- 직접 조종할 때 최고 속도 (m/s)
}

local UnitX = Vector.new(1, 0, 0)
local UnitY = Vector.new(0, 1, 0)
local UnitZ = Vector.new(0, 0, 1)

local State = {
    mYSign = -1,            -- 블렌더에서 만든 길의 y가 엔진에서 뒤집혔는지 (시작 때 레이캐스트로 결정)
    mAuto = true,           -- 자동 주행 중인지
    mCameraMode = 1,        -- 1 옆모습, 2 뒤에서 따라가기
}

local function Clamp(Value, Minimum, Maximum)
    return math.max(Minimum, math.min(Value, Maximum))
end

local function WrapAngle(Angle)
    return (Angle + math.pi) % (2 * math.pi) - math.pi
end

-- road_make.py와 같은 길: 직선 40 → 왼쪽 R30 35도 → 오른쪽 R30 35도 → 직선 60
local function BuildPath(YSign)
    local Points = {}
    local X, Y, H, S = 0, 0, 0, 0
    local Step = 0.5
    local function Push() Points[#Points + 1] = { x = X, y = Y * YSign, s = S } end
    Push()
    local Segments = { { 40 }, { 30, math.rad(35) }, { -30, math.rad(35) }, { 60 } }
    for _, Seg in ipairs(Segments) do
        if #Seg == 1 then
            for _ = 1, math.floor(Seg[1] / Step + 0.5) do
                X = X + Step * math.cos(H); Y = Y + Step * math.sin(H); S = S + Step
                Push()
            end
        else
            local R, Angle = Seg[1], Seg[2]
            local Length = math.abs(R) * Angle
            local Count = math.floor(Length / Step + 0.5)
            local Ds = Length / Count
            for _ = 1, Count do
                local Mid = H + (Ds / R) / 2
                X = X + Ds * math.cos(Mid); Y = Y + Ds * math.sin(Mid)
                H = H + Ds / R; S = S + Ds
                Push()
            end
        end
    end
    return Points
end

local function HitsGround(X, Y)
    local Ok, Hit = pcall(World.Raycast, Vector.new(X, Y, 5), Vector.new(0, 0, -1), Actor)
    return Ok and Hit ~= nil and Hit.IsHit
end

local function Closest(Location, Hint)
    local Best, BestDistance = Hint, math.huge
    local First = math.max(1, Hint - 20)
    local Last = math.min(#State.mPath, Hint + 40)
    for Index = First, Last do
        local P = State.mPath[Index]
        local Dx, Dy = P.x - Location.x, P.y - Location.y
        local Distance = Dx * Dx + Dy * Dy
        if Distance < BestDistance then
            Best, BestDistance = Index, Distance
        end
    end
    return Best, math.sqrt(BestDistance)
end

local function ResetBike()
    local Start = State.mSpawn
    Actor:SetActorTransform(Start)
    State.mMovement:ResetDrive()
    State.mIndex = 1
    State.mIndexStale = false
end

local function LookRotation(Direction)
    local Flat = math.sqrt(Direction.x * Direction.x + Direction.y * Direction.y)
    local Yaw = math.atan2(Direction.y, Direction.x)
    local Pitch = math.atan2(Direction.z, Flat)
    local Rotation = (Quat.new(UnitZ, Yaw) * Quat.new(UnitY, -Pitch)):Normalized()
    -- 축 방향 규칙이 반대면 피치 부호를 뒤집는다.
    if (Rotation:RotateVector(UnitX).z > 0) ~= (Direction.z > 0) and math.abs(Direction.z) > 0.01 then
        Rotation = (Quat.new(UnitZ, Yaw) * Quat.new(UnitY, Pitch)):Normalized()
    end
    return Rotation
end

local function UpdateCamera(Step, Snap)
    local Pose = Actor:GetActorTransform()
    -- 카메라는 자전거가 실제로 향한 방향을 따라간다. 차체는 균형 잡느라 조금씩 흔들리므로 부드럽게 따라간다.
    local BikeForward = Pose.Rotation:RotateVector(UnitX)
    local BikeHeading = math.atan2(BikeForward.y, BikeForward.x)
    local Turn = State.mCameraMode == 1 and Settings.CameraTurn or Settings.ChaseTurn
    if Snap or State.mCameraHeading == nil then
        State.mCameraHeading = BikeHeading
    else
        State.mCameraHeading = State.mCameraHeading + WrapAngle(BikeHeading - State.mCameraHeading) * Clamp(Step * Turn, 0, 1)
    end
    -- 옆모습에서 카메라가 뒤쪽에서 앞쪽으로 아주 천천히 흘러가며 비춘다 (-2.5m → +2.5m 왕복)
    State.mCameraTime = (Snap and 0 or (State.mCameraTime or 0)) + Step
    local Lead = Settings.CameraLead + 2.5 * math.sin(State.mCameraTime * Settings.DriftSpeed / 2.5)
    local Forward = Vector.new(math.cos(State.mCameraHeading), math.sin(State.mCameraHeading), 0)
    local Left = Vector.new(-Forward.y, Forward.x, 0)
    local Focus = Pose.Location + UnitZ * 1.0

    local Desired
    if State.mCameraMode == 1 then
        -- 옆모습 (만화 장면처럼)
        Desired = Focus + Left * Settings.CameraSide + Forward * Lead + UnitZ * (Settings.CameraHeight - 1.0)
    else
        -- 뒤에서 따라가기: 직접 조종할 때. 화면 위쪽 = 자전거가 가는 쪽
        Desired = Focus - Forward * 6.5 + UnitZ * 1.4
    end

    if Snap or State.mCameraLocation == nil then
        State.mCameraLocation = Desired
    else
        local Alpha = Clamp(Step * (State.mCameraMode == 1 and Settings.CameraFollow or Settings.ChaseFollow), 0, 1)
        State.mCameraLocation = State.mCameraLocation + (Desired - State.mCameraLocation) * Alpha
    end

    local Camera = State.mCamera:GetActorTransform()
    Camera.Location = State.mCameraLocation
    Camera.Rotation = LookRotation(Focus - State.mCameraLocation)
    State.mCamera:SetActorTransform(Camera)

    -- 초점은 늘 자전거에: 배경과 앞의 풀만 흐려진다
    if State.mCameraComponent then
        State.mCameraComponent:SetDepthOfFieldFocalDistance((Focus - State.mCameraLocation):Length())
    end
end

function BeginPlay()
    State.mMovement = assert(Actor:GetComponentByClass("UVehicleMovementComponent2W"), "Bike needs UVehicleMovementComponent2W.")
    State.mCamera = assert(World.FindActor("Bike Camera"), "The 'Bike Camera' actor is missing.")
    assert(World.SetActiveCamera(State.mCamera), "Bike Camera needs a camera component.")
    State.mCameraComponent = State.mCamera:GetCameraComponent()

    -- 배경 음악 (엔진을 새로 빌드해야 Audio가 생긴다. 없으면 음악 없이 진행)
    if Audio then
        local Ok, Error = pcall(Audio.PlayMusic, "Audio/Hirosi_BGM.mp3", 0.8, true, 5)
        if not Ok then print(Error) end
    end

    -- 바퀴 레이캐스트는 '달릴 수 있는 면'만 맞는다.
    for _, Name in ipairs({ "Road", "Road Bump" }) do
        local Found = World.FindActor(Name)
        local Mesh = Found and Found:GetComponentByClass("UStaticMeshComponent")
        if Mesh then
            Mesh:SetVehicleDrivable(true)
        end
    end

    -- 길 끝의 S자 출구(89.4, ±10.85)를 찔러 y 방향을 정한다.
    if HitsGround(89.4, 10.85) and not HitsGround(89.4, -10.85) then
        State.mYSign = 1
    elseif HitsGround(89.4, -10.85) and not HitsGround(89.4, 10.85) then
        State.mYSign = -1
    end
    State.mPath = BuildPath(State.mYSign)
    State.mSpawn = Actor:GetActorTransform()
    State.mIndex = 1
    UpdateCamera(0, true)
end

local function StartAuto()
    State.mAuto = true
    State.mCameraMode = 1
end

-- 자동 주행: 길 중심선의 앞쪽 점을 향해 조향하고 정한 속도를 유지. 길 끝에 오면 천천히 멈춘다.
local function AutoInput(Location, Forward, Speed)
    local Index = Closest(Location, State.mIndex)
    State.mIndex = Index

    if Index >= #State.mPath - 6 then
        return 0, 1, 0
    end

    local Target = State.mPath[Index]
    for Ahead = Index, #State.mPath do
        Target = State.mPath[Ahead]
        if Target.s - State.mPath[Index].s >= Settings.LookAhead then break end
    end
    local Error = WrapAngle(math.atan2(Target.y - Location.y, Target.x - Location.x) - math.atan2(Forward.y, Forward.x))
    local Steering = Clamp(Error * Settings.SteerGain, -Settings.MaxSteer, Settings.MaxSteer)
    local Throttle = Clamp((Settings.TargetSpeed - Speed) * 0.6, 0, 1)
    local Brake = Speed > Settings.TargetSpeed + 1.0 and 0.3 or 0
    return Throttle, Brake, Steering
end

-- 직접 조종: W 앞으로, S 브레이크(멈춘 뒤 계속 누르면 뒤로), A/D 조향
local function ManualInput(Speed)
    local Throttle, Brake = 0, 0
    if Input.IsDown(Key.W) then
        Throttle = Clamp((Settings.ManualMaxSpeed - Speed) * 0.6, 0, 1)
    elseif Input.IsDown(Key.S) then
        if Speed > 0.3 then Brake = 1 else Throttle = -0.4 end
    end
    local Steering = ((Input.IsDown(Key.D) and 1 or 0) - (Input.IsDown(Key.A) and 1 or 0)) * Settings.MaxSteer
    return Throttle, Brake, Steering
end

function Tick(DeltaTime)
    local Step = math.min(DeltaTime, 0.1)
    if Input.WasPressed(Key.R) then
        ResetBike()
        StartAuto()
        UpdateCamera(0, true)
        return
    end
    if Input.WasPressed(Key.C) then
        State.mCameraMode = State.mCameraMode == 1 and 2 or 1
    end
    if Input.WasPressed(Key.F) then
        -- F: 자동 주행 다시 켜기 (가장 가까운 길 지점부터 이어서)
        StartAuto()
    end
    -- WASD를 누르는 순간 직접 조종으로 바뀌고, 카메라도 뒤에서 따라가기로 바뀐다
    for _, K in ipairs({ Key.W, Key.A, Key.S, Key.D }) do
        if State.mAuto and Input.WasPressed(K) then
            State.mAuto = false
            State.mCameraMode = 2
        end
    end

    local Pose = Actor:GetActorTransform()
    local Location = Pose.Location
    local Forward = Pose.Rotation:RotateVector(UnitX)
    local _, Speed = State.mMovement:GetDriveState()

    -- 길 밖으로 떨어지면 처음으로
    if Location.z < -3 then
        ResetBike()
        StartAuto()
        UpdateCamera(0, true)
        return
    end

    local Throttle, Brake, Steering
    if State.mAuto then
        -- 직접 조종하다 돌아오면 가장 가까운 지점을 전체에서 다시 찾는다
        if State.mIndexStale then
            local Best, BestDistance = 1, math.huge
            for I, P in ipairs(State.mPath) do
                local D = (P.x - Location.x) ^ 2 + (P.y - Location.y) ^ 2
                if D < BestDistance then Best, BestDistance = I, D end
            end
            State.mIndex = Best
            State.mIndexStale = false
        end
        Throttle, Brake, Steering = AutoInput(Location, Forward, Speed)
    else
        State.mIndexStale = true
        Throttle, Brake, Steering = ManualInput(Speed)
    end
    State.mMovement:SetDriveInput(Throttle, Brake, Steering)

    UpdateCamera(Step, false)
end
