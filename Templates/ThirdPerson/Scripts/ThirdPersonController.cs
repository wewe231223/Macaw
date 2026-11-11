using System.Numerics;
using Macaw;

namespace Game;

public sealed class ThirdPersonController : ScriptComponent {
    public override void BeginPlay() {
        mMesh = GetComponent<SkeletalMeshComponent>() ?? throw new InvalidOperationException("The third person character needs a skeletal mesh.");
        mCamera = World.GetActors<CameraActor>().First(Actor => Actor.GetName() == "Follow Camera").GetComponent<CameraComponent>() ?? throw new InvalidOperationException("The follow camera is missing.");
        mSpawn = Owner.GetActorTransform();
        World.SetActiveCamera(mCamera);
        ResetPlayer();
        UpdateCamera(0.0f);
    }

    public override void Tick(float DeltaTime) {
        float Step = Math.Clamp(DeltaTime, 0.0f, 0.05f);
        Vector2 Mouse = Input.GetMouseDelta();
        mYaw = MathF.IEEERemainder(mYaw + Mouse.X * mLookSensitivity, MathF.Tau);
        mPitch = Math.Clamp(mPitch - Mouse.Y * mLookSensitivity, -1.15f, 0.9f);
        mBusyAge += Step;

        if (Input.WasPressed(Key.Backspace)) {
            ResetPlayer();
        }

        UpdateControls();

        if (mBusyAge > 0.12f && mBusy == "Action" && mMesh.IsAnimGraphPlayerFinished("Action Player")) {
            mBusy = "";
            mAction = false;
        }

        if (mBusyAge > 0.12f && mBusy == "Hit" && mMesh.IsAnimGraphPlayerFinished("Hit Player")) {
            mBusy = "";
            mHit = false;
        }

        MovePlayer(Step);
        UpdateAnimation();
        UpdateCamera(Step);
    }

    private void ResetPlayer() {
        Owner.SetActorTransform(mSpawn);
        mVelocity = Vector2.Zero;
        mVerticalSpeed = 0.0f;
        mFacing = 0.0f;
        mYaw = 0.0f;
        mPitch = -0.22f;
        mCameraArm = 0.0f;
        mSpeed = 0.0f;
        mBusyAge = 0.0f;
        mGrounded = true;
        mCrouch = false;
        mSwim = false;
        mAim = false;
        mDance = false;
        mDead = false;
        mAction = false;
        mHit = false;
        mBusy = "";
        mActionName = "";
        UpdateAnimation();
    }

    private void UpdateControls() {
        if (Input.WasPressed(Key.Delete)) {
            mDead = true;
            mDance = false;
            mBusy = "";
            mAction = false;
            mHit = false;
        }

        if (mDead) {
            return;
        }

        if (Input.WasPressed(Key.C) && mGrounded) {
            mCrouch = !mCrouch;
            mSwim = false;
        }

        if (Input.WasPressed(Key.V) && mGrounded) {
            mSwim = !mSwim;
            mCrouch = false;
        }

        mAim = !mSwim && !mCrouch && Input.IsDown(Key.MouseRight);

        if (Input.WasPressed(Key.H) && mGrounded) {
            StartHit();
        } else if (mBusy.Length == 0 && mGrounded && !mSwim) {
            if (Input.WasPressed(Key.F)) {
                mDance = !mDance;
            } else if (Input.WasPressed(Key.Q)) {
                StartAction("Roll");
            } else if (Input.WasPressed(Key.R)) {
                StartAction("Pistol_Reload");
            } else if (Input.WasPressed(Key.E)) {
                StartAction("Interact");
            } else if (Input.WasPressed(Key.X)) {
                StartAction("Sword_Attack");
            } else if (Input.WasPressed(Key.K)) {
                StartAction("Spell_Simple_Shoot");
            } else if (Input.WasPressed(Key.MouseLeft)) {
                StartAction(mAim ? "Pistol_Shoot" : mAlternatePunch ? "Punch_Cross" : "Punch_Jab");
                mAlternatePunch = !mAlternatePunch;
            }
        }

        if (Input.WasPressed(Key.Space) && mGrounded && mBusy.Length == 0 && !mSwim && !mDance) {
            mGrounded = false;
            mVerticalSpeed = mJumpSpeed;
        }
    }

    private void StartAction(string Name) {
        mActionName = Name;
        mMesh.SetAnimGraphFloat("ActionClip", ClipLibrary.Index(Name));
        mMesh.RestartAnimGraphPlayer("Action Player");
        mAction = true;
        mHit = false;
        mDance = false;
        mBusy = "Action";
        mBusyAge = 0.0f;
    }

