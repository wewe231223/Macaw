local Settings = {
    WalkSpeed = 4.4,
    PushWalkSpeed = 1.15,
    Acceleration = 24.0,
    Gravity = 17.0,
    JumpSpeed = 5.2,
    FloorSnapDistance = 0.24,
    WalkableFloorZ = 0.7,
    PushRange = 1.72,
    PushReleaseRange = 1.80,
    PushPoseRange = 1.90,
    PushClearance = 1.54,
    PushSlopeClearance = 0.20,
    PushSpeed = 1.02,
    PushGain = 1600.0,
    PushSteeringGain = 470.0,
    MaxPushForce = 950.0,
    PushClipSpeed = 0.30,
    AirborneDelay = 0.09,
    AirborneDrop = 0.14,
    SummitRadius = 1.6,
    FallHeight = -8.0,
    CameraDistance = 5.4,
    LookSensitivity = 0.003,
    CapsuleRadius = 0.30,
    CapsuleSegment = 0.60,
    BoulderRadius = 0.92,
    BoulderImpactSpeed = 0.85,
    StoneFriction = 0.5,
    IceStaticFriction = 0.022,
    IceDynamicFriction = 0.014,
    IceGravity = 9.81,
    IceMaxSlideSpeed = 6.5,
    IceSlipEffort = 1.16,
    IceSlipSeconds = 0.53,
    IceSlipCooldown = 1.58,
    PendulumDrop = 3.35,
    PendulumRadius = 0.44,
    RamTravel = 4.05,
    RamRadius = 0.58,
    PlateCooldown = 2.3,
    PlateForce = 1000000,
    PlatePulseSeconds = 0.08,
    PlateFlightSeconds = 1.22,
    PlateEscapeHeight = 320,
    PlayerMass = 81.3,
    BoulderMass = 110,
    RagdollSeconds = 2.50,
    RecoveryGrace = 2.0,
    LimbReactTime = 0.24,
    LimbBlendIn = 0.08,
    LimbBlendOut = 0.24,
    LimbBlendWeight = 0.92,
    RockWarning = 1.0,
    RockLifetime = 8.3,
}

local RockRadii = {0.37, 0.47, 0.32}
local State = {mPartialLimbs = {}}
local Zero = Vector.new()
local UnitX = Vector.new(1, 0, 0)
local UnitY = Vector.new(0, 1, 0)
local Up = Vector.new(0, 0, 1)
local PlateDirections = {
    Vector.new(-0.25, 1, 0),
    Vector.new(-0.30, -1, 0),
    Vector.new(-0.35, -1, 0),
    Vector.new(-1, -0.35, 0),
    Vector.new(-0.50, -1, 0),
    Vector.new(-1, -0.45, 0),
    Vector.new(-0.18, -1, 0),
    Vector.new(-0.30, 1, 0),
    Vector.new(-0.45, 1, 0),
    Vector.new(-1, 0.35, 0),
}
local TrapImpactBones = {
    "pelvis", "spine_01", "spine_02", "spine_03", "neck_01", "Head",
    "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l",
    "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r",
    "thigh_l", "calf_l", "foot_l", "thigh_r", "calf_r", "foot_r",
}
local ArmLimbs = {
    {mRoot = "upperarm_l", mImpact = "lowerarm_l"},
    {mRoot = "upperarm_r", mImpact = "lowerarm_r"},
}
local LegLimbs = {
    {mRoot = "thigh_l", mImpact = "calf_l"},
    {mRoot = "thigh_r", mImpact = "calf_r"},
}
local RockfallSlopes = {
    {mOrigin = Vector.new(16.7, 0, 4.92), mUphill = UnitX, mLength = 16.7, mWidth = 7.7},
    {mOrigin = Vector.new(25.0, 24.6, 11.46), mUphill = UnitY, mLength = 16.6, mWidth = 6.75},
    {mOrigin = Vector.new(49.4, 33.0, 19.33), mUphill = UnitX, mLength = 17.4, mWidth = 5.15},
}

local function Clamp(Value, Minimum, Maximum)
    return math.max(Minimum, math.min(Value, Maximum))
end

local function Smooth(Current, Target, Response, Step)
    return Target + (Current - Target) * math.exp(-Response * Step)
end

local function WrapAngle(Angle)
    return (Angle + math.pi) % (2 * math.pi) - math.pi
end

local function Horizontal(Value)
    return Vector.new(Value.x, Value.y, 0)
end

local function Dot(A, B)
    return A.x * B.x + A.y * B.y + A.z * B.z
end

local function Axis(Positive, Negative)
    return (Input.IsDown(Positive) and 1 or 0) - (Input.IsDown(Negative) and 1 or 0)
end

local function UpdateCamera(Step)
    local Flight = State.mFlight
    local Pitch = Flight and (-0.44 + 0.81 * Clamp(Flight.mElapsed / 0.55, 0, 1)) or State.mPitch
    local Rotation = (Quat.new(Up, State.mYaw) * Quat.new(UnitY, -Pitch)):Normalized()
    local Target = Actor:GetActorLocation() + Up * 0.78
    if Flight then
        Target = Flight.mTarget:GetActorLocation() + Up * 0.55
    elseif State.mKnockedDown then
        Target = State.mMesh:GetBoneWorldLocation("pelvis") + Up * 0.48
    end
    State.mCameraTarget = Flight and Target or Target + (State.mCameraTarget - Target) * math.exp(-18 * Step)
    Target = State.mCameraTarget
    local Back = Rotation:RotateVector(UnitX) * -1
    if Flight then
        State.mCameraArm = 10.5
    else
        local Obstacle = World.Raycast(Target, Back, Actor)
        local Limit = Obstacle.IsHit and Clamp(Obstacle.Distance - 0.18, 0.22, Settings.CameraDistance) or Settings.CameraDistance
        State.mCameraArm = math.min(Limit, State.mCameraArm + Step * 9)
    end

    local Camera = State.mCamera:GetActorTransform()
    Camera.Location = Target + Back * State.mCameraArm
    Camera.Rotation = Rotation
    State.mCamera:SetActorTransform(Camera)
end

