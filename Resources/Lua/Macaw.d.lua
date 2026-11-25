---@meta

---@class MacawVector
---@field x number
---@field y number
---@field z number
---@operator add(MacawVector): MacawVector
---@operator sub(MacawVector): MacawVector
---@operator mul(number): MacawVector
local VectorValue = {}

---@return number
function VectorValue:Length() end

---@return number
function VectorValue:LengthSquared() end

---@return MacawVector
function VectorValue:Normalized() end

Vector = {}

---@overload fun(): MacawVector
---@param X number
---@param Y number
---@param Z number
---@return MacawVector
function Vector.new(X, Y, Z) end

---@class MacawRotator
---@field Pitch number
---@field Yaw number
---@field Roll number

Rotator = {}

---@overload fun(): MacawRotator
---@param Pitch number
---@param Yaw number
---@param Roll number
---@return MacawRotator
function Rotator.new(Pitch, Yaw, Roll) end

---@class MacawQuat
---@operator mul(MacawQuat): MacawQuat
local QuatValue = {}

---@return MacawQuat
function QuatValue:Normalized() end

---@param Value MacawVector
---@return MacawVector
function QuatValue:RotateVector(Value) end

Quat = {}

---@overload fun(): MacawQuat
---@param Axis MacawVector
---@param Angle number
---@return MacawQuat
function Quat.new(Axis, Angle) end

---@class MacawTransform
---@field Location MacawVector
---@field Rotation MacawQuat
---@field Scale MacawVector
local TransformValue = {}

---@return MacawRotator
function TransformValue:GetRotator() end

---@param Value MacawRotator
function TransformValue:SetRotator(Value) end

Transform = {}

---@return MacawTransform
function Transform.new() end

---@enum MacawDepthOfFieldApertureShape
DepthOfFieldApertureShape = {
    Circle = 0,
    Pentagon = 1,
    Hexagon = 2,
}

--- 프레임 안에서 Tick이 실행되는 순서입니다. 물리 스텝은 DuringPhysics와 PostPhysics 사이에 실행됩니다.
---@enum MacawTickGroup
TickGroup = {
    PrePhysics = 0,
    DuringPhysics = 2,
    PostPhysics = 4,
    PostUpdateWork = 5,
}

---@class MacawDepthOfFieldSettings
---@field Enabled boolean
---@field FocalDistance number 초점 평면까지의 거리, 미터
---@field FStop number 조리개 값. 조리개 지름 = 렌즈 초점거리 / FStop
---@field MaxBlurRadius number 블러 반경 상한, 전체 해상도 픽셀
---@field NearBlur boolean
---@field FarBlur boolean
---@field ApertureShape MacawDepthOfFieldApertureShape
DepthOfFieldSettings = {}

---@return MacawDepthOfFieldSettings
function DepthOfFieldSettings.new() end

---@class MacawHitResult
---@field BlockingHit boolean
---@field ImpactPoint MacawVector
---@field Normal MacawVector
---@field Time number
---@field Distance number
---@field Location MacawVector
---@field ImpactNormal MacawVector
---@field StartPenetrating boolean
---@field PenetrationDepth number
HitResult = {}

---@class MacawRaycastHit
---@field IsHit boolean
---@field Position MacawVector
---@field Distance number
RaycastHit = {}

---@class MacawComponent
---@field Name string
---@field Owner MacawActor
Component = {}

---@alias MacawSkeletalMeshComponent MacawComponent
SkeletalMeshComponent = Component

---@alias MacawCameraComponent MacawComponent
CameraComponent = Component

---@return boolean
function Component:IsValid() end

---@return MacawTickGroup
function Component:GetTickGroup() end

--- 컴포넌트 Tick 그룹을 바꿉니다. 다음 프레임부터 적용됩니다. 그룹을 고정한 컴포넌트(차량 이동 등)는 false를 반환합니다.
--- 물리 결과를 따라가는 카메라는 PostPhysics에서 갱신해야 한 프레임 늦지 않습니다.
---@param Group MacawTickGroup
---@return boolean
function Component:SetTickGroup(Group) end

---@param Name string
---@param Value number
---@return boolean
function Component:SetAnimGraphFloat(Name, Value) end

---@param Name string
---@param Value boolean
---@return boolean
function Component:SetAnimGraphBool(Name, Value) end

---@param Name string
---@return boolean
function Component:RestartAnimGraphPlayer(Name) end

---@param Name string
---@return boolean
function Component:IsAnimGraphPlayerFinished(Name) end

--- 설정의 사본을 반환합니다. 카메라 컴포넌트가 아니면 nil을 반환합니다.
---@return MacawDepthOfFieldSettings?
function Component:GetDepthOfFieldSettings() end

---@param Value MacawDepthOfFieldSettings
---@return boolean
function Component:SetDepthOfFieldSettings(Value) end

---@param Distance number 미터
---@return boolean
function Component:SetDepthOfFieldFocalDistance(Distance) end

--- 렌즈 초점거리입니다. 시야각 모드에서는 시야각과 센서 높이로 계산한 값입니다. 카메라 컴포넌트가 아니면 nil을 반환합니다.
---@return number? mm
function Component:GetFocalLength() end