    private void StartHit() {
        mMesh.SetAnimGraphFloat("HitClip", ClipLibrary.Index(mAlternateHit ? "Hit_Head" : "Hit_Chest"));
        mMesh.RestartAnimGraphPlayer("Hit Player");
        mAlternateHit = !mAlternateHit;
        mHit = true;
        mAction = false;
        mDance = false;
        mBusy = "Hit";
        mBusyAge = 0.0f;
    }

    private void MovePlayer(float Step) {
        Vector2 Forward = new(MathF.Cos(mYaw), MathF.Sin(mYaw));
        Vector2 Right = new(-Forward.Y, Forward.X);
        Vector2 Direction = Forward * Axis(Key.W, Key.S) + Right * Axis(Key.D, Key.A);

        if (Direction.LengthSquared() > 1.0f) {
            Direction = Vector2.Normalize(Direction);
        }

        bool Sprint = Input.IsDown(Key.Shift) || Input.IsDown(Key.LeftShift) || Input.IsDown(Key.RightShift);
        float TargetSpeed = mSwim ? 2.2f : mCrouch || mAim || Input.IsDown(Key.Z) ? mWalkSpeed : Sprint ? mSprintSpeed : mMoveSpeed;
        bool Rolling = mBusy == "Action" && mActionName == "Roll";
        bool Blocked = mDead || mDance || mBusy.Length > 0 && !Rolling;
        Vector2 Desired = Blocked ? Vector2.Zero : Rolling ? new Vector2(MathF.Cos(mFacing), MathF.Sin(mFacing)) * 4.4f : Direction * TargetSpeed;
        Vector2 Difference = Desired - mVelocity;
        float Change = (mGrounded ? mAcceleration : mAcceleration * 0.35f) * Step;
        mVelocity += Difference.LengthSquared() > Change * Change ? Vector2.Normalize(Difference) * Change : Difference;

        if (mAim || !Blocked && !Rolling && Direction.LengthSquared() > 0.0001f) {
            float Target = mAim ? mYaw : MathF.Atan2(Direction.Y, Direction.X);
            float Turn = MathF.IEEERemainder(Target - mFacing, MathF.Tau);
            mFacing = MathF.IEEERemainder(mFacing + Math.Clamp(Turn, -mTurnSpeed * Step, mTurnSpeed * Step), MathF.Tau);
        }

        Transform Player = Owner.GetActorTransform();
        Vector3 Previous = Player.Location;
        Vector3 Position = MoveAlong(Previous, Vector3.UnitX, mVelocity.X * Step);
        Position = MoveAlong(Position, Vector3.UnitY, mVelocity.Y * Step);
        mSpeed = Step > 0.0f ? new Vector2(Position.X - Previous.X, Position.Y - Previous.Y).Length() / Step : 0.0f;
        mVerticalSpeed -= mGravity * Step;
        float VerticalDistance = mVerticalSpeed * Step;

        if (VerticalDistance > 0.0f) {
            RaycastHit Ceiling = World.Raycast(Position + Vector3.UnitZ * 1.75f, Vector3.UnitZ, Owner);

            if (Ceiling.IsHit && Ceiling.Distance < VerticalDistance + 0.05f) {
                VerticalDistance = MathF.Max(0.0f, Ceiling.Distance - 0.05f);
                mVerticalSpeed = 0.0f;
            }
        }

        Position.Z += VerticalDistance;
        Vector3 Probe = new(Position.X, Position.Y, MathF.Max(Previous.Z, Position.Z) + mStepHeight);
        RaycastHit Ground = World.Raycast(Probe, -Vector3.UnitZ, Owner);
        float GroundHeight = Ground.Position.Z + 0.025f;
        float SnapDistance = mGrounded ? mStepHeight : 0.0f;
        bool Reachable = Ground.IsHit && GroundHeight <= Previous.Z + SnapDistance + 0.01f;
        bool Contact = Position.Z <= GroundHeight || mGrounded && Position.Z - GroundHeight <= mStepHeight;
        mGrounded = mVerticalSpeed <= 0.0f && Reachable && Contact;

        if (mGrounded) {
            Position.Z = GroundHeight;
            mVerticalSpeed = 0.0f;
        }

        if (Position.Z < -10.0f) {
            ResetPlayer();
            return;
        }

        Player.Location = Position;
        Player.Rotation = Quaternion.CreateFromAxisAngle(Vector3.UnitZ, mFacing) * mSpawn.Rotation;
        Owner.SetActorTransform(Player);
    }