local function UpdateStatus()
    local Flight = State.mFlight
    if Flight then
        local Altitude = math.max(0, Flight.mTarget:GetActorLocation().z - Flight.mStartHeight)
        State.mProgress:SetText(string.format("ALTITUDE   %04.0f m", Altitude))
        State.mPrompt:SetText(Altitude >= Settings.PlateEscapeHeight and "ORBITAL DELIVERY   /   BACK TO EARTH" or "1,000,000 N   /   LIFTOFF")
        return
    end

    local Height = Clamp(State.mStone:GetActorLocation().z - State.mStoneSpawn.Location.z, 0, State.mSummit.z - State.mStoneSpawn.Location.z)
    local Time = math.floor(State.mElapsed)
    State.mProgress:SetText(string.format("%04.1f / %.0f m    |    %d:%02d", Height, State.mSummit.z - State.mStoneSpawn.Location.z, math.floor(Time / 60), Time % 60))

    if State.mFinished then
        State.mPrompt:SetText("SUMMIT REACHED   /   R TO TRY AGAIN")
    elseif State.mKnockedDown then
        State.mPrompt:SetText("KNOCKED DOWN")
    elseif State.mWarnedSlope then
        State.mPrompt:SetText("ROCKFALL AHEAD   /   MOVE ASIDE")
    elseif State.mRetryNotice > 0 then
        State.mPrompt:SetText("BACK AT THE FOOT OF THE HILL")
    else
        State.mPrompt:SetText("WASD  MOVE     MOUSE  LOOK     E + MOVE  PUSH     SPACE  JUMP     R  RESTART")
    end
end

local function ParkRock(Rock)
    Rock.mMesh:SetHiddenInGame(true)
    Rock.mBody:SetSimulatePhysics(false)
    Rock.mActor:SetActorTransform(Rock.mParked)
    Rock.mTime = 0
    Rock.mReleaseAt = nil
    Rock.mSlope = nil
end

local function ResetCourseHazards()
    State.mHazardTime = 0
    if State.mFlight and State.mController and State.mController:IsValid() then
        State.mController:SetTickGroup(State.mControllerTickGroup)
    end
    State.mFlight = nil
    for _, Pendulum in ipairs(State.mPendulums or {}) do
        Pendulum.mPreviousLog = nil
        if Pendulum.mActor:IsValid() then
            Pendulum.mActor:SetActorTransform(Pendulum.mRest)
        end
    end
    for _, Ram in ipairs(State.mRams or {}) do
        Ram.mPrevious = Ram.mRest.Location
        if Ram.mActor:IsValid() then
            Ram.mActor:SetActorTransform(Ram.mRest)
        end
    end
    for _, Plate in ipairs(State.mPlates or {}) do
        Plate.mRemaining = 0
        Plate.mWasMoving = false
        if Plate.mActor:IsValid() then
            Plate.mActor:SetActorTransform(Plate.mRest)
        end
    end
end

local function ResetIceGrip()
    State.mIceEffort = 0
    State.mIceSlipRemaining = 0
    State.mIceSlipCooldown = 0
    State.mIceSlipBlend = 0
    State.mCanIceSlip = State.mMesh:SetAnimGraphFloat("SlipBlend", 0)
end

local function ResetRun(Fell)
    if State.mKnockedDown or next(State.mPartialLimbs) then
        State.mMesh:SetSkeletalCollisionEnabled(false)
    end
    State.mPartialLimbs = {}
    for _, Rock in ipairs(State.mRocks) do
        ParkRock(Rock)
    end
    ResetCourseHazards()
    Actor:SetActorTransform(State.mPlayerSpawn)
    State.mStone:SetActorTransform(State.mStoneSpawn)
    assert(State.mStoneBody:SetPhysicsVelocity(Zero, Zero), "The boulder needs a simulated sphere body.")

    State.mVelocity = Zero
    State.mStoneVelocity = Zero
    State.mLastStonePosition = State.mStoneSpawn.Location
    State.mVerticalSpeed = 0
    State.mGrounded = true
    State.mFloorNormal = Up
    State.mSurfaceFriction = Settings.StoneFriction
    State.mJumping = false
    State.mAirTime = 0
    State.mGroundHeight = State.mPlayerSpawn.Location.z
    State.mPushing = false
    State.mAnimationSpeed = 0
    State.mPushBlend = 0
    State.mYaw = 0
    State.mPitch = -0.19
    State.mFacing = 0
    State.mCameraArm = Settings.CameraDistance
    State.mCameraTarget = State.mPlayerSpawn.Location + Up * 0.78
    State.mElapsed = 0
    State.mStarted = false
    State.mFinished = false
    State.mRetryNotice = Fell and 2.4 or 0
    State.mKnockedDown = false
    State.mRecoverDelay = 0
    State.mImpactCooldown = 0
    State.mPreviousPlayer = State.mPlayerSpawn.Location
    State.mWaveDelay = 2.1 + math.random() * 1.5
    State.mWarnedSlope = nil
    ResetIceGrip()
    State.mMesh:SetAnimGraphFloat("Speed", 0)
    State.mMesh:SetAnimGraphFloat("VerticalSpeed", 0)
    State.mMesh:SetAnimGraphFloat("PushBlend", 0)
    State.mMesh:SetAnimGraphFloat("PushRate", 0)
    State.mMesh:SetAnimGraphBool("IsInAir", false)

    UpdateCamera(0)
    UpdateStatus()
end

local function SweepMove(Delta)
    return select(2, Actor:AddActorWorldOffset(Delta, true))
end

local function IsWalkable(Hit)
    return Hit.BlockingHit and Hit.Normal.z >= Settings.WalkableFloorZ
end

local function SetFloor(Hit)
    State.mFloorNormal = Hit.Normal
    State.mSurfaceFriction = State.mUseSurfaceFriction and Hit.SurfaceFriction or Settings.StoneFriction
end

