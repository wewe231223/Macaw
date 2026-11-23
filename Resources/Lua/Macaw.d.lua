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

---@class MacawHitResult
---@field BlockingHit boolean
---@field ImpactPoint MacawVector
---@field Normal MacawVector
---@field NormalImpulse MacawVector
---@field Time number
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

---@return boolean
function Component:IsValid() end

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

---@class MacawActor
---@field UUID string
---@field Location MacawVector
Actor = {}

---@return boolean
function Actor:IsValid() end

---@return string
function Actor:GetName() end

---@return MacawTransform
function Actor:GetActorTransform() end

---@param Value MacawTransform
---@return boolean
function Actor:SetActorTransform(Value) end

--- 지정한 클래스 또는 그 파생 클래스에 해당하는 첫 번째 컴포넌트를 반환합니다.
--- 일치하는 컴포넌트가 없으면 nil을 반환합니다. 등록되지 않았거나 컴포넌트가 아닌 클래스 이름을 넘기면 오류가 발생합니다.
---@param ClassName string
---@return MacawComponent?
function Actor:GetComponentByClass(ClassName) end

---@return MacawComponent?
function Actor:GetSkeletalMeshComponent() end

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