    private Vector3 MoveAlong(Vector3 Position, Vector3 Axis, float Distance) {
        if (MathF.Abs(Distance) < 0.00001f) {
            return Position;
        }

        Vector3 Direction = Axis * MathF.Sign(Distance);
        float Allowed = MathF.Abs(Distance);

        foreach (float Height in mWallProbeHeights) {
            RaycastHit Wall = World.Raycast(Position + Vector3.UnitZ * Height, Direction, Owner);

            if (Wall.IsHit) {
                Allowed = MathF.Min(Allowed, MathF.Max(0.0f, Wall.Distance - 0.30f));
            }
        }

        return Position + Direction * Allowed;
    }

    private void UpdateAnimation() {
        mMesh.SetAnimGraphFloat("Speed", mSpeed);
        mMesh.SetAnimGraphFloat("VerticalSpeed", mVerticalSpeed);
        mMesh.SetAnimGraphFloat("AimPitch", Math.Clamp(mPitch / 0.58f, -1.0f, 1.0f));
        mMesh.SetAnimGraphFloat("AimWeight", Math.Clamp(1.0f - mSpeed / 5.0f, 0.45f, 1.0f));
        mMesh.SetAnimGraphBool("IsInAir", !mGrounded);
        mMesh.SetAnimGraphBool("IsCrouching", mCrouch);
        mMesh.SetAnimGraphBool("IsSwimming", mSwim);
        mMesh.SetAnimGraphBool("IsAiming", mAim);
        mMesh.SetAnimGraphBool("WantsAction", mAction);
        mMesh.SetAnimGraphBool("WantsHit", mHit);
        mMesh.SetAnimGraphBool("IsDancing", mDance);
        mMesh.SetAnimGraphBool("IsDead", mDead);
    }

    private void UpdateCamera(float Step) {
        Quaternion Rotation = Quaternion.Normalize(Quaternion.CreateFromAxisAngle(Vector3.UnitZ, mYaw) * Quaternion.CreateFromAxisAngle(Vector3.UnitY, -mPitch));
        Vector3 Target = Owner.Location + Vector3.UnitZ * (mCrouch ? 1.12f : 1.4f);
        Vector3 Direction = Vector3.Transform(-Vector3.UnitX, Rotation);
        RaycastHit Obstacle = World.Raycast(Target, Direction, Owner);
        float Distance = mAim ? mCameraDistance * 0.75f : mCameraDistance;
        float Limit = Obstacle.IsHit ? Math.Clamp(Obstacle.Distance - 0.20f, 0.20f, Distance) : Distance;
        mCameraArm = mCameraArm <= 0.0f ? Limit : MathF.Min(Limit, mCameraArm + Step * 8.0f);
        Transform Camera = mCamera.GetTransform();
        Camera.Location = Target + Direction * mCameraArm;
        Camera.Rotation = Rotation;
        mCamera.SetTransform(Camera);
    }

    private static float Axis(Key Positive, Key Negative) {
        return (Input.IsDown(Positive) ? 1.0f : 0.0f) - (Input.IsDown(Negative) ? 1.0f : 0.0f);
    }

    [Field] private float mMoveSpeed = 3.5f;
    [Field] private float mWalkSpeed = 1.5f;
    [Field] private float mSprintSpeed = 6.0f;
    [Field] private float mAcceleration = 24.0f;
    [Field] private float mJumpSpeed = 6.0f;
    [Field] private float mGravity = 15.0f;
    [Field] private float mTurnSpeed = 10.0f;
    [Field] private float mStepHeight = 0.35f;
    [Field] private float mCameraDistance = 4.0f;
    [Field] private float mLookSensitivity = 0.003f;

    private SkeletalMeshComponent mMesh = null!;
    private CameraComponent mCamera = null!;
    private Transform mSpawn = default;
    private Vector2 mVelocity = Vector2.Zero;
    private readonly float[] mWallProbeHeights = [0.40f, 1.60f];
    private string mBusy = "";
    private string mActionName = "";
    private float mBusyAge = 0.0f;
    private float mVerticalSpeed = 0.0f;
    private float mFacing = 0.0f;
    private float mYaw = 0.0f;
    private float mPitch = -0.22f;
    private float mCameraArm = 0.0f;
    private float mSpeed = 0.0f;
    private bool mGrounded = true;
    private bool mCrouch = false;
    private bool mSwim = false;
    private bool mAim = false;
    private bool mDance = false;
    private bool mDead = false;
    private bool mAction = false;
    private bool mHit = false;
    private bool mAlternatePunch = false;
    private bool mAlternateHit = false;
}