local function KnockDown(Velocity, Position, TrapHit)
    if not State.mCanRagdoll or not State.mMesh:SetSkeletalCollisionEnabled(true) then
        return
    end
    if not State.mMesh:SetAllBodiesBelowSimulatePhysics("root", true) or not State.mMesh:IsBodySimulatingPhysics("pelvis") then
        State.mMesh:SetSkeletalCollisionEnabled(false)
        State.mPartialLimbs = {}
        return
    end

    State.mPartialLimbs = {}
    State.mMesh:SetAllBodiesBelowPhysicsBlendWeight("root", 1)
    local Push = Horizontal(Velocity)
    local TripLeg = Position.z < Actor:GetActorLocation().z and "calf_l" or "spine_02"
    if TrapHit then
        local Launch = Push:Normalized() * math.min(Push:Length() * 1.12 + 1.4, 10.0) + Up * 2.15
        for _, Bone in ipairs(TrapImpactBones) do
            State.mMesh:AddImpulseToBody(Bone, Launch, true)
        end
        State.mMesh:AddImpulseToBody(TripLeg, Launch * 0.18, true)
    else
        local Launch = Push:Normalized() * math.min(Push:Length() * 0.54, 4.2) + Up * 0.55
        State.mMesh:AddImpulseToBody("pelvis", Launch, true)
        State.mMesh:AddImpulseToBody(TripLeg, Launch * 1.35, true)
    end
    State.mKnockedDown = true
    State.mRecoverDelay = Settings.RagdollSeconds
    State.mPushing = false
    State.mPushBlend = 0
    ResetIceGrip()
    State.mMesh:SetAnimGraphFloat("PushBlend", 0)
    State.mMesh:SetAnimGraphFloat("PushRate", 0)
    State.mMesh:SetAnimGraphFloat("Speed", 0)
    State.mMesh:SetAnimGraphBool("IsInAir", false)
end

local function TryLimbHit(Velocity, RelativeTravel, Contact, Radius, Position)
    if not State.mCanRagdoll or Contact.z < -0.90 or Contact.z > 0.67 then
        return false
    end

    local Direction = Horizontal(RelativeTravel)
    if Direction:LengthSquared() < 0.000001 then
        return false
    end
    local Across = Vector.new(-Direction.y, Direction.x, 0):Normalized()
    if math.abs(Dot(Across, Contact)) < Radius + 0.085 then
        return false
    end

    local Limbs = Contact.z > -0.15 and ArmLimbs or LegLimbs
    local Point = Position + Contact
    local Left = State.mMesh:GetBoneWorldLocation(Limbs[1].mImpact)
    local Right = State.mMesh:GetBoneWorldLocation(Limbs[2].mImpact)
    if not Left or not Right then
        return false
    end
    local Limb = (Point - Left):LengthSquared() <= (Point - Right):LengthSquared() and Limbs[1] or Limbs[2]
    local Reaction = State.mPartialLimbs[Limb]
    if not Reaction then
        if not State.mMesh:SetSkeletalCollisionEnabled(true) then
            return false
        end
        State.mMesh:SetAllBodiesBelowSimulatePhysics(Limb.mRoot, true)
        if not State.mMesh:IsBodySimulatingPhysics(Limb.mImpact) then
            State.mMesh:SetAllBodiesBelowSimulatePhysics(Limb.mRoot, false)
            if not next(State.mPartialLimbs) then
                State.mMesh:SetSkeletalCollisionEnabled(false)
            end
            return false
        end
        Reaction = {mWeight = 0}
        State.mPartialLimbs[Limb] = Reaction
        State.mMesh:SetAllBodiesBelowPhysicsBlendWeight(Limb.mRoot, 0)
    end
    Reaction.mRemaining = Settings.LimbReactTime
    local Push = Horizontal(Velocity)
    local Deflect = (Push:Normalized() * 0.78 - Horizontal(Contact):Normalized() * 0.42):Normalized()
    local Kick = Deflect * math.min(Push:Length() * 0.48, 3.2) + Up * 0.42
    State.mMesh:AddImpulseToBody(Limb.mImpact, Kick, true)
    State.mMesh:AddImpulseToBody(Limb.mRoot, Kick * 0.50, true)
    return true
end

local function UpdatePartialRagdoll(DeltaTime)
    if not next(State.mPartialLimbs) then
        return
    end

    for Limb, Reaction in pairs(State.mPartialLimbs) do
        Reaction.mRemaining = Reaction.mRemaining - DeltaTime
        local Change = Reaction.mRemaining > 0 and DeltaTime / Settings.LimbBlendIn or -DeltaTime / Settings.LimbBlendOut
        Reaction.mWeight = Clamp(Reaction.mWeight + Change, 0, Settings.LimbBlendWeight)
        State.mMesh:SetAllBodiesBelowPhysicsBlendWeight(Limb.mRoot, Reaction.mWeight)
        if Reaction.mWeight == 0 then
            State.mMesh:SetAllBodiesBelowSimulatePhysics(Limb.mRoot, false)
            State.mPartialLimbs[Limb] = nil
        end
    end
    if not next(State.mPartialLimbs) then
        State.mMesh:SetSkeletalCollisionEnabled(false)
    end
end

local function StandAt(Position)
    local Previous = Actor:GetActorLocation()
    Actor:SetActorLocation(Position + Up * 1.25)
    local Floor = SweepMove(Up * -1.95)
    if IsWalkable(Floor) and not Floor.StartPenetrating then
        SetFloor(Floor)
        return true
    end
    Actor:SetActorLocation(Previous)
    return false
end

local function UpdateRecovery(DeltaTime)
    local Pelvis = State.mMesh:GetBoneWorldLocation("pelvis")
    if Pelvis.z < Settings.FallHeight then
        ResetRun(true)
        return
    end

    State.mRecoverDelay = State.mRecoverDelay - DeltaTime
    if State.mRecoverDelay > 0 then
        return
    end

    if not StandAt(Pelvis) then
        if State.mRecoverDelay > -0.55 then
            return
        end
        if not StandAt(Actor:GetActorLocation() - Up * 0.70) then
            ResetRun(true)
            return
        end
    end

    State.mMesh:SetSkeletalCollisionEnabled(false)
    State.mKnockedDown = false
    State.mImpactCooldown = Settings.RecoveryGrace
    State.mVelocity = Zero
    State.mVerticalSpeed = 0
    State.mAnimationSpeed = 0
    State.mGrounded = true
    State.mJumping = false
    State.mAirTime = 0
    State.mGroundHeight = Actor:GetActorLocation().z
    State.mPreviousPlayer = Actor:GetActorLocation()
end

local function GetRockfallSlope(Position)
    for _, Slope in ipairs(RockfallSlopes) do
        local Offset = Position - Slope.mOrigin
        local Along = Dot(Offset, Slope.mUphill)
        if Along >= -Slope.mLength and Along <= -4 and Horizontal(Offset - Slope.mUphill * Along):LengthSquared() <= Slope.mWidth ^ 2 and math.abs(Offset.z) < 9 then
            return Slope
        end
    end
    return nil
end

