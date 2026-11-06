/**
 * @file FSplashCanvas.cpp
 * @brief 스플래시 배경·로고·텍스트와 픽셀 합성 자원을 관리합니다.
 * @ingroup Platform
 */

#include "FSplashCanvas.h"
#include "Platform/WindowResources.h"

#include <algorithm>
#include <cmath>

namespace Platform {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief RGB 실수 채널 세 개로 이루어진 스플래시 색상 타입입니다.
         */
        using FColor = Platform::FSplashCanvas::FColor;

        /**
         * @brief 스플래시 로고·발광 효과의 빨간색입니다.
         */
        constexpr FColor MacawRed{0.91f, 0.21f, 0.17f};

        /**
         * @brief 스플래시 로고·발광 효과의 파란색입니다.
         */
        constexpr FColor MacawBlue{0.18f, 0.55f, 0.88f};

        /**
         * @brief 스플래시 밝은 부분의 흰색입니다.
         */
        constexpr FColor White{1.0f, 1.0f, 1.0f};

        /**
         * @brief 스플래시 배경 위쪽 RGB 색상입니다.
         */
        constexpr FColor TopBackground{0.075f, 0.09f, 0.13f};

        /**
         * @brief 스플래시 배경 아래쪽 RGB 색상입니다.
         */
        constexpr FColor BottomBackground{0.035f, 0.042f, 0.065f};

        /**
         * @brief 두 RGB 색상을 선형 보간합니다.
         * @param[in] A 첫 번째 피연산자 또는 시작 색상입니다.
         * @param[in] B 두 번째 피연산자 또는 끝 색상입니다.
         * @param[in] T 선형 보간 계수입니다.
         * @return 보간한 RGB 색상입니다.
         */
        FColor Lerp(const FColor& A, const FColor& B, float T) {
            return {A[0] + (B[0] - A[0]) * T, A[1] + (B[1] - A[1]) * T, A[2] + (B[2] - A[2]) * T};
        }

        /**
         * @brief 두 RGB 색상을 지정 배율로 가산합니다.
         * @param[in] A 첫 번째 피연산자 또는 시작 색상입니다.
         * @param[in] B 두 번째 피연산자 또는 끝 색상입니다.
         * @param[in] Scale 두 번째 색상에 곱할 배율입니다.
         * @return 배율을 적용해 가산한 RGB 색상입니다.
         */
        FColor Add(const FColor& A, const FColor& B, float Scale) {
            return {A[0] + B[0] * Scale, A[1] + B[1] * Scale, A[2] + B[2] * Scale};
        }

        /**
         * @brief RGB 색상에 디더를 더하고 32비트 픽셀로 양자화합니다.
         * @param[in] Color 적용하거나 변환할 색상입니다.
         * @param[in] Noise 채널 양자화 전에 더할 디더 값입니다.
         * @return RGB를 양자화한 32비트 픽셀입니다.
         */
        Core::Uint32 Pack(const FColor& Color, float Noise = 0.0f) {
            const auto Channel{[Noise](float Value) {
                return static_cast<Core::Uint32>(std::clamp(Value + Noise, 0.0f, 1.0f) * 255.0f + 0.5f);
            }};

            return (Channel(Color[0]) << 16) | (Channel(Color[1]) << 8) | Channel(Color[2]);
        }

        /**
         * @brief 32비트 픽셀의 RGB 채널을 실수 색상으로 변환합니다.
         * @param[in] Pixel RGB 성분을 꺼낼 32비트 픽셀입니다.
         * @return 픽셀의 정규화한 RGB 색상입니다.
         */
        FColor Unpack(Core::Uint32 Pixel) {
            return {static_cast<float>((Pixel >> 16) & 0xFF) / 255.0f, static_cast<float>((Pixel >> 8) & 0xFF) / 255.0f, static_cast<float>(Pixel & 0xFF) / 255.0f};
        }

        /**
         * @brief 중심까지의 거리와 반경으로 발광 강도를 계산합니다.
         * @param[in] X 가로 좌표입니다.
         * @param[in] Y 세로 좌표입니다.
         * @param[in] CenterX 발광 중심의 가로 좌표입니다.
         * @param[in] CenterY 발광 중심의 세로 좌표입니다.
         * @param[in] Radius 발광 효과의 반경입니다.
         * @return 중심과의 거리에 따른 발광 강도입니다.
         */
        float Glow(float X, float Y, float CenterX, float CenterY, float Radius) {
            const float Falloff{std::max(0.0f, 1.0f - std::hypot(X - CenterX, Y - CenterY) / Radius)};

            return Falloff * Falloff;
        }