--- 렌즈 모드에서는 초점거리를, 시야각 모드에서는 이 초점거리에 해당하는 시야각을 설정합니다.
---@param Millimeters number
---@return boolean
function Component:SetFocalLength(Millimeters) end

---@return number? mm
function Component:GetSensorHeight() end

---@param Millimeters number
---@return boolean
function Component:SetSensorHeight(Millimeters) end

--- true이면 시야각을 직접 지정하고, false이면 초점거리와 센서 높이로 시야각을 계산합니다.
---@param UseFieldOfView boolean
---@return boolean
function Component:SetUseFieldOfView(UseFieldOfView) end

---@param Torque number N·m
---@param WheelIndex integer 0부터 시작하는 바퀴 인덱스
---@return boolean
function Component:SetDriveTorque(Torque, WheelIndex) end

---@param Torque number N·m
---@param WheelIndex integer 0부터 시작하는 바퀴 인덱스
---@return boolean
function Component:SetBrakeTorque(Torque, WheelIndex) end

---@param AngleDegrees number 도 단위 조향각
---@param WheelIndex integer 0부터 시작하는 바퀴 인덱스
---@return boolean
function Component:SetSteerAngle(AngleDegrees, WheelIndex) end

---엔진이 있는 차량(4W 등)의 입력입니다. Simple 차량은 false를 반환합니다.
---@param Throttle number -1부터 1까지, 음수는 후진 요청
---@param Brake number 0부터 1까지
---@param Steering number -1부터 1까지
---@return boolean
function Component:SetDriveInput(Throttle, Brake, Steering) end

---@return boolean
function Component:ResetDrive() end

--- 같은 액터의 충돌이 꺼진 스태틱 메시 컴포넌트를 바퀴에 연결합니다. 물리 스텝 뒤 바퀴 자세를 따라 움직입니다.
--- 메시 원점은 바퀴 중심, 바퀴 축은 로컬 Y여야 합니다. nil을 넘기면 연결을 해제합니다.
---@param WheelIndex integer 0부터 시작하는 바퀴 인덱스
---@param Mesh MacawComponent? 스태틱 메시 컴포넌트
---@return boolean
function Component:SetWheelMesh(WheelIndex, Mesh) end

--- 바퀴 본을 움직일 같은 액터의 충돌이 꺼진 스켈레탈 메시 컴포넌트를 지정합니다. 차체 자신은 지정할 수 없습니다. nil이면 해제합니다.
---@param Mesh MacawComponent? 스켈레탈 메시 컴포넌트
---@return boolean
function Component:SetWheelSkeletalMesh(Mesh) end

--- 바퀴에 스켈레탈 메시의 본을 연결합니다. 바인드 포즈를 mCenterOffset에 놓인 정지 바퀴로 보고
--- 조향·굴림·서스펜션 변화만 더합니다. 본 축 방향은 상관없습니다. nil이나 빈 문자열이면 해제합니다.
---@param WheelIndex integer 0부터 시작하는 바퀴 인덱스
---@param BoneName string?
---@return boolean
function Component:SetWheelBone(WheelIndex, BoneName) end

---@class MacawWheelGeometry
---@field Offset Vector 차체 바디 로컬 바퀴 중심(m)
---@field Radius number 바퀴 반지름(m)
---@field Width number 바퀴 폭(m)

--- 차종 교체용으로 모든 바퀴의 위치·반지름·폭을 바꿉니다. 바퀴 수와 서스펜션 값은 유지합니다.
--- 값이 바뀌면 차량을 다시 등록하므로 기어·입력 상태가 초기화됩니다.
---@param Wheels MacawWheelGeometry[] 현재 바퀴 수와 같은 길이의 배열
---@return boolean
function Component:SetWheelGeometry(Wheels) end

--- 스켈레탈 메시 컴포넌트의 메시와 스켈레톤을 함께 바꿉니다. 포즈는 새 바인드 포즈로 초기화됩니다.
---@param MeshPath string 콘텐츠 상대 스켈레탈 메시 에셋 경로
---@param SkeletonPath string 콘텐츠 상대 스켈레톤 에셋 경로
---@return boolean
function Component:SetSkeletalMesh(MeshPath, SkeletonPath) end

--- 게임(Game·PIE) 월드에서 프리미티브를 그리지 않게 합니다. 충돌과 물리는 그대로이며, 에디터에서는 계속 보입니다.
---@param Hidden boolean
---@return boolean
function Component:SetHiddenInGame(Hidden) end

--- 이 컴포넌트의 스케일만 바꿉니다. Actor:SetActorTransform과 달리 다른 컴포넌트에 비율을 전파하지 않습니다.
--- 물리 바디가 있으면 새 크기로 다시 만들며, 차량 차체라면 차량도 다시 등록됩니다.
---@param Scale Vector 각 축이 0보다 큰 스케일
---@return boolean
function Component:SetComponentScale(Scale) end

---@return integer Gear 후진 -1, 중립 0, 전진 1부터
---@return number ForwardSpeed m/s
function Component:GetDriveState() end