local function QueueRockfall(Slope)
    local Order = {1, 2, 3}
    for Index = #Order, 2, -1 do
        local Swap = math.random(Index)
        Order[Index], Order[Swap] = Order[Swap], Order[Index]
    end
    local Count = math.random() < 0.20 and 2 or 3
    local ReleaseAt = State.mHazardTime
    for Index = 1, Count do
        local Rock = State.mRocks[Order[Index]]
        if Rock.mTime <= 0 and not Rock.mReleaseAt then
            Rock.mSlope = Slope
            Rock.mReleaseAt = ReleaseAt
        end
        ReleaseAt = ReleaseAt + 0.29 + math.random() * 0.64
    end
end

local function ReleaseRock(Rock, Lane)
    local Slope = Rock.mSlope
    local Across = Vector.new(-Slope.mUphill.y, Slope.mUphill.x, 0)
    local Spacing = math.min(Slope.mWidth * 0.52, 3.6) * (0.73 + math.random() * 0.27)
    local Lateral = Lane * Spacing + (math.random() - 0.5) * 1.45
    local Height = Rock.mRadius + 2.15 + math.random() * 3.35
    local Position = Slope.mOrigin + Slope.mUphill * ((math.random() - 0.5) * 1.24) + Across * Lateral + Up * Height
    local Speed = 3.8 + math.random() * 2.4
    local CrossSpeed = (math.random() - 0.5) * 3.8 - Lateral * 0.14
    local Velocity = Slope.mUphill * -Speed + Across * CrossSpeed + Up * (-1.6 + math.random() * 3.5)
    local Spin = Across * (-Speed * (0.55 + math.random() * 0.8) / Rock.mRadius) + Slope.mUphill * ((math.random() - 0.5) * 18)
    Rock.mActor:SetActorLocation(Position)
    Rock.mBody:SetSimulatePhysics(true)
    Rock.mBody:SetPhysicsVelocity(Velocity, Spin)
    Rock.mMesh:SetHiddenInGame(false)
    Rock.mTime = Settings.RockLifetime
    Rock.mPrevious = Position
    Rock.mHitPlayer = false
    Rock.mReleaseAt = nil
    Rock.mSlope = nil
end

local function GetCapsuleGap(Offset)
    return Offset - Up * Clamp(Offset.z, -Settings.CapsuleSegment, Settings.CapsuleSegment)
end

local function GetSweptRockContact(Start, Finish, Radius)
    local Travel = Finish - Start
    local FlatTravel = Horizontal(Travel)
    local Cap = Up * Settings.CapsuleSegment
    local BestDistance = (Radius + Settings.CapsuleRadius) ^ 2
    local Contact = nil
    local TravelSq = Travel:LengthSquared()
    local FlatTravelSq = FlatTravel:LengthSquared()
    local Times = {
        FlatTravelSq > 0 and -Dot(Start, FlatTravel) / FlatTravelSq or 0,
        TravelSq > 0 and Dot(Cap - Start, Travel) / TravelSq or 0,
        TravelSq > 0 and Dot(Cap * -1 - Start, Travel) / TravelSq or 0,
    }
    for _, Time in ipairs(Times) do
        local Point = Start + Travel * Clamp(Time, 0, 1)
        local Gap = GetCapsuleGap(Point)
        local Distance = Gap:LengthSquared()
        if Distance <= BestDistance then
            BestDistance = Distance
            Contact = Point
        end
    end
    return Contact
end

local function UpdateBoulderImpact(Position)
    if State.mFinished or State.mKnockedDown or State.mImpactCooldown > 0 then
        return
    end

    local Start = State.mLastStonePosition - State.mPreviousPlayer
    local Finish = Position - Actor:GetActorLocation()
    local Gap = GetCapsuleGap(Start)
    if -Dot(State.mStoneVelocity, Gap:Normalized()) >= Settings.BoulderImpactSpeed and Dot(Finish - Start, Gap) < 0 and GetSweptRockContact(Start, Finish, Settings.BoulderRadius) then
        KnockDown(State.mStoneVelocity, Position)
    end
end

local function UpdatePendulums(DeltaTime, Position)
    for _, Pendulum in ipairs(State.mPendulums) do
        local Pose = Pendulum.mActor:GetActorTransform()
        local Angle = 1.06 * math.sin(State.mHazardTime * Pendulum.mRate + Pendulum.mPhase)
        Pose.Rotation = Quat.new(Pendulum.mAxis, Angle) * Pendulum.mRest.Rotation
        Pendulum.mActor:SetActorTransform(Pose)

        local Log = Pose.Location + Pose.Rotation:RotateVector(Up) * -Settings.PendulumDrop
        local Previous = Pendulum.mPreviousLog or Log
        local Velocity = (Log - Previous) * (1 / DeltaTime)
        Pendulum.mPreviousLog = Log
        if not State.mFinished and not State.mKnockedDown and State.mImpactCooldown == 0 and Velocity:LengthSquared() > 1.44 and math.abs(Dot(Position - Log, Pendulum.mAxis)) < 2.1 then
            for Sample = -2, 2 do
                local Offset = Pendulum.mAxis * (Sample * 0.43)
                if GetSweptRockContact(Previous + Offset - State.mPreviousPlayer, Log + Offset - Position, Settings.PendulumRadius) then
                    KnockDown(Velocity, Log, true)
                    break
                end
            end
        end
    end
end

local function BoulderTouchesPlate(Plate, Position)
    local Offset = Position - Plate.mRest.Location
    local Height = Dot(Offset, Plate.mNormal)
    if Height < 0.09 then
        return false
    end
    local ForwardGap = math.max(0, math.abs(Dot(Offset, Plate.mSlopeForward)) - 0.95)
    local RightGap = math.max(0, math.abs(Dot(Offset, Plate.mRight)) - 0.93)
    return ForwardGap ^ 2 + RightGap ^ 2 + (Height - 0.09) ^ 2 <= Settings.BoulderRadius ^ 2
end