        // 8비트 그라데이션의 띠를 감추는 ±0.5 LSB 크기의 고정 잡음입니다.

        /**
         * @brief 픽셀 좌표에서 색상 양자화용 미세 잡음을 계산합니다.
         * @param[in] X 가로 좌표입니다.
         * @param[in] Y 세로 좌표입니다.
         * @return 좌표별 미세 디더 값입니다.
         */
        float Dither(Core::Int32 X, Core::Int32 Y) {
            Core::Uint32 Hash{static_cast<Core::Uint32>(X) * 73856093u ^ static_cast<Core::Uint32>(Y) * 19349663u};

            Hash = (Hash ^ (Hash >> 13)) * 1274126177u;

            return (static_cast<float>((Hash >> 8) & 0xFF) / 255.0f - 0.5f) / 255.0f;
        }

        /**
         * @brief 32비트 top-down DIB의 비트맵 형식을 구성합니다.
         * @param[in] Width 픽셀 너비입니다.
         * @param[in] Height 픽셀 높이 또는 예약할 UI 높이입니다.
         * @return 지정 크기의 32비트 top-down DIB 형식입니다.
         */
        BITMAPINFO MakeFormat(Core::Int32 Width, Core::Int32 Height) {
            BITMAPINFO Format{};
            Format.bmiHeader.biSize = sizeof(Format.bmiHeader);
            Format.bmiHeader.biWidth = Width;
            Format.bmiHeader.biHeight = -Height;
            Format.bmiHeader.biPlanes = 1;
            Format.bmiHeader.biBitCount = 32;
            Format.bmiHeader.biCompression = BI_RGB;

            return Format;
        }
    }

    /**
     * @brief 자원이 없는 빈 캔버스를 구성합니다.
     */
    Platform::FSplashCanvas::FSplashCanvas() = default;

    /**
     * @brief DIB와 글꼴을 해제합니다.
     */
    Platform::FSplashCanvas::~FSplashCanvas() {
        if (mDC != nullptr) {
            SelectObject(mDC, mOldBitmap);
            DeleteDC(mDC);
        }

        if (mBitmap != nullptr) {
            DeleteObject(mBitmap);
        }

        for (HFONT Font : {mTitleFont, mCaptionFont, mProjectFont, mStageFont, mDetailFont}) {
            if (Font != nullptr) {
                DeleteObject(Font);
            }
        }

        for (HANDLE Resource : mFontResources) {
            RemoveFontMemResourceEx(Resource);
        }
    }

    /**
     * @brief 프레임 DIB와 글꼴을 만들고 고정 배경을 합성합니다.
     */
    bool Platform::FSplashCanvas::Initialize(Core::Int32 Width, Core::Int32 Height, float Scale, const std::wstring& ProjectName) {
        mWidth = Width;
        mHeight = Height;
        mScale = Scale;

        const BITMAPINFO Format{MakeFormat(Width, Height)};
        void* Bits{};

        mDC = CreateCompatibleDC(nullptr);
        mBitmap = mDC != nullptr ? CreateDIBSection(mDC, &Format, DIB_RGB_COLORS, &Bits, nullptr, 0) : nullptr;

        if (mBitmap == nullptr) {
            return false;
        }

        mBits = static_cast<Core::Uint32*>(Bits);
        mOldBitmap = SelectObject(mDC, mBitmap);

        SetBkMode(mDC, TRANSPARENT);

        mHasPretendard = LoadFonts();
        mTitleFont = MakeFont(52.0f, FW_BOLD);
        mCaptionFont = MakeFont(11.0f, FW_SEMIBOLD);
        mProjectFont = MakeFont(24.0f, FW_SEMIBOLD);
        mStageFont = MakeFont(14.0f, FW_SEMIBOLD);
        mDetailFont = MakeFont(12.0f, FW_NORMAL);

        ComposeBackground(ProjectName);

        return true;
    }

    /**
     * @brief 배경 위에 단계·파일·진행 막대를 그려 대상 DC로 복사합니다.
     */
    void Platform::FSplashCanvas::Draw(HDC Target, const std::wstring& Stage, const std::wstring& Action, const std::wstring& File, float Progress, float Time) {
        GdiFlush();
        std::copy(mBackground.begin(), mBackground.end(), mBits);
        DrawProgress(Progress, Time);

        const Core::Int32 Left{Pixels(40.0f)};
        const Core::Int32 Right{mWidth - Pixels(40.0f)};
        const Core::Int32 StageTop{mHeight - Pixels(84.0f)};
        const Core::Int32 DetailTop{mHeight - Pixels(60.0f)};
        const std::wstring Percent{std::to_wstring(static_cast<Core::Int32>(Progress * 100.0f + 0.5f)) + L"%"};

        SelectObject(mDC, mStageFont);

        SIZE PercentSize{};

        GetTextExtentPoint32W(mDC, Percent.c_str(), static_cast<Core::Int32>(Percent.size()), &PercentSize);
        DrawLine(Percent, {Right - PercentSize.cx, StageTop, Right, StageTop + Pixels(20.0f)}, 0x8C96A8, DT_RIGHT);
        DrawLine(Stage, {Left, StageTop, Right - PercentSize.cx - Pixels(16.0f), StageTop + Pixels(20.0f)}, 0xE8ECF2, DT_END_ELLIPSIS);

        SelectObject(mDC, mDetailFont);

        Core::Int32 FileLeft{Left};

        if (!Action.empty()) {
            SIZE ActionSize{};

            GetTextExtentPoint32W(mDC, Action.c_str(), static_cast<Core::Int32>(Action.size()), &ActionSize);
            DrawLine(Action, {Left, DetailTop, Right, DetailTop + Pixels(18.0f)}, 0xA9B2C2, 0);

            FileLeft += ActionSize.cx + Pixels(8.0f);
        }

        DrawLine(File, {FileLeft, DetailTop, Right, DetailTop + Pixels(18.0f)}, 0x687286, DT_PATH_ELLIPSIS);

        GdiFlush();
        BitBlt(Target, 0, 0, mWidth, mHeight, mDC, 0, 0, SRCCOPY);
    }

    /**
     * @brief 논리 픽셀을 현재 배율의 물리 픽셀로 바꿉니다.
     */
    Core::Int32 Platform::FSplashCanvas::Pixels(float Logical) const {
        return static_cast<Core::Int32>(std::lround(Logical * mScale));
    }

    /**
     * @brief 실행 파일에 포함된 Pretendard 글꼴을 이 프로세스 전용으로 등록합니다.
     */
    bool Platform::FSplashCanvas::LoadFonts() {
        const HMODULE Module{GetModuleHandleW(nullptr)};

        for (Core::Int32 Identifier : {MacawSplashRegularFontResource, MacawSplashSemiBoldFontResource, MacawSplashBoldFontResource}) {
            const HRSRC Resource{FindResourceW(Module, MAKEINTRESOURCEW(Identifier), RT_RCDATA)};
            const HGLOBAL Data{Resource != nullptr ? LoadResource(Module, Resource) : nullptr};
            void* Bytes{Data != nullptr ? LockResource(Data) : nullptr};
            DWORD Count{};
            const HANDLE Font{Bytes != nullptr ? AddFontMemResourceEx(Bytes, SizeofResource(Module, Resource), nullptr, &Count) : nullptr};

            if (Font == nullptr) {
                return false;
            }

            mFontResources.push_back(Font);
        }

        return true;
    }

    /**
     * @brief 현재 배율의 Pretendard 글꼴을 만들고, 등록하지 못했으면 Segoe UI로 대신합니다.
     */
    HFONT Platform::FSplashCanvas::MakeFont(float Size, Core::Int32 Weight) const {
        // 정적 Pretendard SemiBold는 GDI에서 별도 패밀리로 등록되므로 이름으로 고릅니다.
        const bool SemiBold{mHasPretendard && Weight == FW_SEMIBOLD};
        const wchar_t* Face{!mHasPretendard ? L"Segoe UI" : (SemiBold ? L"Pretendard SemiBold" : L"Pretendard")};

        return CreateFontW(-Pixels(Size), 0, 0, 0, SemiBold ? FW_NORMAL : Weight, false, false, false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, Face);
    }

    /**
     * @brief 현재 선택된 글꼴로 한 줄 문자열을 그립니다.
     */
    void Platform::FSplashCanvas::DrawLine(const std::wstring& Text, RECT Bounds, Core::Uint32 Color, UINT Flags) {
        SetTextColor(mDC, RGB((Color >> 16) & 0xFF, (Color >> 8) & 0xFF, Color & 0xFF));
        DrawTextW(mDC, Text.c_str(), static_cast<Core::Int32>(Text.size()), &Bounds, Flags | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);
    }

    /**
     * @brief 실행 파일의 Macaw 아이콘을 미리 곱한 알파의 RGBA 실수 배열로 읽습니다.
     */
    bool Platform::FSplashCanvas::LoadMacawIcon(Core::Int32 Size) {
        const HICON Icon{static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(MacawIconResource), IMAGE_ICON, Size, Size, LR_DEFAULTCOLOR))};

        if (Icon == nullptr) {
            return false;
        }

        ICONINFO Info{};
        std::vector<Core::Uint32> Raw(static_cast<std::size_t>(Size * Size));
        bool Loaded{};

        if (GetIconInfo(Icon, &Info)) {
            BITMAPINFO Format{MakeFormat(Size, Size)};
            const HDC Screen{GetDC(nullptr)};

            Loaded = Info.hbmColor != nullptr && GetDIBits(Screen, Info.hbmColor, 0, static_cast<UINT>(Size), Raw.data(), &Format, DIB_RGB_COLORS) == Size;

            ReleaseDC(nullptr, Screen);
            DeleteObject(Info.hbmColor);
            DeleteObject(Info.hbmMask);
        }

        DestroyIcon(Icon);

        if (!Loaded) {
            return false;
        }

        mIconSize = Size;
        mIconPixels.resize(Raw.size() * 4);

        for (std::size_t Index{}; Index < Raw.size(); ++Index) {
            const FColor Color{Unpack(Raw[Index])};
            const float Alpha{static_cast<float>(Raw[Index] >> 24) / 255.0f};

            mIconPixels[Index * 4 + 0] = Color[0] * Alpha;
            mIconPixels[Index * 4 + 1] = Color[1] * Alpha;
            mIconPixels[Index * 4 + 2] = Color[2] * Alpha;
            mIconPixels[Index * 4 + 3] = Alpha;
        }

        return true;
    }

    /**
     * @brief 아이콘을 쌍선형 보간으로 표본화합니다.
     */
    std::array<float, 4> Platform::FSplashCanvas::SampleIcon(float U, float V) const {
        const float Last{static_cast<float>(mIconSize - 1)};
        const float X{std::clamp(U * static_cast<float>(mIconSize) - 0.5f, 0.0f, Last)};
        const float Y{std::clamp(V * static_cast<float>(mIconSize) - 0.5f, 0.0f, Last)};
        const Core::Int32 X0{static_cast<Core::Int32>(X)};
        const Core::Int32 Y0{static_cast<Core::Int32>(Y)};
        const Core::Int32 X1{std::min(X0 + 1, mIconSize - 1)};
        const Core::Int32 Y1{std::min(Y0 + 1, mIconSize - 1)};
        const float TX{X - static_cast<float>(X0)};
        const float TY{Y - static_cast<float>(Y0)};
        std::array<float, 4> Result{};

        for (Core::Int32 Channel{}; Channel < 4; ++Channel) {
            const auto At{[&](Core::Int32 PX, Core::Int32 PY) {
                return mIconPixels[static_cast<std::size_t>((PY * mIconSize + PX) * 4 + Channel)];
            }};

            const float Top{At(X0, Y0) + (At(X1, Y0) - At(X0, Y0)) * TX};
            const float Bottom{At(X0, Y1) + (At(X1, Y1) - At(X0, Y1)) * TX};

            Result[static_cast<std::size_t>(Channel)] = Top + (Bottom - Top) * TY;
        }

        return Result;
    }

    /**
     * @brief 그라데이션, 빛번짐, 아이콘과 고정 글자를 배경 버퍼에 합성합니다.
     */
    void Platform::FSplashCanvas::ComposeBackground(const std::wstring& ProjectName) {
        const bool HasIcon{LoadMacawIcon(256)};
        const float Width{static_cast<float>(mWidth)};
        const float Height{static_cast<float>(mHeight)};
        const float IconSize{196.0f * mScale};
        const float IconCenterX{Width - 150.0f * mScale};
        const float IconCenterY{148.0f * mScale};
        const float IconLeft{IconCenterX - IconSize * 0.5f};
        const float IconTop{IconCenterY - IconSize * 0.5f};
        const Core::Int32 FooterTop{mHeight - Pixels(104.0f)};
        const Core::Int32 AccentHeight{std::max(Pixels(3.0f), 2)};

        for (Core::Int32 Y{}; Y < mHeight; ++Y) {
            const float FY{static_cast<float>(Y) + 0.5f};

            for (Core::Int32 X{}; X < mWidth; ++X) {
                const float FX{static_cast<float>(X) + 0.5f};
                FColor Color{Lerp(TopBackground, BottomBackground, FY / Height)};

                Color = Add(Color, MacawRed, 0.22f * Glow(FX, FY, IconCenterX, IconCenterY, 230.0f * mScale));
                Color = Add(Color, MacawBlue, 0.14f * Glow(FX, FY, 0.08f * Width, 1.08f * Height, 380.0f * mScale));

                if (HasIcon && FX >= IconLeft && FX < IconLeft + IconSize && FY >= IconTop && FY < IconTop + IconSize) {
                    const std::array<float, 4> Sample{SampleIcon((FX - IconLeft) / IconSize, (FY - IconTop) / IconSize)};
                    const float Inverse{1.0f - Sample[3]};

                    Color = {Sample[0] + Color[0] * Inverse, Sample[1] + Color[1] * Inverse, Sample[2] + Color[2] * Inverse};
                }

                if (Y >= FooterTop) {
                    Color = Lerp(Color, BottomBackground, 0.55f);
                }

                if (Y == FooterTop) {
                    Color = Lerp(Color, White, 0.07f);
                }

                if (Y < AccentHeight) {
                    Color = Lerp(MacawRed, MacawBlue, FX / Width);
                }

                mBits[Y * mWidth + X] = Pack(Color, Dither(X, Y));
            }
        }

        const Core::Int32 Left{Pixels(40.0f)};
        const Core::Int32 TextRight{static_cast<Core::Int32>(IconLeft) - Pixels(12.0f)};

        SelectObject(mDC, mTitleFont);
        SetTextCharacterExtra(mDC, Pixels(6.0f));
        DrawLine(L"MACAW", {Left - Pixels(2.0f), Pixels(44.0f), TextRight, Pixels(116.0f)}, 0xFFFFFF, 0);

        SelectObject(mDC, mCaptionFont);
        SetTextCharacterExtra(mDC, Pixels(3.0f));

        //DrawLine(L"GAME ENGINE  \x00B7  EDITOR", {Left, Pixels(114.0f), TextRight, Pixels(134.0f)}, 0x8C96A8, 0);
        DrawLine(L"PROJECT", {Left, Pixels(170.0f), TextRight, Pixels(188.0f)}, 0x5F6A7E, 0);

        SelectObject(mDC, mProjectFont);
        SetTextCharacterExtra(mDC, 0);
        DrawLine(ProjectName, {Left, Pixels(188.0f), TextRight, Pixels(224.0f)}, 0xFFFFFF, DT_END_ELLIPSIS);

        GdiFlush();
        mBackground.assign(mBits, mBits + static_cast<std::size_t>(mWidth) * static_cast<std::size_t>(mHeight));
    }

    /**
     * @brief 프레임 버퍼에 진행 막대와 반짝임을 그립니다.
     */
    void Platform::FSplashCanvas::DrawProgress(float Progress, float Time) {
        const Core::Int32 Left{Pixels(40.0f)};
        const Core::Int32 Right{mWidth - Pixels(40.0f)};
        const Core::Int32 Top{mHeight - Pixels(32.0f)};
        const Core::Int32 Thickness{std::max(Pixels(4.0f), 2)};
        const float Span{static_cast<float>(Right - Left)};
        const float Filled{static_cast<float>(Left) + Span * std::clamp(Progress, 0.0f, 1.0f)};
        const float ShimmerWidth{110.0f * mScale};
        const float ShimmerCenter{static_cast<float>(Left) - ShimmerWidth + std::fmod(Time * 320.0f * mScale, Span + ShimmerWidth * 2.0f)};

        for (Core::Int32 Y{Top}; Y < Top + Thickness; ++Y) {
            for (Core::Int32 X{Left}; X < Right; ++X) {
                const float FX{static_cast<float>(X) + 0.5f};
                Core::Uint32& Pixel{mBits[Y * mWidth + X]};

                if (FX < Filled) {
                    const float Shimmer{std::max(0.0f, 1.0f - std::abs(FX - ShimmerCenter) / ShimmerWidth)};
                    const FColor Fill{Lerp(MacawRed, MacawBlue, (FX - static_cast<float>(Left)) / Span)};

                    Pixel = Pack(Lerp(Fill, White, 0.45f * Shimmer * Shimmer));
                } else {
                    Pixel = Pack(Lerp(Unpack(Pixel), White, 0.08f));
                }
            }
        }
    }

}