---@param Text string 월드 빌보드에 표시할 문자열
---@return boolean
function Component:SetText(Text) end

--- 현재 바디의 형상을 차량 서스펜션이 감지할 주행면으로 지정합니다.
---@param Drivable boolean
---@return boolean
function Component:SetVehicleDrivable(Drivable) end

---@param Linear MacawVector m/s
---@param Angular MacawVector rad/s
---@return boolean
function Component:SetPhysicsVelocity(Linear, Angular) end

function Component:AddForce(Force) end

---@class MacawActor
---@field UUID string
---@field Location MacawVector
---@field Velocity MacawVector
Actor = {}

--- 이 스크립트를 실행하는 Lua 스크립트 컴포넌트입니다.
---@type MacawComponent
Script = {}

---@return boolean
function Actor:IsValid() end

---@return string
function Actor:GetName() end

---@return MacawTickGroup
function Actor:GetTickGroup() end

--- 액터 기본 Tick 그룹을 바꿉니다. 다음 프레임부터 적용됩니다.
---@param Group MacawTickGroup
---@return boolean
function Actor:SetTickGroup(Group) end

---@return MacawTransform
function Actor:GetActorTransform() end

---@param Value MacawTransform
---@return boolean
function Actor:SetActorTransform(Value) end

---@return MacawVector
function Actor:GetActorLocation() end

--- 액터를 지정한 월드 위치로 옮깁니다. Sweep이 true이면 루트 컴포넌트의 바디로 이동 경로를 쓸어, 응답이 Block인 바디에 막히는 지점에서 멈춥니다.
---@param NewLocation MacawVector
---@param Sweep boolean?
---@return boolean Moved
---@return MacawHitResult SweepHitResult
function Actor:SetActorLocation(NewLocation, Sweep) end

--- 액터를 월드 변위만큼 옮깁니다. Sweep이 true이면 루트 컴포넌트의 바디로 이동 경로를 쓸어, 응답이 Block인 바디에 막히는 지점에서 멈춥니다.
---@param DeltaLocation MacawVector
---@param Sweep boolean?
---@return boolean Moved
---@return MacawHitResult SweepHitResult
function Actor:AddActorWorldOffset(DeltaLocation, Sweep) end

--- 지정한 클래스 또는 그 파생 클래스에 해당하는 첫 번째 컴포넌트를 반환합니다.
--- 일치하는 컴포넌트가 없으면 nil을 반환합니다. 등록되지 않았거나 컴포넌트가 아닌 클래스 이름을 넘기면 오류가 발생합니다.
---@param ClassName string
---@return MacawComponent?
function Actor:GetComponentByClass(ClassName) end

--- 이름이 정확히 일치하는 첫 번째 컴포넌트를 반환합니다. 없으면 nil입니다.
---@param Name string
---@return MacawComponent?
function Actor:GetComponentByName(Name) end

---@return MacawComponent?
function Actor:GetSkeletalMeshComponent() end

---@return MacawCameraComponent?
function Actor:GetCameraComponent() end

function Actor:PrintLocation() end

---@class MacawTasks
Tasks = {}

---@param Function fun()
function Tasks:Spawn(Function) end

---@param Seconds number
function Tasks:Wait(Seconds) end

---@param Predicate fun(): boolean
function Tasks:WaitUntil(Predicate) end

World = {}

---@param Name string
---@return MacawActor?
function World.FindActor(Name) end

---@param Origin MacawVector
---@param Direction MacawVector
---@param Ignored? MacawActor
---@return MacawRaycastHit
function World.Raycast(Origin, Direction, Ignored) end

---@param Value MacawActor
---@return boolean
function World.SetActiveCamera(Value) end

---@class MacawKeys
---@field W integer
---@field A integer
---@field S integer
---@field D integer
---@field C integer
---@field V integer
---@field F integer
---@field Q integer
---@field R integer
---@field E integer
---@field X integer
---@field K integer
---@field H integer
---@field Z integer
---@field Space integer
---@field Backspace integer
---@field Delete integer
---@field Shift integer
---@field LeftShift integer
---@field RightShift integer
---@field MouseLeft integer
---@field MouseRight integer
Key = {}

Audio = {}

---배경 음악 하나를 튼다. 경로는 Content 기준(mp3, wav). 이미 틀어 둔 음악은 끄고 바꾼다.
---@param Path string 예: "Audio/BGM.mp3"
---@param Volume? number 0부터 1까지, 기본 1
---@param Loop? boolean 기본 true
---@param StartSeconds? number 이 시간(초)부터 재생, 기본 0
---@return boolean
function Audio.PlayMusic(Path, Volume, Loop, StartSeconds) end

---@param Volume number 0부터 1까지
function Audio.SetMusicVolume(Volume) end

function Audio.StopMusic() end

Input = {}

---@param KeyCode integer
---@return boolean
function Input.IsDown(KeyCode) end

---@param KeyCode integer
---@return boolean
function Input.WasPressed(KeyCode) end

---@return number X
---@return number Y
function Input.GetMouseDelta() end