local function StartPlateFlight(Plate, LaunchPlayer, LaunchBoulder)
    local Direction = (Up + Plate.mLaunchDirection * 0.25):Normalized()
    local Impulse = Settings.PlateForce * Settings.PlatePulseSeconds
    if LaunchBoulder then
        local Spin = Vector.new(-Plate.mLaunchDirection.y, Plate.mLaunchDirection.x, 0) * 16 + Up * 5
        local Velocity = State.mStoneVelocity + Direction * (Impulse / Settings.BoulderMass)
        LaunchBoulder = State.mStoneBody:SetPhysicsVelocity(Velocity, Spin)
    end
    if not LaunchPlayer and not LaunchBoulder then
        return
    end

    if next(State.mPartialLimbs) then
        State.mMesh:SetSkeletalCollisionEnabled(false)
        State.mPartialLimbs = {}
    end
    local Target = LaunchPlayer and Actor or State.mStone
    State.mFlight = {
        mElapsed = 0,
        mTarget = Target,
        mStartHeight = Target:GetActorLocation().z,
        mPlayerVelocity = LaunchPlayer and Direction * (Impulse / Settings.PlayerMass) or nil,
    }
    Plate.mRemaining = Settings.PlateCooldown
    State.mPushing = false
    State.mPushBlend = 0
    ResetIceGrip()
    State.mMesh:SetAnimGraphFloat("Speed", 0)
    State.mMesh:SetAnimGraphFloat("PushBlend", 0)
    State.mMesh:SetAnimGraphFloat("PushRate", 0)
    if LaunchPlayer then
        State.mGrounded = false
        State.mJumping = true
        State.mMesh:SetAnimGraphBool("IsInAir", true)
        State.mMesh:SetAnimGraphFloat("VerticalSpeed", Settings.JumpSpeed)
    end
    if State.mController then
        State.mController:SetTickGroup(TickGroup.PostPhysics)
    end
end

local function UpdateLaunchPlates(DeltaTime, Position)
    local StonePosition = State.mStone:GetActorLocation()
    for _, Plate in ipairs(State.mPlates) do
        Plate.mRemaining = math.max(0, Plate.mRemaining - DeltaTime)
        if Plate.mRemaining == 0 and not State.mFinished and not State.mFlight then
            local Offset = Position - Plate.mRest.Location
            local PlayerHit = State.mGrounded and not State.mKnockedDown and math.abs(Dot(Offset, Plate.mForward)) < 0.86 and math.abs(Dot(Offset, Plate.mRight)) < 0.88 and Offset.z > 0.54 and Offset.z < 1.22
            local BoulderHit = BoulderTouchesPlate(Plate, StonePosition)
            if PlayerHit or BoulderHit then
                StartPlateFlight(Plate, PlayerHit, BoulderHit)
            end
        end

        local Lift = math.max(0, 1 - (Settings.PlateCooldown - Plate.mRemaining) / 0.28)
        if Lift > 0 or Plate.mWasMoving then
            local Pose = Plate.mActor:GetActorTransform()
            Pose.Location = Plate.mRest.Location + Up * (0.67 * Lift * Lift)
            Plate.mActor:SetActorTransform(Pose)
            Plate.mWasMoving = Lift > 0
        end
    end
end

local function UpdatePlateFlight(DeltaTime)
    local Flight = State.mFlight
    Flight.mElapsed = Flight.mElapsed + DeltaTime
    if Flight.mElapsed >= Settings.PlateFlightSeconds then
        ResetRun(true)
        return
    end

    if Flight.mPlayerVelocity then
        Flight.mPlayerVelocity = Flight.mPlayerVelocity - Up * (Settings.Gravity * DeltaTime)
        local Hit = SweepMove(Flight.mPlayerVelocity * DeltaTime)
        if Hit.BlockingHit then
            Flight.mPlayerVelocity = Flight.mPlayerVelocity - Hit.Normal * math.min(0, Dot(Flight.mPlayerVelocity, Hit.Normal))
        end
    end
    UpdateCamera(DeltaTime)
    UpdateStatus()
end

local function UpdateStoneRams(DeltaTime, Position)
    for _, Ram in ipairs(State.mRams) do
        local Phase = (State.mHazardTime + Ram.mPhase) % Ram.mPeriod
        local Extension = math.min(Clamp(Phase / 0.25, 0, 1), Clamp((0.91 - Phase) / 0.43, 0, 1))
        local Location = Ram.mRest.Location + Ram.mAxis * (Settings.RamTravel * Extension)
        local Movement = Location - Ram.mPrevious
        if Movement:LengthSquared() > 0.0000001 then
            Ram.mActor:SetActorLocation(Location)
        end
        if not State.mFinished and not State.mKnockedDown and State.mImpactCooldown == 0 and Dot(Movement, Ram.mAxis) > 0.75 * DeltaTime and math.abs(Dot(Position - Location, Ram.mForward)) < 1.35 then
            for Sample = -1, 1 do
                local Offset = Ram.mForward * (Sample * 0.41) + Ram.mAxis * 0.31
                if GetSweptRockContact(Ram.mPrevious + Offset - State.mPreviousPlayer, Location + Offset - Position, Settings.RamRadius) then
                    KnockDown(Movement * (1 / DeltaTime), Location, true)
                    break
                end
            end
        end
        Ram.mPrevious = Location
    end
end

local function UpdateCourseHazards(DeltaTime)
    State.mHazardTime = State.mHazardTime + DeltaTime
    local Position = Actor:GetActorLocation()
    UpdatePendulums(DeltaTime, Position)
    UpdateLaunchPlates(DeltaTime, Position)
    UpdateStoneRams(DeltaTime, Position)
end

local function UpdateRockfall(DeltaTime)
    local Position = Actor:GetActorLocation()
    for Index, Rock in ipairs(State.mRocks) do
        if Rock.mReleaseAt and State.mHazardTime >= Rock.mReleaseAt then
            if State.mFinished then
                Rock.mReleaseAt = nil
                Rock.mSlope = nil
            else
                ReleaseRock(Rock, Index - 2)
            end
        elseif Rock.mTime > 0 then
            local Current = Rock.mActor:GetActorLocation()
            local Velocity = (Current - Rock.mPrevious) * (1 / DeltaTime)
            if not Rock.mHitPlayer and not State.mFinished and not State.mKnockedDown and State.mImpactCooldown == 0 and Velocity:LengthSquared() >= 2.25 then
                local Start = Rock.mPrevious - State.mPreviousPlayer
                local Finish = Current - Position
                local Contact = GetSweptRockContact(Start, Finish, Rock.mRadius)
                if Contact then
                    Rock.mHitPlayer = true
                    if not TryLimbHit(Velocity, Finish - Start, Contact, Rock.mRadius, Position) then
                        KnockDown(Velocity, Current)
                    end
                end
            end
            Rock.mPrevious = Current
            Rock.mTime = Rock.mTime - DeltaTime
            if Rock.mTime <= 0 or Current.z < Settings.FallHeight then
                ParkRock(Rock)
            end
        end
    end
    State.mPreviousPlayer = Position

    local Slope = not State.mFinished and not State.mKnockedDown and GetRockfallSlope(Position) or nil
    if State.mWarnedSlope and Slope ~= State.mWarnedSlope then
        State.mWaveDelay = math.max(State.mWaveDelay, 2)
        State.mWarnedSlope = nil
    end
    if Slope then
        State.mWaveDelay = State.mWaveDelay - DeltaTime
        State.mWarnedSlope = State.mWaveDelay <= Settings.RockWarning and Slope or nil
        if State.mWaveDelay <= 0 then
            QueueRockfall(Slope)
            State.mWarnedSlope = nil
            State.mWaveDelay = 10.8 + math.random() * 5.2
        end
    end
end

local function MoveHorizontal(Delta)
    for Attempt = 1, 3 do
        if Delta:LengthSquared() < 0.00000001 then
            return
        end

        local Hit = SweepMove(Delta)
        if not Hit.BlockingHit then
            return
        end

        local Normal = Hit.Normal
        if not IsWalkable(Hit) then
            Normal = Horizontal(Normal):Normalized()
        end

        local Remaining = Delta * (1 - Hit.Time)
        local IntoSurface = Dot(Remaining, Normal)
        if IntoSurface >= -0.0001 then
            return
        end

        Delta = Remaining - Normal * IntoSurface
    end
end

local function MoveVertical(Distance)
    local Hit = SweepMove(Up * Distance)
    if Distance > 0 then
        if Hit.BlockingHit then
            State.mVerticalSpeed = 0
        end

        State.mGrounded = false
        return
    end

    if not Hit.BlockingHit and State.mGrounded then
        local Start = Actor:GetActorLocation()
        Hit = SweepMove(Up * -Settings.FloorSnapDistance)
        if not IsWalkable(Hit) then
            Actor:SetActorLocation(Start)
        end
    end

    State.mGrounded = IsWalkable(Hit)
    if State.mGrounded then
        SetFloor(Hit)
    end
    if State.mGrounded or Hit.BlockingHit then
        State.mVerticalSpeed = 0
    end
end

local function GetPushClearance(Height)
    return Settings.PushClearance + Clamp(Height, 0, 0.65) * Settings.PushSlopeClearance
end

local function IsOnIce()
    return State.mGrounded and State.mSurfaceFriction < Settings.StoneFriction
end

local function GetIceGrip()
    if not IsOnIce() or State.mIceSlipRemaining == 0 then
        return 1
    end
    local Recovery = Clamp((Settings.IceSlipSeconds - State.mIceSlipRemaining) / 0.34, 0, 1)
    return 0.25 + 0.75 * Recovery
end

local function UpdateIceGrip(Direction, Step)
    State.mIceSlipRemaining = math.max(0, State.mIceSlipRemaining - Step)
    State.mIceSlipCooldown = math.max(0, State.mIceSlipCooldown - Step)
    if not IsOnIce() then
        State.mIceEffort = 0
        State.mIceSlipRemaining = 0
        return
    end
    if not State.mCanIceSlip or State.mIceSlipCooldown > 0 then
        return
    end
    if Direction:LengthSquared() < 0.01 or Input.WasPressed(Key.Space) then
        State.mIceEffort = math.max(0, State.mIceEffort - Step * 1.6)
        return
    end

    local Downhill = Horizontal(State.mFloorNormal):Normalized()
    local UphillEffort = math.max(0, -Dot(Direction, Downhill))
    local Turning = State.mVelocity:LengthSquared() > 0.45 ^ 2 and Clamp((1 - Dot(Direction, State.mVelocity:Normalized())) * 0.5, 0, 1) or 0
    local Strain = 0.63 + UphillEffort * 0.49 + Turning * 0.82 + (State.mPushing and 0.24 or 0)
    State.mIceEffort = State.mIceEffort + Step * Strain
    if State.mIceEffort < Settings.IceSlipEffort then
        return
    end

    State.mIceEffort = 0
    State.mIceSlipCooldown = Settings.IceSlipCooldown
    if State.mMesh:RestartAnimGraphPlayer("Ice Stumble") then
        State.mIceSlipRemaining = Settings.IceSlipSeconds
        local Along = Dot(State.mVelocity, Downhill)
        local SlipKick = State.mPushing and 0.48 or 0.88
        State.mVelocity = State.mVelocity + Downhill * (SlipKick - math.min(Along, 0) * 0.82)
    end
end

local function GetPushPoseTarget(Direction, Position)
    if State.mFinished then
        return 0
    end

    if State.mPushing then
        return 1
    end

    local Offset = State.mStone:GetActorLocation() - Position
    local Toward = Horizontal(Offset)
    local Distance = Toward:Length()
    if Distance < 0.1 or Distance > Settings.PushPoseRange or math.abs(Offset.z) > 0.85 then
        return 0
    end

    if State.mPushBlend > 0.2 and Input.IsDown(Key.E) and Dot(Direction, Toward) >= 0.45 * Distance then
        return 1
    end

    local Facing = Vector.new(math.cos(State.mFacing), math.sin(State.mFacing), 0)
    if Distance >= GetPushClearance(Offset.z) and Distance <= Settings.PushReleaseRange and Dot(Facing, Toward) >= 0.65 * Distance and Dot(State.mStoneVelocity, Toward) < -0.45 * Distance then
        return 0.8
    end

    return 0
end

local function UpdateAnimation(Previous, Position, PushTarget, Step, DeltaTime)
    if State.mGrounded then
        State.mJumping = false
        State.mAirTime = 0
        State.mGroundHeight = Position.z
    else
        State.mAirTime = State.mAirTime + Step
    end

    local Falling = State.mAirTime >= Settings.AirborneDelay and Position.z < State.mGroundHeight - Settings.AirborneDrop
    local InAir = State.mJumping or Falling
    PushTarget = InAir and 0 or PushTarget
    if PushTarget > 0 then
        local Offset = State.mStone:GetActorLocation() - Position
        local Clearance = GetPushClearance(Offset.z)
        PushTarget = PushTarget * Clamp(1 - (Clearance - Horizontal(Offset):Length()) / 0.16, 0, 1)
    end
    local Travel = Horizontal(Position - Previous)
    local Speed = Travel:Length() / Step
    State.mAnimationSpeed = Smooth(State.mAnimationSpeed, Speed, 12, Step)
    State.mPushBlend = Smooth(State.mPushBlend, PushTarget, PushTarget > State.mPushBlend and 18 or 12, Step)
    local Facing = Vector.new(math.cos(State.mFacing), math.sin(State.mFacing), 0)
    local Rate = PushTarget > 0 and Dot(Travel, Facing) / (Settings.PushClipSpeed * DeltaTime) or 0
    local SlipWeight = 0
    if IsOnIce() and State.mIceSlipRemaining > 0 then
        local Age = Settings.IceSlipSeconds - State.mIceSlipRemaining
        SlipWeight = 0.94 * math.min(Age / 0.055, State.mIceSlipRemaining / 0.145, 1)
    end
    State.mIceSlipBlend = InAir and 0 or Smooth(State.mIceSlipBlend, SlipWeight, 25, Step)

    State.mMesh:SetAnimGraphFloat("Speed", State.mAnimationSpeed)
    State.mMesh:SetAnimGraphFloat("VerticalSpeed", State.mVerticalSpeed)
    State.mMesh:SetAnimGraphFloat("PushBlend", State.mPushBlend)
    State.mMesh:SetAnimGraphFloat("PushRate", Rate)
    State.mMesh:SetAnimGraphFloat("SlipBlend", State.mIceSlipBlend)
    State.mMesh:SetAnimGraphBool("IsInAir", InAir)
end

local function MovePlayer(Direction, Step, DeltaTime)
    local Previous = Actor:GetActorLocation()
    local HoldingPush = GetPushPoseTarget(Direction, Previous) == 1
    local Desired = Direction * Settings.WalkSpeed
    local Toward, Distance, Clearance
    if HoldingPush then
        local Offset = State.mStone:GetActorLocation() - Previous
        local HorizontalOffset = Horizontal(Offset)
        Toward = HorizontalOffset:Normalized()
        Distance = HorizontalOffset:Length()
        Clearance = GetPushClearance(Offset.z)
        Desired = (Direction * 0.60 + Toward * 0.40):Normalized() * Settings.PushWalkSpeed
    end
    local Difference = Desired - State.mVelocity
    local Change = Settings.Acceleration * (State.mGrounded and 1 or 0.4) * Step
    local OnIce = IsOnIce()
    if OnIce then
        local Traction = Clamp(State.mSurfaceFriction / Settings.StoneFriction, 0, 1)
        local Grip = GetIceGrip()
        if HoldingPush then
            Grip = 0.28 + 0.72 * Grip
        end
        Change = Direction:LengthSquared() > 0 and Change * (0.29 + 0.71 * Traction) * Grip or Settings.IceGravity * State.mSurfaceFriction * State.mFloorNormal.z * Step
    end
    State.mVelocity = State.mVelocity + (Difference:LengthSquared() > Change * Change and Difference:Normalized() * Change or Difference)
    if OnIce then
        State.mVelocity = State.mVelocity + Horizontal(State.mFloorNormal) * (Settings.IceGravity * State.mFloorNormal.z * Step)
        if State.mVelocity:LengthSquared() > Settings.IceMaxSlideSpeed ^ 2 then
            State.mVelocity = State.mVelocity:Normalized() * Settings.IceMaxSlideSpeed
        end
    end

    if Input.WasPressed(Key.Space) and State.mGrounded and not HoldingPush then
        State.mVerticalSpeed = Settings.JumpSpeed
        State.mGrounded = false
        State.mJumping = true
    end

    local Delta = State.mVelocity * Step
    if HoldingPush then
        local Allowed = math.max(Distance - Clearance, -Settings.PushWalkSpeed * Step)
        Delta = Delta - Toward * math.max(Dot(Delta, Toward) - Allowed, 0)
    end

    MoveHorizontal(Delta)
    State.mVerticalSpeed = State.mVerticalSpeed - Settings.Gravity * Step
    MoveVertical(State.mVerticalSpeed * Step)

    local Player = Actor:GetActorTransform()
    local PushTarget = GetPushPoseTarget(Direction, Player.Location)
    local Facing = PushTarget == 1 and Horizontal(State.mStone:GetActorLocation() - Player.Location) or Direction
    if Facing:LengthSquared() > 0.0001 then
        local Target = math.atan2(Facing.y, Facing.x)
        local Turn = WrapAngle(Target - State.mFacing)
        State.mFacing = WrapAngle(State.mFacing + Clamp(Turn, -12 * Step, 12 * Step))
    end

    Player.Rotation = Quat.new(Up, State.mFacing) * State.mPlayerSpawn.Rotation
    Actor:SetActorTransform(Player)

    UpdateAnimation(Previous, Player.Location, PushTarget, Step, DeltaTime)
end

local function CanPush(Direction)
    if State.mFinished or not State.mGrounded or not Input.IsDown(Key.E) then
        return false
    end

    local Offset = State.mStone:GetActorLocation() - Actor:GetActorLocation()
    local Toward = Horizontal(Offset)
    local Distance = Toward:Length()
    local Range = State.mPushing and Settings.PushReleaseRange or Settings.PushRange
    if Distance < 0.1 or Distance > Range or math.abs(Offset.z) > 0.85 then
        return false
    end

    return Dot(Toward, Direction) >= 0.45 * Distance
end

local function UpdateRun(DeltaTime, Step)
    local StonePosition = State.mStone:GetActorLocation()
    if Actor:GetActorLocation().z < Settings.FallHeight or StonePosition.z < Settings.FallHeight then
        ResetRun(true)
        return false
    end

    local Sample = (StonePosition - State.mLastStonePosition) * (1 / DeltaTime)
    State.mStoneVelocity = State.mStoneVelocity + (Sample - State.mStoneVelocity) * math.min(12 * Step, 1)

    State.mImpactCooldown = math.max(0, State.mImpactCooldown - DeltaTime)
    State.mRetryNotice = math.max(0, State.mRetryNotice - DeltaTime)
    if State.mStarted and not State.mFinished then
        State.mElapsed = State.mElapsed + DeltaTime
        local Offset = StonePosition - State.mSummit
        if math.abs(Offset.z) < 0.45 and Horizontal(Offset):LengthSquared() < Settings.SummitRadius * Settings.SummitRadius then
            State.mFinished = true
            State.mStoneBody:SetPhysicsVelocity(Zero, Zero)
        end
    end

    UpdateBoulderImpact(StonePosition)
    State.mLastStonePosition = StonePosition
    return true
end

function BeginPlay()
    State.mPlayerSpawn = Actor:GetActorTransform()
    State.mMesh = assert(Actor:GetSkeletalMeshComponent())
    State.mCanRagdoll = type(State.mMesh.SetSkeletalCollisionEnabled) == "function" and type(State.mMesh.GetBoneWorldLocation) == "function"
    State.mController = Actor:GetComponentByName("Sisyphus Controller")
    State.mControllerTickGroup = State.mController and State.mController:GetTickGroup()
    State.mCamera = assert(World.FindActor("Ascent Camera"))
    State.mProgress = assert(State.mCamera:GetComponentByName("Progress"))
    State.mPrompt = assert(State.mCamera:GetComponentByName("Controls"))
    State.mStone = assert(World.FindActor("Boulder"))
    State.mStoneSpawn = State.mStone:GetActorTransform()
    State.mStoneBody = assert(State.mStone:GetComponentByClass("USphereComponent"))
    State.mSummit = assert(World.FindActor("Summit")):GetActorLocation()
    local Ice = World.FindActor("Ice Incline")
    local IceBody = Ice and Ice:GetComponentByClass("UBoxComponent")
    State.mUseSurfaceFriction = IceBody and type(IceBody.SetPhysicsFriction) == "function" and IceBody:SetPhysicsFriction(Settings.IceStaticFriction, Settings.IceDynamicFriction)
    State.mPendulums = {}
    State.mPlates = {}
    State.mRams = {}
    for Index = 1, 2 do
        local Pendulum = World.FindActor("Pendulum " .. Index)
        if Pendulum then
            local Rest = Pendulum:GetActorTransform()
            table.insert(State.mPendulums, {mActor = Pendulum, mRest = Rest, mAxis = Rest.Rotation:RotateVector(UnitX), mRate = 1.38 + Index * 0.19, mPhase = Index * 2.25})
        end
        local Ram = World.FindActor("Stone Ram " .. Index)
        if Ram then
            local Rest = Ram:GetActorTransform()
            local Forward = Rest.Rotation:RotateVector(UnitX)
            local Across = Rest.Rotation:RotateVector(UnitY)
            table.insert(State.mRams, {mActor = Ram, mRest = Rest, mForward = Forward, mAxis = Across, mPhase = Index * 1.45, mPeriod = 3.30 + Index * 0.30})
        end
    end
    for Index, Direction in ipairs(PlateDirections) do
        local Plate = World.FindActor("Launch Plate " .. Index)
        if Plate then
            local Rest = Plate:GetActorTransform()
            local SlopeForward = Rest.Rotation:RotateVector(UnitX)
            local Forward = Horizontal(SlopeForward):Normalized()
            local Right = Vector.new(-Forward.y, Forward.x, 0)
            local Launch = (Forward * Direction.x + Right * Direction.y):Normalized()
            table.insert(State.mPlates, {mActor = Plate, mRest = Rest, mForward = Forward, mRight = Right, mSlopeForward = SlopeForward, mNormal = Rest.Rotation:RotateVector(Up), mLaunchDirection = Launch, mWasMoving = false})
        end
    end
    State.mRocks = {}
    for Index, Radius in ipairs(RockRadii) do
        local Rock = assert(World.FindActor("Rockfall " .. Index))
        State.mRocks[Index] = {
            mActor = Rock,
            mBody = assert(Rock:GetComponentByClass("USphereComponent")),
            mMesh = assert(Rock:GetComponentByClass("UStaticMeshComponent")),
            mParked = Rock:GetActorTransform(),
            mRadius = Radius,
            mTime = 0,
        }
    end
    assert(World.SetActiveCamera(State.mCamera))

    ResetRun(false)
end

function Tick(DeltaTime)
    if Input.WasPressed(Key.R) then
        ResetRun(false)
        return
    end

    if DeltaTime <= 0 then
        return
    end
    if State.mFlight then
        UpdateLaunchPlates(DeltaTime, Actor:GetActorLocation())
        UpdatePlateFlight(DeltaTime)
        return
    end

    local Step = math.min(DeltaTime, 0.05)
    if not UpdateRun(DeltaTime, Step) then
        return
    end

    local MouseX, MouseY = Input.GetMouseDelta()
    State.mYaw = WrapAngle(State.mYaw + MouseX * Settings.LookSensitivity)
    State.mPitch = Clamp(State.mPitch - MouseY * Settings.LookSensitivity, -1.14, 0.63)

    UpdateCourseHazards(DeltaTime)
    if State.mFlight then
        UpdatePlateFlight(DeltaTime)
        return
    end
    UpdateRockfall(DeltaTime)
    UpdatePartialRagdoll(DeltaTime)
    if State.mKnockedDown then
        UpdateRecovery(DeltaTime)
        UpdateCamera(Step)
        UpdateStatus()
        return
    end

    local Forward = Vector.new(math.cos(State.mYaw), math.sin(State.mYaw), 0)
    local Right = Vector.new(-Forward.y, Forward.x, 0)
    local Direction = Forward * Axis(Key.W, Key.S) + Right * Axis(Key.D, Key.A)
    if Direction:LengthSquared() > 1 then
        Direction = Direction:Normalized()
    end

    UpdateIceGrip(Direction, Step)
    local Pushing = CanPush(Direction)
    State.mPushing = Pushing
    if Pushing then
        local Speed = Dot(State.mStoneVelocity, Direction)
        local Force = Clamp((Settings.PushSpeed - Speed) * Settings.PushGain, 0, Settings.MaxPushForce)
        local SideVelocity = Horizontal(State.mStoneVelocity) - Direction * Speed
        local Push = (Direction * Force - SideVelocity * Settings.PushSteeringGain) * (0.61 + 0.39 * GetIceGrip())
        if Push:LengthSquared() > 0.01 and State.mStoneBody:AddForce(Push) then
            State.mStarted = true
        end
    end

    MovePlayer(Direction, Step, DeltaTime)
    UpdateCamera(Step)
    UpdateStatus()
end

function EndPlay()
    if State.mKnockedDown or next(State.mPartialLimbs) then
        State.mMesh:SetSkeletalCollisionEnabled(false)
    end
    State.mPartialLimbs = {}
    if State.mFlight and State.mStone:IsValid() then
        Actor:SetActorTransform(State.mPlayerSpawn)
        State.mStone:SetActorTransform(State.mStoneSpawn)
        State.mStoneBody:SetPhysicsVelocity(Zero, Zero)
    end
    for _, Rock in ipairs(State.mRocks or {}) do
        if Rock.mActor:IsValid() then
            ParkRock(Rock)
        end
    end
    ResetCourseHazards()
end
